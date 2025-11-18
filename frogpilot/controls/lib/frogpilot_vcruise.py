#!/usr/bin/env python3
from openpilot.common.conversions import Conversions as CV
from openpilot.common.realtime import DT_MDL
from openpilot.selfdrive.controls.lib.longitudinal_mpc_lib.long_mpc import COMFORT_BRAKE

from openpilot.frogpilot.common.frogpilot_variables import CRUISING_SPEED, PLANNER_TIME
from openpilot.frogpilot.controls.lib.curve_speed_controller import CurveSpeedController
from openpilot.frogpilot.controls.lib.speed_limit_controller import SpeedLimitController

class FrogPilotVCruise:
  def __init__(self, FrogPilotPlanner):
    self.frogpilot_planner = FrogPilotPlanner

    self.csc = CurveSpeedController(self)
    self.slc = SpeedLimitController()

    # Progressive speed controller - dynamic step-by-step acceleration
    self.progressive_target = 0
    self.progressive_enabled = False

    self.forcing_stop = False
    self.override_force_stop = False

    self.override_force_stop_timer = 0

  def update_progressive_speed(self, v_ego, v_cruise_final):
    """
    漸進式速度控制：避免一次性大幅加速
    當目標速度與當前速度差距大時，分段逐步提升目標速度
    例如：時速 20 km/h 要加速到 60 km/h 時，會分成 30, 40, 50, 60 逐步進行

    :param v_ego: 當前速度 (m/s)
    :param v_cruise_final: 最終目標速度 (m/s)
    :return: 當前階段目標速度 (m/s)
    """
    from openpilot.common.conversions import Conversions as CV
    import math

    STEP_SIZE = 10 * CV.KPH_TO_MS  # 每次增加 10 km/h
    APPROACH_THRESHOLD = 2 * CV.KPH_TO_MS  # 接近閾值 2 km/h（提前升段更流暢）
    MIN_STEP_TRIGGER = 10 * CV.KPH_TO_MS  # 最小觸發差距 10 km/h

    # 計算下一個對齊到 10 的倍數的目標速度（最小 20 km/h）
    def calculate_next_target(v_ego_ms):
      v_ego_kph = v_ego_ms * CV.MS_TO_KPH
      # 對齊到最近的 10 的倍數（向上取整）
      # 例如: 38 km/h → 40, 40 km/h → 40, 42 km/h → 50
      next_target_kph = math.ceil(v_ego_kph / 10) * 10
      # 確保最小目標速度為 20 km/h（避免 0→10 太慢）
      next_target_kph = max(next_target_kph, 20)
      return next_target_kph * CV.KPH_TO_MS

    speed_diff = v_cruise_final - v_ego

    # 如果速度差距小於觸發閾值，直接使用最終目標
    if speed_diff < MIN_STEP_TRIGGER:
      self.progressive_enabled = False
      return v_cruise_final

    # 煞車場景檢測 - 如果當前速度比目標低太多，重新設定
    reset_target = False
    if self.progressive_enabled and v_ego < (self.progressive_target - STEP_SIZE):
      # 煞車了，重新設定漸進目標（對齊到 10 的倍數）
      self.progressive_target = calculate_next_target(v_ego)
      reset_target = True

    # 初始化或重新設定漸進目標（當目標速度降低時也重置）
    if not self.progressive_enabled or self.progressive_target > v_cruise_final:
      self.progressive_enabled = True
      self.progressive_target = calculate_next_target(v_ego)
      reset_target = True

    # 如果接近當前漸進目標，增加下一個步進
    # 但如果剛剛重置過，就不要增加（避免立即跳升 10）
    if not reset_target and v_ego >= (self.progressive_target - APPROACH_THRESHOLD):
      self.progressive_target = min(self.progressive_target + STEP_SIZE, v_cruise_final)

    # 確保不超過最終目標
    self.progressive_target = min(self.progressive_target, v_cruise_final)

    return self.progressive_target

  def update(self, gps_position, now, time_validated, v_cruise, v_ego, sm, frogpilot_toggles):
    force_stop = self.frogpilot_planner.cem.stop_light_detected and sm["controlsState"].enabled and frogpilot_toggles.force_stops
    force_stop &= self.frogpilot_planner.model_stopped
    force_stop &= self.override_force_stop_timer <= 0

    self.force_stop_timer = self.force_stop_timer + DT_MDL if force_stop else 0

    force_stop_enabled = self.force_stop_timer >= 1

    self.override_force_stop |= sm["carState"].gasPressed
    self.override_force_stop |= sm["frogpilotCarState"].accelPressed
    self.override_force_stop &= force_stop_enabled

    if self.override_force_stop:
      self.override_force_stop_timer = 10
    elif self.override_force_stop_timer > 0:
      self.override_force_stop_timer -= DT_MDL

    v_cruise_cluster = max(sm["controlsState"].vCruiseCluster * CV.KPH_TO_MS, v_cruise)
    v_cruise_diff = v_cruise_cluster - v_cruise

    v_ego_cluster = max(sm["carState"].vEgoCluster, v_ego)
    v_ego_diff = v_ego_cluster - v_ego

    # FrogsGoMoo's Curve Speed Controller
    if v_ego > CRUISING_SPEED and sm["controlsState"].enabled and self.frogpilot_planner.road_curvature_detected and frogpilot_toggles.curve_speed_controller:
      self.csc.update_target(v_ego)

      self.csc_controlling_speed = True

      self.csc_target = self.csc.target
    else:
      self.csc.log_data(v_ego, sm)

      self.csc_controlling_speed = False
      self.csc.target_set = False

      self.csc_target = v_cruise

    # Mike's extended lead linear braking
    if self.frogpilot_planner.lead_one.vLead < v_ego > CRUISING_SPEED and sm["controlsState"].enabled and self.frogpilot_planner.tracking_lead and frogpilot_toggles.human_following:
      if not self.frogpilot_planner.frogpilot_following.following_lead:
        decel_rate = (v_ego - self.frogpilot_planner.lead_one.vLead)**2 / self.frogpilot_planner.lead_one.dRel
        self.braking_target = max(v_ego - (decel_rate * DT_MDL), self.frogpilot_planner.lead_one.vLead + CRUISING_SPEED)
      else:
        self.braking_target = v_cruise
    else:
      self.braking_target = v_cruise

    # Pfeiferj's Speed Limit Controller
    self.slc.frogpilot_toggles = frogpilot_toggles

    if frogpilot_toggles.speed_limit_controller:
      self.slc.update_limits(sm["frogpilotCarState"].dashboardSpeedLimit, gps_position, sm["frogpilotNavigation"].navigationSpeedLimit, now, time_validated, v_cruise, v_ego, sm)
      self.slc.update_override(v_cruise, v_cruise_diff, v_ego, v_ego_diff, sm)

      self.slc_offset = self.slc.offset
      self.slc_target = self.slc.target
    elif frogpilot_toggles.show_speed_limits:
      self.slc.update_limits(sm["frogpilotCarState"].dashboardSpeedLimit, gps_position, sm["frogpilotNavigation"].navigationSpeedLimit, now, time_validated, v_cruise, v_ego, sm)

      self.slc_offset = 0
      self.slc_target = self.slc.target
    else:
      self.slc_offset = 0
      self.slc_target = 0

    if force_stop_enabled and not self.override_force_stop:
      self.forcing_stop |= not sm["carState"].standstill

      self.tracked_model_length = max(self.tracked_model_length - (v_ego * DT_MDL), 0)
      v_cruise = min((self.tracked_model_length // PLANNER_TIME), v_cruise)

    else:
      self.forcing_stop = False

      self.tracked_model_length = self.frogpilot_planner.model_length

      # CSC (Curve Speed Controller) takes priority - no progressive delay on safety features
      if self.csc_controlling_speed:
        # CSC active: use curve speed directly and reset progressive state
        # Progressive will restart from current speed after exiting curve
        self.progressive_enabled = False
        v_cruise = min(self.csc_target, v_cruise)
      else:
        # Normal operation: apply progressive acceleration control to v_cruise
        # This prevents sudden large accelerations by stepping up speed gradually (10 km/h increments)
        v_cruise = self.update_progressive_speed(v_ego, v_cruise)

    return v_cruise
