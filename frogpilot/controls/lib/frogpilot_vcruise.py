#!/usr/bin/env python3
import math
import numpy as np

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

    # Progressive speed controller
    self.progressive_target = 0
    self.forcing_stop = False
    self.override_force_stop = False

    self.override_force_stop_timer = 0
    self.force_stop_timer = 0
    self.tracked_model_length = 0

    # Vision Safety Controller (VSC)
    self.vrel_history = []
    self.drel_history = []
    self.vsc_history_size = 50  # 約 2.5 秒 (20Hz)
    self.vsc_target = 0
    self.vsc_active = False

  def update(self, gps_position, now, time_validated, v_cruise, v_ego, sm, frogpilot_toggles):

    v_cruise_cluster = max(sm["controlsState"].vCruiseCluster * CV.KPH_TO_MS, v_cruise)
    v_cruise_diff = v_cruise_cluster - v_cruise

    v_ego_cluster = max(sm["carState"].vEgoCluster, v_ego)
    v_ego_diff = v_ego_cluster - v_ego

    # ========== CSC (Curve Speed Controller) ==========
    if v_ego > CRUISING_SPEED and sm["controlsState"].enabled and self.frogpilot_planner.road_curvature_detected and frogpilot_toggles.curve_speed_controller:
      self.csc.update_target(v_ego)
      self.csc_controlling_speed = True
      self.csc_target = self.csc.target
    else:
      self.csc.log_data(v_ego, sm)
      self.csc_controlling_speed = False
      self.csc.target_set = False
      self.csc_target = v_cruise

    # ========== Human Following (有 toggle 控制) ==========
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


    # ========== VSC (Vision Safety Controller) ==========
    if sm["controlsState"].enabled:
      if not sm["radarState"].leadOne.status:
        self.vrel_history.clear()
        self.drel_history.clear()
        self.vsc_active = False
        v_cruise_vsc = v_cruise
      else:
        lead = sm["radarState"].leadOne
        dRel = lead.dRel
        vRel = lead.vRel

        # 更新歷史
        self.vrel_history.append(vRel)
        self.drel_history.append(dRel)
        if len(self.vrel_history) > self.vsc_history_size:
          self.vrel_history.pop(0)
          self.drel_history.pop(0)

        # 計算指標
        ttc = dRel / abs(vRel) if vRel < -0.1 else 99
        th = dRel / v_ego if v_ego > 0.1 else 99

        # vRel_stuck 檢測
        vrel_stuck = False
        if len(self.vrel_history) >= self.vsc_history_size:
          vrel_std = np.std(self.vrel_history)
          drel_change = self.drel_history[0] - self.drel_history[-1]
          vrel_stuck = vrel_std < 0.5 and drel_change > 20

        # 風險評估
        risk_level = 0
        if ttc < 2.5:
          risk_level = max(risk_level, 3)
        elif ttc < 4:
          risk_level = max(risk_level, 2)

        if th < 0.8:
          risk_level = max(risk_level, 3)
        elif th < 1.2:
          risk_level = max(risk_level, 2)

        if vrel_stuck:
          risk_level = max(risk_level, 2)

        # 降速比例 (以當前時速 v_ego 為基準)
        if risk_level >= 3:
          reduction = 0.8  # 高風險: 減速 20%
        elif risk_level >= 2:
          reduction = 0.9  # 中風險: 減速 10%
        else:
          reduction = 1.0

        self.vsc_active = reduction < 1.0
        if self.vsc_active:
          self.vsc_target = max(v_ego * reduction, 30 * CV.KPH_TO_MS)  # 最低 30 km/h
        else:
          self.vsc_target = v_cruise
        v_cruise_vsc = self.vsc_target
    else:
      v_cruise_vsc = v_cruise
      self.vsc_active = False

    # ========== Progressive Speed ==========
    STEP_SIZE = 10 * CV.KPH_TO_MS
    APPROACH_THRESHOLD = 2 * CV.KPH_TO_MS
    MIN_STEP_TRIGGER = 10 * CV.KPH_TO_MS

    # Progressive 基於原始 v_cruise，與 VSC 獨立運作
    v_cruise_final = v_cruise

    # 計算下一個對齊到 10 的倍數的目標速度
    v_ego_kph = v_ego * CV.MS_TO_KPH
    next_target_kph = math.ceil(v_ego_kph / 10) * 10
    next_target_kph = max(next_target_kph, 20)
    next_target = next_target_kph * CV.KPH_TO_MS

    speed_diff = v_cruise_final - v_ego

    if speed_diff < MIN_STEP_TRIGGER:
      v_cruise_prog = v_cruise_final
    else:
      reset_target = False

      # 初始化 progressive_target
      if self.progressive_target == 0:
        self.progressive_target = next_target
        reset_target = True

      # 煞車場景檢測
      if v_ego < (self.progressive_target - STEP_SIZE):
        self.progressive_target = next_target
        reset_target = True

      # 目標速度降低時重置
      if self.progressive_target > v_cruise_final:
        self.progressive_target = next_target
        reset_target = True

      # 接近目標時增加步進
      if not reset_target and v_ego >= (self.progressive_target - APPROACH_THRESHOLD):
        self.progressive_target = min(self.progressive_target + STEP_SIZE, v_cruise_final)

      self.progressive_target = min(self.progressive_target, v_cruise_final)
      v_cruise_prog = self.progressive_target

    # ========== 最終計算 ==========
    # 三個控制器獨立運作，取最小值
    v_cruise = min(self.csc_target, v_cruise_prog, v_cruise_vsc)

    return v_cruise
