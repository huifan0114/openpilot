#!/usr/bin/env python3
import numpy as np

#################################
from openpilot.selfdrive.controls.lib.longitudinal_mpc_lib.long_mpc import COMFORT_BRAKE, LEAD_DANGER_FACTOR, STOP_DISTANCE, desired_follow_distance, get_jerk_factor, get_T_FOLLOW, get_dynamic_T_FOLLOW_stock_style
#################################

from openpilot.frogpilot.common.frogpilot_variables import CITY_SPEED_LIMIT, CRUISING_SPEED, MAX_T_FOLLOW

TRAFFIC_MODE_BP = [0., CITY_SPEED_LIMIT]

class FrogPilotFollowing:
  def __init__(self, FrogPilotPlanner):
    self.frogpilot_planner = FrogPilotPlanner

    self.following_lead = False
#################################
    self.slower_lead = False
    self.base_acceleration_jerk = 0  # 基準值（供 frogpilotPlan 發布用）
    self.base_speed_jerk = 0  # 基準值（供 frogpilotPlan 發布用）
#################################
    self.acceleration_jerk = 0
    self.danger_jerk = 0
    self.desired_follow_distance = 0
    self.speed_jerk = 0
    self.t_follow = 0

  def update(self, v_ego, sm, frogpilot_toggles):
#################################
    # === T_FOLLOW 設定 ===
    if sm["controlsState"].enabled:
      # 使用原車 ACC 動態 Time Headway 公式（簡單公式 + 60m 上限）
      self.t_follow = get_dynamic_T_FOLLOW_stock_style(v_ego)
    else:
      self.base_acceleration_jerk = 0
      self.base_danger_jerk = 0
      self.base_speed_jerk = 0
      self.t_follow = 0

    # === 純視覺車輛安全設計 ===
    # 視覺偵測不穩定（加速時、轉彎時可能誤判無前車）
    # 所以：加速平順（安全），減速可稍靈敏
    if sm["controlsState"].enabled:
        if sm["carState"].aEgo >= 0:
            # 加速中：保持平順（安全優先，視覺可能誤判）
            self.acceleration_jerk = 1.8
            self.speed_jerk = 1.8
            self.danger_jerk = 1.8
        else:
            # 減速中：稍微靈敏一點（減速時視覺較穩定）
            self.acceleration_jerk = 1.5
            self.speed_jerk = 1.5
            self.danger_jerk = 1.5
        # 基準值：保存未被預判機制修改的值
        self.base_acceleration_jerk = self.acceleration_jerk
        self.base_speed_jerk = self.speed_jerk

    # 擴大閾值從 +1 → +5，減少 47.1% 誤判（有前車但 following_lead = False）
    self.following_lead = self.frogpilot_planner.tracking_lead and self.frogpilot_planner.lead_one.dRel < (self.t_follow + 5) * v_ego
#################################
    if sm["controlsState"].enabled and self.frogpilot_planner.tracking_lead:
      if not sm["frogpilotCarState"].trafficModeEnabled and frogpilot_toggles.human_following:
        self.update_follow_values(self.frogpilot_planner.lead_one.dRel, v_ego, self.frogpilot_planner.lead_one.vLead, frogpilot_toggles)
      self.desired_follow_distance = int(desired_follow_distance(v_ego, self.frogpilot_planner.lead_one.vLead, self.t_follow))
    else:
      self.desired_follow_distance = 0

  def update_follow_values(self, lead_distance, v_ego, v_lead, frogpilot_toggles):
#################################
    # === 預判機制：在接近中時提前準備減速 ===
    # 設計原則：
    # 1. 不等到 dist_ratio < 1.0 才減速
    # 2. 在 dist_ratio 1.0~1.3 且正在接近時，提前調整
    # 3. 增加 t_follow（讓 MPC 更早減速）
    # 4. 降低減速 jerk（讓減速更靈敏）

    # 計算 dist_ratio
    desired_dist = desired_follow_distance(v_ego, v_lead, self.t_follow)
    dist_ratio = lead_distance / desired_dist if desired_dist > 0 else 99.0

    # 預判參數
    ANTICIPATION_START = 1.8   # 開始預判的 dist_ratio（從 1.3 提高到 1.8，給更多反應時間）
    ANTICIPATION_END = 1.0     # 結束預判（進入 MPC 正常控制）
    MAX_T_FOLLOW_BOOST = 1.25  # t_follow 最多增加 25%
    MIN_JERK_FACTOR = 1.2      # jerk 最低降到 1.2

    # 預判條件：在緩衝區內 且 正在接近中（v_lead < v_ego）
    if v_lead < v_ego and ANTICIPATION_END <= dist_ratio < ANTICIPATION_START:
      # 計算預判強度 (0~1)，越接近 1.0 越強
      anticipation_factor = (ANTICIPATION_START - dist_ratio) / (ANTICIPATION_START - ANTICIPATION_END)

      # 1. 增加 t_follow（讓 MPC 更早開始減速）
      t_follow_boost = 1.0 + anticipation_factor * (MAX_T_FOLLOW_BOOST - 1.0)
      self.t_follow *= t_follow_boost

      # 2. 降低減速 jerk（讓減速更靈敏）
      # 從目前值（1.5）降到 MIN_JERK_FACTOR（1.2）
      current_jerk = self.acceleration_jerk
      target_jerk = MIN_JERK_FACTOR
      jerk_reduction = current_jerk - anticipation_factor * (current_jerk - target_jerk)
      self.acceleration_jerk = jerk_reduction
      self.speed_jerk = jerk_reduction
#################################
