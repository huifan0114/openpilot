#!/usr/bin/env python3
import json
import math

from cereal import log
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

    check_lane_width = frogpilot_toggles.adjacent_paths or frogpilot_toggles.adjacent_path_metrics or frogpilot_toggles.blind_spot_path or frogpilot_toggles.lane_detection
    if check_lane_width and v_ego >= frogpilot_toggles.minimum_lane_change_speed:
      self.lane_width_left = calculate_lane_width(sm["modelV2"].laneLines[0], sm["modelV2"].laneLines[1], sm["modelV2"].roadEdges[0])
      self.lane_width_right = calculate_lane_width(sm["modelV2"].laneLines[3], sm["modelV2"].laneLines[2], sm["modelV2"].roadEdges[1])
    else:
      self.lane_width_left = 0
      self.lane_width_right = 0

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
    PROFILE_LIMITS = {1: (40, 59), 2: (60, 89), 3: (90, 119), 4: (120, float("inf"))}
    # ---------- Stopmark 參數 ----------
    STOPMARK_MIN_SPEED = 10.0
    STOPMARK_MIN_DISTANCE = 10.0
    STOPMARK_MAX_DISTANCE = 150.0
    # ------------autoacc--------------
    if frogpilot_toggles.autoacc and not current_isengaged :
      autoacc_caraway_status = self.params_memory.get_int("AutoACCCarAwaystatus")
      autoacc_greenlight_status = self.params_memory.get_int("AutoACCGreenLightstatus")
      auto_acc_pass = v_ego_kph > frogpilot_toggles.autoacc_speed
      if auto_acc_pass or autoacc_caraway_status == 1 or autoacc_greenlight_status == 1:
        has_map_or_nav_sl = frogpilot_toggles.navspeed and detect_sl_raw > 0 and slc_source in ("Map Data", "Navigation")
        self.params_memory.put_bool("KeyResume", True)
        self.params_memory.put_bool("KeyChanged", True)
        self.params_memory.put_int("AutoACCCarAwaystatus", 0)
        self.params_memory.put_int("AutoACCGreenLightstatus", 0)
        self.params_memory.put_bool("StopmarkApplied", False)
        # 速限變更邏輯（AutoACC 觸發時，每次都檢查）
        # 優先序：1) Map Data / Navigation -> 2) roadtype_profile名
        if has_map_or_nav_sl:
          # 整合變更偵測：只在速限真正變化時才設置 flag
          if detect_sl_raw != self.detect_speed_prev:
            self.detect_speed_prev = detect_sl_raw
            self.params_memory.put_int("DetectSpeedLimit", detect_sl_raw)
            self.params_memory.put_bool("SpeedLimitChanged", True)
            self.params_memory.put_int("KeySetSpeed", detect_sl_raw)
            self.params_memory.put_bool("KeyChanged", True)
        else:
          current_setspeed = self.params_memory.get_int("KeySetSpeed")
          roadtype_profile = self.params.get_int("RoadtypeProfile")
          key_set_speed = 0
          # 2) 用 profile 兜底
          if key_set_speed == 0 and roadtype_profile != 0:
            profile = roadtype_profile
            if profile in PROFILE_LIMITS:
              min_speed, max_speed = PROFILE_LIMITS[profile]
              if not (min_speed <= current_setspeed < max_speed):
                key_set_speed = min_speed

          if key_set_speed > 0:
            # 若有有效的即時偵測速限（navspeed 啟用且 map/nav 來源），則用其夾住路名結果
            # 避免拿過期的 detect_speedlimit（可能是高速留下的舊值）來放大路名建議
            if frogpilot_toggles.navspeed and detect_sl_raw > 0 and slc_source in ("Map Data", "Navigation"):
              key_set_speed = min(key_set_speed, detect_sl_raw)
            self.params_memory.put_int("KeySetSpeed", key_set_speed)
            self.params_memory.put_bool("KeyChanged", True)

    # =========================================================
    # 道路名稱與道路類型檔案變更檢測（當道路改變時，即時更新速限）
    # =========================================================
    current_road_name = self.params_memory.get("RoadName", encoding="utf8")
    current_roadtype_profile = self.params.get_int("RoadtypeProfile")

    # 檢測道路名稱或道路類型檔案是否改變
    road_changed = (current_road_name != self.previous_road_name) or (current_roadtype_profile != self.previous_roadtype_profile)

    if road_changed:
      self.previous_road_name = current_road_name
      self.previous_roadtype_profile = current_roadtype_profile

      # 強制重新計算速限（重置 detect_speed_prev 以觸發速限更新）
      # 這樣即使速限值相同，也會因為道路名稱或道路類型改變而更新
      if frogpilot_toggles.navspeed and detect_sl_raw > 0:
        self.params_memory.put_int("DetectSpeedLimit", detect_sl_raw)
        self.params_memory.put_bool("SpeedLimitChanged", True)
        current_setspeed_val = self.params_memory.get_int("KeySetSpeed")
        if current_setspeed_val != detect_sl_raw:
          self.params_memory.put_int("KeySetSpeed", detect_sl_raw)
          self.params_memory.put_bool("KeyChanged", True)
    # =========================================================
    # Stopmark 防抖與狀態管理（需穩定維持 1 秒以上才觸發）
    # =========================================================
    STOPMARK_STABLE_TIME = 1.0  # 需要穩定維持 1 秒才觸發（降低體感延遲）
    STOPMARK_UPDATE_INTERVAL = 0.2  # 最小更新間隔（秒），降低高頻寫入導致的 UI 重繪

    if frogpilot_toggles.stopmarkslowsdown:
      stopDistance = self.params_memory.get_int("StopmarkDistance")
      stopmark_active = self.params_memory.get_bool("StopmarkActive")
      currentSpeedLimit = self.params_memory.get_int("KeySetSpeed")

      # 穩定性計時器更新
      if stopmark_active:
        self.stopmark_active_timer += DT_MDL  # 累加時間
      else:
        self.stopmark_active_timer = 0.0  # 重置計時器

      # 只有當 StopmarkActive 穩定維持超過閾值時才執行邏輯
      if self.stopmark_active_timer >= STOPMARK_STABLE_TIME:
        prev_stopmark_active = self.stopmark_active_prev
        if stopmark_active != prev_stopmark_active:
          self.stopmark_active_prev = stopmark_active

          # 第一次進入 Stopmark（穩定後）
          if not self.params_memory.get_bool("StopmarkApplied"):
            speed_to_save = currentSpeedLimit
            if not frogpilot_toggles.navspeed or detect_sl_raw == 0:
              speed_to_save = min(currentSpeedLimit, 60)

            self.params_memory.put_int("OriginalKeySetSpeed", speed_to_save)
            self.params_memory.put_bool("StopmarkApplied", True)
        else:
          # 狀態未改變，繼續漸進式降速（需 StopmarkActive 且已套用）
          if stopmark_active and self.params_memory.get_bool("StopmarkApplied"):
            # 邊界檢查：stopDistance 必須有效
            if stopDistance <= STOPMARK_MIN_DISTANCE:
              target_speed_limit = STOPMARK_MIN_SPEED
            else:
              # 計算目標降速速限
              original_speed = self.params_memory.get_int("OriginalKeySetSpeed")
              if original_speed <= 0:
                original_speed = STOPMARK_MIN_SPEED

              target_stopmark_speed = STOPMARK_MIN_SPEED + (
                  (stopDistance - STOPMARK_MIN_DISTANCE)
                  * (original_speed - STOPMARK_MIN_SPEED)
                  / (STOPMARK_MAX_DISTANCE - STOPMARK_MIN_DISTANCE)
              )
              target_speed_limit = max(round(target_stopmark_speed), STOPMARK_MIN_SPEED)

            # 🔧 改為漸進式降速（每次最多降 2 km/h，加快反應）
            MAX_SPEED_DECREASE = 5  # 每個週期最多降低 2 km/h

            if currentSpeedLimit > target_speed_limit and (now.timestamp() - self.stopmark_last_update_time) >= STOPMARK_UPDATE_INTERVAL:
              newSpeedLimit = max(currentSpeedLimit - MAX_SPEED_DECREASE, target_speed_limit)
              if newSpeedLimit != currentSpeedLimit:
                self.params_memory.put_int("KeySetSpeed", newSpeedLimit)
                self.params_memory.put_bool("KeyChanged", True)
                self.stopmark_last_update_time = now.timestamp()

      # 結束偵測（狀態轉換時觸發恢復流程）
      prev_stopmark_active = self.stopmark_active_prev
      if prev_stopmark_active and not stopmark_active:
        self.params_memory.put_bool("StopmarkRecovering", True)
        self.params_memory.put_bool("StopmarkActive", False)
        self.stopmark_active_prev = stopmark_active
        self.stopmark_active_timer = 0.0  # 重置計時器

    # =========================================================
    # 統一恢復出口（踩油門立即恢復）
    # =========================================================
    if (sm["carState"].aEgo > 0 and self.params_memory.get_bool("StopmarkApplied")) or self.params_memory.get_bool("StopmarkRecovering"):
        # 使用 ACC 啟動時判斷的速限
        restore_speed = self.params_memory.get_int("OriginalKeySetSpeed")

        # 優先使用最新的導航速限（僅在速限真正變化時才通知）
        if frogpilot_toggles.navspeed and detect_sl_raw > 0 and slc_source in ("Map Data", "Navigation"):
            # 只在速限真的改變時才設置變更 flag
            if detect_sl_raw != self.detect_speed_prev:
                self.params_memory.put_int("DetectSpeedLimit", detect_sl_raw)
                self.params_memory.put_bool("SpeedLimitChanged", True)
                self.detect_speed_prev = detect_sl_raw
            else:
                # 速限未變，靜默更新
                self.params_memory.put_int("DetectSpeedLimit", detect_sl_raw)
        elif restore_speed > 0:
            # 恢復到 ACC 啟動時判斷的速限（不觸發變更提示）
            current_setspeed_val = self.params_memory.get_int("KeySetSpeed")
            if restore_speed != current_setspeed_val:
              self.params_memory.put_int("KeySetSpeed", restore_speed)
              self.params_memory.put_bool("KeyChanged", True)
            # self.params_memory.put_int("SpeedPrev", 0)

        # 重置偵測狀態（使用當前值避免異常觸發）
        if detect_sl_raw > 0 and self.detect_speed_prev != detect_sl_raw:
            self.detect_speed_prev = detect_sl_raw

        # 清除 Stopmark 狀態
        self.params_memory.put_bool("StopmarkApplied", False)
        self.params_memory.put_bool("StopmarkRecovering", False)
    #################################################################
    # 速限變更偵測（僅在未被 AutoACC 處理時執行，避免重複設置）
    if frogpilot_toggles.navspeed and v_ego_kph > 5:
      if detect_sl != self.detect_speed_prev:
        self.detect_speed_prev = detect_sl  # 無論如何都更新，避免重複進入
        # 只在有有效速限時才更新 DetectSpeedLimit 和設置 flag
        if detect_sl > 0:
          self.params_memory.put_int("DetectSpeedLimit", detect_sl)
          self.params_memory.put_bool("SpeedLimitChanged", True)
          current_setspeed_val = self.params_memory.get_int("KeySetSpeed")
          if detect_sl != current_setspeed_val:
            self.params_memory.put_int("KeySetSpeed", detect_sl)
            self.params_memory.put_bool("KeyChanged", True)
        else:
          self.detect_speed_prev = 0
          self.params_memory.put_int("DetectSpeedLimit", 0 )

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
      speedlimit = int(self.params_memory.get_int('DetectSpeedLimit')*1.1)
      speed_over = v_ego_kph >= 40 and speedlimit >= 40 and (v_ego_kph - speedlimit) >= 1
      self.speed_over = speed_over

      # 當超速且啟用自動重置時，調降設定速度到當前偵測速限
      if speed_over and frogpilot_toggles.speedreminderreset and detect_speedlimit > 0:
          # 使用原始偵測速限重置（drive_helpers 會自動套用 +10%）
          self.params_memory.put_int("DetectSpeedLimit", detect_speedlimit)
          self.params_memory.put_bool("SpeedLimitChanged", True)
      elif v_ego_kph < 40:
          self.speed_over = False
####################################################################################
  def update_lead_status(self):
    following_lead = self.lead_one.status
    following_lead &= self.lead_one.dRel < self.model_length + STOP_DISTANCE

    self.tracking_lead_filter.update(following_lead)
    return self.tracking_lead_filter.x >= THRESHOLD

  def publish(self, theme_updated, toggles_updated, sm, pm, frogpilot_toggles):
    frogpilot_plan_send = messaging.new_message("frogpilotPlan")
    frogpilot_plan_send.valid = sm.all_checks(service_list=["carState", "controlsState"])
    frogpilotPlan = frogpilot_plan_send.frogpilotPlan

    frogpilotPlan.accelerationJerk = A_CHANGE_COST * self.frogpilot_following.acceleration_jerk
    frogpilotPlan.accelerationJerkStock = A_CHANGE_COST * self.frogpilot_following.base_acceleration_jerk
    frogpilotPlan.dangerFactor = self.frogpilot_following.danger_factor
    frogpilotPlan.dangerJerk = DANGER_ZONE_COST * self.frogpilot_following.danger_jerk
    frogpilotPlan.speedJerk = J_EGO_COST * self.frogpilot_following.speed_jerk
    frogpilotPlan.speedJerkStock = J_EGO_COST * self.frogpilot_following.base_speed_jerk
    frogpilotPlan.tFollow = self.frogpilot_following.t_follow

    frogpilotPlan.cscControllingSpeed = self.frogpilot_vcruise.csc_controlling_speed
    frogpilotPlan.cscSpeed = self.frogpilot_vcruise.csc_target
    frogpilotPlan.cscTraining = self.frogpilot_vcruise.csc.enable_training

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
    ########################################################
    frogpilotPlan.weatherDaytime = self.frogpilot_weather.is_daytime
    frogpilotPlan.weatherId = self.frogpilot_weather.weather_id

    pm.send("frogpilotPlan", frogpilot_plan_send)
