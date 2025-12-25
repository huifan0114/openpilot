#include "frogpilot/ui/qt/offroad/sounds_settings.h"

void playSound(const QString &alert, int volume) {
  QString stockPath = "../../selfdrive/assets/sounds/" + alert + ".wav";
  QString themePath = "../../frogpilot/assets/active_theme/sounds/" + alert + ".wav";

  QString filePath = QFile::exists(themePath) ? themePath : stockPath;

  QProcess::execute("pkill", {"-f", "ffplay"});

  int clampedVolume = std::clamp(volume, 0, 100);

  QProcess::startDetached("ffplay", {"-nodisp", "-autoexit", "-volume", QString::number(clampedVolume), filePath});
}

FrogPilotSoundsPanel::FrogPilotSoundsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *soundsLayout = new QStackedLayout();
  addItem(soundsLayout);

  FrogPilotListWidget *soundsList = new FrogPilotListWidget(this);

  ScrollView *soundsPanel = new ScrollView(soundsList, this);

  soundsLayout->addWidget(soundsPanel);

  FrogPilotListWidget *alertVolumeControlList = new FrogPilotListWidget(this);
  FrogPilotListWidget *customAlertsList = new FrogPilotListWidget(this);

  ScrollView *alertVolumeControlPanel = new ScrollView(alertVolumeControlList, this);
  ScrollView *customAlertsPanel = new ScrollView(customAlertsList, this);

  soundsLayout->addWidget(alertVolumeControlPanel);
  soundsLayout->addWidget(customAlertsPanel);

  // 聲音除錯按鈕
  ButtonControl *soundDebugBtn = new ButtonControl(tr("聲音來源"), tr("查看"));
  QObject::connect(soundDebugBtn, &ButtonControl::clicked, [this]() {
    QString debugInfo = QString::fromStdString(params_memory.get("SoundDebugInfo"));
    if (debugInfo.isEmpty()) {
      debugInfo = tr("尚未載入聲音資訊\n請先啟動一次 openpilot");
    }
    ConfirmationDialog::alert(debugInfo, this);
  });
  soundsList->addItem(soundDebugBtn);

  // 當前播放按鈕
  ButtonControl *nowPlayingBtn = new ButtonControl(tr("最近播放"), tr("查看"));
  QObject::connect(nowPlayingBtn, &ButtonControl::clicked, [this]() {
    QString nowPlaying = QString::fromStdString(params_memory.get("SoundNowPlaying"));
    if (nowPlaying.isEmpty()) {
      nowPlaying = tr("尚未播放任何聲音");
    }
    ConfirmationDialog::alert(nowPlaying, this);
  });
  soundsList->addItem(nowPlayingBtn);

  const std::vector<std::tuple<QString, QString, QString, QString>> soundsToggles {
    {"AlertVolumeControl", tr("提醒音量控制"), tr("<b>設定每種類型的 openpilot 提示音量</b>，以避免日常提示造成干擾。"), "../../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"DisengageVolume", tr("解除音量"), tr("<b>設定 openpilot 解除控制時的提示音量。</b><br><br>例如：『定速故障：請重新啟動車輛』、『手煞車已拉起』、『踩下油門踏板』。"), ""},
    {"EngageVolume", tr("啟動音量"), tr("<b>設定 openpilot 啟動時的提示音量</b>，例如按下方向盤上的『RESUME』或『SET』按鈕後。"), ""},
    {"PromptVolume", tr("提示音量"), tr("<b>設定需要注意之提示的音量。</b><br><br>例如：『偵測到盲點車輛』、『方向暫時不可用』、『轉向超出限制』。"), ""},
    {"PromptDistractedVolume", tr("分心提示音量"), tr("<b>設定當 openpilot 偵測到駕駛分心或無反應時的提示音量。</b><br><br>例如：『請注意』、『觸摸方向盤』。"), ""},
    {"RefuseVolume", tr("拒絕啟動音量"), tr("<b>設定 openpilot 拒絕啟動時的提示音量。</b><br><br>例如：『剎車保持啟用』、『車門未關閉』、『安全帶未扣上』。"), ""},
    {"WarningSoftVolume", tr("軟性警告音量"), tr("<b>設定針對潛在風險之較柔和警告音量。</b><br><br>例如：『剎車！有碰撞風險』、『方向暫時不可用』。"), ""},
    {"WarningImmediateVolume", tr("緊急警告音量"), tr("<b>設定最需立即注意之最大音量警告。</b><br><br>例如：『立即解除 — 駕駛分心』、『立即解除 — 駕駛無反應』。"), ""},

    {"CustomAlerts", tr("FrogPilot 提示"), tr("<b>選用的 FrogPilot 提示</b>，可更明顯地標示行車事件。"), "../../frogpilot/assets/toggle_icons/icon_green_light.png"},
    {"GoatScream", tr("山羊尖叫"), tr("<b>當方向控制器達到極限時播放著名的「山羊尖叫」。</b> 基於『轉向超出限制』事件。"), ""},
    {"GreenLightAlert", tr("綠燈提示"), tr("<b>當模型預測紅燈轉為綠燈時播放提示。</b><br><br><i><b>免責聲明</b>：openpilot 並不直接偵測交通號誌。此提示基於相機輸入的端對端模型預測，可能會在號誌未變更時觸發。</i>"), ""},
    {"LeadDepartingAlert", tr("前車起步提示"), tr("<b>當前車從停車狀態起步時播放提示。</b>"), ""},
    {"LoudBlindspotAlert", tr("大聲「偵測到盲點車輛」提示"), tr("<b>當嘗試變換車道時，如盲點有車輛則播放較大聲的提示。</b> 基於『偵測到盲點車輛』事件。"), ""},
    {"SpeedLimitChangedAlert", tr("速限變更提示"), tr("<b>當路 posted 速限變更時播放提示。</b>"), ""}
  };

  for (const auto &[param, title, desc, icon] : soundsToggles) {
    AbstractControl *soundsToggle;

    if (param == "AlertVolumeControl") {
      FrogPilotManageControl *alertVolumeControlToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(alertVolumeControlToggle, &FrogPilotManageControl::manageButtonClicked, [soundsLayout, alertVolumeControlPanel]() {
        soundsLayout->setCurrentWidget(alertVolumeControlPanel);
      });
      soundsToggle = alertVolumeControlToggle;
    } else if (alertVolumeControlKeys.contains(param)) {
      std::map<float, QString> volumeLabels;
      for (int i = 0; i <= 101; ++i) {
        volumeLabels[i] = i == 0 ? tr("Muted") : i == 101 ? tr("Auto") : QString::number(i) + "%";
      }
      std::vector<QString> alertButton{tr("Test")};
      if (param == "WarningImmediateVolume" || param == "WarningSoftVolume") {
        soundsToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 25, 101, QString(), volumeLabels, 1, true, {}, alertButton, false, false);
      } else {
        soundsToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 101, QString(), volumeLabels, 1, true, {}, alertButton, false, false);
      }

    } else if (param == "CustomAlerts") {
      FrogPilotManageControl *customAlertsToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(customAlertsToggle, &FrogPilotManageControl::manageButtonClicked, [soundsLayout, customAlertsPanel]() {
        soundsLayout->setCurrentWidget(customAlertsPanel);
      });
      soundsToggle = customAlertsToggle;

    } else {
      soundsToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = soundsToggle;

    if (alertVolumeControlKeys.contains(param)) {
      alertVolumeControlList->addItem(soundsToggle);
    } else if (customAlertsKeys.contains(param)) {
      customAlertsList->addItem(soundsToggle);
    } else {
      soundsList->addItem(soundsToggle);

      parentKeys.insert(param);
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(soundsToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(soundsToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(soundsToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  for (const QString &key : alertVolumeControlKeys) {
    FrogPilotParamValueButtonControl *toggle = static_cast<FrogPilotParamValueButtonControl*>(toggles[key]);
    QObject::connect(toggle, &FrogPilotParamValueButtonControl::buttonClicked, [key, toggle, this]() {
      toggle->updateParam();

      updateFrogPilotToggles();

      util::sleep_for(UI_FREQ);

      QString keyWithoutVolume = key;
      keyWithoutVolume.remove("Volume");

      QString camelCaseAlert = keyWithoutVolume;
      camelCaseAlert[0] = camelCaseAlert[0].toLower();

      QString snakeCaseAlert;
      for (int i = 0; i < keyWithoutVolume.size(); ++i) {
        QChar c = keyWithoutVolume[i];
        if (c.isUpper() && i > 0) {
          snakeCaseAlert += '_';
        }
        snakeCaseAlert += c.toLower();
      }

      if (started) {
        params_memory.put("TestAlert", camelCaseAlert.toStdString());
      } else {
        std::thread([key, snakeCaseAlert, this]() {
          playSound(snakeCaseAlert, params.getInt(key.toStdString()));
        }).detach();
      }
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [soundsLayout, soundsPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    soundsLayout->setCurrentWidget(soundsPanel);
  });
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotSoundsPanel::updateState);

  for (auto &[key, toggle] : toggles) {
    if (alertVolumeControlKeys.contains(key)) {
      toggle->setVisible(true);
    }
  }

  updateToggles();
}

void FrogPilotSoundsPanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  updateToggles();
}

void FrogPilotSoundsPanel::updateState(const UIState &s) {
  if (!isVisible()) {
    return;
  }

  started = s.scene.started;
}

void FrogPilotSoundsPanel::updateToggles() {
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

    if (key == "LoudBlindspotAlert") {
      setVisible &= parent->hasBSM;
    }

    else if (key == "SpeedLimitChangedAlert") {
      setVisible &= params.getBool("ShowSpeedLimits") || (parent->hasOpenpilotLongitudinal && params.getBool("SpeedLimitController"));
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (alertVolumeControlKeys.contains(key)) {
        toggles["AlertVolumeControl"]->setVisible(true);
      } else if (customAlertsKeys.contains(key)) {
        toggles["CustomAlerts"]->setVisible(true);
      }
    }
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
