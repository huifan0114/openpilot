#!/usr/bin/env python3
import json
import math
import time

#################################
from cereal import car,log
#################################
import cereal.messaging as messaging

#########################################
from openpilot.common.params import Params
#########################################
from openpilot.common.conversions import Conversions as CV
from openpilot.common.filter_simple import FirstOrderFilter
from openpilot.common.realtime import DT_MDL
from openpilot.selfdrive.controls.lib.drive_helpers import V_CRUISE_MAX
from openpilot.selfdrive.controls.lib.longitudinal_mpc_lib.long_mpc import A_CHANGE_COST, DANGER_ZONE_COST, J_EGO_COST, STOP_DISTANCE

from openpilot.frogpilot.common.frogpilot_utilities import calculate_lane_width, calculate_road_curvature
from openpilot.frogpilot.common.frogpilot_variables import CRUISING_SPEED, MINIMUM_LATERAL_ACCELERATION, PLANNER_TIME, THRESHOLD, params, params_memory
from openpilot.frogpilot.controls.lib.conditional_experimental_mode import ConditionalExperimentalMode
from openpilot.frogpilot.controls.lib.frogpilot_acceleration import FrogPilotAcceleration
from openpilot.frogpilot.controls.lib.frogpilot_events import FrogPilotEvents
from openpilot.frogpilot.controls.lib.frogpilot_following import FrogPilotFollowing
from openpilot.frogpilot.controls.lib.frogpilot_vcruise import FrogPilotVCruise
from openpilot.frogpilot.controls.lib.weather_checker import WeatherChecker

class FrogPilotPlanner:
  def __init__(self, error_log, ThemeManager):
    #########################################
    self.params = Params()
    self.params_memory = Params("/dev/shm/params")
    #########################################
    self.cem = ConditionalExperimentalMode(self)
    self.frogpilot_acceleration = FrogPilotAcceleration(self)
    self.frogpilot_events = FrogPilotEvents(self, error_log, ThemeManager)
    self.frogpilot_following = FrogPilotFollowing(self)
    self.frogpilot_vcruise = FrogPilotVCruise(self)
    self.frogpilot_weather = WeatherChecker()

#################################
    with car.CarParams.from_bytes(params.get("CarParams", block=True)) as msg:
      self.CP = msg
#################################

    self.tracking_lead_filter = FirstOrderFilter(0, 0.5, DT_MDL)

    self.driving_in_curve = False
    self.lateral_check = False
    self.model_stopped = False
    self.road_curvature_detected = False
    self.slower_lead = False
    self.tracking_lead = False

    self.lane_width_left = 0
    self.lane_width_right = 0
    self.lateral_acceleration = 0
    self.model_length = 0
    self.road_curvature = 0
    self.time_to_curve = 0
    self.v_cruise = 0
#########################################
    self.detect_speed_prev = 0
    self.speed_over = False
    self.previous_road_name = ""  # 記錄上次的路名，避免重複設定
    self.previous_roadtype_profile = 0  # 記錄上次的道路類型檔案，避免重複設定
    self.stopmark_active_prev = False  # 追踪上一次的 StopmarkActive 狀態（防抖）
    self.stopmark_active_timer = 0.0  # Stopmark 激活狀態持續時間計時器
    self.stopmark_last_update_time = 0.0  # Stopmark 降速參數寫入節流時間戳
    # CSC 動態控制相關變數
    self.steer_usage = 0.0
    self.undershooting = False
    self.turning = False
    self.steer_saturated = False
#########################################

  def update(self, now, time_validated, sm, frogpilot_toggles):
    self.lead_one = sm["radarState"].leadOne

    v_cruise = min(sm["controlsState"].vCruise, V_CRUISE_MAX) * CV.KPH_TO_MS
    v_ego = max(sm["carState"].vEgo, 0)

############
    v_ego_kph = v_ego *3.6
    if v_ego_kph == 0 :
      self.params_memory.put_int("MapSpeed", 2)
    elif v_ego_kph >= 1 and v_ego_kph < 10 :
      self.params_memory.put_int("MapSpeed", 0)
    elif v_ego_kph >= 10 and v_ego_kph < 30 :
      self.params_memory.put_int("MapSpeed", 1)
    elif v_ego_kph >= 30 and v_ego_kph < 50 :
      self.params_memory.put_int("MapSpeed", 2)
    elif v_ego_kph >= 50 and v_ego_kph < 70 :
      self.params_memory.put_int("MapSpeed", 3)
    elif v_ego_kph >= 70 and v_ego_kph < 90 :
      self.params_memory.put_int("MapSpeed", 4)
    elif v_ego_kph >= 90 :
      self.params_memory.put_int("MapSpeed", 5)
############

    if sm["controlsState"].enabled:
      self.frogpilot_acceleration.update(v_ego, sm, frogpilot_toggles)
    else:
      self.frogpilot_acceleration.max_accel = 0
      self.frogpilot_acceleration.min_accel = 0

    if sm["controlsState"].enabled and frogpilot_toggles.conditional_experimental_mode:
      self.cem.update(v_ego, sm, frogpilot_toggles)
    else:
      self.cem.curve_detected = False
      self.cem.stop_sign_and_light(v_ego, sm, PLANNER_TIME - 2)

    self.driving_in_curve = abs(self.lateral_acceleration) >= MINIMUM_LATERAL_ACCELERATION

    self.frogpilot_events.update(v_cruise, sm, frogpilot_toggles)

    self.frogpilot_following.update(v_ego, sm, frogpilot_toggles)

    localizer_valid = (sm["liveLocationKalman"].status == log.LiveLocationKalman.Status.valid) and sm["liveLocationKalman"].positionGeodetic.valid
    if sm["liveLocationKalman"].gpsOK and localizer_valid:
      gps_position = {
        "latitude": sm["liveLocationKalman"].positionGeodetic.value[0],
        "longitude": sm["liveLocationKalman"].positionGeodetic.value[1],
        "bearing": math.degrees(sm["liveLocationKalman"].calibratedOrientationNED.value[2])
      }

      params_memory.put("LastGPSPosition", json.dumps(gps_position))
    else:
      gps_position = None

      params_memory.remove("LastGPSPosition")

#########################################
    self.lateral_acceleration = v_ego**2 * (sm["carState"].steeringAngleDeg - sm["liveParameters"].angleOffsetDeg) * CV.DEG_TO_RAD / (self.CP.steerRatio * self.CP.wheelbase)

    # CSC 動態控制 - 計算 steer 使用率和物理極限檢測
    # actuators.steer 範圍: -1.0 到 1.0
    self.steer_usage = abs(sm["carControl"].actuators.steer)

    # 期望橫向加速度 (從模型期望的轉向角度計算)
    desired_steer = sm["carControl"].actuatorsOutput.steer if hasattr(sm["carControl"], 'actuatorsOutput') else sm["carControl"].actuators.steer
    desired_lat_acc = abs(v_ego**2 * desired_steer * self.CP.steerRatio * CV.DEG_TO_RAD / self.CP.wheelbase) if v_ego > 1 else 0

    # 實際橫向加速度
    actual_lat_acc = abs(self.lateral_acceleration)

    # 三個條件檢測 (與 steerSaturated event 相同邏輯)
    # 1. saturated: 方向盤輸出達到 90%
    saturated = self.steer_usage >= 0.9
    # 2. undershooting: 實際轉向不足 (期望/實際 > 1.2)
    self.undershooting = (actual_lat_acc > 0.5 and desired_lat_acc / actual_lat_acc > 1.2) if actual_lat_acc > 0.1 else False
    # 3. turning: 正在轉彎 (期望橫向加速度 > 1.0 m/s²)
    self.turning = desired_lat_acc > 1.0

    # 物理極限判定 = 三個條件同時滿足
    self.steer_saturated = saturated and self.undershooting and self.turning
#########################################

    check_lane_width = frogpilot_toggles.adjacent_paths or frogpilot_toggles.adjacent_path_metrics or frogpilot_toggles.blind_spot_path or frogpilot_toggles.lane_detection
    if check_lane_width and v_ego >= frogpilot_toggles.minimum_lane_change_speed:
      self.lane_width_left = calculate_lane_width(sm["modelV2"].laneLines[0], sm["modelV2"].laneLines[1], sm["modelV2"].roadEdges[0])
      self.lane_width_right = calculate_lane_width(sm["modelV2"].laneLines[3], sm["modelV2"].laneLines[2], sm["modelV2"].roadEdges[1])
      ###################################
      # 寫入右邊路徑寬度到 params_memory 供 C++ 端使用
      # self.params_memory.put_float("LaneWidthRight", self.lane_width_right)
      ###################################
    else:
      self.lane_width_left = 0
      self.lane_width_right = 0
      ###################################
      # self.params_memory.put_float("LaneWidthRight", 0.0)
      ###################################

    self.lateral_acceleration = v_ego**2 * sm["controlsState"].curvature

    self.lateral_check = v_ego >= frogpilot_toggles.pause_lateral_below_speed
    self.lateral_check |= not (sm["carState"].leftBlinker or sm["carState"].rightBlinker) and frogpilot_toggles.pause_lateral_below_signal
    self.lateral_check |= sm["carState"].standstill

    self.model_length = sm["modelV2"].position.x[-1]

    self.model_stopped = self.model_length < CRUISING_SPEED * PLANNER_TIME
    self.model_stopped |= self.frogpilot_vcruise.forcing_stop

    self.road_curvature, self.time_to_curve = calculate_road_curvature(sm["modelV2"], v_ego)

    self.road_curvature_detected = (1 / abs(self.road_curvature))**0.5 < v_ego > CRUISING_SPEED and not (sm["carState"].leftBlinker or sm["carState"].rightBlinker)

    if not sm["carState"].standstill:
      self.tracking_lead = self.update_lead_status()

#########################################
    # 使用 calibrated lateral_acceleration 來偵測彎道 (與 CSC 目標計算一致)
    lat_acc = self.frogpilot_vcruise.csc.lateral_acceleration
    csc_trigger_speed = (lat_acc / abs(self.road_curvature))**0.5 if abs(self.road_curvature) > 0.0001 else 999

    # FIX: 提前觸發 CSC，避免等到超速才開始減速
    # 原本邏輯：v_ego > csc_trigger_speed 才觸發（太晚了！）
    # 新邏輯：
    #   1. 速度預警：當車速接近目標速度時提前觸發
    #   2. 時間預警：當彎道很近時（< 3 秒）且車速接近目標，提前觸發
    SPEED_BUFFER = 3 * CV.KPH_TO_MS  # 3 km/h 的速度預警緩衝
    TIME_BUFFER = 3.0  # 3 秒的時間預警

    # 條件 1: 車速接近或超過目標速度
    speed_trigger = v_ego > (csc_trigger_speed - SPEED_BUFFER)
    # 條件 2: 彎道很近且車速達到目標的 90%
    time_trigger = self.time_to_curve < TIME_BUFFER and v_ego > csc_trigger_speed * 0.9

    early_trigger = speed_trigger or time_trigger
    # 方向燈抑制：只有在 40 km/h 以上打方向燈時才停用 CSC（低速彎道仍需減速）
    blinker_suppress = (sm["carState"].leftBlinker or sm["carState"].rightBlinker) and v_ego > 40 * CV.KPH_TO_MS
    self.road_curvature_detected = early_trigger and v_ego > CRUISING_SPEED and not blinker_suppress

    #if not sm["carState"].standstill:
    self.tracking_lead = self.update_lead_status()
#########################################

    self.v_cruise = self.frogpilot_vcruise.update(gps_position, now, time_validated, v_cruise, v_ego, sm, frogpilot_toggles)

    if gps_position and time_validated and frogpilot_toggles.weather_presets:
      self.frogpilot_weather.update_weather(gps_position, now, frogpilot_toggles)
    else:
      self.frogpilot_weather.weather_id = 0

##################定義參數##################################################################
    current_isengaged = self.params.get_bool("IsEngaged")

    detect_sl_raw = int(self.frogpilot_vcruise.slc.target * 3.6) if self.frogpilot_vcruise.slc.target > 0 else 0
    detect_sl = detect_sl_raw
    slc_source = self.frogpilot_vcruise.slc.source

    # ---------- Roadtype Profile 速限建議參數 ----------
    PROFILE_LIMITS = {1: (30, 45), 2: (50, 77), 3: (80, 99), 4: (120, float("inf"))}

    # =========================================================
    # 統一速限更新函數（避免重複代碼和邏輯混亂）
    # =========================================================
    SPEED_LIMIT_MIN_KPH = 30
    SPEED_LIMIT_MAX_KPH = 120

    def clamp_speed_limit_kph(speed_kph):
      speed_kph = int(round(speed_kph))
      return max(SPEED_LIMIT_MIN_KPH, min(SPEED_LIMIT_MAX_KPH, speed_kph))

    def update_speed_limit(new_speed_limit, force_update=False):
      """統一的速限更新函數，處理所有速限變更邏輯"""
      if new_speed_limit <= 0:
        return False

      new_speed_limit = clamp_speed_limit_kph(new_speed_limit)

      current_setspeed = self.params_memory.get_int("KeySetSpeed")
      current_detect_speed = self.params_memory.get_int("DetectSpeedLimit")

      # 檢查是否真的需要更新（避免不必要的寫入）
      speed_changed = (new_speed_limit != current_setspeed) or force_update
      detect_changed = (new_speed_limit != current_detect_speed) or force_update

      if speed_changed or detect_changed:
        if detect_changed:
          self.params_memory.put_int("DetectSpeedLimit", new_speed_limit)
          if new_speed_limit != self.detect_speed_prev:
            self.params_memory.put_bool("SpeedLimitChanged", True)
            self.detect_speed_prev = new_speed_limit

        if speed_changed:
          self.params_memory.put_int("KeySetSpeed", new_speed_limit)
          self.params_memory.put_bool("KeyChanged", True)

        return True
      return False

    def clamp_offline_or_road_limit(speed_kph, source=None, is_road_profile=False):
      if speed_kph <= 0:
        return speed_kph
      if is_road_profile or source in ("Map Data", "Navigation"):
        return max(30, min(120, speed_kph))
      return speed_kph
    # ------------autoacc--------------
    if frogpilot_toggles.autoacc and not current_isengaged :
      autoacc_caraway_status = self.params_memory.get_int("AutoACCCarAwaystatus")
      autoacc_greenlight_status = self.params_memory.get_int("AutoACCGreenLightstatus")
      auto_acc_pass = v_ego_kph > frogpilot_toggles.autoacc_speed
      if auto_acc_pass or autoacc_caraway_status == 1 or autoacc_greenlight_status == 1:
        has_map_or_nav_sl = frogpilot_toggles.navspeed and detect_sl_raw > 0 and slc_source in ("Map Data", "Navigation")
        self.params_memory.put_bool("KeyResume", True)
        self.params_memory.put_int("AutoACCCarAwaystatus", 0)
        self.params_memory.put_int("AutoACCGreenLightstatus", 0)
        self.params_memory.put_bool("StopmarkApplied", False)

        # 🔧 優化：使用統一函數處理速限更新
        if has_map_or_nav_sl:
          adjusted_sl = int(round(detect_sl_raw * 1.1))  # 導航/地圖速限自動 +10%
          adjusted_sl = clamp_offline_or_road_limit(adjusted_sl, source=slc_source)
          update_speed_limit(adjusted_sl, force_update=True)
        else:
          # 使用 profile 兜底
          current_setspeed = self.params_memory.get_int("KeySetSpeed")
          roadtype_profile = self.params.get_int("RoadtypeProfile")
          key_set_speed = 0

          if roadtype_profile != 0 and roadtype_profile in PROFILE_LIMITS:
            min_speed, max_speed = PROFILE_LIMITS[roadtype_profile]
            if not (min_speed <= current_setspeed < max_speed):
              key_set_speed = min_speed

          if key_set_speed > 0:
            # 避免拿過期的 detect_speedlimit 來放大路名建議
            if has_map_or_nav_sl:
               adjusted_sl = int(round(detect_sl_raw * 1.1))
               key_set_speed = min(key_set_speed, adjusted_sl)
            update_speed_limit(key_set_speed, force_update=True)

    # =========================================================
    # 道路名稱與道路類型檔案變更檢測（當道路改變時，即時更新速限）
    # 🔧 優化：即使ACC啟動也能自動更新速限，但要避免與Stopmark衝突
    # =========================================================
    current_road_name = self.params_memory.get("RoadName", encoding="utf8")
    current_roadtype_profile = self.params.get_int("RoadtypeProfile")

    # 🔧 優化：只在真正變更時才處理，並使用統一更新函數
    road_changed = (current_road_name != self.previous_road_name) or (current_roadtype_profile != self.previous_roadtype_profile)

    if road_changed:
      # 🔧 優化：檢查 Stopmark 狀態，避免與紅燈減速衝突
      is_stopmark_active = self.params_memory.get_bool("StopmarkApplied") or self.params_memory.get_bool("StopmarkRecovering")
      # 🔧 優化：檢查用戶按鈕保護狀態，避免覆蓋用戶手動調整
      is_button_protected = hasattr(self, 'frogpilot_vcruise') and self.frogpilot_vcruise.is_button_protected()

      # 只有在 Stopmark 未激活且無按鈕保護時才更新歷史紀錄和速限
      if not is_stopmark_active and not is_button_protected:
        self.previous_road_name = current_road_name
        self.previous_roadtype_profile = current_roadtype_profile
        # 道路變更時的速限更新邏輯（優先順序：導航速限 > Profile速限）
        # 第一優先：有效導航速限則優先使用
        if frogpilot_toggles.navspeed and detect_sl_raw > 0 and slc_source in ("Map Data", "Navigation"):
          adjusted_sl = int(round(detect_sl_raw * 1.1))
          adjusted_sl = clamp_offline_or_road_limit(adjusted_sl, source=slc_source)
          update_speed_limit(adjusted_sl, force_update=True)
        else:
          # 第二優先：根據 RoadtypeProfile 設定速限（即使ACC啟動也執行）
          if current_roadtype_profile != 0 and current_roadtype_profile in PROFILE_LIMITS:
            min_speed, max_speed = PROFILE_LIMITS[current_roadtype_profile]
            current_setspeed = self.params_memory.get_int("KeySetSpeed")

            # 若當前速限不在該Profile的範圍內，則更新為該Profile的最小速限
            if not (min_speed <= current_setspeed < max_speed):
              final_speed = min_speed
              # 平滑過渡：高速降級/速限降低時限制每秒最多降 10 km/h，避免急煞
              try:
                prev_profile = self.previous_roadtype_profile
              except AttributeError:
                prev_profile = self.params.get_int("RoadtypeProfile")
              curr_profile = self.params.get_int("RoadtypeProfile")

              now_ts = now if isinstance(now, (int, float)) else time.time()
              if not hasattr(self, "last_speed_update_ts"):
                self.last_speed_update_ts = now_ts
              dt = max(0.05, min(1.0, now_ts - self.last_speed_update_ts))

              is_highway_downgrade = (prev_profile == 4 and curr_profile in (2, 3))
              target_lower_than_current = (final_speed < current_setspeed)
              if is_highway_downgrade or target_lower_than_current:
                max_drop_per_sec = 5
                max_drop = int(round(max_drop_per_sec * dt))
                allowed_min = max(0, current_setspeed - max_drop)
                if final_speed < allowed_min:
                  final_speed = allowed_min

              self.last_speed_update_ts = now_ts
              final_speed = clamp_offline_or_road_limit(final_speed, is_road_profile=True)
              update_speed_limit(final_speed, force_update=True)

    # ---------- Stopmark 參數 ----------
    # 以 KeySetSpeed 與 slc.target(換算 km/h) 取較小者為基準，取其 50%
    # 優先順序：slcOverriddenSpeed > slcSpeedLimit > slc.target
    # 若兩者皆無有效值，基準為 30 km/h
    key_set_speed_kph = int(self.params_memory.get_int("KeySetSpeed") or 0)
    if key_set_speed_kph <= 0:
      key_set_speed_kph = int(round(sm["controlsState"].vCruise))

    # 比照 UI 層的優先順序邏輯
    slc_target_kph = 0
    if self.frogpilot_vcruise.slc.overridden_speed != 0:
      slc_target_kph = int(self.frogpilot_vcruise.slc.overridden_speed * 3.6)
    else:
      slc_target_kph = detect_sl_raw

    candidates = [s for s in (key_set_speed_kph, slc_target_kph) if s > 0]
    base_speed_kph = min(candidates) if candidates else 30

    STOPMARK_MIN_SPEED = max(30, int(round(base_speed_kph * 0.5)))

    STOPMARK_MIN_DISTANCE = 50.0  # 🔧 優化：從 10m 擴大到 50m，給予更大的有效減速範圍
    STOPMARK_MAX_DISTANCE = 600.0  # 🔧 優化：從 300m 擴大到 500m，提前更早開始減速
    # =========================================================
    # Stopmark 防抖與狀態管理（快速反應+平滑執行）
    # =========================================================
    STOPMARK_STABLE_TIME = 0.2  # 🔧 優化：從2秒減到0.5秒（10個循環），加快反應速度
    STOPMARK_UPDATE_INTERVAL = 0.1  # 🔧 優化：從0.2秒減到0.1秒（2個循環），讓減速更平滑

    if frogpilot_toggles.stopmarkslowsdown:
      stopDistance = self.params_memory.get_int("StopmarkDistance")
      stopmark_active = self.params_memory.get_bool("StopmarkActive")
      currentSpeedLimit = self.params_memory.get_int("KeySetSpeed")
      if currentSpeedLimit <= 0:
        currentSpeedLimit = int(round(sm["controlsState"].vCruise))

      # 穩定性計時器更新
      if stopmark_active:
        self.stopmark_active_timer += DT_MDL  # 累加時間
      else:
        self.stopmark_active_timer = 0.0  # 重置計時器

      # 只有當 StopmarkActive 穩定維持超過閾值時才執行邏輯
      if self.stopmark_active_timer >= STOPMARK_STABLE_TIME:
        # 第一次進入 Stopmark（初始化原始速限）
        if stopmark_active and not self.params_memory.get_bool("StopmarkApplied"):
          speed_to_save = currentSpeedLimit
          if speed_to_save <= 0:
            speed_to_save = int(round(sm["controlsState"].vCruise))
          if not frogpilot_toggles.navspeed or detect_sl_raw == 0:
            speed_to_save = min(currentSpeedLimit, 60)

          self.params_memory.put_int("OriginalKeySetSpeed", speed_to_save)
          self.params_memory.put_bool("StopmarkApplied", True)
          self.stopmark_active_prev = True

        # 持續執行降速邏輯（只要 stopmark_active 為 True 且已套用）
        if stopmark_active and self.params_memory.get_bool("StopmarkApplied"):
          # 邊界檢查：stopDistance 必須有效
          if stopDistance <= STOPMARK_MIN_DISTANCE:
            target_speed_limit = STOPMARK_MIN_SPEED
          else:
            # 計算目標降速速限
            original_speed = self.params_memory.get_int("OriginalKeySetSpeed")
            if original_speed <= 0:
              original_speed = STOPMARK_MIN_SPEED

            # 🔧 優化：非線性減速曲線（平方根函數）
            # 遠距離（靠近 600m）：減速緩和，保持舒適性
            # 近距離（靠近 50m）：積極減速，確保安全停車
            # 使用平方根讓曲線更符合人類駕駛習慣
            effective_distance_range = STOPMARK_MAX_DISTANCE - STOPMARK_MIN_DISTANCE
            distance_ratio = max(0, min(1, (stopDistance - STOPMARK_MIN_DISTANCE) / effective_distance_range))

            # 平方根曲線：讓遠距離下降幅度小，近距離下降幅度大
            smooth_ratio = math.sqrt(distance_ratio)
            speed_reduction_range = original_speed - STOPMARK_MIN_SPEED
            target_stopmark_speed = STOPMARK_MIN_SPEED + smooth_ratio * speed_reduction_range

            target_speed_limit = max(round(target_stopmark_speed), STOPMARK_MIN_SPEED)

          # 🔧 改為直接更新到目標速限（無固定降速限制，平滑由曲線決定）
          if currentSpeedLimit > target_speed_limit and (now.timestamp() - self.stopmark_last_update_time) >= STOPMARK_UPDATE_INTERVAL:
             self.params_memory.put_int("StopmarkTargetSpeed", int(round(target_speed_limit)))
             # 平滑限制：每秒最多降 5 km/h，避免急煞
             MAX_STOPMARK_DROP_PER_SEC = 5  # km/h per second
             dt = (now.timestamp() - self.stopmark_last_update_time) if self.stopmark_last_update_time > 0 else STOPMARK_UPDATE_INTERVAL
             dt = max(0.05, min(1.0, dt))  # 夾在 50ms~1s，避免異常大步長
             max_drop = max(1, int(round(MAX_STOPMARK_DROP_PER_SEC * dt)))
             allowed_min = max(0, currentSpeedLimit - max_drop)
             newSpeedLimit = max(target_speed_limit, allowed_min)
             if newSpeedLimit != currentSpeedLimit:
               new_speed_limit = clamp_speed_limit_kph(newSpeedLimit)
               if new_speed_limit != currentSpeedLimit:
                 self.params_memory.put_int("KeySetSpeed", new_speed_limit)
                 self.params_memory.put_bool("KeyChanged", True)
                 self.stopmark_last_update_time = now.timestamp()

      # 結束偵測（狀態轉換時觸發恢復流程）
      if self.stopmark_active_prev and not stopmark_active:
        self.params_memory.put_int("StopmarkTargetSpeed", 0)
        self.params_memory.put_bool("StopmarkRecovering", True)
        self.params_memory.put_bool("StopmarkActive", False)
        self.stopmark_active_prev = False
        self.stopmark_active_timer = 0.0  # 重置計時器

    # =========================================================
    # 統一恢復出口（踩油門立即恢復）
    # =========================================================
    if (sm["carState"].aEgo > 0 and self.params_memory.get_bool("StopmarkApplied")) or self.params_memory.get_bool("StopmarkRecovering"):
        restore_speed = self.params_memory.get_int("OriginalKeySetSpeed")
        final_speed = restore_speed  # 預設使用保存的速限

        # 🔧 優化：優先使用最新的導航速限
        if frogpilot_toggles.navspeed and detect_sl_raw > 0 and slc_source in ("Map Data", "Navigation"):
          final_speed = int(round(detect_sl_raw * 1.1))
          final_speed = clamp_offline_or_road_limit(final_speed, source=slc_source)
        # 備選方案：若保存的速限無效，使用當前 Profile 速限
        elif final_speed <= 0:
          current_roadtype_profile = self.params.get_int("RoadtypeProfile")
          if current_roadtype_profile != 0 and current_roadtype_profile in PROFILE_LIMITS:
            min_speed, max_speed = PROFILE_LIMITS[current_roadtype_profile]
            final_speed = min_speed

        # 使用統一更新函數
        if final_speed > 0:
          # Stopmark 恢復只允許提高速限，不降低
          current_setspeed = self.params_memory.get_int("KeySetSpeed")
          if final_speed > current_setspeed:
            update_speed_limit(final_speed, force_update=True)

        # 清除 Stopmark 狀態
        self.params_memory.put_bool("StopmarkApplied", False)
        self.params_memory.put_bool("StopmarkRecovering", False)
    #################################################################
    # 🔧 優化：速限變更偵測（統一處理，避免重複）
    if frogpilot_toggles.navspeed and v_ego_kph > 5:
      # 排除 Stopmark 相關狀態（避免衝突）
      is_stopmark_active = self.params_memory.get_bool("StopmarkApplied") or self.params_memory.get_bool("StopmarkRecovering")
      # 🔧 優化：檢查用戶按鈕保護狀態，避免覆蓋用戶手動調整
      is_button_protected = hasattr(self, 'frogpilot_vcruise') and self.frogpilot_vcruise.is_button_protected()

      if not is_stopmark_active and not is_button_protected:
        # 只在速限真正改變時才更新
        if detect_sl > 0 and detect_sl != self.detect_speed_prev:
          adjusted_sl = int(round(detect_sl * 1.1))
          adjusted_sl = clamp_offline_or_road_limit(adjusted_sl, source=slc_source)
          update_speed_limit(adjusted_sl)
        elif detect_sl == 0 and self.detect_speed_prev != 0:
          # 速限消失時重置
          self.detect_speed_prev = 0
          self.params_memory.put_int("DetectSpeedLimit", 0)

#################################################################
    if frogpilot_toggles.auto_speeddistance:
      leadtimeGapScaled = self.lead_one.dRel / max(v_ego, 1.0)
      leadtimeGapScaledInt = int(leadtimeGapScaled * 1000)
      lead_distance = self.lead_one.dRel
      prev_increased_stopped_distance = self.params.get_int("IncreasedStoppedDistance")

      if lead_distance < 10 or v_ego_kph < 10:
        self.params_memory.put_int("leadspeeddiffProfile", 0)

      if v_ego_kph > 50:
        if leadtimeGapScaledInt > 3000:
          stopping_distance = 5
        else:
          stopping_distance = 4
      elif v_ego_kph >= 30:
        if leadtimeGapScaledInt > 2000:
          stopping_distance = 3
        else:
          stopping_distance = 2
      elif v_ego_kph >= 10:
        if leadtimeGapScaledInt > 2000:
          stopping_distance = 3
        else:
          stopping_distance = 2
      else:  # v_ego_kph < 10
        if leadtimeGapScaledInt > 1000:
          stopping_distance = 1
        else:
          stopping_distance = 0

      if stopping_distance != prev_increased_stopped_distance:
        self.params.put_int("IncreasedStoppedDistance", stopping_distance)

    #################################################################
    # 超速偵測與自動調降速限
    if frogpilot_toggles.speedoverreminder:
      detect_speedlimit = self.params_memory.get_int("DetectSpeedLimit")
      speedlimit = int(detect_speedlimit * 1.1)
      speed_over = v_ego_kph >= 40 and speedlimit >= 40 and (v_ego_kph - speedlimit) >= 1
      self.speed_over = speed_over

      # 🔧 優化：當超速且啟用自動重置時，使用統一函數更新
      if speed_over and frogpilot_toggles.speedreminderreset and detect_speedlimit > 0:
          update_speed_limit(detect_speedlimit, force_update=True)
      elif v_ego_kph < 40:
          self.speed_over = False
####################################################################################
  def update_lead_status(self):
    following_lead = self.lead_one.status
#########################################
    #following_lead &= self.lead_one.dRel < self.model_length + 2
    #following_lead &= self.lead_one.dRel < self.model_length + STOP_DISTANCE
#########################################

    self.tracking_lead_filter.update(following_lead)
    return self.tracking_lead_filter.x >= THRESHOLD

  def publish(self, theme_updated, toggles_updated, sm, pm, frogpilot_toggles):
    frogpilot_plan_send = messaging.new_message("frogpilotPlan")
    frogpilot_plan_send.valid = sm.all_checks(service_list=["carState", "controlsState"])
    frogpilotPlan = frogpilot_plan_send.frogpilotPlan

    frogpilotPlan.accelerationJerk = A_CHANGE_COST * self.frogpilot_following.acceleration_jerk
    frogpilotPlan.accelerationJerkStock = A_CHANGE_COST * self.frogpilot_following.base_acceleration_jerk
    #frogpilotPlan.dangerFactor = self.frogpilot_following.danger_factor
    frogpilotPlan.dangerJerk = DANGER_ZONE_COST * self.frogpilot_following.danger_jerk
    frogpilotPlan.speedJerk = J_EGO_COST * self.frogpilot_following.speed_jerk
    frogpilotPlan.speedJerkStock = J_EGO_COST * self.frogpilot_following.base_speed_jerk
    frogpilotPlan.tFollow = self.frogpilot_following.t_follow

    frogpilotPlan.cscControllingSpeed = self.frogpilot_vcruise.csc_controlling_speed
    frogpilotPlan.cscSpeed = self.frogpilot_vcruise.csc_target
    frogpilotPlan.cscTraining = self.frogpilot_vcruise.csc.enable_training
#########################################
    frogpilotPlan.vscSpeed = self.frogpilot_vcruise.vsc_target
    frogpilotPlan.vscActive = self.frogpilot_vcruise.vsc_active
    frogpilotPlan.progressiveSpeed = self.frogpilot_vcruise.progressive_target
#########################################
    frogpilotPlan.desiredFollowDistance = self.frogpilot_following.desired_follow_distance

    frogpilotPlan.experimentalMode = self.cem.experimental_mode or self.frogpilot_vcruise.slc.experimental_mode

    frogpilotPlan.forcingStop = self.frogpilot_vcruise.forcing_stop
    frogpilotPlan.forcingStopLength = self.frogpilot_vcruise.tracked_model_length

    frogpilotPlan.frogpilotEvents = self.frogpilot_events.events.to_msg()

    frogpilotPlan.increasedStoppedDistance = frogpilot_toggles.increase_stopped_distance if not sm["frogpilotCarState"].trafficModeEnabled else 0
    if self.frogpilot_weather.weather_id != 0:
      frogpilotPlan.increasedStoppedDistance += self.frogpilot_weather.increase_stopped_distance

    frogpilotPlan.laneWidthLeft = self.lane_width_left
    frogpilotPlan.laneWidthRight = self.lane_width_right

    frogpilotPlan.lateralCheck = self.lateral_check

    frogpilotPlan.maxAcceleration = self.frogpilot_acceleration.max_accel
    frogpilotPlan.minAcceleration = self.frogpilot_acceleration.min_accel

    frogpilotPlan.redLight = self.cem.stop_light_detected

    frogpilotPlan.roadCurvature = self.road_curvature

    frogpilotPlan.slcMapSpeedLimit = self.frogpilot_vcruise.slc.map_speed_limit
    frogpilotPlan.slcMapboxSpeedLimit = self.frogpilot_vcruise.slc.mapbox_limit
    frogpilotPlan.slcNextSpeedLimit = self.frogpilot_vcruise.slc.next_speed_limit
#########################################
    frogpilotPlan.slcNextSpeedLimitDistance = self.frogpilot_vcruise.slc.next_speed_limit_distance
    frogpilotPlan.mapdSpeedLimit = params_memory.get_float("MapSpeedLimit")
#########################################
    frogpilotPlan.slcOverriddenSpeed = self.frogpilot_vcruise.slc.overridden_speed
    frogpilotPlan.slcSpeedLimit = self.frogpilot_vcruise.slc_target
    frogpilotPlan.slcSpeedLimitOffset = self.frogpilot_vcruise.slc_offset
    frogpilotPlan.slcSpeedLimitSource = self.frogpilot_vcruise.slc.source
    frogpilotPlan.speedLimitChanged = self.frogpilot_vcruise.slc.speed_limit_changed_timer > DT_MDL
    frogpilotPlan.unconfirmedSlcSpeedLimit = self.frogpilot_vcruise.slc.unconfirmed_speed_limit

    frogpilotPlan.themeUpdated = theme_updated or params_memory.get_bool("UseActiveTheme")

    frogpilotPlan.togglesUpdated = toggles_updated

    frogpilotPlan.trackingLead = self.tracking_lead

    frogpilotPlan.vCruise = self.v_cruise
    #######################################################
    frogpilotPlan.speedover = self.speed_over
    road_name = params_memory.get("RoadName", encoding="utf-8")
    frogpilotPlan.roadName = road_name if road_name else ""
    ########################################################
    frogpilotPlan.weatherDaytime = self.frogpilot_weather.is_daytime
    frogpilotPlan.weatherId = self.frogpilot_weather.weather_id

    pm.send("frogpilotPlan", frogpilot_plan_send)
