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
    self.spee_dover = False
    self.previous_road_name = ""  # 記錄上次的路名，避免重複設定
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

####################################################################################
    # 从 SpeedLimitController 获取速限和来源
    detect_sl_raw = int(self.frogpilot_vcruise.slc.target * 3.6) if self.frogpilot_vcruise.slc.target > 0 else 0
    slc_source = self.frogpilot_vcruise.slc.source
    current_isengaged = self.params.get_bool("IsEngaged")

    autoacc_caraway_status = self.params_memory.get_int("AutoACCCarAwaystatus")
    autoacc_greenlight_status = self.params_memory.get_int("AutoACCGreenLightstatus")

    detect_sl = int(self.frogpilot_vcruise.slc.target * 3.6) if self.frogpilot_vcruise.slc.target > 0 else 0
    speedlimit = int(self.params_memory.get_int('DetectSpeedLimit')*1.1)
    detect_speedlimit = self.params_memory.get_int("DetectSpeedLimit")
    # stopmark_on = self.params_memory.get_bool("StopmarkOn")  # 讀取停止標記狀態
    # stopDistance = self.params_memory.get_int("stopmarkDistance")



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

    if frogpilot_toggles.autoacc and not current_isengaged :
      auto_acc_pass = v_ego_kph > frogpilot_toggles.autoacc_speed
      if auto_acc_pass or autoacc_caraway_status == 1 or autoacc_greenlight_status == 1:
        self.params_memory.put_bool("KeyResume", True)
        self.params_memory.put_bool("KeyChanged", True)
        self.params_memory.put_int("AutoACCCarAwaystatus", 0)
        self.params_memory.put_int("AutoACCGreenLightstatus", 0)
        # self.params_memory.put_bool("StopmarkApplied", False)

        # 速限變更邏輯
        if detect_speedlimit != 0 and frogpilot_toggles.roadtype_profile != 0:
          if frogpilot_toggles.navspeed:
            # 根据速限来源决定是否加10%
            # Map Data（离线地图）或 Navigation（导航）来源时自动+10%
            if detect_sl_raw > 0 and (slc_source == "Map Data" or slc_source == "Navigation"):
              detect_sl_adjusted = int(detect_sl_raw * 1.1)  # 加10%
              # 限制在 40-120 km/h 范围内
              detect_sl = max(40, min(120, detect_sl_adjusted))
            else:
              detect_sl = detect_sl_raw
            self.params_memory.put_bool("SpeedLimitChanged", True)
        elif detect_speedlimit == 0 and frogpilot_toggles.roadtype_profile != 0:
          # 无速限来源时的备用处理逻辑
          if frogpilot_toggles.roadtype:
            # 根據路名自動判斷道路類型並設定速限與優先級
            road_name = self.params_memory.get("RoadName", encoding="utf-8") or ""
            key_set_speed = 0
            suggested_speed = 0

            # 只有在路名改變時才重新判斷
            if road_name and road_name != self.previous_road_name:
              current_setspeed = self.params_memory.get_int('KeySetSpeed')
              # 判斷道路類型並設定建議速限與 SLC 優先級模式
              if "高速" in road_name or "國道" in road_name:
                # 高速公路：使用 Highest 模式（選擇最高速限）
                suggested_speed = 120
                self.params.put("SLCPriority1", "Highest")
                self.params.put("SLCPriority2", "None")
                self.params.put("SLCPriority3", "None")
              elif "快速" in road_name:
                # 快速道路：建議 70-80 km/h，使用 Map Data → Navigation → Dashboard
                suggested_speed = 80
                self.params.put("SLCPriority1", "Map Data")
                self.params.put("SLCPriority2", "Navigation")
                self.params.put("SLCPriority3", "Dashboard")
              elif "交流道" in road_name:
                # 交流道：建議 60 km/h，使用 Map Data → Navigation → Dashboard
                suggested_speed = 50
                self.params.put("SLCPriority1", "Map Data")
                self.params.put("SLCPriority2", "Navigation")
                self.params.put("SLCPriority3", "Dashboard")
              elif "街" in road_name or "巷" in road_name or "弄" in road_name:
                # 市區道路：建議 40-50 km/h，使用 Map Data → Navigation → Dashboard
                suggested_speed = 40
                self.params.put("SLCPriority1", "Map Data")
                self.params.put("SLCPriority2", "Navigation")
                self.params.put("SLCPriority3", "Dashboard")
              else:
                # 鄉村/一般道路：建議 50 km/h，使用 Map Data → Navigation → Dashboard
                suggested_speed = 50
                self.params.put("SLCPriority1", "Map Data")
                self.params.put("SLCPriority2", "Navigation")
                self.params.put("SLCPriority3", "Dashboard")
              self.params.putBool("FrogPilotTogglesUpdated", True)

              # 記錄當前路名
              self.previous_road_name = road_name

              # 如果根據路名判斷出建議速限，且與當前設定不同，則更新
              if suggested_speed > 0 and current_setspeed != suggested_speed:
                key_set_speed = suggested_speed
            else:
              # 沒有路名時使用原有的 profile 邏輯
              profile_limits = {1: (40, 60), 2: (60, 90), 3: (90, 120), 4: (120, float("inf"))}
              profile = frogpilot_toggles.roadtype_profile
              current_setspeed = self.params_memory.get_int('KeySetSpeed')

              if profile in profile_limits:
                min_speed, max_speed = profile_limits[profile]
                if not (min_speed <= current_setspeed < max_speed):
                  key_set_speed = min_speed

            if key_set_speed > 0:
              self.params_memory.put_int("KeySetSpeed", key_set_speed)
              self.params_memory.put_bool("KeyChanged", True)
              self.params_memory.put_int("SpeedPrev", 0)
          else:
            # 既無速限來源，也未啟用路名自動判斷，使用基礎 profile 邏輯
            profile_limits = {1: (40, 60), 2: (60, 90), 3: (90, 120), 4: (120, float("inf"))}
            profile = frogpilot_toggles.roadtype_profile
            current_setspeed = self.params_memory.get_int('KeySetSpeed')

            if profile in profile_limits:
              min_speed, max_speed = profile_limits[profile]
              if not (min_speed <= current_setspeed < max_speed):
                key_set_speed = min_speed
                self.params_memory.put_int("KeySetSpeed", key_set_speed)
                self.params_memory.put_bool("KeyChanged", True)
                self.params_memory.put_int("SpeedPrev", 0)

    # if stopmark_on:
    #   minSpeedLimit = 10.0   # 最低速限
    #   maxDistance = 100.0    # 最大距離
    #   minDistance = 10.0     # 最小距離

    #   # 取得當前速限
    #   currentSpeedLimit = self.params_memory.get_int("KeySetSpeed")

    #   # **只在 stopmark_on 第一次啟動時記錄當前速限**
    #   if not self.params_memory.get_bool("StopmarkApplied"):
    #       self.params_memory.put_int("OriginalKeySetSpeed", currentSpeedLimit)
    #       self.params_memory.put_bool("StopmarkApplied", True)

    #   # **以當時速限作為最大速限**
    #   maxSpeedLimit = currentSpeedLimit

    #   # **計算線性降速**
    #   stopmarkspeedLimit = minSpeedLimit + (stopDistance - minDistance) * (maxSpeedLimit - minSpeedLimit) / (maxDistance - minDistance)
    #   newSpeedLimit = round(stopmarkspeedLimit)

    #   # **只有當前速限比新計算的速限高時，才更新**
    #   if currentSpeedLimit > newSpeedLimit:
    #       self.params_memory.put_int("KeySetSpeed", newSpeedLimit)
    #       self.params_memory.put_bool("KeyChanged", True)
    #       self.params_memory.put_int("SpeedPrev", 0)
    #       self.params_memory.put_bool("StopmarkOn", False)

    #   # **確保恢復機制可再次執行**
    #   self.params_memory.put_bool("StopmarkRestored", False)

    # else:
    #   # **只執行一次恢復邏輯**
    #   if not self.params_memory.get_bool("StopmarkRestored") and (autoacc_caraway_status == 1 or autoacc_greenlight_status == 1):
    #     originalSpeedLimit = self.params_memory.get_int("OriginalKeySetSpeed")
    #     self.params_memory.put_bool("StopmarkApplied", False)  # 清除 Stopmark 記錄

    #     if frogpilot_toggles.navspeed:
    #       self.params_memory.put_bool("SpeedLimitChanged", True)
    #     elif originalSpeedLimit > 0:
    #       self.params_memory.put_int("KeySetSpeed", originalSpeedLimit)
    #       self.params_memory.put_bool("KeyChanged", True)
    #       self.params_memory.put_int("SpeedPrev", 0)

    #     self.params_memory.put_bool("StopmarkRestored", True)  # 避免重複執行
        # # **恢復原本的 KeySetSpeed**
        # if self.params_memory.get_bool('StopmarkApplied'):
        #     originalSpeedLimit = self.params_memory.get_int('OriginalKeySetSpeed')
        #     self.params_memory.put_bool("StopmarkApplied", False)  # 清除標記
        # else:
        #     # **如果沒有存過原始速限,則預設為當前 KeySetSpeed**
        #     originalSpeedLimit = self.params_memory.get_int('KeySetSpeed')

        # # **直接恢復為當時的最高速限**
        # key_set_speed = originalSpeedLimit

        # # **更新速限**
        # if key_set_speed > 0:
        #     self.params_memory.put_int("KeySetSpeed", key_set_speed)
        #     self.params_memory.put_bool("KeyChanged", True)
        #     self.params_memory.put_int("SpeedPrev", 0)

        # #**設定 StopmarkRestored，避免重複執行**
        # self.params_memory.put_bool("StopmarkRestored", True)
    #################################################################
    # 速限變更偵測
    if frogpilot_toggles.navspeed:
      if detect_sl != self.detect_speed_prev and v_ego_kph > 5:
        self.detect_speed_prev = detect_sl if detect_sl > 0 else 0
        self.params_memory.put_int("DetectSpeedLimit", self.detect_speed_prev)
        # 只有當速限有效時才設定 SpeedLimitChanged
        if detect_sl > 0:
          self.params_memory.put_bool("SpeedLimitChanged", True)
    #超速偵測
    if frogpilot_toggles.speedoverreminder:
      speed_over = v_ego_kph >= 40 and speedlimit >= 40 and (v_ego_kph - speedlimit) >= 1
      self.speed_over = speed_over  # 修正變數名稱
      # detect_speedlimit = self.params_memory.get_int('DetectSpeedLimit')

      if speed_over and detect_speedlimit != 0 and frogpilot_toggles.speedreminderreset:
          self.params_memory.put_int("DetectSpeedLimit", max(detect_speedlimit, 40))
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
    frogpilotPlan.speedover = self.spee_dover
    ########################################################
    frogpilotPlan.weatherDaytime = self.frogpilot_weather.is_daytime
    frogpilotPlan.weatherId = self.frogpilot_weather.weather_id

    pm.send("frogpilotPlan", frogpilot_plan_send)
