#include "frogpilot/ui/screenrecorder/screenrecorder.h"
#include "frogpilot/ui/qt/offroad/device_settings.h"

FrogPilotDevicePanel::FrogPilotDevicePanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  ScreenRecorder *screenRecorder = new ScreenRecorder(this);
  screenRecorder->setVisible(false);

  QStackedLayout *deviceLayout = new QStackedLayout();
  addItem(deviceLayout);

  FrogPilotListWidget *deviceList = new FrogPilotListWidget(this);

  ScrollView *devicePanel = new ScrollView(deviceList, this);

  deviceLayout->addWidget(devicePanel);

  FrogPilotListWidget *deviceManagementList = new FrogPilotListWidget(this);
  FrogPilotListWidget *screenList = new FrogPilotListWidget(this);

  ScrollView *deviceManagementPanel = new ScrollView(deviceManagementList, this);
  ScrollView *screenPanel = new ScrollView(screenList, this);

  deviceLayout->addWidget(deviceManagementPanel);
  deviceLayout->addWidget(screenPanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> deviceToggles {
    {"DeviceManagement", tr("裝置設定"), tr("<b>控制裝置執行、關機及管理行車數據的設定。</b>"), "../../frogpilot/assets/toggle_icons/icon_device.png"},
    {"DeviceShutdown", tr("裝置關閉計時器"), tr("在行駛結束後設定時間內保持裝置開啟，才自動關閉。"), ""},
    {"NoLogging", tr("停用日誌記錄"), QString("<b>%1</b><br><br>%2").arg(tr("警告：這將防止您的行駛被記錄，所有數據將無法獲得！")).arg(tr("<b>防止裝置保存行車數據。</b>")), ""},
    {"NoUploads", tr("停用上傳"), QString("<b>%1</b><br><br>%2").arg(tr("警告：這將防止您的行駛被上傳到 <b>comma connect</b>，會影響偵錯和逗號官方支援！")).arg(tr("<b>防止裝置上傳行車數據。</b>")), ""},
    {"HigherBitrate", tr("高品質錄影"), tr("<b>以更高的視頻品質保存行車影像。</b>"), ""},
    {"LowVoltageShutdown", tr("低電壓截止"), tr("停止時，如果電池電壓低於設定水平，裝置會關閉以防止電池過度放電。"), ""},
    {"IncreaseThermalLimits", tr("提高溫度限制"), QString("<b>%1</b><br><br>%2").arg(tr("警告：在更高溫度下運行可能會損壞您的裝置！")).arg(tr("<b>允許裝置在更高溫度下運行</b>，然後限流或關閉。僅在您了解風險時使用！")), ""},
    {"UseKonikServer", tr("使用 Konik 伺服器"), tr("<b>將行車數據上傳到 \"connect.konik.ai\" 而不是 \"connect.comma.ai\"。</b>"), ""},

    {"ScreenManagement", tr("螢幕設定"), tr("<b>控制螢幕亮度、螢幕錄影和逾時時長的設定。</b>"), "../../frogpilot/assets/toggle_icons/icon_light.png"},
    {"ScreenBrightness", tr("螢幕亮度（停止時）"), tr("<b>未駕駛時的螢幕亮度。</b>"), ""},
    {"ScreenBrightnessOnroad", tr("螢幕亮度（駕駛中）"), tr("<b>駕駛時的螢幕亮度。</b>"), ""},
    {"ScreenRecorder", tr("螢幕錄影機"), tr("<b>在駕駛螢幕上添加按鈕以記錄顯示屏。</b>"), ""},
    {"ScreenTimeout", tr("螢幕逾時（停止時）"), tr("<b>在未駕駛時點擊後螢幕保持開啟的時長。</b>"), ""},
    {"ScreenTimeoutOnroad", tr("螢幕逾時（駕駛中）"), tr("<b>在駕駛時點擊後螢幕保持開啟的時長。</b>"), ""},
    {"StandbyMode", tr("待機模式"), tr("<b>在駕駛時關閉螢幕，並自動喚醒以接收警報或參與狀態變化。</b>"), ""},

    {"IgnoreMe", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""},
    {"IgnoreMe2", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""},
    {"IgnoreMe3", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""},
    {"IgnoreMe4", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""},
    {"IgnoreMe5", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""}
  };

  for (const auto &[param, title, desc, icon] : deviceToggles) {
    AbstractControl *deviceToggle;

    if (param == "DeviceManagement") {
      FrogPilotManageControl *deviceManagementToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(deviceManagementToggle, &FrogPilotManageControl::manageButtonClicked, [deviceLayout, deviceManagementPanel]() {
        deviceLayout->setCurrentWidget(deviceManagementPanel);
      });
      deviceToggle = deviceManagementToggle;
    } else if (param == "DeviceShutdown") {
      std::map<float, QString> shutdownLabels;
      for (int i = 0; i <= 33; ++i) {
        shutdownLabels[i] = i == 0 ? tr("5 分鐘") : i <= 3 ? QString::number(i * 15) + tr(" 分鐘") : QString::number(i - 3) + (i == 4 ? tr(" 小時") : tr(" 小時"));
      }
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 33, QString(), shutdownLabels, 1, true);
    } else if (param == "NoUploads") {
      std::vector<QString> uploadsToggles{"DisableOnroadUploads"};
      std::vector<QString> uploadsToggleNames{tr("僅停用駕駛中上傳")};
      deviceToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, uploadsToggles, uploadsToggleNames);
    } else if (param == "LowVoltageShutdown") {
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 11.8, 12.5, tr(" 伏特"), std::map<float, QString>(), 0.1);

    } else if (param == "ScreenManagement") {
      FrogPilotManageControl *screenToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(screenToggle, &FrogPilotManageControl::manageButtonClicked, [deviceLayout, screenPanel]() {
        deviceLayout->setCurrentWidget(screenPanel);
      });
      deviceToggle = screenToggle;
    } else if (param == "ScreenBrightness" || param == "ScreenBrightnessOnroad") {
      std::map<float, QString> brightnessLabels;
      int minBrightness = (param == "ScreenBrightnessOnroad") ? 0 : 1;
      for (int i = 0; i <= 101; ++i) {
        brightnessLabels[i] = i == 0 ? tr("螢幕關閉") : i == 101 ? tr("自動") : QString::number(i) + "%";
      }
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, minBrightness, 101, QString(), brightnessLabels, 1, true);
    } else if (param == "ScreenRecorder") {
      std::vector<QString> recorderButtonNames{tr("開始錄影"), tr("停止錄影")};
      FrogPilotButtonControl *recorderToggle = new FrogPilotButtonControl(param, title, desc, icon, recorderButtonNames, true);
      QObject::connect(recorderToggle, &FrogPilotButtonControl::buttonClicked, [recorderToggle, screenRecorder](int id) {
        if (id == 0) {
          recorderToggle->setCheckedButton(1);

          recorderToggle->setVisibleButton(0, false);
          recorderToggle->setVisibleButton(1, true);

          screenRecorder->startRecording();
        } else if (id == 1) {
          recorderToggle->clearCheckedButtons(true);

          recorderToggle->setVisibleButton(0, true);
          recorderToggle->setVisibleButton(1, false);

          screenRecorder->stopRecording();
        }
      });
      recorderToggle->setVisibleButton(1, false);
      deviceToggle = recorderToggle;
    } else if (param == "ScreenTimeout" || param == "ScreenTimeoutOnroad") {
      deviceToggle = new FrogPilotParamValueControl(param, title, desc, icon, 5, 60, tr(" 秒"), {}, 5);

    } else {
      deviceToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = deviceToggle;

    if (deviceManagementKeys.contains(param)) {
      deviceManagementList->addItem(deviceToggle);
    } else if (screenKeys.contains(param)) {
      screenList->addItem(deviceToggle);
    } else {
      deviceList->addItem(deviceToggle);

      parentKeys.insert(param);
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(deviceToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(deviceToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(deviceToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  static_cast<ParamControl*>(toggles["IncreaseThermalLimits"])->setConfirmation(true, false);
  static_cast<ParamControl*>(toggles["NoLogging"])->setConfirmation(true, false);
  static_cast<ParamControl*>(toggles["NoUploads"])->setConfirmation(true, false);

  QSet<QString> brightnessKeys = {"ScreenBrightness", "ScreenBrightnessOnroad"};
  for (const QString &key : brightnessKeys) {
    FrogPilotParamValueControl *paramControl = static_cast<FrogPilotParamValueControl*>(toggles[key]);
    QObject::connect(paramControl, &FrogPilotParamValueControl::valueChanged, [key, this](int value) {
      if (!started && key == "ScreenBrightness") {
        Hardware::set_brightness(value);
      } else if (started && key == "ScreenBrightnessOnroad") {
        Hardware::set_brightness(value);
      }
    });
  }

  QSet<QString> forceUpdateKeys = {"NoUploads"};
  for (const QString &key : forceUpdateKeys) {
    QObject::connect(static_cast<FrogPilotButtonToggleControl*>(toggles[key]), &FrogPilotButtonToggleControl::buttonClicked, this, &FrogPilotDevicePanel::updateToggles);
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, this, &FrogPilotDevicePanel::updateToggles);
  }

  QSet<QString> rebootKeys = {"HigherBitrate", "UseKonikServer"};
  for (const QString &key : rebootKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, [key, this](bool state) {
      QString filePath;
      if (key == "HigherBitrate") {
        filePath = "/cache/use_HD";
      } else if (key == "UseKonikServer") {
        filePath = "/cache/use_konik";
      }

      if (!filePath.isEmpty()) {
        QFile toggleFile(filePath);
        if (state) {
          if (!toggleFile.exists()) {
            toggleFile.open(QIODevice::WriteOnly);
            toggleFile.close();
          }
        } else {
          if (toggleFile.exists()) {
            toggleFile.remove();
          }
        }
      }

      if (FrogPilotConfirmationDialog::toggleReboot(this)) {
        Hardware::reboot();
      }
    });
  }

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [deviceLayout, devicePanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    deviceLayout->setCurrentWidget(devicePanel);
  });
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotDevicePanel::updateState);
}

void FrogPilotDevicePanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  updateToggles();
}

void FrogPilotDevicePanel::updateState(const UIState &s) {
  if (!isVisible()) {
    return;
  }

  started = s.scene.started;
}

void FrogPilotDevicePanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      toggle->setVisible(false);
    }
  }

  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      continue;
    }

    bool setVisible = parent->tuningLevel >= frogpilotToggleLevels[key].toDouble();

    if (key == "HigherBitrate") {
      setVisible &= params.getBool("DeviceManagement") && params.getBool("NoUploads") && !params.getBool("DisableOnroadUploads");
    }

    else if (key == "UseKonikServer" && QFile("/data/not_vetted").exists()) {
      static_cast<ToggleControl*>(toggle)->forceOn(true);
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (deviceManagementKeys.contains(key)) {
        toggles["DeviceManagement"]->setVisible(true);
      } else if (screenKeys.contains(key)) {
        toggles["ScreenManagement"]->setVisible(true);
      }
    }
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
