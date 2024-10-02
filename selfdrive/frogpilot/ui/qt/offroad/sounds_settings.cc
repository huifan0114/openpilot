#include "selfdrive/frogpilot/ui/qt/offroad/sounds_settings.h"

FrogPilotSoundsPanel::FrogPilotSoundsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> soundsToggles {
    {"AlertVolumeControl", tr("警報音量控制器"), tr("控制 openpilot 中每個聲音的音量級別."), "../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"DisengageVolume", tr("Disengage 音量"), tr("相關提醒:\n\nAdaptive Cruise Disabled\nParking Brake Engaged\nBrake Pedal Pressed\nSpeed too Low"), ""},
    {"EngageVolume", tr("Engage 音量"), tr("相關提醒:\n\nNNFF Torque Controller loaded\nopenpilot engaged"), ""},
    {"PromptVolume", tr("Prompt 音量"), tr("相關提醒:\n\nCar Detected in Blindspot\nSpeed too Low\nSteer Unavailable Below 'X'\nTake Control, Turn Exceeds Steering Limit"), ""},
    {"PromptDistractedVolume", tr("Prompt Distracted 音量"), tr("相關提醒:\n\nPay Attention, Driver Distracted\nTouch Steering Wheel, Driver Unresponsive"), ""},
    {"RefuseVolume", tr("Refuse 音量"), tr("相關提醒:\n\nopenpilot Unavailable"), ""},
    {"WarningSoftVolume", tr("警告 音量"), tr("相關提醒:\n\nBRAKE!, Risk of Collision\nTAKE CONTROL IMMEDIATELY"), ""},
    {"WarningImmediateVolume", tr("立即警告 音量"), tr("相關提醒:\n\nDISENGAGE IMMEDIATELY, Driver Distracted\nDISENGAGE IMMEDIATELY, Driver Unresponsive"), ""},

    {"CustomAlerts", tr("自訂警報"), tr("為 openpilot 事件啟用自訂警報."), "../frogpilot/assets/toggle_icons/icon_green_light.png"},
    {"GoatScream", tr("山羊尖叫轉向飽和警報"), tr("啟用著名的“山羊尖叫”，它給世界各地的 FrogPilot 用戶帶來了歡樂和憤怒!"), ""},
    {"GreenLightAlert", tr("綠燈警報"), tr("當交通燈由紅變綠時收到警報."), ""},
    {"LeadDepartingAlert", tr("前車遠離警報"), tr("當領頭車輛在靜止狀態下開始出發時收到警報."), ""},
    {"LoudBlindspotAlert", tr("大聲盲點警報"), tr("當嘗試變換車道時在盲點偵測到車輛時，啟用更響亮的警報."), ""},
    {"SpeedLimitChangedAlert", tr("限速變更警報"), tr("當速度限制改變時觸發警報."), ""},
  };

  for (const auto &[param, title, desc, icon] : soundsToggles) {
    AbstractControl *soundsToggle;

    if (param == "AlertVolumeControl") {
      FrogPilotParamManageControl *alertVolumeControlToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(alertVolumeControlToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(alertVolumeControlKeys);
      });
      soundsToggle = alertVolumeControlToggle;
    } else if (alertVolumeControlKeys.find(param) != alertVolumeControlKeys.end()) {
      if (param == "WarningImmediateVolume") {
        soundsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 25, 100, "%");
      } else {
        soundsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, "%");
      }

    } else if (param == "CustomAlerts") {
      FrogPilotParamManageControl *customAlertsToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(customAlertsToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedCustomAlertsKeys = customAlertsKeys;

        if (!hasBSM) {
          modifiedCustomAlertsKeys.erase("LoudBlindspotAlert");
        }

        if (!(hasOpenpilotLongitudinal && params.getBool("SpeedLimitController"))) {
          modifiedCustomAlertsKeys.erase("SpeedLimitChangedAlert");
        }

        showToggles(modifiedCustomAlertsKeys);
      });
      soundsToggle = customAlertsToggle;

    } else {
      soundsToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(soundsToggle);
    toggles[param] = soundsToggle;

    makeConnections(soundsToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(soundsToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotSoundsPanel::openParentToggle);
    }

    QObject::connect(soundsToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotSoundsPanel::hideToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateCarToggles, this, &FrogPilotSoundsPanel::updateCarToggles);
}

void FrogPilotSoundsPanel::updateCarToggles() {
  hasBSM = parent->hasBSM;
  hasOpenpilotLongitudinal = parent->hasOpenpilotLongitudinal;

  hideToggles();
}

void FrogPilotSoundsPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotSoundsPanel::hideToggles() {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    bool subToggles = alertVolumeControlKeys.find(key) != alertVolumeControlKeys.end() ||
                      customAlertsKeys.find(key) != customAlertsKeys.end();
    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}
