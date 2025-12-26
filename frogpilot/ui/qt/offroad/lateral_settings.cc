#include "frogpilot/ui/qt/offroad/lateral_settings.h"

FrogPilotLateralPanel::FrogPilotLateralPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *lateralLayout = new QStackedLayout();
  addItem(lateralLayout);

  FrogPilotListWidget *lateralList = new FrogPilotListWidget(this);

  ScrollView *lateralPanel = new ScrollView(lateralList, this);

  lateralLayout->addWidget(lateralPanel);

  FrogPilotListWidget *advancedLateralTuneList = new FrogPilotListWidget(this);
  FrogPilotListWidget *aolList = new FrogPilotListWidget(this);
  FrogPilotListWidget *laneChangeList = new FrogPilotListWidget(this);
  FrogPilotListWidget *lateralTuneList = new FrogPilotListWidget(this);
  FrogPilotListWidget *qolList = new FrogPilotListWidget(this);

  ScrollView *advancedLateralTunePanel = new ScrollView(advancedLateralTuneList, this);
  ScrollView *aolPanel = new ScrollView(aolList, this);
  ScrollView *laneChangePanel = new ScrollView(laneChangeList, this);
  ScrollView *lateralTunePanel = new ScrollView(lateralTuneList, this);
  ScrollView *qolPanel = new ScrollView(qolList, this);

  lateralLayout->addWidget(advancedLateralTunePanel);
  lateralLayout->addWidget(aolPanel);
  lateralLayout->addWidget(laneChangePanel);
  lateralLayout->addWidget(lateralTunePanel);
  lateralLayout->addWidget(qolPanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> lateralToggles {
    {"AdvancedLateralTune", tr("進階轉向調整"), tr("<b>進階轉向控制變更以微調 openpilot 的駕駛方式。</b>"), "../../frogpilot/assets/toggle_icons/icon_advanced_lateral_tune.png"},
    {"SteerDelay", parent->steerActuatorDelay != 0 ? QString(tr("執行器延遲 (預設: %1)")).arg(QString::number(parent->steerActuatorDelay, 'f', 2)) : tr("執行器延遲"), tr("<b>openpilot 的轉向命令和車輛回應之間的時間。</b>如果車輛反應晚請增加；如果感覺跳躍請減少。預設為自動學習。"), ""},
    {"SteerFriction", parent->friction != 0 ? QString(tr("摩擦力 (預設: %1)")).arg(QString::number(parent->friction, 'f', 2)) : tr("摩擦力"), tr("<b>補償轉向摩擦。</b>如果方向盤在中心附近卡住請增加；如果抖動請減少。預設為自動學習。"), ""},
    {"SteerKP", parent->steerKp != 0 ? QString(tr("Kp 係數 (預設: %1)")).arg(QString::number(parent->steerKp, 'f', 2)) : tr("Kp 係數"), tr("<b>openpilot 修正車道位置的強度。</b>較高較緊但更易抖動；較低更平穩但較慢。預設為自動學習。"), ""},
    {"SteerLatAccel", parent->latAccelFactor != 0 ? QString(tr("橫向加速度 (預設: %1)")).arg(QString::number(parent->latAccelFactor, 'f', 2)) : tr("橫向加速度"), tr("<b>將轉向扭矩映射到轉向反應。</b>增加用於更急轉彎；減少用於更溫和的轉向。預設為自動學習。"), ""},
    {"SteerRatio", parent->steerRatio != 0 ? QString(tr("轉向比 (預設: %1)")).arg(QString::number(parent->steerRatio, 'f', 2)) : tr("轉向比"), tr("<b>方向盤旋轉與路面輪角之間的關係。</b>如果轉向感覺太快或抖動請增加；如果感覺太慢或太弱請減少。預設為自動學習。"), ""},
    {"ForceAutoTune", tr("強制啟用自動調整"), tr("<b>強制啟用 openpilot 對 \"摩擦力\" 和 \"橫向加速度\" 的即時自動調整。</b>"), ""},
    {"ForceAutoTuneOff", tr("強制停用自動調整"), tr("<b>強制停用 openpilot 對 \"摩擦力\" 和 \"橫向加速度\" 的即時自動調整，改用設定值。</b>"), ""},
    {"ForceTorqueController", tr("強制扭矩控制器"), tr("<b>使用扭矩型轉向控制而不是角度型控制以獲得更平穩的車道保持，特別是在彎道中。</b>"), ""},

    {"AlwaysOnLateral", tr("持續橫向控制"), tr("<b>即使在踩踏油門或煞車踏板時，openpilot 的轉向仍保持活躍。</b>"), "../../frogpilot/assets/toggle_icons/icon_always_on_lateral.png"},
    {"AlwaysOnLateralMain", tr("與定速巡航一起啟用"), tr("<b>每當 \"定速巡航\" 啟用時，啟用 \"持續橫向控制\"，即使 openpilot 未啟用。</b>"), ""},
    {"AlwaysOnLateralLKAS", tr("與 LKAS 一起啟用"), tr("<b>每當 \"LKAS\" 啟用時，啟用 \"持續橫向控制\"，即使 openpilot 未啟用。</b>"), ""},
    {"PauseAOLOnBrake", tr("在以下速度暫停煞車"), tr("<b>在踩踏煞車踏板時，在設定速度下方暫停 \"持續橫向控制\"。</b>"), ""},

    {"LaneChanges", tr("車道變更"), tr("<b>允許 openpilot 變更車道。</b>"), "../../frogpilot/assets/toggle_icons/icon_lane.png"},
    {"NudgelessLaneChange", tr("自動車道變更"), tr("<b>當轉向信號啟用時，openpilot 將自動變更車道。</b>不需要轉向盤推動！"), ""},
    {"LaneChangeTime", tr("車道變更延遲"), tr("<b>轉向信號啟用與自動車道變更開始之間的延遲。</b>"), ""},
    {"MinimumLaneChangeSpeed", tr("最小車道變更速度"), tr("<b>openpilot 將變更車道的最低速度。</b>"), ""},
    {"LaneDetectionWidth", tr("最小車道寬度"), tr("<b>防止自動車道變更進入比設定寬度更狹窄的車道。</b>"), ""},
    {"OneLaneChange", tr("每個信號一次車道變更"), tr("<b>限制自動車道變更為每個轉向信號啟用一次。</b>"), ""},

    {"LateralTune", tr("轉向調整"), tr("<b>雜項轉向控制變更</b>以微調 openpilot 的駕駛方式。"), "../../frogpilot/assets/toggle_icons/icon_lateral_tune.png"},
    {"TurnDesires", tr("在最小車道變更速度以下強制轉向慾望"), tr("<b>當以下於最小車道變更速度行駛且轉向信號啟用時，指示 openpilot 向左/右轉向。</b>"), ""},
    {"NNFF", tr("神經網路前饋 (NNFF)"), tr("<b>Twilsonco 的 \"神經網路前饋\" 模型控制器，用於根據您的車輛數據訓練的更平穩的基於模型的轉向。</b>"), ""},
    {"NNFFLite", tr("平穩曲線處理"), tr("<b>Twilsonco 的基於扭矩的調整以平穩曲線中的轉向。</b>"), ""},

    {"QOLLateral", tr("生活品質"), tr("<b>轉向控制變更以微調 openpilot 的駕駛方式。</b>"), "../../frogpilot/assets/toggle_icons/icon_quality_of_life.png"},
    {"PauseLateralSpeed", tr("在以下速度暫停轉向"), tr("<b>在設定速度以下暫停轉向。</b>"), ""},

    {"IgnoreMe", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""},
    {"IgnoreMe2", "Ignore Me", "This is simply used to fix the layout when the user opens the descriptions and the menu gets wonky. No idea why it happens, but I can't be asked to properly fix it so whatever. Sue me.", ""}
  };

  for (const auto &[param, title, desc, icon] : lateralToggles) {
    AbstractControl *lateralToggle;

    if (param == "AdvancedLateralTune") {
      FrogPilotManageControl *advancedLateralTuneToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(advancedLateralTuneToggle, &FrogPilotManageControl::manageButtonClicked, [lateralLayout, advancedLateralTunePanel]() {
        lateralLayout->setCurrentWidget(advancedLateralTunePanel);
      });
      lateralToggle = advancedLateralTuneToggle;
    } else if (param == "SteerDelay") {
      std::vector<QString> steerDelayButton{"Reset"};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0.01, 1, QString(), std::map<float, QString>(), 0.01, false, {}, steerDelayButton, false, false);
    } else if (param == "SteerFriction") {
      std::vector<QString> steerFrictionButton{"Reset"};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 0.5, QString(), std::map<float, QString>(), 0.01, false, {}, steerFrictionButton, false, false);
    } else if (param == "SteerKP") {
      std::vector<QString> steerKPButton{"Reset"};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, parent->steerKp * 0.5, parent->steerKp * 1.5, QString(), std::map<float, QString>(), 0.01, false, {}, steerKPButton, false, false);
    } else if (param == "SteerLatAccel") {
      std::vector<QString> steerLatAccelButton{"Reset"};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, parent->latAccelFactor * 0.75, parent->latAccelFactor * 1.25, QString(), std::map<float, QString>(), 0.01, false, {}, steerLatAccelButton, false, false);
    } else if (param == "SteerRatio") {
      std::vector<QString> steerRatioButton{"Reset"};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, parent->steerRatio * 0.5, parent->steerRatio * 1.5, QString(), std::map<float, QString>(), 0.01, false, {}, steerRatioButton, false, false);

    } else if (param == "AlwaysOnLateral") {
      FrogPilotManageControl *aolToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(aolToggle, &FrogPilotManageControl::manageButtonClicked, [lateralLayout, aolPanel]() {
        lateralLayout->setCurrentWidget(aolPanel);
      });
      lateralToggle = aolToggle;
    } else if (param == "PauseAOLOnBrake") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, QString(), std::map<float, QString>(), 1, true);

    } else if (param == "LaneChanges") {
      FrogPilotManageControl *laneChangeToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(laneChangeToggle, &FrogPilotManageControl::manageButtonClicked, [lateralLayout, laneChangePanel]() {
        lateralLayout->setCurrentWidget(laneChangePanel);
      });
      lateralToggle = laneChangeToggle;
    } else if (param == "LaneChangeTime") {
      std::map<float, QString> laneChangeTimeLabels;
      for (float i = 0; i <= 5; i += 0.1) {
        laneChangeTimeLabels[i] = i == 0 ? tr("即時") : std::lround(i / 0.1) == 1 / 0.1 ? QString::number(i, 'f', 1) + tr(" 秒") : QString::number(i, 'f', 1) + tr(" 秒");
      }
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 5, QString(), laneChangeTimeLabels, 0.1);
    } else if (param == "LaneDetectionWidth") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 15, QString(), std::map<float, QString>(), 0.1, true);
    } else if (param == "MinimumLaneChangeSpeed") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, QString(), std::map<float, QString>(), 1, true);

    } else if (param == "LateralTune") {
      FrogPilotManageControl *lateralTuneToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(lateralTuneToggle, &FrogPilotManageControl::manageButtonClicked, [lateralLayout, lateralTunePanel]() {
        lateralLayout->setCurrentWidget(lateralTunePanel);
      });
      lateralToggle = lateralTuneToggle;

    } else if (param == "QOLLateral") {
      FrogPilotManageControl *qolLateralToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(qolLateralToggle, &FrogPilotManageControl::manageButtonClicked, [lateralLayout, qolPanel]() {
        lateralLayout->setCurrentWidget(qolPanel);
      });
      lateralToggle = qolLateralToggle;
    } else if (param == "PauseLateralSpeed") {
      std::vector<QString> pauseLateralToggles{"PauseLateralOnSignal"};
      std::vector<QString> pauseLateralToggleNames{tr("僅轉向信號")};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 99, QString(), std::map<float, QString>(), 1, true, pauseLateralToggles, pauseLateralToggleNames, true);

    } else {
      lateralToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = lateralToggle;

    if (advancedLateralTuneKeys.contains(param)) {
      advancedLateralTuneList->addItem(lateralToggle);
    } else if (aolKeys.contains(param)) {
      aolList->addItem(lateralToggle);
    } else if (laneChangeKeys.contains(param)) {
      laneChangeList->addItem(lateralToggle);
    } else if (lateralTuneKeys.contains(param)) {
      lateralTuneList->addItem(lateralToggle);
    } else if (qolKeys.contains(param)) {
      qolList ->addItem(lateralToggle);
    } else {
      lateralList->addItem(lateralToggle);

      parentKeys.insert(param);
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(lateralToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(lateralToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(lateralToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QSet<QString> forceUpdateKeys = {"ForceAutoTune", "ForceAutoTuneOff", "LateralTune", "NNFF", "NudgelessLaneChange"};
  for (const QString &key : forceUpdateKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, this, &FrogPilotLateralPanel::updateToggles);
  }

  QSet<QString> rebootKeys = {"AlwaysOnLateral", "ForceTorqueController", "NNFF", "NNFFLite"};
  for (const QString &key : rebootKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, [key, this](bool state) {
      if (started) {
        if (key == "AlwaysOnLateral" && state) {
          if (FrogPilotConfirmationDialog::toggleReboot(this)) {
            Hardware::reboot();
          }
        } else if (key != "AlwaysOnLateral") {
          if (FrogPilotConfirmationDialog::toggleReboot(this)) {
            Hardware::reboot();
          }
        }
      }
    });
  }

  steerDelayToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerDelay"]);
  QObject::connect(steerDelayToggle, &FrogPilotParamValueButtonControl::buttonClicked, [parent, this]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("重設 <b>執行器延遲</b> 為預設值？"), this)) {
      params.putFloat("SteerDelay", parent->steerActuatorDelay);
      steerDelayToggle->refresh();
    }
  });

  steerFrictionToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerFriction"]);
  QObject::connect(steerFrictionToggle, &FrogPilotParamValueButtonControl::buttonClicked, [parent, this]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("重設 <b>摩擦力</b> 為預設值？"), this)) {
      params.putFloat("SteerFriction", parent->friction);
      steerFrictionToggle->refresh();
    }
  });

  steerKPToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerKP"]);
  QObject::connect(steerKPToggle, &FrogPilotParamValueButtonControl::buttonClicked, [parent, this]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("重設 <b>Kp 係數</b> 為預設值？"), this)) {
      params.putFloat("SteerKP", parent->steerKp);
      steerKPToggle->refresh();
    }
  });

  steerLatAccelToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerLatAccel"]);
  QObject::connect(steerLatAccelToggle, &FrogPilotParamValueButtonControl::buttonClicked, [parent, this]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("重設 <b>橫向加速度</b> 為預設值？"), this)) {
      params.putFloat("SteerLatAccel", parent->latAccelFactor);
      steerLatAccelToggle->refresh();
    }
  });

  steerRatioToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerRatio"]);
  QObject::connect(steerRatioToggle, &FrogPilotParamValueButtonControl::buttonClicked, [parent, this]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("重設 <b>轉向比</b> 為預設值？"), this)) {
      params.putFloat("SteerRatio", parent->steerRatio);
      steerRatioToggle->refresh();
    }
  });

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [lateralLayout, lateralPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    lateralLayout->setCurrentWidget(lateralPanel);
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotLateralPanel::updateMetric);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotLateralPanel::updateState);
}

void FrogPilotLateralPanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  steerDelayToggle->setTitle(QString(tr("執行器延遲 (預設: %1)")).arg(QString::number(parent->steerActuatorDelay, 'f', 2)));
  steerFrictionToggle->setTitle(QString(tr("摩擦力 (預設: %1)")).arg(QString::number(parent->friction, 'f', 2)));
  steerKPToggle->setTitle(QString(tr("Kp 係數 (預設: %1)")).arg(QString::number(parent->steerKp, 'f', 2)));
  steerKPToggle->updateControl(parent->steerKp * 0.5, parent->steerKp * 1.5);
  steerLatAccelToggle->setTitle(QString(tr("橫向加速度 (預設: %1)")).arg(QString::number(parent->latAccelFactor, 'f', 2)));
  steerLatAccelToggle->updateControl(parent->latAccelFactor * 0.75, parent->latAccelFactor * 1.25);
  steerRatioToggle->setTitle(QString(tr("轉向比 (預設: %1)")).arg(QString::number(parent->steerRatio, 'f', 2)));
  steerRatioToggle->updateControl(parent->steerRatio * 0.5, parent->steerRatio * 1.5);

  updateToggles();
}

void FrogPilotLateralPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  started = s.scene.started;
}

void FrogPilotLateralPanel::updateMetric(bool metric, bool bootRun) {
  static bool previousMetric;
  if (metric != previousMetric && !bootRun) {
    double distanceConversion = metric ? FOOT_TO_METER : METER_TO_FOOT;
    double speedConversion = metric ? MILE_TO_KM : KM_TO_MILE;

    params.putFloatNonBlocking("LaneDetectionWidth", params.getFloat("LaneDetectionWidth") * distanceConversion);

    params.putIntNonBlocking("MinimumLaneChangeSpeed", params.getInt("MinimumLaneChangeSpeed") * speedConversion);
    params.putIntNonBlocking("PauseAOLOnBrake", params.getInt("PauseAOLOnBrake") * speedConversion);
    params.putIntNonBlocking("PauseLateralSpeed", params.getInt("PauseLateralSpeed") * speedConversion);
  }
  previousMetric = metric;

  static std::map<float, QString> imperialDistanceLabels;
  static std::map<float, QString> imperialSpeedLabels;
  static std::map<float, QString> metricDistanceLabels;
  static std::map<float, QString> metricSpeedLabels;

  static bool labelsInitialized = false;
  if (!labelsInitialized) {
    for (int i = 0; i <= 150; ++i) {
      float key = i / 10.0f;
      imperialDistanceLabels[key] = key == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" 英尺") : QString::number(key, 'f', 1) + tr(" 英尺");
    }

    for (int i = 0; i <= 99; ++i) {
      imperialSpeedLabels[i] = i == 0 ? tr("關閉") : QString::number(i) + tr(" 英里/小時");
    }

    for (int i = 0; i <= 50; ++i) {
      float key = i / 10.0f;
      metricDistanceLabels[key] = key == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" 米") : QString::number(key, 'f', 1) + tr(" 米");
    }

    for (int i = 0; i <= 150; ++i) {
      metricSpeedLabels[i] = i == 0 ? tr("關閉") : QString::number(i) + tr(" 公里/小時");
    }

    labelsInitialized = true;
  }

  FrogPilotParamValueControl *laneWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneDetectionWidth"]);
  FrogPilotParamValueControl *minimumLaneChangeSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["MinimumLaneChangeSpeed"]);
  FrogPilotParamValueControl *pauseAOLOnBrakeToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseAOLOnBrake"]);
  FrogPilotParamValueControl *pauseLateralToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseLateralSpeed"]);

  if (metric) {
    laneWidthToggle->updateControl(0, 5, metricDistanceLabels);

    minimumLaneChangeSpeedToggle->updateControl(0, 150, metricSpeedLabels);
    pauseAOLOnBrakeToggle->updateControl(0, 150, metricSpeedLabels);
    pauseLateralToggle->updateControl(0, 150, metricSpeedLabels);
  } else {
    laneWidthToggle->updateControl(0, 15, imperialDistanceLabels);

    minimumLaneChangeSpeedToggle->updateControl(0, 99, imperialSpeedLabels);
    pauseAOLOnBrakeToggle->updateControl(0, 99, imperialSpeedLabels);
    pauseLateralToggle->updateControl(0, 99, imperialSpeedLabels);
  }
}

void FrogPilotLateralPanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      toggle->setVisible(false);
    }
  }

  bool forcingAutoTune = !parent->hasAutoTune && params.getBool("ForceAutoTune");
  bool forcingAutoTuneOff = parent->hasAutoTune && params.getBool("ForceAutoTuneOff");
  bool forcingTorqueController = !parent->isAngleCar && params.getBool("ForceTorqueController");
  bool usingNNFF = parent->hasNNFFLog && params.getBool("LateralTune") && params.getBool("NNFF");

  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      continue;
    }

    bool setVisible = parent->tuningLevel >= frogpilotToggleLevels[key].toDouble();

    if (key == "AlwaysOnLateralLKAS") {
      setVisible &= parent->isHKGCanFd;
      setVisible &= !parent->hasOpenpilotLongitudinal;
    }

    else if (key == "AlwaysOnLateralMain") {
      setVisible &= !parent->isHKGCanFd;
      setVisible |= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "ForceAutoTune") {
      setVisible &= !parent->hasAutoTune;
      setVisible &= !parent->isAngleCar;
      setVisible &= parent->isTorqueCar || forcingTorqueController || usingNNFF;
    }

    else if (key == "ForceAutoTuneOff") {
      setVisible &= parent->hasAutoTune;
    }

    else if (key == "ForceTorqueController") {
      setVisible &= !parent->isAngleCar;
      setVisible &= !parent->isTorqueCar;
    }

    else if (key == "LaneChangeTime") {
      setVisible &= params.getBool("LaneChanges") && params.getBool("NudgelessLaneChange");
    }

    else if (key == "LaneDetectionWidth") {
      setVisible &= params.getBool("LaneChanges") && params.getBool("NudgelessLaneChange");
    }

    else if (key == "NNFF") {
      setVisible &= parent->hasNNFFLog;
      setVisible &= !parent->isAngleCar;
    }

    else if (key == "NNFFLite") {
      setVisible &= !usingNNFF;
      setVisible &= !parent->isAngleCar;
    }

    else if (key == "SteerDelay") {
      setVisible &= parent->steerActuatorDelay != 0;
    }

    else if (key == "SteerFriction") {
      setVisible &= parent->friction != 0;
      setVisible &= parent->hasAutoTune ? forcingAutoTuneOff : !forcingAutoTune;
      setVisible &= parent->isTorqueCar || forcingTorqueController || usingNNFF;
      setVisible &= !usingNNFF;
    }

    else if (key == "SteerKP") {
      setVisible &= parent->steerKp != 0;
      setVisible &= parent->isTorqueCar || forcingTorqueController || usingNNFF;
      setVisible &= !parent->isAngleCar;
    }

    else if (key == "SteerLatAccel") {
      setVisible &= parent->latAccelFactor != 0;
      setVisible &= parent->hasAutoTune ? forcingAutoTuneOff : !forcingAutoTune;
      setVisible &= parent->isTorqueCar || forcingTorqueController || usingNNFF;
      setVisible &= !usingNNFF;
    }

    else if (key == "SteerRatio") {
      setVisible &= parent->steerRatio != 0;
      setVisible &= parent->hasAutoTune ? forcingAutoTuneOff : !forcingAutoTune;
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (advancedLateralTuneKeys.contains(key)) {
        toggles["AdvancedLateralTune"]->setVisible(true);
      } else if (aolKeys.contains(key)) {
        toggles["AlwaysOnLateral"]->setVisible(true);
      } else if (laneChangeKeys.contains(key)) {
        toggles["LaneChanges"]->setVisible(true);
      } else if (lateralTuneKeys.contains(key)) {
        toggles["LateralTune"]->setVisible(true);
      } else if (qolKeys.contains(key)) {
        toggles["QOLLateral"]->setVisible(true);
      }
    }
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
