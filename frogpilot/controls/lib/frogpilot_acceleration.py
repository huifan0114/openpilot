#!/usr/bin/env python3
import json
import numpy as np

from openpilot.common.params import Params
from openpilot.selfdrive.controls.lib.longitudinal_planner import ACCEL_MIN, get_max_accel

from openpilot.frogpilot.common.frogpilot_variables import CITY_SPEED_LIMIT, params_memory

A_CRUISE_MIN_ECO =   ACCEL_MIN / 2
A_CRUISE_MIN_SPORT = ACCEL_MIN * 2

                  # MPH = [0.0,  11,  22,  34,  45,  56,  89]
A_CRUISE_MAX_BP_CUSTOM =  [0.0,  5., 10., 15., 20., 25., 40.]
A_CRUISE_MAX_VALS_ECO =   [2.5, 2.0, 1.5, 1.2, 0.9, 0.6, 0.4]
A_CRUISE_MAX_VALS_SPORT = [3.5, 3.0, 2.5, 2.0, 1.5, 1.2, 0.9]

def get_max_accel_eco(v_ego):
  return float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, A_CRUISE_MAX_VALS_ECO))

def get_max_accel_sport(v_ego):
  return float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, A_CRUISE_MAX_VALS_SPORT))

def get_max_accel_low_speeds(max_accel, v_cruise):
  return float(np.interp(v_cruise, [0., CITY_SPEED_LIMIT / 2, CITY_SPEED_LIMIT], [max_accel / 3, max_accel / 1.5, max_accel]))

def get_max_accel_ramp_off(max_accel, v_cruise, v_ego):
  return float(np.interp(v_cruise - v_ego, [0., 1., 5.], [0., 0.7, max_accel]))

def get_max_allowed_accel(v_ego):
  return float(np.interp(v_ego, [0., 5., 20.], [4.0, 4.0, 2.0]))  # ISO 15622:2018

class DrivingStyleLearner:
  """學習駕駛風格並動態調整加速參數"""
  def __init__(self):
    self.params = Params()

    # 學習參數
    self.accel_samples = []
    self.gas_samples = []
    self.sample_count = 0
    self.max_samples = 100  # 收集100個樣本後開始學習

    # 駕駛風格係數 (0.8 = 溫和, 1.0 = 標準, 1.5 = 積極, 2.0 = 激進)
    self.style_factor = 1.0
    self.learned = False

    # 載入已學習的數據
    self._load_learned_style()
    self._publish_style_stats()

  def _load_learned_style(self):
    """從持久化存儲載入已學習的駕駛風格"""
    try:
      data = self.params.get("DrivingStyleLearned")
      if data:
        learned = json.loads(data)
        self.style_factor = learned.get("style_factor", 1.0)
        self.learned = learned.get("learned", False)
        self.sample_count = learned.get("sample_count", 0)
    except:
      pass

  def _publish_style_stats(self, avg_accel=0.0, avg_gas=0.0, p90_accel=0.0, delta=0.0):
    stats = {
      "style_factor": float(self.style_factor),
      "delta": float(delta),
      "learned": bool(self.learned),
      "sample_count": int(self.sample_count),
      "avg_accel": float(avg_accel),
      "avg_gas": float(avg_gas),
      "p90_accel": float(p90_accel),
    }
    params_memory.put("DrivingStyleStats", json.dumps(stats))

  def _save_learned_style(self):
    """保存學習的駕駛風格到持久化存儲"""
    data = {
      "style_factor": float(self.style_factor),
      "learned": self.learned,
      "sample_count": self.sample_count
    }
    self.params.put("DrivingStyleLearned", json.dumps(data))

  def update(self, v_ego, a_ego, gas_pressed, gas_value):
    """更新學習數據"""
    # 只在手動駕駛且車輛移動時收集數據
    if gas_pressed and v_ego > 1.0 and a_ego > 0.1:
      self.accel_samples.append(a_ego)
      self.gas_samples.append(gas_value)
      self.sample_count += 1

      # 限制樣本數量以節省記憶體
      if len(self.accel_samples) > self.max_samples:
        self.accel_samples.pop(0)
        self.gas_samples.pop(0)

      # 每收集30個樣本後重新計算
      if self.sample_count % 30 == 0:
        self._calculate_style()

  def _calculate_style(self):
    """計算駕駛風格係數"""
    if len(self.accel_samples) < 30:  # 至少需要30個樣本
      return

    accel_array = np.array(self.accel_samples)
    gas_array = np.array(self.gas_samples)

    # 計算統計指標
    avg_accel = np.mean(accel_array)
    p90_accel = np.percentile(accel_array, 90)  # 90百分位數
    avg_gas = np.mean(gas_array)

    # 根據統計指標計算風格係數
    # 平均加速度: 0.5-1.5 m/s² -> 0.8-1.3
    accel_factor = np.clip(0.5 + avg_accel * 0.4, 0.8, 1.5)

    # 最大加速度: 1.0-3.0 m/s² -> 0.8-1.4
    max_factor = np.clip(0.5 + p90_accel * 0.3, 0.8, 1.5)

    # 油門使用: 0.3-0.8 -> 0.9-1.4
    gas_factor = np.clip(0.6 + avg_gas * 1.0, 0.8, 1.5)

    # 綜合計算 (加權平均)
    previous_factor = self.style_factor
    self.style_factor = (accel_factor * 0.4 + max_factor * 0.3 + gas_factor * 0.3)
    self.style_factor = np.clip(self.style_factor, 0.8, 2.0)
    delta = self.style_factor - previous_factor

    self.learned = True
    self._save_learned_style()
    self._publish_style_stats(avg_accel=avg_accel, avg_gas=avg_gas, p90_accel=p90_accel, delta=delta)

  def get_adjusted_accel_vals(self, base_vals):
    """根據學習的駕駛風格調整加速參數"""
    if not self.learned:
      return base_vals

    # 應用風格係數，但保持合理範圍
    adjusted = [min(val * self.style_factor, 4.0) for val in base_vals]
    return adjusted

  def reset_learning(self):
    """重置學習數據"""
    self.accel_samples = []
    self.gas_samples = []
    self.sample_count = 0
    self.style_factor = 1.0
    self.learned = False
    self.params.remove("DrivingStyleLearned")
    self._publish_style_stats()

class FrogPilotAcceleration:
  def __init__(self, FrogPilotPlanner):
    self.frogpilot_planner = FrogPilotPlanner

    self.max_accel = 0
    self.min_accel = 0

    # 初始化駕駛風格學習器
    self.style_learner = DrivingStyleLearner()

  def update(self, v_ego, sm, frogpilot_toggles):
    eco_gear = sm["frogpilotCarState"].ecoGear
    sport_gear = sm["frogpilotCarState"].sportGear

    # 收集駕駛風格數據 (手動駕駛或ACC啟動時踩油門都收集)
    car_state = sm["carState"]
    if frogpilot_toggles.aggressive_acceleration_learning:
      a_ego = car_state.aEgo
      gas_pressed = car_state.gasPressed
      gas_value = car_state.gas if hasattr(car_state, 'gas') else 0.0
      # 只要踩油門加速就收集數據（包含手動駕駛和ACC啟動時的人工干預）
      if gas_pressed:
        self.style_learner.update(v_ego, a_ego, gas_pressed, gas_value)

    # 獲取學習調整後的加速參數
    learned_eco_vals = self.style_learner.get_adjusted_accel_vals(A_CRUISE_MAX_VALS_ECO)
    learned_sport_vals = self.style_learner.get_adjusted_accel_vals(A_CRUISE_MAX_VALS_SPORT)

#################################
    self.max_accel = float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, learned_eco_vals))
#################################

    if sm["frogpilotCarState"].trafficModeEnabled:
      self.max_accel = get_max_accel(v_ego)
    elif (eco_gear or sport_gear) and frogpilot_toggles.map_acceleration:
      if eco_gear:
        self.max_accel = float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, learned_eco_vals))
      else:
        if frogpilot_toggles.acceleration_profile == 2:
          self.max_accel = float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, learned_sport_vals))
        else:
          self.max_accel = get_max_allowed_accel(v_ego)
    else:
      if frogpilot_toggles.acceleration_profile == 1:
        self.max_accel = float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, learned_eco_vals))
      elif frogpilot_toggles.acceleration_profile == 2:
        self.max_accel = float(np.interp(v_ego, A_CRUISE_MAX_BP_CUSTOM, learned_sport_vals))
      elif frogpilot_toggles.acceleration_profile == 3:
        self.max_accel = get_max_allowed_accel(v_ego)
      else:
        self.max_accel = get_max_accel(v_ego)

    if frogpilot_toggles.human_acceleration:
      self.max_accel = min(get_max_accel_low_speeds(self.max_accel, self.frogpilot_planner.v_cruise), self.max_accel)
      self.max_accel = min(get_max_accel_ramp_off(self.max_accel, self.frogpilot_planner.v_cruise, v_ego), self.max_accel)

    if self.frogpilot_planner.frogpilot_weather.weather_id != 0:
      self.max_accel -= self.max_accel * self.frogpilot_planner.frogpilot_weather.reduce_acceleration

    if self.frogpilot_planner.tracking_lead:
      self.min_accel = ACCEL_MIN
    elif sm["frogpilotCarState"].forceCoast:
      self.min_accel = A_CRUISE_MIN_ECO
    elif (eco_gear or sport_gear) and frogpilot_toggles.map_deceleration:
      if eco_gear:
        self.min_accel = A_CRUISE_MIN_ECO
      else:
        self.min_accel = A_CRUISE_MIN_SPORT
    else:
      if frogpilot_toggles.deceleration_profile == 1:
        self.min_accel = A_CRUISE_MIN_ECO
      elif frogpilot_toggles.deceleration_profile == 2:
        self.min_accel = A_CRUISE_MIN_SPORT
      else:
        self.min_accel = ACCEL_MIN
