#!/usr/bin/env python3
import json
import numpy as np

from openpilot.common.realtime import DT_MDL

from openpilot.frogpilot.common.frogpilot_variables import CRUISING_SPEED, DEFAULT_LATERAL_ACCELERATION, PLANNER_TIME, params

CALIBRATION_PROGRESS_THRESHOLD = 10 / DT_MDL
MAX_CURVATURE = 0.1
MIN_CURVATURE = 0.001
#################################
PERCENTILE = 75  # 從 90 降到 75，更保守的極限設定
#################################
ROUNDING_PRECISION = 5
STEP = 0.001
#################################
# ========== CSC 基礎常數 ==========
BASE_LATERAL_ACCELERATION = 2.58  # 預設基準值 (m/s²)
MIN_LATERAL_ACCELERATION = 1.8    # 最低橫向加速度 (m/s²)
MAX_LATERAL_ACCELERATION = 3.5    # 最高橫向加速度 (m/s²)

# ========== 過彎後學習 (end_curve_session) ==========
# 用於統計這次過彎的 steer_usage 分布，決定是否調整 adjustment
SESSION_STEER_HIGH = 0.8    # 統計「高使用率」的門檻
SESSION_STEER_LOW = 0.5     # 統計「低使用率」的門檻
LAT_ACC_ADJUST_STEP = 0.08  # 每次過彎後調整幅度

# ========== 即時動態調整 (update_target) ==========
# 目標：讓 steer_usage 維持在 0.8~0.88（接近極限但不觸發 steer_saturated）
# 只有在接近危險區域時才輕微減速，不要過度減速
REALTIME_STEER_THRESHOLD = 0.75  # steer_usage 達到 75% 才開始調整（盡量用到極限）
REALTIME_ADJUST_RATE = 2.0       # 調整係數 (steer_usage 每增加 0.1 → lat_acc 降低 20%)
MAX_LAT_ACC_REDUCTION = 0.25     # 最多降低 25%（避免過度減速）
#################################

class CurveSpeedController:
  def __init__(self, FrogPilotVCruise):
    self.frogpilot_planner = FrogPilotVCruise.frogpilot_planner

    self.enable_training = False
    self.target_set = False

    self.training_timer = 0

    self.curvature_data = json.loads(params.get("CurvatureData") or "{}")

    self.required_curvatures = [str(round(road_curvature, ROUNDING_PRECISION)) for road_curvature in np.arange(MIN_CURVATURE, MAX_CURVATURE + STEP, STEP)]

#################################
    # 兩層學習機制（分離儲存，避免互相覆蓋）:
    # 1. calibrated_base: log_data 的 P90 結果（訓練學習）
    # 2. adjustment: end_curve_session 的累積調整（過彎反饋）
    calibrated = params.get_float("CalibratedLateralAcceleration")
    self.calibrated_base = calibrated if calibrated > 0 else BASE_LATERAL_ACCELERATION

    adjustment = params.get_float("LateralAccelerationAdjustment")
    self.adjustment = adjustment if adjustment != 0 else 0.0

    # 實際使用的 lateral_acceleration = base + adjustment
    self.lateral_acceleration = float(np.clip(
      self.calibrated_base + self.adjustment,
      MIN_LATERAL_ACCELERATION,
      MAX_LATERAL_ACCELERATION
    ))

    # CSC 過彎期間的監控變數
    self.curve_session_active = False
    self.saturated_count = 0        # 這次過彎中 steer_saturated 發生次數
    self.high_usage_count = 0       # 這次過彎中高使用率 (>=SESSION_STEER_HIGH) 次數
    self.low_usage_count = 0        # 這次過彎中低使用率 (<SESSION_STEER_LOW) 次數
    self.total_frames = 0           # 這次過彎的總幀數

    # 平滑減速用的變數
    self.current_reduction = 0.0    # 當前的 lat_acc 降低比例（平滑後）
#################################

  def log_data(self, v_ego, sm):
    self.enable_training = v_ego > CRUISING_SPEED
    self.enable_training &= not self.frogpilot_planner.tracking_lead
#################################
    # 移除 longActive 限制 - 原本只允許手動駕駛時訓練，但這樣用戶永遠無法完成訓練
    # self.enable_training &= not sm["carControl"].longActive
#################################

    if self.enable_training:
      self.training_timer += DT_MDL
#################################
      # 關鍵修改：只有在「用力過彎」時才收集數據
      # 這樣才能學習到真正的極限，而不是舒適駕駛的數據
      steer_usage = self.frogpilot_planner.steer_usage
      MIN_STEER_FOR_TRAINING = 0.6  # 只收集 steer_usage > 60% 的數據
#################################
      if self.training_timer >= PLANNER_TIME and self.frogpilot_planner.driving_in_curve and not (sm["carState"].leftBlinker or sm["carState"].rightBlinker):
#################################
        # 只有用力過彎時才收集（學習極限而非舒適駕駛）
        if steer_usage < MIN_STEER_FOR_TRAINING:
          return  # 太輕鬆的過彎不收集
#################################

        lateral_acceleration = abs(self.frogpilot_planner.lateral_acceleration)
        road_curvature = abs(round(self.frogpilot_planner.road_curvature, ROUNDING_PRECISION))

        key = str(road_curvature)
        if key in self.curvature_data:
          data = self.curvature_data[key]
#################################
          current_max = data.get("max", data.get("average", 0))  # 相容舊格式

          # 關鍵修改：存「最大值」而非「平均值」
          # 這樣才能學習到車輛在該曲率下的極限
          self.curvature_data[key] = {
            "max": max(current_max, lateral_acceleration),
            "count": data["count"] + 1
          }
        else:
          self.curvature_data[key] = {
            "max": lateral_acceleration,
            "count": 1
#################################
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
#################################
    """更新 calibrated_base（訓練學習的極限值）

    修改後的學習邏輯：
    - 只收集 steer_usage > 60% 的數據（用力過彎）
    - 每個曲率點存「最大值」（代表該曲率下的極限）
    - 使用 P75 計算 calibrated_base（各曲率極限的 P75）

    這樣 calibrated_base 代表「車輛在各種彎道的極限橫向加速度」
    """
#################################
    if self.curvature_data:
#################################
      # 相容新舊格式：優先用 "max"，沒有則用 "average"
      all_samples = [data.get("max", data.get("average", 0)) for data in self.curvature_data.values()]
      all_samples = [s for s in all_samples if s > 0]  # 過濾無效數據
      if all_samples:
        self.calibrated_base = float(np.percentile(all_samples, PERCENTILE))
      else:
        self.calibrated_base = DEFAULT_LATERAL_ACCELERATION
    else:
      self.calibrated_base = DEFAULT_LATERAL_ACCELERATION

    # 只更新 calibrated_base，不影響 adjustment
    params.put_float_nonblocking("CalibratedLateralAcceleration", self.calibrated_base)

    # 重新計算實際使用的 lateral_acceleration
    self.lateral_acceleration = float(np.clip(
      self.calibrated_base + self.adjustment,
      MIN_LATERAL_ACCELERATION,
      MAX_LATERAL_ACCELERATION
    ))
#################################
  def update_target(self, v_ego):
#################################
    # ========== 記錄過彎期間的 steer 使用情況 ==========
    steer_usage = self.frogpilot_planner.steer_usage
    steer_saturated = self.frogpilot_planner.steer_saturated
    driving_in_curve = self.frogpilot_planner.driving_in_curve

    # 只有在實際轉彎時才統計（避免直路上誤統計）
    if driving_in_curve:
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
      if steer_usage >= SESSION_STEER_HIGH:
        self.high_usage_count += 1
      elif steer_usage < SESSION_STEER_LOW:
        self.low_usage_count += 1

    # ========== 計算有效 lateral_acceleration ==========
    # 核心思路：當 steer_usage 高時，代表當前的 lateral_acceleration 設定太樂觀
    # 動態降低有效值，讓 csc_speed 更保守，自然觸發減速
    # 這個方法天然考慮了彎道曲率（通過 csc_speed 公式）
    base_lat_acc = self.lateral_acceleration
    road_curvature = abs(self.frogpilot_planner.road_curvature)

    # 防止除以零
    if road_curvature < 0.0001:
      road_curvature = 0.0001

    # ========== 平滑的動態調整 ==========
    # 計算目標 reduction（根據 steer_usage）
    if steer_usage > REALTIME_STEER_THRESHOLD:
      # steer_usage = 0.75 → 目標降低 0%
      # steer_usage = 0.80 → 目標降低 10%
      # steer_usage = 0.85 → 目標降低 20%
      # steer_usage = 0.90+ → 目標降低 25% (上限)
      target_reduction = min((steer_usage - REALTIME_STEER_THRESHOLD) * REALTIME_ADJUST_RATE, MAX_LAT_ACC_REDUCTION)
    else:
      # 警告消失 → 目標回到 0（不再額外減速）
      target_reduction = 0.0

    # 平滑過渡（低通濾波）
    # 升高時快一點（安全），降低時慢一點（舒適）
    if target_reduction > self.current_reduction:
      # 需要更多減速 → 較快響應
      SMOOTH_UP = 0.15  # 每幀變化 15%
      self.current_reduction += (target_reduction - self.current_reduction) * SMOOTH_UP
    else:
      # 可以減少減速 → 較慢恢復（避免突然加速）
      SMOOTH_DOWN = 0.05  # 每幀變化 5%
      self.current_reduction += (target_reduction - self.current_reduction) * SMOOTH_DOWN

    # 應用平滑後的 reduction
    effective_lat_acc = base_lat_acc * (1.0 - self.current_reduction)
    effective_lat_acc = max(effective_lat_acc, MIN_LATERAL_ACCELERATION)

    # 使用調整後的 lateral_acceleration 計算 csc_speed
    csc_speed = (effective_lat_acc / road_curvature)**0.5
    time_to_curve = max(self.frogpilot_planner.time_to_curve, 0.5)
    distance_to_curve = v_ego * time_to_curve  # 到彎道的距離

    # ========== 舒適性參數 ==========
    COMFORT_DECEL = 2.0   # m/s² - 舒適減速
    MAX_DECEL = 2.5       # m/s² - 最大減速
    SMOOTHING = 0.15      # 減速平滑係數
    EARLY_DECEL_MARGIN = 1.1  # 提前減速的速度比例（當 v_ego > csc_speed * 1.1 就開始）
#################################

    if self.target_set:
#################################
      # ===== 計算提前減速門檻 =====
      # 當 v_ego 接近 csc_speed 時就開始減速，不要等到超過
      early_threshold = csc_speed * EARLY_DECEL_MARGIN

      # ===== 優先級 1: 需要減速 (v_ego > csc_speed * 1.1) =====
      # 提前開始減速，避免「到彎道才發現速度太高」
      if v_ego > early_threshold:
        # 計算需要的減速距離: d = (v² - v_target²) / (2 * a)
        required_distance = (v_ego**2 - csc_speed**2) / (2 * COMFORT_DECEL)

        if distance_to_curve <= required_distance * 1.5:
          # 距離不夠 → 必須立即開始減速（提高安全係數到 1.5）
          actual_decel = (v_ego**2 - csc_speed**2) / (2 * max(distance_to_curve, 5))
          actual_decel = float(np.clip(actual_decel, COMFORT_DECEL, MAX_DECEL))
          self.target -= actual_decel * DT_MDL
        else:
          # 還有足夠距離 → 平滑過渡到 csc_speed
          self.target = self.target * (1 - SMOOTHING) + csc_speed * SMOOTHING

        # 減速時：下限 CRUISING_SPEED，上限 csc_speed
        self.target = float(np.clip(self.target, CRUISING_SPEED, csc_speed))

      # ===== 優先級 2: 接近極限 (csc_speed < v_ego <= csc_speed * 1.1) =====
      elif v_ego > csc_speed:
        # 速度剛超過 csc_speed，開始輕微減速
        self.target = self.target * (1 - SMOOTHING) + csc_speed * SMOOTHING
        self.target = float(np.clip(self.target, CRUISING_SPEED, csc_speed))

      # ===== 不需要減速 (v_ego <= csc_speed) → CSC 不限制 =====
      else:
        # 出彎/彎中不需減速：target 設為無限大，讓 min() 選擇 v_cruise
        # 這樣 Progressive 可以控制加速，CSC 不會擋住
        self.target = float('inf')
#################################

    else:
      self.target_set = True
#################################
      self.target = min(v_ego, csc_speed)

  def end_curve_session(self):
    """CSC 結束時評估這次過彎並調整 adjustment（過彎反饋偏移）"""
    if not self.curve_session_active or self.total_frames < 10:
      # 過彎時間太短，不做調整
      self.curve_session_active = False
      return

    adjusted = False

    # 計算比例
    high_usage_ratio = self.high_usage_count / self.total_frames
    low_usage_ratio = self.low_usage_count / self.total_frames
    # 理想區間：steer_usage 在 0.5~0.8 之間（有餘裕但不浪費）
    mid_usage_ratio = 1.0 - high_usage_ratio - low_usage_ratio

    # 判斷是否需要調整 adjustment（不影響 calibrated_base）
    if self.saturated_count > 0:
      # 觸發過 steer_saturated → 必須降低
      self.adjustment -= LAT_ACC_ADJUST_STEP
      adjusted = True
    elif high_usage_ratio > 0.5:
      # 超過50%時間高使用率 → 降低（接近極限太久）
      self.adjustment -= LAT_ACC_ADJUST_STEP * 0.5
      adjusted = True
    elif high_usage_ratio < 0.2 and low_usage_ratio > 0.3:
      # 高使用率很少 且 低使用率較多 → 還有餘裕，可以提高
      self.adjustment += LAT_ACC_ADJUST_STEP * 0.5
      adjusted = True
    elif mid_usage_ratio > 0.6 and high_usage_ratio < 0.3:
      # 大部分時間在理想區間，且高使用率不多 → 微幅提高，探索極限
      self.adjustment += LAT_ACC_ADJUST_STEP * 0.3
      adjusted = True

    # 限制 adjustment 範圍（避免調整過度）
    # 最多降低 0.5，最多提高 0.3
    self.adjustment = float(np.clip(self.adjustment, -0.5, 0.3))

    # 重新計算實際使用的 lateral_acceleration
    self.lateral_acceleration = float(np.clip(
      self.calibrated_base + self.adjustment,
      MIN_LATERAL_ACCELERATION,
      MAX_LATERAL_ACCELERATION
    ))

    # 寫入 params (持久化) - 只寫入 adjustment，不覆蓋 calibrated_base
    if adjusted:
      params.put_float_nonblocking("LateralAccelerationAdjustment", self.adjustment)

    # 重置 session
    self.curve_session_active = False
    self.saturated_count = 0
    self.high_usage_count = 0
    self.low_usage_count = 0
    self.total_frames = 0
#################################
