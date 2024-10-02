import cereal.messaging as messaging

from openpilot.common.conversions import Conversions as CV
from openpilot.common.params import Params

from openpilot.selfdrive.controls.lib.drive_helpers import V_CRUISE_UNSET
from openpilot.selfdrive.controls.lib.longitudinal_mpc_lib.long_mpc import A_CHANGE_COST, DANGER_ZONE_COST, J_EGO_COST, STOP_DISTANCE
from openpilot.selfdrive.controls.lib.longitudinal_planner import Lead

from openpilot.selfdrive.frogpilot.controls.lib.conditional_experimental_mode import ConditionalExperimentalMode
from openpilot.selfdrive.frogpilot.controls.lib.frogpilot_acceleration import FrogPilotAcceleration
from openpilot.selfdrive.frogpilot.controls.lib.frogpilot_events import FrogPilotEvents
from openpilot.selfdrive.frogpilot.controls.lib.frogpilot_following import FrogPilotFollowing
from openpilot.selfdrive.frogpilot.controls.lib.frogpilot_vcruise import FrogPilotVCruise
from openpilot.selfdrive.frogpilot.frogpilot_functions import MovingAverageCalculator, calculate_lane_width, calculate_road_curvature, update_frogpilot_toggles
from openpilot.selfdrive.frogpilot.frogpilot_variables import CRUISING_SPEED, MODEL_LENGTH, NON_DRIVING_GEARS, PLANNER_TIME, THRESHOLD

class FrogPilotPlanner:
  def __init__(self):
    #########################################
    self.params = Params()
    #########################################
    self.params_memory = Params("/dev/shm/params")

    self.cem = ConditionalExperimentalMode(self)
    self.frogpilot_acceleration = FrogPilotAcceleration(self)
    self.frogpilot_events = FrogPilotEvents(self)
    self.frogpilot_following = FrogPilotFollowing(self)
    self.frogpilot_vcruise = FrogPilotVCruise(self)
    self.lead_one = Lead()

    self.tracking_lead_mac = MovingAverageCalculator()

    self.lateral_check = False
    self.lead_departing = False
    self.model_stopped = False
    self.slower_lead = False
    self.taking_curve_quickly = False
    self.tracking_lead = False

    self.model_length = 0
    self.road_curvature = 1
    self.tracking_lead_distance = 0
    self.v_cruise = 0
#########################################
    self.detect_speed_prev = 0
    self.spee_dover = False
#########################################

  def update(self, carState, controlsState, frogpilotCarControl, frogpilotCarState, frogpilotNavigation, modelData, radarless_model, radarState, frogpilot_toggles):
    if radarless_model:
      model_leads = list(modelData.leadsV3)
      if len(model_leads) > 0:
        model_lead = model_leads[0]
        self.lead_one.update(model_lead.x[0], model_lead.y[0], model_lead.v[0], model_lead.a[0], model_lead.prob)
      else:
        self.lead_one.reset()
    else:
      self.lead_one = radarState.leadOne

    v_cruise = min(controlsState.vCruise, V_CRUISE_UNSET) * CV.KPH_TO_MS
    v_ego = max(carState.vEgo, 0)
############
    v_ego_kph = v_ego *3.6
    if v_ego_kph < 10 :
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
    v_lead = self.lead_one.vLead

    driving_gear = carState.gearShifter not in NON_DRIVING_GEARS

    distance_offset = max(frogpilot_toggles.increase_stopped_distance + min(10 - v_ego, 0), 0) if not frogpilotCarState.trafficModeActive else 0
    lead_distance = self.lead_one.dRel - distance_offset
    stopping_distance = STOP_DISTANCE + distance_offset

    self.frogpilot_acceleration.update(controlsState, frogpilotCarState, v_cruise, v_ego, frogpilot_toggles)

    run_cem = frogpilot_toggles.conditional_experimental_mode or frogpilot_toggles.force_stops or frogpilot_toggles.green_light_alert or frogpilot_toggles.show_stopping_point
    if run_cem and (controlsState.enabled or frogpilotCarControl.alwaysOnLateralActive) and driving_gear:
      self.cem.update(carState, frogpilotNavigation, modelData, v_ego, v_lead, frogpilot_toggles)
    else:
      self.cem.stop_light_detected = False

    self.frogpilot_events.update(carState, controlsState, frogpilotCarControl, frogpilotCarState, modelData, frogpilot_toggles)
    self.frogpilot_following.update(carState.aEgo, controlsState, frogpilotCarState, lead_distance, stopping_distance, v_ego, v_lead, frogpilot_toggles)

    check_lane_width = frogpilot_toggles.adjacent_lanes or frogpilot_toggles.adjacent_path_metrics or frogpilot_toggles.blind_spot_path or frogpilot_toggles.lane_detection
    if check_lane_width and v_ego >= frogpilot_toggles.minimum_lane_change_speed:
      self.lane_width_left = calculate_lane_width(modelData.laneLines[0], modelData.laneLines[1], modelData.roadEdges[0])
      self.lane_width_right = calculate_lane_width(modelData.laneLines[3], modelData.laneLines[2], modelData.roadEdges[1])
    else:
      self.lane_width_left = 0
      self.lane_width_right = 0

############
    if self.tracking_lead_distance < 10 :
      if frogpilot_toggles.lead_departing_alert and self.tracking_lead and driving_gear and carState.standstill:
        if self.tracking_lead_distance == 0:
          self.tracking_lead_distance = lead_distance

        self.lead_departing = lead_distance - self.tracking_lead_distance > 1
        self.lead_departing &= v_lead > 1
      else:
        self.lead_departing = False
        self.tracking_lead_distance = 0
############

    self.lateral_check = v_ego >= frogpilot_toggles.pause_lateral_below_speed
    self.lateral_check |= frogpilot_toggles.pause_lateral_below_signal and not (carState.leftBlinker or carState.rightBlinker)
    self.lateral_check |= carState.standstill

    self.model_length = modelData.position.x[MODEL_LENGTH - 1]
    self.model_stopped = self.model_length < CRUISING_SPEED * PLANNER_TIME
    self.model_stopped |= self.frogpilot_vcruise.forcing_stop

    self.road_curvature = calculate_road_curvature(modelData, v_ego) if not carState.standstill else 1

    if frogpilot_toggles.random_events and v_ego > CRUISING_SPEED and driving_gear:
      self.taking_curve_quickly = v_ego > (1 / self.road_curvature)**0.5 * 2 > CRUISING_SPEED * 2 and abs(carState.steeringAngleDeg) > 30
    else:
      self.taking_curve_quickly = False

    self.tracking_lead = self.set_lead_status(lead_distance, stopping_distance, v_ego)
    self.v_cruise = self.frogpilot_vcruise.update(carState, controlsState, frogpilotCarControl, frogpilotCarState, frogpilotNavigation, modelData, v_cruise, v_ego, frogpilot_toggles)

    if self.frogpilot_events.frame == 1:  # Force update to check the current state of "Always On Lateral" and holiday theme
      update_frogpilot_toggles()

####################################################################################
    v_ego_kph = v_ego * 3.6
    detect_sl = self.frogpilot_vcruise.slc.desired_speed_limit * 3.6
    speedlimit = int(self.params_memory.get_int('DetectSpeedLimit')*1.1)

    auto_acc = self.params.get_bool("AutoACC")
    autoacc_speed = self.params.get_int("AutoACCspeed")
    auto_acc_pass = v_ego_kph > autoacc_speed
    autoacc_caraway_status = self.params_memory.get_int("AutoACCCarAwaystatus")
    autoacc_greenlight_status = self.params_memory.get_int("AutoACCGreenLightstatus")
    current_isengaged = self.params.get_bool("IsEngaged")
    roadtype = self.params.get_bool("Roadtype")
    roadtype_profile = self.params.get_int("RoadtypeProfile")
    navspeed = self.params.get_bool("Navspeed")
    current_setspeed = self.params_memory.get_int("KeySetSpeed")
    detect_speedlimit = self.params_memory.get_int("DetectSpeedLimit")
    speedover_reminder = self.params.get_bool("speedoverreminder")
    speedreminder_reset = self.params.get_bool("speedreminderreset")

    if auto_acc and not current_isengaged:
      if (auto_acc_pass) or (autoacc_caraway_status == 1 ) or (autoacc_greenlight_status == 1 ):
        self.params_memory.put_bool("KeyResume", True)
        self.params_memory.put_bool("KeyChanged", True)
        self.params_memory.put_int("AutoACCCarAwaystatus", 0)
        self.params_memory.put_int("AutoACCGreenLightstatus", 0)

        if detect_speedlimit != 0 and roadtype_profile != 0:
          if navspeed  :
              self.params_memory.put_bool("SpeedLimitChanged", True)
        else:
          if roadtype:
            key_set_speed = 0
            if roadtype_profile == 1 and (current_setspeed < 40 or current_setspeed >= 60 ):
              key_set_speed = 40
            elif roadtype_profile == 2 and (current_setspeed < 60 or current_setspeed >= 90 ):
              key_set_speed = 60
            elif roadtype_profile == 3 and (current_setspeed < 90 or current_setspeed >= 120 ):
              key_set_speed = 90
            elif roadtype_profile == 4 and current_setspeed < 120:
              key_set_speed = 120
            if key_set_speed > 0:
              self.params_memory.put_int("KeySetSpeed", key_set_speed)
              self.params_memory.put_bool("KeyChanged", True)
              self.params_memory.put_int("SpeedPrev", 0)

    #################################################################
    # 速限變更偵測
    if navspeed :
      if detect_sl != self.detect_speed_prev and v_ego_kph > 5:
        if detect_sl > 0:
          self.params_memory.put_int("DetectSpeedLimit", detect_sl)
          self.params_memory.put_bool("SpeedLimitChanged", True)
          self.detect_speed_prev = detect_sl
        else:
          self.detect_speed_prev = 0
          self.params_memory.put_int("DetectSpeedLimit", 0 )
      else:
        self.params_memory.put_bool("SpeedLimitChanged", False)
    #超速偵測
    if speedover_reminder :
      if v_ego_kph >=40 and speedlimit >= 40 :
        if (v_ego_kph - speedlimit) >= 1:
          self.spee_dover = True
          if detect_speedlimit !=0 and speedreminder_reset:
            if detect_speedlimit <40:
              self.params_memory.put_int("DetectSpeedLimit",40)
            else:
              self.params_memory.put_bool("SpeedLimitChanged", True)
        else:
          self.spee_dover = False
      elif v_ego_kph <40 :
        self.spee_dover = False
    #print("speed_limit=", detect_sl)
####################################################################################

  def set_lead_status(self, lead_distance, stopping_distance, v_ego):
    following_lead = self.lead_one.status
    following_lead &= 1 < lead_distance < self.model_length + stopping_distance
    following_lead &= v_ego > CRUISING_SPEED or self.tracking_lead

    self.tracking_lead_mac.add_data(following_lead)
    return self.tracking_lead_mac.get_moving_average() >= THRESHOLD

  def publish(self, sm, pm, frogpilot_toggles):
    frogpilot_plan_send = messaging.new_message('frogpilotPlan')
    frogpilot_plan_send.valid = sm.all_checks(service_list=['carState', 'controlsState'])
    frogpilotPlan = frogpilot_plan_send.frogpilotPlan

    frogpilotPlan.accelerationJerk = float(A_CHANGE_COST * self.frogpilot_following.acceleration_jerk)
    frogpilotPlan.accelerationJerkStock = float(A_CHANGE_COST * self.frogpilot_following.base_acceleration_jerk)
    frogpilotPlan.dangerJerk = float(DANGER_ZONE_COST * self.frogpilot_following.danger_jerk)
    frogpilotPlan.speedJerk = float(J_EGO_COST * self.frogpilot_following.speed_jerk)
    frogpilotPlan.speedJerkStock = float(J_EGO_COST * self.frogpilot_following.base_speed_jerk)
    frogpilotPlan.tFollow = float(self.frogpilot_following.t_follow)

    frogpilotPlan.adjustedCruise = float(min(self.frogpilot_vcruise.mtsc_target, self.frogpilot_vcruise.vtsc_target) * (CV.MS_TO_KPH if frogpilot_toggles.is_metric else CV.MS_TO_MPH))
    frogpilotPlan.vtscControllingCurve = bool(self.frogpilot_vcruise.mtsc_target > self.frogpilot_vcruise.vtsc_target)

    frogpilotPlan.desiredFollowDistance = self.frogpilot_following.safe_obstacle_distance - self.frogpilot_following.stopped_equivalence_factor
    frogpilotPlan.safeObstacleDistance = self.frogpilot_following.safe_obstacle_distance
    frogpilotPlan.safeObstacleDistanceStock = self.frogpilot_following.safe_obstacle_distance_stock
    frogpilotPlan.stoppedEquivalenceFactor = self.frogpilot_following.stopped_equivalence_factor

    frogpilotPlan.experimentalMode = self.cem.experimental_mode or self.frogpilot_vcruise.slc.experimental_mode

    frogpilotPlan.forcingStop = self.frogpilot_vcruise.forcing_stop

    frogpilotPlan.frogpilotEvents = self.frogpilot_events.events.to_msg()

    frogpilotPlan.laneWidthLeft = self.lane_width_left
    frogpilotPlan.laneWidthRight = self.lane_width_right

    frogpilotPlan.lateralCheck = self.lateral_check

    frogpilotPlan.maxAcceleration = float(self.frogpilot_acceleration.max_accel)
    frogpilotPlan.minAcceleration = float(self.frogpilot_acceleration.min_accel)

    frogpilotPlan.redLight = bool(self.cem.stop_light_detected)

    frogpilotPlan.slcOverridden = bool(self.frogpilot_vcruise.override_slc)
    frogpilotPlan.slcOverriddenSpeed = float(self.frogpilot_vcruise.overridden_speed)
    frogpilotPlan.slcSpeedLimit = self.frogpilot_vcruise.slc_target
    frogpilotPlan.slcSpeedLimitOffset = self.frogpilot_vcruise.slc.offset
    frogpilotPlan.speedLimitChanged = self.frogpilot_vcruise.speed_limit_changed
    frogpilotPlan.unconfirmedSlcSpeedLimit = self.frogpilot_vcruise.slc.desired_speed_limit

    frogpilotPlan.vCruise = self.v_cruise
    #######################################################
    frogpilotPlan.speedover = self.spee_dover
    ########################################################

    pm.send('frogpilotPlan', frogpilot_plan_send)
