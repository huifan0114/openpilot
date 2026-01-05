#!/usr/bin/env python3
import bisect
import math
import os
from enum import IntEnum
from collections.abc import Callable
from types import SimpleNamespace

from cereal import log, car, custom
import cereal.messaging as messaging
from openpilot.common.conversions import Conversions as CV
from openpilot.common.git import get_short_branch
from openpilot.common.params import Params
from openpilot.common.realtime import DT_CTRL
from openpilot.selfdrive.locationd.calibrationd import MIN_SPEED_FILTER

AlertSize = log.ControlsState.AlertSize
AlertStatus = log.ControlsState.AlertStatus
FrogPilotAlertStatus = custom.FrogPilotControlsState.AlertStatus
VisualAlert = car.CarControl.HUDControl.VisualAlert
AudibleAlert = car.CarControl.HUDControl.AudibleAlert
FrogPilotAudibleAlert = custom.FrogPilotCarControl.HUDControl.AudibleAlert
EventName = car.CarEvent.EventName
FrogPilotEventName = custom.FrogPilotCarEvent.EventName


# Alert priorities
class Priority(IntEnum):
  LOWEST = 0
  LOWER = 1
  LOW = 2
  MID = 3
  HIGH = 4
  HIGHEST = 5


# Event types
class ET:
  ENABLE = 'enable'
  PRE_ENABLE = 'preEnable'
  OVERRIDE_LATERAL = 'overrideLateral'
  OVERRIDE_LONGITUDINAL = 'overrideLongitudinal'
  NO_ENTRY = 'noEntry'
  WARNING = 'warning'
  USER_DISABLE = 'userDisable'
  SOFT_DISABLE = 'softDisable'
  IMMEDIATE_DISABLE = 'immediateDisable'
  PERMANENT = 'permanent'


# get event name from enum
EVENT_NAME = {v: k for k, v in EventName.schema.enumerants.items()}
FROGPILOT_EVENT_NAME = {v: k for k, v in FrogPilotEventName.schema.enumerants.items()}


class Events:
  def __init__(self, frogpilot=False):
    self.events: list[int] = []
    self.static_events: list[int] = []
    self.event_counters = dict.fromkeys((FROGPILOT_EVENTS if frogpilot else EVENTS).keys(), 0)

    # FrogPilot variables
    self.frogpilot = frogpilot

  @property
  def names(self) -> list[int]:
    return self.events

  def __len__(self) -> int:
    return len(self.events)

  def add(self, event_name: int, static: bool=False) -> None:
    if static:
      bisect.insort(self.static_events, event_name)
    bisect.insort(self.events, event_name)

  def clear(self) -> None:
    self.event_counters = {k: (v + 1 if k in self.events else 0) for k, v in self.event_counters.items()}
    self.events = self.static_events.copy()

  def contains(self, event_type: str) -> bool:
    return any(event_type in (FROGPILOT_EVENTS if self.frogpilot else EVENTS).get(e, {}) for e in self.events)

  def create_alerts(self, event_types: list[str], callback_args=None):
    if callback_args is None:
      callback_args = []

    ret = []
    for e in self.events:
      types = (FROGPILOT_EVENTS if self.frogpilot else EVENTS)[e].keys()
      for et in event_types:
        if et in types:
          alert = (FROGPILOT_EVENTS if self.frogpilot else EVENTS)[e][et]
          if not isinstance(alert, Alert):
            alert = alert(*callback_args)

          if DT_CTRL * (self.event_counters[e] + 1) >= alert.creation_delay:
            alert.alert_type = f"{(FROGPILOT_EVENT_NAME if self.frogpilot else EVENT_NAME)[e]}/{et}"
            alert.event_type = et
            ret.append(alert)
    return ret

  def add_from_msg(self, events):
    for e in events:
      bisect.insort(self.events, e.name.raw)

  def to_msg(self):
    ret = []
    for event_name in self.events:
      event = (custom.FrogPilotCarEvent if self.frogpilot else car.CarEvent).new_message()
      event.name = event_name
      for event_type in (FROGPILOT_EVENTS if self.frogpilot else EVENTS).get(event_name, {}):
        setattr(event, event_type, True)
      ret.append(event)
    return ret


class Alert:
  def __init__(self,
               alert_text_1: str,
               alert_text_2: str,
               alert_status: log.ControlsState.AlertStatus,
               alert_size: log.ControlsState.AlertSize,
               priority: Priority,
               visual_alert: car.CarControl.HUDControl.VisualAlert,
               audible_alert: car.CarControl.HUDControl.AudibleAlert,
               duration: float,
               alert_rate: float = 0.,
               creation_delay: float = 0.):

    self.alert_text_1 = alert_text_1
    self.alert_text_2 = alert_text_2
    self.alert_status = alert_status
    self.alert_size = alert_size
    self.priority = priority
    self.visual_alert = visual_alert
    self.audible_alert = audible_alert

    self.duration = int(duration / DT_CTRL)

    self.alert_rate = alert_rate
    self.creation_delay = creation_delay

    self.alert_type = ""
    self.event_type: str | None = None

  def __str__(self) -> str:
    return f"{self.alert_text_1}/{self.alert_text_2} {self.priority} {self.visual_alert} {self.audible_alert}"

  def __gt__(self, alert2) -> bool:
    if not isinstance(alert2, Alert):
      return False
    return self.priority > alert2.priority


class NoEntryAlert(Alert):
  def __init__(self, alert_text_2: str,
               alert_text_1: str = "openpilot 無法使用",
               visual_alert: car.CarControl.HUDControl.VisualAlert=VisualAlert.none):
    super().__init__(alert_text_1, alert_text_2, AlertStatus.normal,
                     AlertSize.mid, Priority.LOW, visual_alert,
                     AudibleAlert.none, 3.)


class SoftDisableAlert(Alert):
  def __init__(self, alert_text_2: str):
    super().__init__("請立刻接管車輛控制", alert_text_2,
                     AlertStatus.userPrompt, AlertSize.full,
                     Priority.MID, VisualAlert.steerRequired,
                     AudibleAlert.warningSoft, 2.),


# less harsh version of SoftDisable, where the condition is user-triggered
class UserSoftDisableAlert(SoftDisableAlert):
  def __init__(self, alert_text_2: str):
    super().__init__(alert_text_2),
    self.alert_text_1 = "openpilot即將失效"


class ImmediateDisableAlert(Alert):
  def __init__(self, alert_text_2: str):
    super().__init__("請立刻接管車輛控制", alert_text_2,
                     AlertStatus.critical, AlertSize.full,
                     Priority.HIGHEST, VisualAlert.steerRequired,
                     AudibleAlert.warningImmediate, 4.),


class EngagementAlert(Alert):
  def __init__(self, audible_alert: car.CarControl.HUDControl.AudibleAlert):
    super().__init__("", "",
                     AlertStatus.normal, AlertSize.none,
                     Priority.MID, VisualAlert.none,
                     audible_alert, .2),


class NormalPermanentAlert(Alert):
  def __init__(self, alert_text_1: str, alert_text_2: str = "", duration: float = 0.2, priority: Priority = Priority.LOWER, creation_delay: float = 0.):
    super().__init__(alert_text_1, alert_text_2,
                     AlertStatus.normal, AlertSize.mid if len(alert_text_2) else AlertSize.small,
                     priority, VisualAlert.none, AudibleAlert.none, duration, creation_delay=creation_delay),


class StartupAlert(Alert):
  def __init__(self, alert_text_1: str, alert_text_2: str = "注意路況並雙手放於方向盤上", alert_status=AlertStatus.normal):
    super().__init__(alert_text_1, alert_text_2,
                     alert_status, AlertSize.mid,
                     Priority.LOWER, VisualAlert.none, AudibleAlert.none, 5.),


# ********** helper functions **********
def get_display_speed(speed_ms: float, metric: bool) -> str:
  speed = int(round(speed_ms * (CV.MS_TO_KPH if metric else CV.MS_TO_MPH)))
  unit = 'km/h' if metric else 'mph'
  return f"{speed} {unit}"


# ********** alert callback functions **********

AlertCallbackType = Callable[[car.CarParams, car.CarState, messaging.SubMaster, bool, int], Alert]


def soft_disable_alert(alert_text_2: str) -> AlertCallbackType:
  def func(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
    if soft_disable_time < int(0.5 / DT_CTRL):
      return ImmediateDisableAlert(alert_text_2)
    return SoftDisableAlert(alert_text_2)
  return func

def user_soft_disable_alert(alert_text_2: str) -> AlertCallbackType:
  def func(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
    if soft_disable_time < int(0.5 / DT_CTRL):
      return ImmediateDisableAlert(alert_text_2)
    return UserSoftDisableAlert(alert_text_2)
  return func

def startup_master_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  branch = get_short_branch()  # Ensure get_short_branch is cached to avoid lags on startup
  if "REPLAY" in os.environ:
    branch = "replay"

  return StartupAlert("~行車平安 旅途愉快~", branch, alert_status=AlertStatus.userPrompt)

def below_engage_speed_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  return NoEntryAlert(f"提高速度至 {get_display_speed(CP.minEnableSpeed, metric)} 開始使用")


def below_steer_speed_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  return Alert(
    f"轉向無法使用因低於{get_display_speed(CP.minSteerSpeed, metric)}",
    "",
    AlertStatus.userPrompt, AlertSize.small,
    Priority.LOW, VisualAlert.steerRequired, AudibleAlert.prompt, 0.4)


def calibration_incomplete_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  first_word = '重新校正' if sm['liveCalibration'].calStatus == log.LiveCalibrationData.Status.recalibrating else '校正'
  return Alert(
    f"{first_word} 進行中: {sm['liveCalibration'].calPerc:.0f}%",
    f"把時速提高超過 {get_display_speed(MIN_SPEED_FILTER, metric)}",
    AlertStatus.normal, AlertSize.mid,
    Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .2)


# *** debug alerts ***

def out_of_space_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  full_perc = round(100. - sm['deviceState'].freeSpacePercent)
  return NormalPermanentAlert("儲存空間不足", f"{full_perc}% 滿載")


def posenet_invalid_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  mdl = sm['modelV2'].velocity.x[0] if len(sm['modelV2'].velocity.x) else math.nan
  err = CS.vEgo - mdl
  msg = f"時速錯誤: {err:.1f} m/s"
  return NoEntryAlert(msg, alert_text_1="Posenet 時速錯誤")


def process_not_running_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  not_running = [p.name for p in sm['managerState'].processes if not p.running and p.shouldBeRunning]
  msg = ', '.join(not_running)
  return NoEntryAlert(msg, alert_text_1="程序未正確運行")


def comm_issue_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  bs = [s for s in sm.data.keys() if not sm.all_checks([s, ])]
  msg = ', '.join(bs[:4])  # can't fit too many on one line
  return NoEntryAlert(msg, alert_text_1="程式間的訊號有問題")


def camera_malfunction_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  all_cams = ('roadCameraState', 'driverCameraState', 'wideRoadCameraState')
  bad_cams = [s.replace('State', '') for s in all_cams if s in sm.data.keys() and not sm.all_checks([s, ])]
  return NormalPermanentAlert("鏡頭故障", ', '.join(bad_cams))


def calibration_invalid_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  rpy = sm['liveCalibration'].rpyCalib
  yaw = math.degrees(rpy[2] if len(rpy) == 3 else math.nan)
  pitch = math.degrees(rpy[1] if len(rpy) == 3 else math.nan)
  angles = f"Remount Device (Pitch: {pitch:.1f}°, Yaw: {yaw:.1f}°)"
  return NormalPermanentAlert("校正無效", angles)


def overheat_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  cpu = max(sm['deviceState'].cpuTempC, default=0.)
  gpu = max(sm['deviceState'].gpuTempC, default=0.)
  temp = max((cpu, gpu, sm['deviceState'].memoryTempC))
  return NormalPermanentAlert("系統過熱", f"{temp:.0f} °C")


def low_memory_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  return NormalPermanentAlert("記憶體不足", f"{sm['deviceState'].memoryUsagePercent}% 已使用")


def high_cpu_usage_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  x = max(sm['deviceState'].cpuUsagePercent, default=0.)
  return NormalPermanentAlert("CPU使用率過高", f"{x}% 已使用")


def modeld_lagging_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  return NormalPermanentAlert("駕駛模型延遲", f"{sm['modelV2'].frameDropPerc:.1f}% frames dropped")


def wrong_car_mode_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  if frogpilot_toggles.has_cc_long:
    text = "啟用定速來啟動OPENPILOT"
  elif CP.carName == "honda":
    text = "Enable Main Switch to Engage"
  else:
    text = "Enable Adaptive Cruise to Engage"
  return NoEntryAlert(text)


def joystick_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  axes = sm['testJoystick'].axes
  gb, steer = list(axes)[:2] if len(axes) else (0., 0.)
  vals = f"Gas: {round(gb * 100.)}%, Steer: {round(steer * 100.)}%"
  return NormalPermanentAlert("Joystick Mode", vals)


# FrogPilot alerts
def custom_startup_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  return StartupAlert(frogpilot_toggles.startup_alert_top, frogpilot_toggles.startup_alert_bottom, alert_status=FrogPilotAlertStatus.frogpilot)


def forcing_stop_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  model_length = sm["frogpilotPlan"].forcingStopLength
  model_length_msg = f"{model_length:.1f} meters" if metric else f"{model_length * CV.METER_TO_FOOT:.1f} feet"

  return Alert(
    f"Forcing the car to stop in {model_length_msg}",
    "Press the gas pedal or 'Resume' button to override",
    FrogPilotAlertStatus.frogpilot, AlertSize.mid,
    Priority.MID, VisualAlert.none, AudibleAlert.prompt, 1.)


def holiday_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  holiday_messages = {
    "new_years": "Happy New Year! 🎉",
    "valentines": "Happy Valentine's Day! ❤️",
    "st_patricks": "Happy St. Patrick's Day! 🍀",
    "world_frog_day": "Happy World Frog Day! 🐸",
    "april_fools": "Happy April Fool's Day! 🤡",
    "easter_week": "Happy Easter! 🐰",
    "may_the_fourth": "May the 4th be with you! 🚀",
    "cinco_de_mayo": "¡Feliz Cinco de Mayo! 🌮",
    "stitch_day": "Happy Stitch Day! 💙",
    "fourth_of_july": "Happy Fourth of July! 🎆",
    "halloween_week": "Happy Halloween! 🎃",
    "thanksgiving_week": "Happy Thanksgiving! 🦃",
    "christmas_week": "Merry Christmas! 🎄",
  }

  return Alert(
    holiday_messages.get(frogpilot_toggles.current_holiday_theme, ""),
    "",
    AlertStatus.normal, AlertSize.small,
    Priority.LOWEST, VisualAlert.none, FrogPilotAudibleAlert.startup, 5.)


def no_lane_available_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  lane_width = sm["frogpilotPlan"].laneWidthLeft if CS.leftBlinker else sm["frogpilotPlan"].laneWidthRight
  lane_width_msg = f"{lane_width:.1f} 公尺" if metric else f"{lane_width * CV.METER_TO_FOOT:.1f} 英尺"

  return Alert(
    "無可用車道",
    f"車道寬度僅為 {lane_width_msg}",
    AlertStatus.normal, AlertSize.mid,
    Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .2)


def torque_nn_load_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, frogpilot_toggles: SimpleNamespace) -> Alert:
  model_name = Params().get("NNFFModelName", encoding="utf-8")
  if model_name is None:
    return Alert(
      "NNFF 扭矩控制無法載入",
      "提供LOGS給Twilsonco以便加入您的車型!",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.LOW, VisualAlert.none, AudibleAlert.prompt, 10.0)
  else:
    return Alert(
      "NNFF 扭矩控制已載入",
      model_name,
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 5.0)


EVENTS: dict[int, dict[str, Alert | AlertCallbackType]] = {
  # ********** events with no alerts **********

  EventName.stockFcw: {},
  EventName.actuatorsApiUnavailable: {},

  # ********** events only containing alerts displayed in all states **********

  EventName.joystickDebug: {
    ET.WARNING: joystick_alert,
    ET.PERMANENT: NormalPermanentAlert("搖桿模式"),
  },

  EventName.controlsInitializing: {
    ET.NO_ENTRY: NoEntryAlert("系統初始化"),
  },

  EventName.startup: {
    ET.PERMANENT: StartupAlert("請隨時準備接管")
  },

  EventName.startupMaster: {
    ET.PERMANENT: startup_master_alert,
  },

  # Car is recognized, but marked as dashcam only
  EventName.startupNoControl: {
    ET.PERMANENT: StartupAlert("行車記錄模式"),
    ET.NO_ENTRY: NoEntryAlert("行車記錄模式"),
  },

  # Car is not recognized
  EventName.startupNoCar: {
    ET.PERMANENT: StartupAlert("適用於不支援的汽車的行車記錄器模式"),
  },

  EventName.startupNoFw: {
    ET.PERMANENT: StartupAlert("汽車無法識別",
                               "檢查逗號電源連接",
                               alert_status=AlertStatus.userPrompt),
  },

  EventName.startupNoSecOcKey: {
    ET.PERMANENT: NormalPermanentAlert("行車記錄儀模式",
                                       "Security Key Not Available",
                                       priority=Priority.HIGH),
  },

  EventName.dashcamMode: {
    ET.PERMANENT: NormalPermanentAlert("行車記錄模式",
                                       priority=Priority.LOWEST),
  },

  EventName.invalidLkasSetting: {
    ET.PERMANENT: NormalPermanentAlert("Stock LKAS is on",
                                       "Turn off stock LKAS to engage"),
  },

  EventName.cruiseMismatch: {
    #ET.PERMANENT: ImmediateDisableAlert("openpilot 取消巡航失敗"),
  },

  # openpilot doesn't recognize the car. This switches openpilot into a
  # read-only mode. This can be solved by adding your fingerprint.
  # See https://github.com/commaai/openpilot/wiki/Fingerprinting for more information
  EventName.carUnrecognized: {
    ET.PERMANENT: NormalPermanentAlert("行車記錄儀模式",
                                       "Car Unrecognized",
                                       priority=Priority.LOWEST),
  },

  EventName.stockAeb: {
    ET.PERMANENT: Alert(
      "立刻煞車!",
      "Stock AEB: Risk of Collision",
      AlertStatus.critical, AlertSize.full,
      Priority.HIGHEST, VisualAlert.fcw, AudibleAlert.none, 2.),
    ET.NO_ENTRY: NoEntryAlert("Stock AEB: Risk of Collision"),
  },

  EventName.fcw: {
    ET.PERMANENT: Alert(
      "立刻煞車!",
      "Risk of Collision",
      AlertStatus.critical, AlertSize.full,
      Priority.HIGHEST, VisualAlert.fcw, AudibleAlert.warningSoft, 2.),
  },

  EventName.ldw: {
    ET.PERMANENT: Alert(
      "偏離車道",
      "",
      AlertStatus.userPrompt, AlertSize.small,
      Priority.LOW, VisualAlert.ldw, AudibleAlert.prompt, 3.),
  },

##################NAV語音#####################################################
  # EventName.navturn: {
  #   ET.WARNING: Alert(
  #     "準備轉彎!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navturn, 1.),
  # },

  # EventName.navuturn: {
  #   ET.WARNING: Alert(
  #     "準備迴轉!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navuturn, 1.),
  # },

  # EventName.navturnleft: {
  #   ET.WARNING: Alert(
  #     "準備左轉!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navturnleft, 1.),
  # },

  # EventName.navturnright: {
  #   ET.WARNING: Alert(
  #     "準備右轉!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navturnright, 1.),
  # },

  # EventName.navsharpleft: {
  #   ET.WARNING: Alert(
  #     "準備緊急左轉!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navsharpleft, 1.),
  # },

  # EventName.navsharpright: {
  #   ET.WARNING: Alert(
  #     "準備緊急右轉!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navsharpright, 1.),
  # },

  # EventName.navofframp: {
  #   ET.WARNING: Alert(
  #     "準備下交流道!!",
  #     "",
  #     AlertStatus.userPrompt, AlertSize.none,
  #     Priority.LOW, VisualAlert.none, AudibleAlert.navofframp, 1.),
  # },
  ##################NAV語音#####################################################
  EventName.speedover: {
    ET.WARNING: Alert(
      "注意!!   超速了!!!",
      "",
      AlertStatus.userPrompt, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.engage, 1.),
  },

  EventName.doorOpen1: {
    ET.PERMANENT: Alert(
      "記得熄火",
      "",
      AlertStatus.normal, AlertSize.full,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.warningSoft, .2, creation_delay=0.5),
    ET.USER_DISABLE: ImmediateDisableAlert("記得熄火"),
    ET.NO_ENTRY: NoEntryAlert("記得熄火"),
  },

###############################

  # ********** events only containing alerts that display while engaged **********

  EventName.steerTempUnavailableSilent: {
    ET.WARNING: Alert(
      "怠速熄火啟動中",
      "",
      AlertStatus.userPrompt, AlertSize.small,
      Priority.LOW, VisualAlert.steerRequired, AudibleAlert.none, 1.8),
  },

  EventName.preDriverDistracted: {
    ET.PERMANENT: Alert(
      "注意!!",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, .1),
  },

  EventName.promptDriverDistracted: {
    ET.PERMANENT: Alert(
      "注意!!",
      "駕駛分心",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.MID, VisualAlert.steerRequired, AudibleAlert.promptDistracted, .1),
  },

  EventName.driverDistracted: {
    ET.PERMANENT: Alert(
      "立即解除控制",
      "駕駛分心",
      AlertStatus.critical, AlertSize.full,
      Priority.HIGH, VisualAlert.steerRequired, AudibleAlert.warningImmediate, .1),
  },

  EventName.preDriverUnresponsive: {
    ET.PERMANENT: Alert(
      "手須放在方向盤: 沒有偵測到臉部",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.steerRequired, AudibleAlert.none, .1, alert_rate=0.75),
  },

  EventName.promptDriverUnresponsive: {
    ET.PERMANENT: Alert(
      "手須放在方向盤",
      "駕駛沒有反應",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.MID, VisualAlert.steerRequired, AudibleAlert.promptDistracted, .1),
  },

  EventName.driverUnresponsive: {
    ET.PERMANENT: Alert(
      "立即解除控制",
      "駕駛沒有反應",
      AlertStatus.critical, AlertSize.full,
      Priority.HIGH, VisualAlert.steerRequired, AudibleAlert.warningImmediate, .1),
  },

  EventName.manualRestart: {
    ET.WARNING: Alert(
      "立刻介入控制",
      "恢復手動駕駛",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, .2),
  },

  EventName.resumeRequired: {
    ET.WARNING: Alert(
      "按壓 Resume 解除停止狀態",
      "",
      AlertStatus.userPrompt, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, .2),
  },

  EventName.belowSteerSpeed: {
    ET.WARNING: below_steer_speed_alert,
  },

  EventName.preLaneChangeLeft: {
    ET.WARNING: Alert(
      "向左轉方向盤安全變換車道一次",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, .1, alert_rate=0.75),
  },

  EventName.preLaneChangeRight: {
    ET.WARNING: Alert(
      "向右轉方向盤安全變換車道一次",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, .1, alert_rate=0.75),
  },

  EventName.laneChangeBlocked: {
    ET.WARNING: Alert(
      "盲點偵測到車輛暫停變換車道",
      "",
      AlertStatus.userPrompt, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.lanechangeblockedsound, .1),
  },

  EventName.laneChange: {
    ET.WARNING: Alert(
      "變換車道中",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.lanechangesound, .1),
  },

  EventName.steerSaturated: {
    ET.WARNING: Alert(
      "接管控制",
      "轉彎幅度超出轉向限制",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.LOW, VisualAlert.steerRequired, AudibleAlert.promptRepeat, 2.),
  },

  # Thrown when the fan is driven at >50% but is not rotating
  EventName.fanMalfunction: {
    ET.PERMANENT: NormalPermanentAlert("風扇故障", "可能硬體有問題"),
  },

  # Camera is not outputting frames
  EventName.cameraMalfunction: {
    ET.PERMANENT: camera_malfunction_alert,
    ET.SOFT_DISABLE: soft_disable_alert("鏡頭故障"),
    ET.NO_ENTRY: NoEntryAlert("鏡頭故障請重啟設備"),
  },
  # Camera framerate too low
  EventName.cameraFrameRate: {
    ET.PERMANENT: NormalPermanentAlert("鏡頭畫面頻率偏低", "請重啟設備"),
    ET.SOFT_DISABLE: soft_disable_alert("鏡頭畫面頻率偏低"),
    ET.NO_ENTRY: NoEntryAlert("鏡頭畫面頻率偏低: 請重啟設備"),
  },

  # Unused

  EventName.locationdTemporaryError: {
    ET.NO_ENTRY: NoEntryAlert("定位臨時錯誤"),
    ET.SOFT_DISABLE: soft_disable_alert("定位臨時錯誤"),
  },

  EventName.locationdPermanentError: {
    ET.NO_ENTRY: NoEntryAlert("定位錯誤"),
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("定位錯誤"),
    ET.PERMANENT: NormalPermanentAlert("定位錯誤"),
  },

  # openpilot tries to learn certain parameters about your car by observing
  # how the car behaves to steering inputs from both human and openpilot driving.
  # This includes:
  # - steer ratio: gear ratio of the steering rack. Steering angle divided by tire angle
  # - tire stiffness: how much grip your tires have
  # - angle offset: most steering angle sensors are offset and measure a non zero angle when driving straight
  # This alert is thrown when any of these values exceed a sanity check. This can be caused by
  # bad alignment or bad sensor data. If this happens consistently consider creating an issue on GitHub
  EventName.paramsdTemporaryError: {
    ET.NO_ENTRY: NoEntryAlert("paramsd 臨時錯誤"),
    ET.SOFT_DISABLE: soft_disable_alert("paramsd 臨時錯誤"),
  },

  EventName.paramsdPermanentError: {
    ET.NO_ENTRY: NoEntryAlert("paramsd 永久錯誤"),
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("paramsd 永久錯誤"),
    ET.PERMANENT: NormalPermanentAlert("paramsd 永久錯誤"),
  },

  # ********** events that affect controls state transitions **********

  EventName.pcmEnable: {
    ET.ENABLE: EngagementAlert(AudibleAlert.none),
  },

  EventName.buttonEnable: {
    ET.ENABLE: EngagementAlert(AudibleAlert.none),
  },

  EventName.pcmDisable: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
  },

  EventName.buttonCancel: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("取消鍵被按著"),
  },

  EventName.brakeHold: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("煞車維持啟動"),
  },

  EventName.parkBrake: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("手煞車使用中"),
  },

  EventName.pedalPressed: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("煞車踩著",
                              visual_alert=VisualAlert.brakePressed),
  },

  EventName.preEnableStandstill: {
    ET.PRE_ENABLE: Alert(
      "放開煞車開始定速",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1, creation_delay=1.),
  },

  EventName.gasPressedOverride: {
    ET.OVERRIDE_LONGITUDINAL: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1),
  },

  EventName.steerOverride: {
    ET.OVERRIDE_LATERAL: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1),
  },

  EventName.wrongCarMode: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: wrong_car_mode_alert,
  },

  EventName.resumeBlocked: {
    ET.NO_ENTRY: NoEntryAlert("需先按Set啟動控制"),
  },

  EventName.wrongCruiseMode: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.disengage),
    ET.NO_ENTRY: NoEntryAlert("定速關閉"),
  },

  EventName.steerTempUnavailable: {
    ET.SOFT_DISABLE: soft_disable_alert("方向盤暫時無法使用"),
    ET.NO_ENTRY: NoEntryAlert("方向盤暫時無法使用"),
  },

  EventName.steerTimeLimit: {
    ET.SOFT_DISABLE: soft_disable_alert("車輛方向盤使用超時"),
    ET.NO_ENTRY: NoEntryAlert("車輛方向盤使用超時"),
  },

  EventName.outOfSpace: {
    ET.PERMANENT: out_of_space_alert,
    ET.NO_ENTRY: NoEntryAlert("Out of Storage"),
  },

  EventName.belowEngageSpeed: {
    ET.NO_ENTRY: below_engage_speed_alert,
  },

  EventName.sensorDataInvalid: {
    ET.PERMANENT: Alert(
      "感應器數據無效",
      "確保設備安裝正確",
      AlertStatus.normal, AlertSize.mid,
      Priority.LOWER, VisualAlert.none, AudibleAlert.none, .2, creation_delay=1.),
    ET.NO_ENTRY: NoEntryAlert("感應器數據無效"),
    ET.SOFT_DISABLE: soft_disable_alert("感應器數據無效"),
  },

  EventName.noGps: {
    ET.PERMANENT: Alert(
      "GPS 訊號不良",
      "若在看的見天空的環境下，可能硬體有問題",
      AlertStatus.normal, AlertSize.mid,
      Priority.LOWER, VisualAlert.none, AudibleAlert.none, .2, creation_delay=600.)
  },

  EventName.soundsUnavailable: {
    ET.PERMANENT: NormalPermanentAlert("喇叭失效", "請重啟設備"),
    ET.NO_ENTRY: NoEntryAlert("喇叭失效"),
  },

  EventName.tooDistracted: {
    ET.NO_ENTRY: NoEntryAlert("分心程度太高"),
  },

  EventName.overheat: {
    ET.PERMANENT: overheat_alert,
    ET.SOFT_DISABLE: soft_disable_alert("系統過熱"),
    ET.NO_ENTRY: NoEntryAlert("系統過熱"),
  },

  EventName.wrongGear: {
    ET.SOFT_DISABLE: user_soft_disable_alert("不在 D 檔"),
    ET.NO_ENTRY: NoEntryAlert("不在 D 檔"),
  },

  # This alert is thrown when the calibration angles are outside of the acceptable range.
  # For example if the device is pointed too much to the left or the right.
  # Usually this can only be solved by removing the mount from the windshield completely,
  # and attaching while making sure the device is pointed straight forward and is level.
  # See https://comma.ai/setup for more information
  EventName.calibrationInvalid: {
    ET.PERMANENT: calibration_invalid_alert,
    ET.SOFT_DISABLE: soft_disable_alert("校正失敗：重新安裝設備並重新校正"),
    ET.NO_ENTRY: NoEntryAlert("校正失敗：重新安裝設備並重新校正"),
  },

  EventName.calibrationIncomplete: {
    ET.PERMANENT: calibration_incomplete_alert,
    ET.SOFT_DISABLE: soft_disable_alert("校正不完整"),
    ET.NO_ENTRY: NoEntryAlert("校正進行中"),
  },

  EventName.calibrationRecalibrating: {
    ET.PERMANENT: calibration_incomplete_alert,
    ET.SOFT_DISABLE: soft_disable_alert("偵測到設備重新安裝：請重新校正"),
    ET.NO_ENTRY: NoEntryAlert("偵測到設備重新安裝：請重新校正"),
  },

  EventName.doorOpen: {
    ET.SOFT_DISABLE: user_soft_disable_alert("車門開啟"),
    ET.NO_ENTRY: NoEntryAlert("車門開啟"),
  },

  EventName.seatbeltNotLatched: {
    ET.SOFT_DISABLE: user_soft_disable_alert("未使用安全帶"),
    ET.NO_ENTRY: NoEntryAlert("未使用安全帶"),
  },

  EventName.espDisabled: {
    ET.SOFT_DISABLE: soft_disable_alert("電子穩定控制禁用"),
    ET.NO_ENTRY: NoEntryAlert("電子穩定控制禁用"),
  },

  EventName.lowBattery: {
    ET.SOFT_DISABLE: soft_disable_alert("電池電壓過低"),
    ET.NO_ENTRY: NoEntryAlert("電池電壓過低"),
  },

  # Different openpilot services communicate between each other at a certain
  # interval. If communication does not follow the regular schedule this alert
  # is thrown. This can mean a service crashed, did not broadcast a message for
  # ten times the regular interval, or the average interval is more than 10% too high.
  EventName.commIssue: {
    ET.SOFT_DISABLE: soft_disable_alert("程序通訊有問題"),
    ET.NO_ENTRY: comm_issue_alert,
  },
  EventName.commIssueAvgFreq: {
    ET.SOFT_DISABLE: soft_disable_alert("程序通訊頻率低"),
    ET.NO_ENTRY: NoEntryAlert("程序通訊頻率低"),
  },

  EventName.controlsdLagging: {
    ET.SOFT_DISABLE: soft_disable_alert("控制延遲"),
    ET.NO_ENTRY: NoEntryAlert("控制程序延遲: 請重啟設備"),
  },

  # Thrown when manager detects a service exited unexpectedly while driving
  EventName.processNotRunning: {
    ET.NO_ENTRY: process_not_running_alert,
    ET.SOFT_DISABLE: soft_disable_alert("程序未正確運行"),
  },

  EventName.radarFault: {
    ET.SOFT_DISABLE: soft_disable_alert("雷達錯誤: 請重新發動車子"),
    ET.NO_ENTRY: NoEntryAlert("雷達錯誤: 請重新發動車子"),
  },

  # Every frame from the camera should be processed by the model. If modeld
  # is not processing frames fast enough they have to be dropped. This alert is
  # thrown when over 20% of frames are dropped.
  EventName.modeldLagging: {
    ET.SOFT_DISABLE: soft_disable_alert("駕駛模型延遲"),
    ET.NO_ENTRY: NoEntryAlert("駕駛模型延遲"),
    ET.PERMANENT: modeld_lagging_alert,
  },

  # Besides predicting the path, lane lines and lead car data the model also
  # predicts the current velocity and rotation speed of the car. If the model is
  # very uncertain about the current velocity while the car is moving, this
  # usually means the model has trouble understanding the scene. This is used
  # as a heuristic to warn the driver.
  EventName.posenetInvalid: {
    ET.SOFT_DISABLE: soft_disable_alert("Posenet 速度無效"),
    ET.NO_ENTRY: posenet_invalid_alert,
  },

  # When the localizer detects an acceleration of more than 40 m/s^2 (~4G) we
  # alert the driver the device might have fallen from the windshield.
  EventName.deviceFalling: {
    ET.SOFT_DISABLE: soft_disable_alert("設備從支架掉落"),
    ET.NO_ENTRY: NoEntryAlert("設備從支架掉落"),
  },

  EventName.lowMemory: {
    ET.SOFT_DISABLE: soft_disable_alert("記憶體不足: 請重新啟動設備"),
    ET.PERMANENT: low_memory_alert,
    ET.NO_ENTRY: NoEntryAlert("記憶體不足: 請重新啟動設備"),
  },

  EventName.highCpuUsage: {
    #ET.SOFT_DISABLE: soft_disable_alert("系統故障：重新啟動您的裝置"),
    #ET.PERMANENT: NormalPermanentAlert("系統故障”，“重新啟動設備"),
    ET.NO_ENTRY: high_cpu_usage_alert,
  },

  EventName.accFaulted: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("定速異常：請重新啟動汽車"),
    ET.PERMANENT: NormalPermanentAlert("定速失敗: 請重新啟動車輛恢復使用"),
    ET.NO_ENTRY: NoEntryAlert("定速異常：請重新啟動汽車"),
  },

  EventName.controlsMismatch: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("控制不匹配"),
    ET.NO_ENTRY: NoEntryAlert("控制不匹配"),
  },

  EventName.roadCameraError: {
    ET.PERMANENT: NormalPermanentAlert("鏡頭錯誤 - 路面",
                                       duration=1.,
                                       creation_delay=30.),
  },

  EventName.wideRoadCameraError: {
    ET.PERMANENT: NormalPermanentAlert("鏡頭錯誤 - 路面魚眼",
                                       duration=1.,
                                       creation_delay=30.),
  },

  EventName.driverCameraError: {
    ET.PERMANENT: NormalPermanentAlert("鏡頭錯誤 - 駕駛",
                                       duration=1.,
                                       creation_delay=30.),
  },

  # Sometimes the USB stack on the device can get into a bad state
  # causing the connection to the panda to be lost
  EventName.usbError: {
    ET.SOFT_DISABLE: soft_disable_alert("USB 錯誤: 請重啟設備"),
    ET.PERMANENT: NormalPermanentAlert("USB 錯誤: 請重啟設備", ""),
    ET.NO_ENTRY: NoEntryAlert("USB 錯誤: 請重啟設備"),
  },

  # This alert can be thrown for the following reasons:
  # - No CAN data received at all
  # - CAN data is received, but some message are not received at the right frequency
  # If you're not writing a new car port, this is usually cause by faulty wiring
  EventName.canError: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("CAN 資料錯誤"),
    ET.PERMANENT: Alert(
      "CAN 錯誤: 請確認訊號線路連接正常",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 1., creation_delay=1.),
    ET.NO_ENTRY: NoEntryAlert("CAN 錯誤: 請確認訊號線路連接正常"),
  },

  EventName.canBusMissing: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("CAN 總線斷開"),
    ET.PERMANENT: Alert(
      "CAN 總線斷開: 可能是線路故障",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 1., creation_delay=1.),
    ET.NO_ENTRY: NoEntryAlert("CAN 總線斷開: 請確認連接"),
  },

  EventName.steerUnavailable: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("全時置中啟動中"),
    ET.PERMANENT: NormalPermanentAlert("全時置中啟動中"),
    ET.NO_ENTRY: NoEntryAlert("全時置中啟動中"),
  },

  EventName.reverseGear: {
    ET.PERMANENT: Alert(
      "倒車中",
      "",
      AlertStatus.normal, AlertSize.full,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .2, creation_delay=0.5),
    ET.USER_DISABLE: ImmediateDisableAlert("倒車中"),
    ET.NO_ENTRY: NoEntryAlert("倒車中"),
  },

  # On cars that use stock ACC the car can decide to cancel ACC for various reasons.
  # When this happens we can no long control the car so the user needs to be warned immediately.
  EventName.cruiseDisabled: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("定速巡航已關閉"),
  },

  # When the relay in the harness box opens the CAN bus between the LKAS camera
  # and the rest of the car is separated. When messages from the LKAS camera
  # are received on the car side this usually means the relay hasn't opened correctly
  # and this alert is thrown.
  EventName.relayMalfunction: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("線束繼電器故障"),
    ET.PERMANENT: NormalPermanentAlert("線束繼電器故障", "請檢查硬體"),
    ET.NO_ENTRY: NoEntryAlert("線束繼電器故障"),
  },

  EventName.speedTooLow: {
    ET.IMMEDIATE_DISABLE: Alert(
      "openpilot 被取消",
      "速度太低",
      AlertStatus.normal, AlertSize.mid,
      Priority.HIGH, VisualAlert.none, AudibleAlert.none, 3.),
  },

  # When the car is driving faster than most cars in the training data, the model outputs can be unpredictable.
  EventName.speedTooHigh: {
    ET.WARNING: Alert(
      "速度太高",
      "模型在此速度下不確定",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.HIGH, VisualAlert.steerRequired, AudibleAlert.promptRepeat, 4.),
    ET.NO_ENTRY: NoEntryAlert("降低速度恢復使用"),
  },

  EventName.lowSpeedLockout: {
    ET.PERMANENT: NormalPermanentAlert("定速失敗: 請重新啟動車輛恢復使用"),
    ET.NO_ENTRY: NoEntryAlert("定速異常：請重新啟動汽車"),
  },

  EventName.lkasDisabled: {
    ET.PERMANENT: NormalPermanentAlert("車道置中 關閉: 開啟 LKAS 恢復使用"),
    ET.NO_ENTRY: NoEntryAlert("車道置中 關閉"),
  },

  EventName.vehicleSensorsInvalid: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("車輛感應器無效"),
    ET.PERMANENT: NormalPermanentAlert("車輛感應器校正中", "開車去校正"),
    ET.NO_ENTRY: NoEntryAlert("車輛感應器校正中"),
  },
}

FROGPILOT_EVENTS: dict[int, dict[str, Alert | AlertCallbackType]] = {
  FrogPilotEventName.blockUser: {
    ET.PERMANENT: Alert(
      "Don't use the 'Development' branch!",
      "Forcing you into 'Dashcam Mode' for your safety",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.HIGHEST, VisualAlert.none, AudibleAlert.none, 1.),
  },

  FrogPilotEventName.customStartupAlert: {
    ET.PERMANENT: custom_startup_alert,
  },

  FrogPilotEventName.forcingStop: {
    ET.WARNING: forcing_stop_alert,
  },

  FrogPilotEventName.goatSteerSaturated: {
    ET.WARNING: Alert(
      "JESUS TAKE THE WHEEL!!",
      "Turn Exceeds Steering Limit",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.LOW, VisualAlert.steerRequired, FrogPilotAudibleAlert.goat, 2.),
  },

  FrogPilotEventName.greenLight: {
    ET.PERMANENT: Alert(
      "綠燈 GO!!",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.MID, VisualAlert.none, AudibleAlert.greenlightsound, 3.),
  },

  FrogPilotEventName.holidayActive: {
    ET.PERMANENT: holiday_alert,
  },

  FrogPilotEventName.laneChangeBlockedLoud: {
    ET.WARNING: Alert(
      "盲點偵測到車輛暫停變換車道",
      "",
      AlertStatus.userPrompt, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.lanechangeblockedsound, .1),
  },

  FrogPilotEventName.leadDeparting: {
    ET.PERMANENT: Alert(
      "前車遠離",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.MID, VisualAlert.none, AudibleAlert.carawayed, 3.),
  },

  FrogPilotEventName.noLaneAvailable: {
    ET.WARNING: no_lane_available_alert,
  },

  FrogPilotEventName.openpilotCrashed: {
    ET.IMMEDIATE_DISABLE: Alert(
      "openpilot 無法使用",
      "請在 FrogPilot Discord 中發布錯誤日誌!",
      AlertStatus.critical, AlertSize.mid,
      Priority.HIGHEST, VisualAlert.none, AudibleAlert.prompt, .1),

    ET.NO_ENTRY: Alert(
      "openpilot 無法使用",
      "請在 FrogPilot Discord 中發布錯誤日誌!",
      AlertStatus.critical, AlertSize.mid,
      Priority.HIGHEST, VisualAlert.none, AudibleAlert.prompt, .1),
  },

  FrogPilotEventName.pedalInterceptorNoBrake: {
    ET.WARNING: Alert(
      "無法煞車",
      "切換至 L",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.HIGH, VisualAlert.wrongGear, AudibleAlert.promptRepeat, 4.),
  },

  FrogPilotEventName.speedLimitChanged: {
    ET.PERMANENT: Alert(
      "速度限制已更改",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.prompt, 3.),
  },

  FrogPilotEventName.thisIsFineSteerSaturated: {
    ET.WARNING: Alert(
      "This is fine ☕",
      "Turn Exceeds Steering Limit",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.LOW, VisualAlert.steerRequired, FrogPilotAudibleAlert.thisIsFine, 2.),
  },

  FrogPilotEventName.torqueNNLoad: {
    ET.PERMANENT: torque_nn_load_alert,
  },

  FrogPilotEventName.trafficModeActive: {
    ET.WARNING: Alert(
      "塞車模式啟動",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 3.),
  },

  FrogPilotEventName.trafficModeInactive: {
    ET.WARNING: Alert(
      "塞車模式關閉",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.prompt, 3.),
  },

  FrogPilotEventName.turningLeft: {
    ET.WARNING: Alert(
      "左轉",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1, alert_rate=0.75),
  },

  FrogPilotEventName.turningRight: {
    ET.WARNING: Alert(
      "右轉",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1, alert_rate=0.75),
  },

  # Random Events
  FrogPilotEventName.accel30: {
    ET.WARNING: Alert(
      "UwU u went a bit fast there!",
      "(⁄ ⁄•⁄ω⁄•⁄ ⁄)",
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.LOW, VisualAlert.none, FrogPilotAudibleAlert.uwu, 4.),
  },

  FrogPilotEventName.accel35: {
    ET.WARNING: Alert(
      "I ain't giving you no tree-fiddy",
      "You damn Loch Ness Monsta!",
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.LOW, VisualAlert.none, FrogPilotAudibleAlert.nessie, 4.),
  },

  FrogPilotEventName.accel40: {
    ET.WARNING: Alert(
      "Great Scott!",
      "🚗💨",
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.LOW, VisualAlert.none, FrogPilotAudibleAlert.doc, 4.),
  },

  FrogPilotEventName.dejaVuCurve: {
    ET.PERMANENT: Alert(
      "♬♪ Deja vu! ᕕ(⌐■_■)ᕗ ♪♬",
      "🏎️",
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.LOW, VisualAlert.none, FrogPilotAudibleAlert.dejaVu, 4.),
  },

  FrogPilotEventName.firefoxSteerSaturated: {
    ET.WARNING: Alert(
      "IE Has Stopped Responding...",
      "Turn Exceeds Steering Limit",
      AlertStatus.userPrompt, AlertSize.mid,
      Priority.LOW, VisualAlert.steerRequired, FrogPilotAudibleAlert.firefox, 4.),
  },

  FrogPilotEventName.hal9000: {
    ET.WARNING: Alert(
      "I'm sorry Dave",
      "I'm afraid I can't do that...",
      AlertStatus.normal, AlertSize.mid,
      Priority.HIGH, VisualAlert.none, FrogPilotAudibleAlert.hal9000, 4.),
  },

  FrogPilotEventName.openpilotCrashedRandomEvent: {
    ET.IMMEDIATE_DISABLE: Alert(
      "openpilot crashed 💩",
      "Please post the 'Error Log' in the FrogPilot Discord!",
      AlertStatus.normal, AlertSize.mid,
      Priority.HIGHEST, VisualAlert.none, FrogPilotAudibleAlert.fart, 10.),

    ET.NO_ENTRY: Alert(
      "openpilot crashed 💩",
      "Please post the 'Error Log' in the FrogPilot Discord!",
      AlertStatus.normal, AlertSize.mid,
      Priority.HIGHEST, VisualAlert.none, FrogPilotAudibleAlert.fart, 10.),
  },

  FrogPilotEventName.toBeContinued: {
    ET.PERMANENT: Alert(
      "To be continued...",
      "⬅️",
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.MID, VisualAlert.none, FrogPilotAudibleAlert.continued, 7.),
  },

  FrogPilotEventName.vCruise69: {
    ET.WARNING: Alert(
      "Lol 69",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.LOW, VisualAlert.none, FrogPilotAudibleAlert.noice, 2.),
  },

  FrogPilotEventName.yourFrogTriedToKillMe: {
    ET.PERMANENT: Alert(
      "Your Frog tried to kill me...",
      "👺",
      FrogPilotAlertStatus.frogpilot, AlertSize.mid,
      Priority.MID, VisualAlert.none, FrogPilotAudibleAlert.angry, 5.),
  },

  FrogPilotEventName.youveGotMail: {
    ET.WARNING: Alert(
      "You've got mail! 📧",
      "",
      FrogPilotAlertStatus.frogpilot, AlertSize.small,
      Priority.LOW, VisualAlert.none, FrogPilotAudibleAlert.mail, 3.),
  },
}

if __name__ == '__main__':
  # print all alerts by type and priority
  from cereal.services import SERVICE_LIST
  from collections import defaultdict

  event_names = {v: k for k, v in EventName.schema.enumerants.items()}
  alerts_by_type: dict[str, dict[Priority, list[str]]] = defaultdict(lambda: defaultdict(list))

  CP = car.CarParams.new_message()
  CS = car.CarState.new_message()
  sm = messaging.SubMaster(list(SERVICE_LIST.keys()))

  for i, alerts in EVENTS.items():
    for et, alert in alerts.items():
      if callable(alert):
        alert = alert(CP, CS, sm, False, 1)
      alerts_by_type[et][alert.priority].append(event_names[i])

  all_alerts: dict[str, list[tuple[Priority, list[str]]]] = {}
  for et, priority_alerts in alerts_by_type.items():
    all_alerts[et] = sorted(priority_alerts.items(), key=lambda x: x[0], reverse=True)

  for status, evs in sorted(all_alerts.items(), key=lambda x: x[0]):
    print(f"**** {status} ****")
    for p, alert_list in evs:
      print(f"  {repr(p)}:")
      print("   ", ', '.join(alert_list), "\n")
