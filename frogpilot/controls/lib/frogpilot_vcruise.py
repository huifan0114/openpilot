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

    # VSC 狀態變數
    self.prev_vLead = None
    self.prev_vRel = None
    self.approaching_frames = 0
    self.vsc_cooldown = 0  # 冷卻計時器 (防止震盪)

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
      # 過彎結束時評估並調整 lateral_acceleration (持久化學習)
      self.csc.end_curve_session()
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
        # 無前車時重置所有狀態
        self.vrel_history.clear()
        self.drel_history.clear()
        self.prev_vLead = None
        self.prev_vRel = None
        self.approaching_frames = 0
        self.vsc_active = False
        v_cruise_vsc = v_cruise
      else:
        lead = sm["radarState"].leadOne
        dRel = lead.dRel
        vRel = lead.vRel
        vLead = lead.vLead * CV.MS_TO_KPH  # 轉換為 km/h
        v_ego_kph = v_ego * CV.MS_TO_KPH

        # 更新歷史
        self.vrel_history.append(vRel)
        self.drel_history.append(dRel)
        if len(self.vrel_history) > self.vsc_history_size:
          self.vrel_history.pop(0)
          self.drel_history.pop(0)

        # 計算變化率
        delta_vLead = vLead - self.prev_vLead if self.prev_vLead is not None else 0
        delta_vRel = vRel - self.prev_vRel if self.prev_vRel is not None else 0
        self.prev_vLead = vLead
        self.prev_vRel = vRel

        # 計算跟車距離設定: dRel = (0.576 × v_kph + 2.96) × 0.8
        follow_dist = min((0.576 * v_ego_kph + 2.96) * 0.8, 60.0)

        # 計算距離比例 (核心指標)
        dist_ratio = dRel / follow_dist if follow_dist > 0 else 99

        # 計算 TTC
        ttc = dRel / abs(vRel) if vRel < -0.1 else 99

        # === 持續接近計數 ===
        if vRel < -0.5:  # 放寬條件，更早開始計數
          self.approaching_frames += 1
        else:
          self.approaching_frames = max(0, self.approaching_frames - 1)

        # === 冷卻時間處理 ===
        if self.vsc_cooldown > 0:
          self.vsc_cooldown -= 1

        # === 當距離開始偏近時，取消冷卻 (放寬到 0.97) ===
        if dist_ratio < 0.97 and vRel < -0.3 and self.vsc_cooldown > 0:
          self.vsc_cooldown = 0

        # === 基於距離比例的漸進式風險判斷 ===
        # 核心理念：早減速、小力道，避免晚減速、大力道

        if dist_ratio >= 1.0 and vRel >= -0.5:
          # 距離足夠且沒有明顯接近，不需要 VSC
          self.vsc_target = v_cruise
          self.vsc_active = False
          self.vsc_cooldown = 5  # 0.25 秒短冷卻

        elif self.vsc_cooldown > 0 and dist_ratio >= 0.95:
          # 冷卻中且距離還行，不觸發
          self.vsc_target = v_cruise
          self.vsc_active = False

        else:
          # === 五級漸進式減速 (基於百分比，不依賴前車速度) ===
          # 核心：dist_ratio 越小，減速百分比越大

          # 緊急 (dist < 60%): 減速 20%
          if dist_ratio < 0.6 or ttc < 3.0:
            self.vsc_target = max(v_ego * 0.80, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 危險 (dist < 70%): 減速 15%
          elif dist_ratio < 0.7 or ttc < 4.0:
            self.vsc_target = max(v_ego * 0.85, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 偏近 (dist < 80%): 減速 10%
          elif dist_ratio < 0.8 or (ttc < 5.0 and dist_ratio < 0.85):
            self.vsc_target = max(v_ego * 0.90, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 警戒 (dist < 90%): 減速 5%
          elif dist_ratio < 0.9 or (dist_ratio < 0.95 and vRel < -1.0):
            self.vsc_target = max(v_ego * 0.95, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 預警 (dist < 97% 且持續接近): 不加速，維持現速
          elif dist_ratio < 0.97 and vRel < -0.5 and self.approaching_frames >= 3:
            self.vsc_target = v_ego
            self.vsc_active = True

          # 正常
          else:
            self.vsc_target = v_cruise
            self.vsc_active = False

        v_cruise_vsc = self.vsc_target
    else:
      v_cruise_vsc = v_cruise
      self.vsc_active = False

    # ========== Progressive Speed ==========
    STEP_SIZE = 10 * CV.KPH_TO_MS
    APPROACH_THRESHOLD = (3 if v_ego < 40 * CV.KPH_TO_MS else 2) * CV.KPH_TO_MS
    MIN_STEP_TRIGGER = 10 * CV.KPH_TO_MS

    # Progressive 基於原始 v_cruise，與 VSC 獨立運作
    v_cruise_final = v_cruise

    # 計算下一個對齊到 10 的倍數的目標速度
    v_ego_kph = v_ego * CV.MS_TO_KPH
    next_target_kph = math.ceil(v_ego_kph / 10) * 10
    next_target_kph = max(next_target_kph, 30)
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
