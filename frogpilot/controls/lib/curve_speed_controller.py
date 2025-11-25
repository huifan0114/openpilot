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

class CurveSpeedController:
  def __init__(self, FrogPilotVCruise):
    self.frogpilot_planner = FrogPilotVCruise.frogpilot_planner

    self.enable_training = False
    self.target_set = False

    self.training_timer = 0

    self.curvature_data = json.loads(params.get("CurvatureData") or "{}")

    self.required_curvatures = [str(round(road_curvature, ROUNDING_PRECISION)) for road_curvature in np.arange(MIN_CURVATURE, MAX_CURVATURE + STEP, STEP)]

    self.update_lateral_acceleration()

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
