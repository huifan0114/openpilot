#!/usr/bin/env python3
"""
NavBridge 守護進程
從 Android NavBridge App 取得 Google Maps 導航資料，
發送 frogpilotNavigation 訊息供條件實驗模式使用。

當 NavBridge App 未運行時，會持續發送空的導航訊息 (approachingTurn=False, approachingIntersection=False)，
不會影響 FrogPilot 的正常運作。
"""

import requests

import cereal.messaging as messaging
from openpilot.common.params import Params
from openpilot.common.realtime import Ratekeeper

# NavBridge 設定
DEFAULT_NAVBRIDGE_HOST = "localhost"
NAVBRIDGE_PORT = 5002
NAVBRIDGE_TIMEOUT = 2.0  # 秒

# 接近判斷距離 (公尺)
APPROACHING_TURN_DISTANCE = 500
APPROACHING_INTERSECTION_DISTANCE = 500

# 需要轉向的 maneuver modifier
TURN_MODIFIERS = ["left", "right", "slight_left", "slight_right", "sharp_left", "sharp_right", "uturn"]

# 路口類型 (交流道出口等)
INTERSECTION_TYPES = ["off_ramp", "on_ramp", "roundabout", "rotary"]


class NavBridge:
  def __init__(self, pm):
    self.pm = pm
    self.params = Params()

    # NavBridge 主機設定
    self.navbridge_host = self.params.get("NavBridgeHost", encoding='utf8') or DEFAULT_NAVBRIDGE_HOST

    # frogpilotNavigation 狀態
    self.approaching_intersection = False
    self.approaching_turn = False

    # NavBridge 連線狀態 (供 UI 顯示)
    self.connected = False
    self.distance_to_maneuver = 0
    self.maneuver_type = ""
    self.maneuver_modifier = ""

  def fetch_nav_data(self):
    """從 NavBridge App 取得導航資料"""
    try:
      url = f"http://{self.navbridge_host}:{NAVBRIDGE_PORT}/nav"
      response = requests.get(url, timeout=NAVBRIDGE_TIMEOUT)
      if response.status_code == 200:
        return response.json()
      return None
    except Exception:
      return None

  def update_navigation_state(self, nav_data):
    """根據導航資料更新 approaching 狀態"""
    self.approaching_intersection = False
    self.approaching_turn = False

    if nav_data is None:
      self.connected = False
      self.distance_to_maneuver = 0
      self.maneuver_type = ""
      self.maneuver_modifier = ""
      return

    # 有收到資料就是已連接
    self.connected = True

    if not nav_data.get("isNavigating", False):
      self.distance_to_maneuver = 0
      self.maneuver_type = ""
      self.maneuver_modifier = ""
      return

    distance = nav_data.get("distanceToNext", 0)
    maneuver_type = nav_data.get("maneuverType", "").lower()
    maneuver_modifier = nav_data.get("maneuverModifier", "").lower()

    # 保存原始資料供 UI 顯示
    self.distance_to_maneuver = distance
    self.maneuver_type = maneuver_type
    self.maneuver_modifier = maneuver_modifier

    # 檢查是否接近轉彎
    if distance > 0 and distance <= APPROACHING_TURN_DISTANCE:
      # 檢查 modifier 是否為轉向
      if any(turn in maneuver_modifier for turn in TURN_MODIFIERS):
        self.approaching_turn = True

      # 檢查是否為路口類型 (交流道等)
      if any(intersection in maneuver_type for intersection in INTERSECTION_TYPES):
        self.approaching_intersection = True

  def send_navigation_message(self):
    """發送 frogpilotNavigation 訊息"""
    msg = messaging.new_message('frogpilotNavigation')
    msg.valid = True

    nav = msg.frogpilotNavigation
    nav.approachingIntersection = self.approaching_intersection
    nav.approachingTurn = self.approaching_turn
    nav.navigationSpeedLimit = 0  # 不使用速限

    # NavBridge 狀態 (供 UI 顯示)
    nav.navBridgeConnected = self.connected
    nav.distanceToManeuver = self.distance_to_maneuver
    nav.maneuverType = self.maneuver_type
    nav.maneuverModifier = self.maneuver_modifier

    self.pm.send('frogpilotNavigation', msg)

  def update(self):
    """主更新循環"""
    nav_data = self.fetch_nav_data()
    self.update_navigation_state(nav_data)
    self.send_navigation_message()


def main():
  pm = messaging.PubMaster(['frogpilotNavigation'])
  navbridge = NavBridge(pm)
  rk = Ratekeeper(1.0)  # 1 Hz

  while True:
    navbridge.update()
    rk.keep_time()


if __name__ == "__main__":
  main()
