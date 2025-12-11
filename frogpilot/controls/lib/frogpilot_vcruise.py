#!/usr/bin/env python3
import math
import numpy as np
from collections import deque

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

    # Vision Safety Controller (VSC) - 使用平滑值 + 趨勢判斷
    self.vsc_dRel_history = deque(maxlen=10)   # 0.5 秒窗口 (20Hz)
    self.vsc_vRel_history = deque(maxlen=10)
    self.vsc_dist_ratio_history = deque(maxlen=40)  # 2 秒窗口，用於趨勢判斷
    self.vsc_target = 0
    self.vsc_active = False

    # VSC 狀態變數
    self.vsc_approaching_frames = 0

  def get_vsc_smoothed(self):
    """取得平滑值 (移動中位數) - 過濾視覺跳動"""
    if len(self.vsc_dRel_history) < 3:
      return None, None
    return np.median(list(self.vsc_dRel_history)), np.median(list(self.vsc_vRel_history))

  def is_vsc_trend_decreasing(self):
    """判斷 dist_ratio 是否持續下降 (趨勢預警)"""
    if len(self.vsc_dist_ratio_history) < 20:  # 至少 1 秒數據
      return False
    # 比較前半和後半的平均值
    mid = len(self.vsc_dist_ratio_history) // 2
    first_half = list(self.vsc_dist_ratio_history)[:mid]
    second_half = list(self.vsc_dist_ratio_history)[mid:]
    avg_first = np.mean(first_half)
    avg_second = np.mean(second_half)
    # 如果後半比前半低 5% 以上，視為下降趨勢
    return avg_second < avg_first * 0.95

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
    # 設計原則：
    # 1. 使用平滑值 (移動中位數) 過濾視覺跳動
    # 2. dist_ratio 為主要指標，不依賴 TTC (視覺數據下太不穩定)
    # 3. 底線保護：絕對不低於 MPC 跟車距離
    # 4. 趨勢預警：持續接近時提前介入
    # 5. 與 MPC 協調：不牴觸正常跟車

    if sm["controlsState"].enabled:
      if not sm["radarState"].leadOne.status:
        # 無前車時重置所有狀態
        self.vsc_dRel_history.clear()
        self.vsc_vRel_history.clear()
        self.vsc_dist_ratio_history.clear()
        self.vsc_approaching_frames = 0
        self.vsc_active = False
        v_cruise_vsc = v_cruise
      else:
        lead = sm["radarState"].leadOne
        dRel = lead.dRel
        vRel = lead.vRel
        v_ego_kph = v_ego * CV.MS_TO_KPH

        # 更新歷史 (使用 deque，自動維護大小)
        self.vsc_dRel_history.append(dRel)
        self.vsc_vRel_history.append(vRel)

        # 取得平滑值
        dRel_smooth, vRel_smooth = self.get_vsc_smoothed()

        if dRel_smooth is None:
          # 數據不足，不觸發 VSC
          self.vsc_target = v_cruise
          self.vsc_active = False
          v_cruise_vsc = v_cruise
        else:
          # 計算跟車距離底線: dRel = (0.576 × v_kph + 2.96) × 0.8
          follow_dist = min((0.576 * v_ego_kph + 2.96) * 0.8, 60.0)

          # 使用平滑值計算 dist_ratio
          dist_ratio = dRel_smooth / follow_dist if follow_dist > 0 else 99
          self.vsc_dist_ratio_history.append(dist_ratio)

          # === 持續接近計數 (使用平滑值) ===
          if vRel_smooth < -0.8:
            self.vsc_approaching_frames += 1
          elif vRel_smooth > -0.3:
            self.vsc_approaching_frames = max(0, self.vsc_approaching_frames - 2)

          # 趨勢判斷
          trend_decreasing = self.is_vsc_trend_decreasing()

          # === 基於平滑 dist_ratio 的漸進式風險判斷 ===

          # 1. 緊急：dist_ratio < 0.7 → 減速 15%
          if dist_ratio < 0.7:
            self.vsc_target = max(v_ego * 0.85, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 2. 危險：dist_ratio < 0.8 → 減速 10%
          elif dist_ratio < 0.8:
            self.vsc_target = max(v_ego * 0.90, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 3. 警戒：dist_ratio < 0.9 → 減速 5%
          elif dist_ratio < 0.9:
            self.vsc_target = max(v_ego * 0.95, 30 * CV.KPH_TO_MS)
            self.vsc_active = True

          # 4. 預警：dist_ratio < 1.0 且在接近 → 維持現速
          elif dist_ratio < 1.0 and vRel_smooth < -0.5:
            self.vsc_target = v_ego
            self.vsc_active = True

          # 5. 趨勢預警：dist_ratio < 1.15 且持續下降且在接近
          elif dist_ratio < 1.15 and trend_decreasing and vRel_smooth < -0.8 and self.vsc_approaching_frames >= 10:
            # 輕微減速，不干擾 MPC
            self.vsc_target = max(v_ego * 0.98, v_cruise * 0.95)
            self.vsc_active = True

          # 6. 正常：讓 MPC 控制
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
      # 接近目標，直接使用最終目標速度
      v_cruise_prog = v_cruise_final
      self.progressive_target = v_cruise_final  # 同步更新 progressive_target 供 UI 顯示
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
