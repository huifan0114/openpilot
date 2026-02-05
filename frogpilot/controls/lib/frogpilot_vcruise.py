#!/usr/bin/env python3
#################################
import math
import numpy as np
import time
from collections import deque
#################################

from openpilot.common.conversions import Conversions as CV
from openpilot.common.realtime import DT_MDL
from openpilot.selfdrive.controls.lib.longitudinal_mpc_lib.long_mpc import COMFORT_BRAKE

from openpilot.frogpilot.common.frogpilot_variables import CRUISING_SPEED, PLANNER_TIME
from openpilot.frogpilot.controls.lib.curve_speed_controller import CurveSpeedController
from openpilot.frogpilot.controls.lib.speed_limit_controller import SpeedLimitController

# 用戶按鈕操作保護期（分鐘）- 預設值
BUTTON_PRESS_PROTECT_TIME = 0.5

class FrogPilotVCruise:
  def __init__(self, FrogPilotPlanner):
    self.frogpilot_planner = FrogPilotPlanner

    self.csc = CurveSpeedController(self)
    self.slc = SpeedLimitController()

#################################
    # 記錄用戶最後一次按鈕操作的時間（用於保護自動更新的干擾）
    self.last_button_press_time = 0

    # CSC (Curve Speed Controller) state
    self.csc_controlling_speed = False
    self.csc_target = 0

    # SLC (Speed Limit Controller) state
    self.slc_offset = 0
    self.slc_target = 0

    # Progressive speed controller
    self.progressive_target = 0

    # Force stop
    self.force_stop_timer = 0
    self.tracked_model_length = 0

    # Human Following
    self.braking_target = 0

    # VSC (Vision Safety Controller) target
    self.vsc_target = 0
#################################

    self.forcing_stop = False
    self.override_force_stop = False

    self.override_force_stop_timer = 0


#################################
    # Vision Safety Controller (VSC) - 整合版
    # 整合所有有用指標：dist_ratio, TTC, 趨勢, 快速接近, vRel_stuck, aRel
    self.vsc_dRel_history = deque(maxlen=10)   # 0.5 秒窗口 (20Hz) - 平滑用
    self.vsc_vRel_history = deque(maxlen=10)
    self.vsc_dist_ratio_history = deque(maxlen=40)  # 2 秒窗口 - 趨勢判斷用
    self.vsc_aRel_history = deque(maxlen=5)  # 0.25 秒窗口 - 前車減速檢測
    self.vsc_active = False

    # VSC 狀態變數
    self.vsc_last_vRel = None  # 用於計算相對加速度
    self.vsc_approaching_frames = 0  # 持續接近計數

    # VSC 平滑機制 (純視覺車輛需要過濾跳動)
    self.vsc_smoothed_target = None  # 平滑後的目標速度

  def get_vsc_smoothed(self):
    """取得平滑值 (移動平均) - 過濾視覺跳動，使用平均值代替中位數加速計算"""
    if len(self.vsc_dRel_history) < 3:
      return None, None
    # 使用 mean 代替 median（快 3-5 倍）
    return np.mean(self.vsc_dRel_history), np.mean(self.vsc_vRel_history)

  def get_vsc_ttc(self, dRel, vRel):
    """計算 Time To Collision (碰撞時間)"""
    if vRel >= 0 or dRel <= 0:
      return 99.0  # 沒有接近或無效距離
    return dRel / abs(vRel)

  def get_vsc_aRel(self):
    """取得平滑的相對加速度"""
    if len(self.vsc_aRel_history) < 2:
      return 0.0
    # 使用 mean 代替 median（快 3-5 倍）
    return np.mean(self.vsc_aRel_history)

  def is_vsc_trend_decreasing(self):
    """判斷 dist_ratio 是否持續下降 (趨勢預警) - 優化版"""
    if len(self.vsc_dist_ratio_history) < 20:  # 至少 1 秒數據
      return False
    # 直接比較最前5個和最後5個的平均值（避免 list 轉換）
    # 使用 islice 避免創建新列表
    from itertools import islice
    first_5 = sum(islice(self.vsc_dist_ratio_history, 5)) / 5
    last_5 = sum(list(self.vsc_dist_ratio_history)[-5:]) / 5
    # 如果最後5個比最前5個低 5% 以上，視為下降趨勢
    return last_5 < first_5 * 0.95

  def is_vsc_vrel_stuck(self):
    """檢測 vRel 數據是否卡住 (視覺異常) - 優化版"""
    if len(self.vsc_vRel_history) < 10 or len(self.vsc_dRel_history) < 10:
      return False
    # 直接使用 deque，避免 list() 轉換
    vrel_std = np.std(self.vsc_vRel_history)
    drel_change = abs(self.vsc_dRel_history[0] - self.vsc_dRel_history[-1])
    # vRel 變化很小但 dRel 變化很大 → 數據異常
    return vrel_std < 0.5 and drel_change > 20

  def get_vsc_integrated_threat(self, dist_ratio, ttc, vRel_smooth, aRel_smooth):
    """
    整合版威脅等級判斷 - 取所有指標的最高等級
    返回: (level, reasons)
    level: 0=正常, 0.5=預警, 1=警戒, 2=危險, 3=緊急
    """
    level = 0.0
    reasons = []

    # === 1. dist_ratio 威脅 (主要指標，VSC 規範) ===
    if dist_ratio < 0.7:
      level = max(level, 3)
      reasons.append(f"ratio<0.7({dist_ratio:.2f})")
    elif dist_ratio < 0.8:
      level = max(level, 2)
      reasons.append(f"ratio<0.8({dist_ratio:.2f})")
    elif dist_ratio < 0.9:
      level = max(level, 1)
      reasons.append(f"ratio<0.9({dist_ratio:.2f})")
    elif dist_ratio < 1.0 and vRel_smooth < -0.5:
      level = max(level, 0.5)
      reasons.append(f"ratio<1.0({dist_ratio:.2f})")

    # === 2. TTC 威脅 (碰撞時間) ===
    # 設計原則：TTC 越短 → 越緊急 → dist_ratio 限制越寬鬆
    # TTC 很短代表很快會碰撞，即使距離稍遠也應該反應
    if ttc < 2.0 and dist_ratio < 1.2:
      level = max(level, 3)
      reasons.append(f"TTC<2({ttc:.1f}s)")
    elif ttc < 3.0 and dist_ratio < 1.1:
      level = max(level, 2)
      reasons.append(f"TTC<3({ttc:.1f}s)")
    elif ttc < 4.0 and dist_ratio < 1.0:
      level = max(level, 1)
      reasons.append(f"TTC<4({ttc:.1f}s)")

    # === 3. vRel_stuck 威脅 (數據異常) ===
    # 修正：只有在 dist_ratio < 1.1 時才觸發，避免遠距離誤觸發
    if self.is_vsc_vrel_stuck() and dist_ratio < 1.1:
      level = max(level, 2)
      reasons.append("vRel_stuck")

    # === 4. 趨勢預警 (level 0.5) ===
    # 預警等級最輕 → dist_ratio 條件要最嚴格
    trend_decreasing = self.is_vsc_trend_decreasing()
    if dist_ratio < 1.0 and trend_decreasing and vRel_smooth < -0.8 and self.vsc_approaching_frames >= 10:
      level = max(level, 0.5)
      reasons.append(f"趨勢↓({dist_ratio:.2f})")

    # === 5. 快速接近預警 (level 0.5) ===
    # 預警等級最輕 → dist_ratio 條件要最嚴格
    if dist_ratio < 1.0 and vRel_smooth < -2.0 and self.vsc_approaching_frames >= 5:
      level = max(level, 0.5)
      reasons.append(f"快接近(vRel={vRel_smooth:.1f})")

    # === 6. aRel 加權 (前車減速時更敏感) ===
    # 修正：只有在 dist_ratio < 1.0 且已有威脅時才加權（避免遠距離誤觸發）
    if dist_ratio < 1.0 and level > 0:
      if aRel_smooth < -2.0:
        level = min(level + 0.5, 3)
        reasons.append(f"前車急煞(aRel={aRel_smooth:.1f})")
      elif aRel_smooth < -1.0:
        level = min(level + 0.25, 3)
        reasons.append(f"前車減速(aRel={aRel_smooth:.1f})")

    return level, reasons
#################################

  def is_button_protected(self):
    """檢查是否在用戶按鈕操作的保護期內（此期間自動速限不應覆蓋用戶設定）"""
    # 從參數中動態讀取保護時間設定（單位：分鐘），預設 0.5 分鐘（30秒）
    params_memory = self.frogpilot_planner.params_memory
    protect_time_min = params_memory.get_int("ButtonPressProtectTime") or BUTTON_PRESS_PROTECT_TIME
    protect_time_sec = protect_time_min * 60  # 轉換為秒
    elapsed = time.time() - self.last_button_press_time
    return elapsed < protect_time_sec

  def update(self, gps_position, now, time_validated, v_cruise, v_ego, sm, frogpilot_toggles):
#################################
    # 保存原始 v_cruise，避免各控制器之間的交叉污染
    v_cruise_original = v_cruise
#################################

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
#################################
    # ========== CSC (Curve Speed Controller) ==========
    if v_ego > CRUISING_SPEED and sm["controlsState"].enabled and self.frogpilot_planner.road_curvature_detected and frogpilot_toggles.curve_speed_controller:
      self.csc.update_target(v_ego)

      self.csc_controlling_speed = True

      self.csc_target = min(self.csc.target, v_cruise_original)  # 使用原始定速
    else:
      # 只有當有活躍的 session 時才調用 end_curve_session（避免無效調用）
      if self.csc.curve_session_active:
        self.csc.end_curve_session()
      self.csc_controlling_speed = False
      self.csc.target_set = False
      self.csc_target = v_cruise_original  # 使用原始定速

    # ========== Human Following (有 toggle 控制) ==========
    if self.frogpilot_planner.lead_one.vLead < v_ego > CRUISING_SPEED and sm["controlsState"].enabled and self.frogpilot_planner.tracking_lead and frogpilot_toggles.human_following:
      if not self.frogpilot_planner.frogpilot_following.following_lead:
        decel_rate = (v_ego - self.frogpilot_planner.lead_one.vLead)**2 / self.frogpilot_planner.lead_one.dRel
        self.braking_target = max(v_ego - (decel_rate * DT_MDL), self.frogpilot_planner.lead_one.vLead + CRUISING_SPEED)
      else:
        self.braking_target = v_cruise_original  # 使用原始定速
    else:
      self.braking_target = v_cruise_original  # 使用原始定速
#################################
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

    # ========== Force Stop (紅燈停車) ==========
    if force_stop_enabled and not self.override_force_stop:
      self.forcing_stop |= not sm["carState"].standstill
      self.tracked_model_length = max(self.tracked_model_length - (v_ego * DT_MDL), 0)
      v_cruise_force_stop = min((self.tracked_model_length // PLANNER_TIME), v_cruise_original)
    else:
      self.forcing_stop = False
      self.tracked_model_length = self.frogpilot_planner.model_length
      v_cruise_force_stop = v_cruise_original

#################################
    # ========== VSC (Vision Safety Controller) - 整合版 ==========
    # 設計原則：
    # 1. 整合所有指標：dist_ratio, TTC, 趨勢, 快速接近, vRel_stuck, aRel
    # 2. 取所有指標的最高威脅等級
    # 3. 固定百分比減速（可預測）
    # 4. VSC 規範：dist_ratio < 1.0 必須動作
    # 5. 使用原始 v_cruise，避免被 Force Stop 污染

    if sm["controlsState"].enabled:
      lead = sm["radarState"].leadOne
      if not lead.status or lead.vRel > 0:
        # 無前車 或 前車速度大於自車 (正在遠離) 時，解除 VSC
        self.vsc_dRel_history.clear()
        self.vsc_vRel_history.clear()
        self.vsc_dist_ratio_history.clear()
        self.vsc_aRel_history.clear()
        self.vsc_last_vRel = None
        self.vsc_approaching_frames = 0
        self.vsc_active = False
        self.vsc_smoothed_target = None
        self.vsc_target = v_cruise_original  # 使用原始定速
        v_cruise_vsc = v_cruise_original
      else:
        dRel = lead.dRel
        vRel = lead.vRel
        v_ego_kph = v_ego * CV.MS_TO_KPH

        # 更新歷史 (使用 deque，自動維護大小)
        self.vsc_dRel_history.append(dRel)
        self.vsc_vRel_history.append(vRel)

        # 計算相對加速度 (vRel 的變化率)
        if self.vsc_last_vRel is not None:
          # 20Hz -> dt = 0.05s
          aRel = (vRel - self.vsc_last_vRel) / 0.05
          self.vsc_aRel_history.append(aRel)
        self.vsc_last_vRel = vRel

        # 取得平滑值
        dRel_smooth, vRel_smooth = self.get_vsc_smoothed()

        if dRel_smooth is None:
          # 數據不足，不觸發 VSC
          self.vsc_target = v_cruise_original  # 使用原始定速
          self.vsc_active = False
          v_cruise_vsc = v_cruise_original
        else:
          # 計算跟車距離底線: dRel = (0.576 × v_kph + 2.96) × 0.8
          follow_dist = min((0.576 * v_ego_kph + 2.96) * 0.8, 60.0)

          # 計算關鍵指標
          dist_ratio = dRel_smooth / follow_dist if follow_dist > 0 else 99
          ttc = self.get_vsc_ttc(dRel_smooth, vRel_smooth)
          aRel_smooth = self.get_vsc_aRel()

          # 更新歷史
          self.vsc_dist_ratio_history.append(dist_ratio)

          # 更新持續接近計數
          if vRel_smooth < -0.8:
            self.vsc_approaching_frames += 1
          elif vRel_smooth > -0.3:
            self.vsc_approaching_frames = max(0, self.vsc_approaching_frames - 2)

          # === 整合版威脅等級判斷 ===
          threat_level, reasons = self.get_vsc_integrated_threat(dist_ratio, ttc, vRel_smooth, aRel_smooth)

          # === 平滑參數 ===
          # 純視覺車輛，dRel/vRel 變化大，需要平滑過濾
          # 但不能加「延遲解除」，否則配合 progressive + ECO 會讓前車距離越拉越長
          VSC_TARGET_SMOOTH = 0.15  # 目標平滑係數（減速用，安全優先）

          # === 根據威脅等級計算目標速度 ===
          if threat_level >= 3:
            # 緊急：減速 15%
            calculated_target = float(max(v_ego * 0.85, 30 * CV.KPH_TO_MS))
            self.vsc_active = True

          elif threat_level >= 2:
            # 危險：減速 10%
            calculated_target = float(max(v_ego * 0.90, 30 * CV.KPH_TO_MS))
            self.vsc_active = True

          elif threat_level >= 1:
            # 警戒：減速 5%
            calculated_target = float(max(v_ego * 0.95, 30 * CV.KPH_TO_MS))
            self.vsc_active = True

          elif threat_level >= 0.5:
            # 預警：維持現速 (不加速)
            calculated_target = float(v_ego)
            self.vsc_active = True

          else:
            # 正常：立即解除，交給 MPC
            calculated_target = v_cruise_original  # 使用原始定速
            self.vsc_active = False

          # === 目標平滑過渡（只平滑減速，解除時立即恢復） ===
          if self.vsc_smoothed_target is None:
            self.vsc_smoothed_target = v_cruise_original

          if self.vsc_active:
            # VSC 啟動中：平滑過渡到目標（過濾視覺跳動）
            if calculated_target < self.vsc_smoothed_target:
              # 減速：平滑過渡（安全+舒適）
              self.vsc_smoothed_target = (
                self.vsc_smoothed_target * (1 - VSC_TARGET_SMOOTH) +
                calculated_target * VSC_TARGET_SMOOTH
              )
            else:
              # 威脅降低：較快回升
              self.vsc_smoothed_target = (
                self.vsc_smoothed_target * 0.85 +
                calculated_target * 0.15
              )
            self.vsc_target = float(self.vsc_smoothed_target)
          else:
            # VSC 解除：立即恢復原始定速（不延遲！）
            self.vsc_smoothed_target = v_cruise_original
            self.vsc_target = v_cruise_original

          v_cruise_vsc = self.vsc_target
    else:
      v_cruise_vsc = v_cruise_original
      self.vsc_target = v_cruise_original
      self.vsc_active = False
      self.vsc_smoothed_target = None

    # ========== Progressive Speed ==========
    STEP_SIZE = (20 if v_ego < 60 * CV.KPH_TO_MS else 30) * CV.KPH_TO_MS
    APPROACH_THRESHOLD = (4 if v_ego < 40 * CV.KPH_TO_MS else 5) * CV.KPH_TO_MS
    MIN_STEP_TRIGGER = 10 * CV.KPH_TO_MS

    # Progressive 基於原始 v_cruise，與其他控制器獨立運作
    v_cruise_final = v_cruise_original

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
    # 四個控制器獨立運作，取最小值（誰要求最慢，就用誰的）
    # 1. CSC: 彎道限速
    # 2. Progressive: 分段加速
    # 3. VSC: 前車安全距離
    # 4. Force Stop: 紅燈強制停車
    v_cruise = min(self.csc_target, v_cruise_prog, v_cruise_vsc, v_cruise_force_stop)
#################################

    return v_cruise
