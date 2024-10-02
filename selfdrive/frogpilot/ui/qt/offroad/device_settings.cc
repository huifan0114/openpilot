#include "selfdrive/frogpilot/ui/qt/offroad/device_settings.h"

FrogPilotDevicePanel::FrogPilotDevicePanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> deviceToggles {
    {"DeviceManagement", tr("設備設定"), tr("設備行為設定."), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"DeviceShutdown", tr("設備關機定時器"), tr("停止駕駛後設備會保持開啟狀態多久."), ""},
    {"OfflineMode", tr("停用網路需求"), tr("只要您需要，該設備可以在沒有網路連線的情況下運作."), ""},
    {"IncreaseThermalLimits", tr("提高熱安全極限"), tr("該設備可以在比建議溫度更高的溫度下運行."), ""},
    {"LowVoltageShutdown", tr("低電量關閉閾值"), tr("當汽車電池電量過低時關閉設備，以防止損壞 12V 電池."), ""},
    {"NoLogging", tr("關閉數據追蹤"), tr("禁用所有追蹤以提高隱私性."), ""},
    {"NoUploads", tr("關閉數據上傳"), tr("停止設備向伺服器發送任何數據."), ""},

    {"ScreenManagement", tr("螢幕設定"), tr("Screen behavior settings."), "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"ScreenBrightness", tr("螢幕亮度 (停止時)"), tr("不開車時的螢幕亮度."), ""},
    {"ScreenBrightnessOnroad", tr("螢幕亮度 (行進時)"), tr("駕駛時的螢幕亮度."), ""},
    {"ScreenRecorder", tr("螢幕錄影機"), tr("在 onroad UI 中顯示一個按鈕來錄製螢幕."), ""},
    {"ScreenTimeout", tr("螢幕超時 (停止時)"), tr("當您不開車時螢幕需要多長時間才會關閉."), ""},
    {"ScreenTimeoutOnroad", tr("螢幕超時 (行進時)"), tr("開車時螢幕需要多長時間才會關閉."), ""}
  };

  for (const auto &[param, title, desc, icon] : deviceToggles) {
    AbstractControl *deviceToggle;

    if (param == "DeviceManagement") {
      FrogPilotParamManageControl *deviceManagementToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(deviceManagementToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(deviceManagementKeys);
      });
      deviceToggle = deviceManagementToggle;
    } else if (param == "DeviceShutdown") {
      std::map<int, QString> shutdownLabels;
      for (int i = 0; i <= 33; ++i) {
        shutdownLabels[i] = i == 0 ? tr("5 分鐘") : i <= 3 ? QString::number(i * 15) + tr(" 分鐘") : QString::number(i - 3) + (i == 4 ? tr(" 小時]") : tr(" 小時"));
      }
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 33, QString(), shutdownLabels);
    } else if (param == "NoUploads") {
      std::vector<QString> uploadsToggles{"DisableOnroadUploads"};
      std::vector<QString> uploadsToggleNames{tr("僅在公路上")};
      deviceToggle = new FrogPilotButtonToggleControl(param, title, desc, uploadsToggles, uploadsToggleNames);
    } else if (param == "LowVoltageShutdown") {
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 11.8, 12.5, tr(" 伏特"), std::map<int, QString>(), 0.01);

    } else if (param == "ScreenManagement") {
      FrogPilotParamManageControl *screenToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(screenToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(screenKeys);
      });
      deviceToggle = screenToggle;
    } else if (param == "ScreenBrightness" || param == "ScreenBrightnessOnroad") {
      std::map<int, QString> brightnessLabels;
      int minBrightness = (param == "ScreenBrightnessOnroad") ? 0 : 1;
      for (int i = 1; i <= 101; ++i) {
        brightnessLabels[i] = (i == 101) ? tr("自動") : QString::number(i) + "%";
      }
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, minBrightness, 101, QString(), brightnessLabels, 1, false, true);
    } else if (param == "ScreenTimeout" || param == "ScreenTimeoutOnroad") {
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 5, 60, tr(" 秒"));

    } else {
      deviceToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(deviceToggle);
    toggles[param] = deviceToggle;

    makeConnections(deviceToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(deviceToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotDevicePanel::openParentToggle);
    }

    QObject::connect(deviceToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["IncreaseThermalLimits"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("警告：如果設備運作超過 Comma 建議的溫度限制，可能會導致過早磨損或損壞!"),
        tr("我了解風險."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NoLogging"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("警告：這將阻止您的驅動器被記錄並且資料將無法獲取!"),
        tr("我了解風險."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NoUploads"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("警告：這將阻止您的驅動器出現在逗號連接上，這可能會影響調試和支援!"),
        tr("我了解風險."), this);
    }
  });

  FrogPilotParamValueControl *screenBrightnessToggle = static_cast<FrogPilotParamValueControl*>(toggles["ScreenBrightness"]);
  QObject::connect(screenBrightnessToggle, &FrogPilotParamValueControl::valueChanged, [this](float value) {
    if (!started) {
      uiState()->scene.screen_brightness = value;
    }
  });

  FrogPilotParamValueControl *screenBrightnessOnroadToggle = static_cast<FrogPilotParamValueControl*>(toggles["ScreenBrightnessOnroad"]);
  QObject::connect(screenBrightnessOnroadToggle, &FrogPilotParamValueControl::valueChanged, [this](float value) {
    if (started) {
      uiState()->scene.screen_brightness_onroad = value;
    }
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotDevicePanel::hideToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotDevicePanel::updateState);

  hideToggles();
}

void FrogPilotDevicePanel::updateState(const UIState &s) {
  started = s.scene.started;
}

void FrogPilotDevicePanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotDevicePanel::hideToggles() {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    bool subToggles = deviceManagementKeys.find(key) != deviceManagementKeys.end() ||
                      screenKeys.find(key) != screenKeys.end();
    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}
