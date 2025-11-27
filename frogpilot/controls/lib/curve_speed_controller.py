#!/usr/bin/env python3
import json
import numpy as np

from openpilot.common.realtime import DT_MDL

from openpilot.frogpilot.common.frogpilot_variables import CRUISING_SPEED, DEFAULT_LATERAL_ACCELERATION, PLANNER_TIME, params

CALIBRATION_PROGRESS_THRESHOLD = 10 / DT_MDL
MAX_CURVATURE = 0.1
MIN_CURVATURE = 0.001
PERCENTILE = 90
ROUNDING_PRECISION = 5
STEP = 0.001

# CSC 動態控制常數
BASE_LATERAL_ACCELERATION = 2.58  # 預設基準值 (m/s²)
STEER_USAGE_HIGH = 0.9   # 高使用率門檻
STEER_USAGE_LOW = 0.5    # 低使用率門檻 (可考慮提高 lat_acc)
MIN_LATERAL_ACCELERATION = 1.5   # 最低橫向加速度 (m/s²)
MAX_LATERAL_ACCELERATION = 3.5   # 最高橫向加速度 (m/s²)
LAT_ACC_ADJUST_STEP = 0.05       # 每次過彎後調整幅度 (m/s²)

class CurveSpeedController:
  def __init__(self, FrogPilotVCruise):
    self.frogpilot_planner = FrogPilotVCruise.frogpilot_planner

    self.enable_training = False
    self.target_set = False

    self.training_timer = 0

    self.curvature_data = json.loads(params.get("CurvatureData") or "{}")

    self.required_curvatures = [str(round(road_curvature, ROUNDING_PRECISION)) for road_curvature in np.arange(MIN_CURVATURE, MAX_CURVATURE + STEP, STEP)]

    # 使用已校準值或預設基準值
    calibrated = params.get_float("CalibratedLateralAcceleration")
    self.lateral_acceleration = calibrated if calibrated > 0 else BASE_LATERAL_ACCELERATION

    # CSC 過彎期間的監控變數
    self.curve_session_active = False
    self.saturated_count = 0        # 這次過彎中 steer_saturated 發生次數
    self.high_usage_count = 0       # 這次過彎中高使用率 (>=0.9) 次數
    self.low_usage_count = 0        # 這次過彎中低使用率 (<0.5) 次數
    self.total_frames = 0           # 這次過彎的總幀數

  def log_data(self, v_ego, sm):
    self.enable_training = v_ego > CRUISING_SPEED
    self.enable_training &= not self.frogpilot_planner.tracking_lead
    # 移除 longActive 限制 - 原本只允許手動駕駛時訓練，但這樣用戶永遠無法完成訓練
    # self.enable_training &= not sm["carControl"].longActive

    if self.enable_training:
      self.training_timer += DT_MDL

      if self.training_timer >= PLANNER_TIME and self.frogpilot_planner.driving_in_curve and not (sm["carState"].leftBlinker or sm["carState"].rightBlinker):
        lateral_acceleration = abs(self.frogpilot_planner.lateral_acceleration)
        road_curvature = abs(round(self.frogpilot_planner.road_curvature, ROUNDING_PRECISION))

        key = str(road_curvature)
        if key in self.curvature_data:
          data = self.curvature_data[key]

          average = data["average"]
          count = data["count"]

          self.curvature_data[key] = {
            "average": ((average * count) + lateral_acceleration) / (count + 1),
            "count": count + 1
          }
        else:
          self.curvature_data[key] = {
            "average": lateral_acceleration,
            "count": 1
          }

        self.update_lateral_acceleration()
      else:
        self.enable_training = False

    elif self.training_timer >= PLANNER_TIME:
      progress = 0.0
      for key in self.required_curvatures:
        if key in self.curvature_data:
          progress += min(self.curvature_data[key]["count"] / CALIBRATION_PROGRESS_THRESHOLD, 1.0)

      params.put_float_nonblocking("CalibrationProgress", (progress / len(self.required_curvatures)) * 100)
      params.put_nonblocking("CurvatureData", json.dumps(self.curvature_data))

      self.enable_training = False

      self.training_timer = 0

    else:
      self.enable_training = False

      self.training_timer = 0

  def update_lateral_acceleration(self):
    if self.curvature_data:
      all_samples = [data["average"] for data in self.curvature_data.values()]
      self.lateral_acceleration = float(np.percentile(all_samples, PERCENTILE))
    else:
      self.lateral_acceleration = DEFAULT_LATERAL_ACCELERATION

    params.put_float_nonblocking("CalibratedLateralAcceleration", self.lateral_acceleration)

  def update_target(self, v_ego):
    # ========== 記錄過彎期間的 steer 使用情況 ==========
    steer_usage = self.frogpilot_planner.steer_usage
    steer_saturated = self.frogpilot_planner.steer_saturated

    # 開始新的過彎 session
    if not self.curve_session_active:
      self.curve_session_active = True
      self.saturated_count = 0
      self.high_usage_count = 0
      self.low_usage_count = 0
      self.total_frames = 0

    # 統計這次過彎的 steer 使用情況
    self.total_frames += 1
    if steer_saturated:
      self.saturated_count += 1
    if steer_usage >= STEER_USAGE_HIGH:
      self.high_usage_count += 1
    elif steer_usage < STEER_USAGE_LOW:
      self.low_usage_count += 1

    # ========== 計算 CSC 目標速度 ==========
    lateral_acceleration = self.lateral_acceleration
    road_curvature = abs(self.frogpilot_planner.road_curvature)

    # 防止除以零
    if road_curvature < 0.0001:
      road_curvature = 0.0001

    csc_speed = (lateral_acceleration / road_curvature)**0.5

    if self.target_set:
      decel_rate = (v_ego - csc_speed) / self.frogpilot_planner.time_to_curve

      self.target -= decel_rate * DT_MDL
      # 修正：clip 上限為 min(csc_speed, v_ego)，防止 target 增加超過當前車速
      self.target = float(np.clip(self.target, CRUISING_SPEED, min(csc_speed, v_ego)))
    else:
      self.target_set = True
      # 修正：初始化為 min(v_ego, csc_speed)，直接從安全速度開始
      self.target = min(v_ego, csc_speed)

  def end_curve_session(self):
    """CSC 結束時評估這次過彎並調整 lateral_acceleration"""
    if not self.curve_session_active or self.total_frames < 10:
      # 過彎時間太短，不做調整
      self.curve_session_active = False
      return

    old_lat_acc = self.lateral_acceleration
    adjusted = False

    # 計算比例
    saturated_ratio = self.saturated_count / self.total_frames
    high_usage_ratio = self.high_usage_count / self.total_frames
    low_usage_ratio = self.low_usage_count / self.total_frames

    # 判斷是否需要調整
    if self.saturated_count > 0 or high_usage_ratio > 0.3:
      # 發生過物理極限 或 超過30%時間高使用率 → 降低 lateral_acceleration
      self.lateral_acceleration -= LAT_ACC_ADJUST_STEP
      adjusted = True
    elif low_usage_ratio > 0.8 and self.lateral_acceleration < MAX_LATERAL_ACCELERATION:
      # 超過80%時間都是低使用率 → 可以微幅提高
      self.lateral_acceleration += LAT_ACC_ADJUST_STEP * 0.5  # 提高幅度較保守
      adjusted = True

    # 限制範圍
    self.lateral_acceleration = float(np.clip(
      self.lateral_acceleration,
      MIN_LATERAL_ACCELERATION,
      MAX_LATERAL_ACCELERATION
    ))

    # 寫入 params (持久化)
    if adjusted:
      params.put_float_nonblocking("CalibratedLateralAcceleration", self.lateral_acceleration)

    # 重置 session
    self.curve_session_active = False
    self.saturated_count = 0
    self.high_usage_count = 0
    self.low_usage_count = 0
    self.total_frames = 0
