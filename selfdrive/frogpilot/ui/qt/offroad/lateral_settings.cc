#include "selfdrive/frogpilot/ui/qt/offroad/lateral_settings.h"

FrogPilotLateralPanel::FrogPilotLateralPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> lateralToggles {
    {"AlwaysOnLateral", tr("全時置中模式"), tr("即使踩下煞車或油門踏板，橫向轉向控制仍保持活動狀態.\n\n僅使用「巡航控制」按鈕才能停用."), "../frogpilot/assets/toggle_icons/icon_always_on_lateral.png"},
    {"AlwaysOnLateralLKAS", tr("使用 LKAS 按鈕控制"), tr("'使用“LKAS”按鈕開啟或關閉“全時置中模式”."), ""},
    {"AlwaysOnLateralMain", tr("啟用巡航控制"), tr("'按下「巡航控制」按鈕即可開啟「全時置中模式」功能，繞過手動啟動的要求."), ""},
    {"PauseAOLOnBrake", tr("踩下煞車暫停"), tr("'當踩下煞車踏板且低於設定速度時，「全時置中模式」會暫停."), ""},
    {"HideAOLStatusBar", tr("隱藏狀態列"), tr("隱藏「全時置中模式」狀態列."), ""},

    {"LaneChangeCustomizations", tr("變換車道設定"), tr("設定變換車道方式."), "../frogpilot/assets/toggle_icons/icon_lane.png"},
    {"NudgelessLaneChange", tr("自動變換車道"), tr("方向燈訊號啟動時無需觸碰方向盤即可進行變換車道."), ""},
    {"LaneChangeTime", tr("延遲變換車道"), tr("在變換車道之前的等待時間."), ""},
    {"LaneDetectionWidth", tr("車道寬度需求"), tr("偵測車道寬度符合才進行自動換道."), ""},
    {"MinimumLaneChangeSpeed", tr("變換車道最低速度"), tr("啟動自動變換車道所需的最低速度."), ""},
    {"OneLaneChange", tr("單次變換車道"), tr("每次方向燈啟動時，僅變換車道一次."), ""},

    {"LateralTune", tr("橫向調整"), tr("控制管理轉向的設定."), "../frogpilot/assets/toggle_icons/icon_lateral_tune.png"},
    {"NNFF", tr("神經網路前饋 (NNFF)"), tr("Twilsonco 的「神經網路前饋」可實現更精確的轉向控制."), ""},
    {"NNFFLite", tr("平滑曲線處理"), tr("透過 Twilsonco 的扭矩調節，進入和退出彎道時轉向更加平穩."), ""},

    {"QOLLateral", tr("橫行控制改善"), tr("各種橫向功能可改善您的整體駕駛體驗."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"PauseLateralSpeed", tr("暫停轉向速度設定"), tr("當行駛速度低於設定速度時，暫停轉向控制。"), ""}
  };

  for (const auto &[param, title, desc, icon] : lateralToggles) {
    AbstractControl *lateralToggle;

    if (param == "AlwaysOnLateral") {
      FrogPilotParamManageControl *aolToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(aolToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedAOLKeys = aolKeys;

        if (isSubaru || (params.getBool("ExperimentalModeActivation") && params.getBool("ExperimentalModeViaLKAS"))) {
          modifiedAOLKeys.erase("AlwaysOnLateralLKAS");
        }

        showToggles(modifiedAOLKeys);
      });
      lateralToggle = aolToggle;
    } else if (param == "PauseAOLOnBrake") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr("英里/小時"));

    } else if (param == "LateralTune") {
      FrogPilotParamManageControl *lateralTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(lateralTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedLateralTuneKeys = lateralTuneKeys;

        bool usingNNFF = hasNNFFLog && params.getBool("LateralTune") && params.getBool("NNFF");
        if (!hasNNFFLog) {
          modifiedLateralTuneKeys.erase("NNFF");
        } else if (usingNNFF) {
          modifiedLateralTuneKeys.erase("NNFFLite");
        }

        showToggles(modifiedLateralTuneKeys);
      });
      lateralToggle = lateralTuneToggle;

    } else if (param == "QOLLateral") {
      FrogPilotParamManageControl *qolLateralToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(qolLateralToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(qolKeys);
      });
      lateralToggle = qolLateralToggle;
    } else if (param == "PauseLateralSpeed") {
      std::vector<QString> pauseLateralToggles{"PauseLateralOnSignal"};
      std::vector<QString> pauseLateralToggleNames{"Turn Signal Only"};
      lateralToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 99, tr("英里/小時"), std::map<int, QString>(), 1, pauseLateralToggles, pauseLateralToggleNames);
    } else if (param == "PauseLateralOnSignal") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr("英里/小時"));

    } else if (param == "LaneChangeCustomizations") {
      FrogPilotParamManageControl *laneChangeToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(laneChangeToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(laneChangeKeys);
      });
      lateralToggle = laneChangeToggle;
    } else if (param == "LaneChangeTime") {
      std::map<int, QString> laneChangeTimeLabels;
      for (int i = 0; i <= 10; ++i) {
        laneChangeTimeLabels[i] = i == 0 ? "Instant" : QString::number(i / 2.0) + " seconds";
      }
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, QString(), laneChangeTimeLabels);
    } else if (param == "LaneDetectionWidth") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, tr(" feet"), std::map<int, QString>(), 0.1);
    } else if (param == "MinimumLaneChangeSpeed") {
      lateralToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr("英里/小時"));

    } else {
      lateralToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(lateralToggle);
    toggles[param] = lateralToggle;

    makeConnections(lateralToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(lateralToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotLateralPanel::openParentToggle);
    }

    QObject::connect(lateralToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["AlwaysOnLateralLKAS"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state && params.getBool("ExperimentalModeViaLKAS")) {
      params.putBoolNonBlocking("ExperimentalModeViaLKAS", false);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NNFF"]), &ToggleControl::toggleFlipped, [this](bool state) {
    std::set<QString> modifiedLateralTuneKeys = lateralTuneKeys;

    bool usingNNFF = hasNNFFLog && state;
    if (!hasNNFFLog) {
      modifiedLateralTuneKeys.erase("NNFF");
    } else if (usingNNFF) {
      modifiedLateralTuneKeys.erase("NNFFLite");
    }

    showToggles(modifiedLateralTuneKeys);
  });

  std::set<QString> rebootKeys = {"AlwaysOnLateral", "NNFF", "NNFFLite"};
  for (const QString &key : rebootKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key.toStdString().c_str()]), &ToggleControl::toggleFlipped, [this, key](bool state) {
      if (started) {
        if (key == "AlwaysOnLateral" && state) {
          if (FrogPilotConfirmationDialog::toggle(tr("需要重新啟動才能生效."), tr("馬上重啟"), this)) {
            Hardware::reboot();
          }
        } else if (key != "AlwaysOnLateral") {
          if (FrogPilotConfirmationDialog::toggle(tr("需要重新啟動才能生效."), tr("馬上重啟"), this)) {
            Hardware::reboot();
          }
        }
      }
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotLateralPanel::hideToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateCarToggles, this, &FrogPilotLateralPanel::updateCarToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotLateralPanel::updateMetric);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotLateralPanel::updateState);

  updateMetric();
}

void FrogPilotLateralPanel::updateCarToggles() {
  hasAutoTune = parent->hasAutoTune;
  hasNNFFLog = parent->hasNNFFLog;
  isSubaru = parent->isSubaru;

  hideToggles();
}

void FrogPilotLateralPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  started = s.scene.started;
}

void FrogPilotLateralPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;
    double speedConversion = isMetric ? MILE_TO_KM : KM_TO_MILE;

    params.putFloatNonBlocking("LaneDetectionWidth", params.getFloat("LaneDetectionWidth") * distanceConversion);

    params.putFloatNonBlocking("MinimumLaneChangeSpeed", params.getFloat("MinimumLaneChangeSpeed") * speedConversion);
    params.putFloatNonBlocking("PauseAOLOnBrake", params.getFloat("PauseAOLOnBrake") * speedConversion);
    params.putFloatNonBlocking("PauseLateralOnSignal", params.getFloat("PauseLateralOnSignal") * speedConversion);
    params.putFloatNonBlocking("PauseLateralSpeed", params.getFloat("PauseLateralSpeed") * speedConversion);
  }

  FrogPilotParamValueControl *laneWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneDetectionWidth"]);
  FrogPilotParamValueControl *minimumLaneChangeSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["MinimumLaneChangeSpeed"]);
  FrogPilotParamValueControl *pauseAOLOnBrakeToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseAOLOnBrake"]);
  FrogPilotParamValueControl *pauseLateralToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseLateralSpeed"]);

  if (isMetric) {
    minimumLaneChangeSpeedToggle->updateControl(0, 150, tr("公里/小時"));
    pauseAOLOnBrakeToggle->updateControl(0, 99, tr("公里/小時"));
    pauseLateralToggle->updateControl(0, 99, tr("公里/小時"));

    laneWidthToggle->updateControl(0, 5, tr(" 米"));
  } else {
    minimumLaneChangeSpeedToggle->updateControl(0, 99, tr("英里/小時"));
    pauseAOLOnBrakeToggle->updateControl(0, 99, tr("公里/小時"));
    pauseLateralToggle->updateControl(0, 99, tr("公里/小時"));

    laneWidthToggle->updateControl(0, 15, tr(" feet"));
  }
}

void FrogPilotLateralPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotLateralPanel::hideToggles() {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    bool subToggles = aolKeys.find(key) != aolKeys.end() ||
                      laneChangeKeys.find(key) != laneChangeKeys.end() ||
                      lateralTuneKeys.find(key) != lateralTuneKeys.end() ||
                      qolKeys.find(key) != qolKeys.end();

    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}
