#include "selfdrive/frogpilot/ui/qt/offroad/longitudinal_settings.h"

FrogPilotLongitudinalPanel::FrogPilotLongitudinalPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> longitudinalToggles {
    {"ConditionalExperimental", tr("條件實驗模式"), tr("當滿足特定條件時自動切換到“實驗模式”."), "../frogpilot/assets/toggle_icons/icon_conditional.png"},
    {"CESpeed", tr("低速"), tr("當在沒有領頭車輛的情況下以低於設定速度行駛時，「實驗模式」處於活動狀態."), ""},
    {"CECurves", tr("前方偵測到彎道"), tr("當偵測到前方道路有彎道時，「實驗模式」處於活動狀態."), ""},
    {"CELead", tr("前方偵測到車子"), tr("當偵測到前方有較慢或停止的車輛時，「實驗模式」處於活動狀態."), ""},
    {"CENavigation", tr("導航數據"), tr("「實驗模式」根據導航數據激活，例如即將到來的十字路口或轉彎."), ""},
    {"CEModelStopTime", tr("openpilot 想要停下來"), tr("當 openpilot 想要停車（例如遇到停車標誌或紅燈時）時，「實驗模式」處於活動狀態."), ""},
    {"CESignalSpeed", tr("Turn Signal Below"), tr("'當使用低於設定速度的方向燈時，「實驗模式」處於活動狀態."), ""},
    {"HideCEMStatusBar", tr("隱藏狀態列"), tr("「條件實驗模式」的狀態列被隱藏。"), ""},

    {"CurveSpeedControl", tr("彎道速度控制"), tr("自動減速前方或透過下載的地圖偵測到的彎道."), "../frogpilot/assets/toggle_icons/icon_speed_map.png"},
    {"CurveDetectionMethod", tr("曲線檢測法"), tr("檢測曲線的方法."), ""},
    {"MTSCCurvatureCheck", tr("曲線檢測故障保護"), tr("只有當偵測到前方有彎道時才會觸發彎道控制。使用“基於地圖”方法時，將此用作故障保護以防止誤報."), ""},
    {"CurveSensitivity", tr("曲線靈敏度"), tr("openpilot 對偵測曲線的敏感度如何。較高的值會觸發較早的反應，但存在觸發過於頻繁的風險，而較低的值會增加信心，但存在觸發頻率過低的風險."), ""},
    {"TurnAggressiveness", tr("轉彎速度 積極性"), tr("開放駕駛輪流有多激進。較高的值會導致更快的轉彎，而較低的值會提供更柔和的轉彎."), ""},
    {"DisableCurveSpeedSmoothing", tr("在 UI 中禁用速度值平滑"), tr("UI 中禁用速度值平滑，而是顯示曲線控制項請求的準確速度."), ""},

    {"ExperimentalModeActivation", tr("實驗模式激活"), tr("使用方向盤按鈕或螢幕控制可關閉/開啟“實驗模式”.\n\nThis overrides 'Conditional Experimental Mode'."), "../assets/img_experimental_white.svg"},
    {"ExperimentalModeViaLKAS", tr("點選 LKAS 按鈕"), tr("按下方向盤上的“LKAS”按鈕可切換“實驗模式”."), ""},
    {"ExperimentalModeViaTap", tr("按兩下螢幕"), tr("「實驗模式」可透過在 0.5 秒內雙擊公路 UI 來切換."), ""},
    {"ExperimentalModeViaDistance", tr("長按距離按鈕"), tr("按住方向盤上的「距離」按鈕 0.5 秒以上即可切換「實驗模式」."), ""},

    {"LongitudinalTune", tr("縱向調整"), tr("控制 openpilot 如何管理速度和加速度的設定."), "../frogpilot/assets/toggle_icons/icon_longitudinal_tune.png"},
    {"AccelerationProfile", tr("加速曲線"), tr("選擇運動型或環保加速度."), ""},
    {"DecelerationProfile", tr("減速度曲線"), tr("選擇運動型或環保型減速率."), ""},
    {"HumanAcceleration", tr("類人加速"), tr("起飛時使用引導者的加速度，並在接近最大設定速度時逐漸減少加速度，以實現更平穩的最大速度接近."), ""},
    {"HumanFollowing", tr("類人跟隨距離"), tr("Dynamically adjusts the following distance to feel more natural when approaching slower or stopped vehicles."), ""},
    {"IncreasedStoppedDistance", tr("增加停止距離"), tr("增加在車輛後方停車的距離."), ""},

    {"QOLLongitudinal", tr("提升駕駛體驗"), tr("各種縱向重點功能可改善您的整體開放駕駛體驗."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"CustomCruise", tr("巡航增加間隔"), tr("Interval used when increasing the cruise control speed."), ""},
    {"CustomCruiseLong", tr("自訂巡航間隔 (Long Press)"), tr("Interval used when increasing the cruise control speed when holding down the button for 0.5+ seconds."), ""},
    {"MapGears", tr("將加速/減速映射到齒輪"), tr("將加速和減速曲線映射到“Eco”或“Sport”設定模式."), ""},
    {"OnroadDistanceButton", tr("行進個性按鈕"), tr("目前的駕駛個性顯示在螢幕上。點擊切換個性，或長按2.5秒啟動“交通模式”."), ""},
    {"ReverseCruise", tr("反向巡航增加"), tr("長按功能相反，可將速度提高 5 英里/小時而不是 1 英里/小時."), ""},

    {"SpeedLimitController", tr("限速控制器"), tr("使用「開放街道地圖」、「在 openpilot 上導航」或汽車儀表板（僅限豐田/雷克薩斯/HKG）自動調整您的最大速度以匹配速度限制."), "../assets/offroad/icon_speed_limit.png"},
    {"SLCConfirmation", tr("確認新的速度限制"), tr("使用新的速度限制之前需要手動確認."), ""},
    {"SLCFallback", tr("回退方法"), tr("選擇沒有可用速度限制資料時發生的情況."), ""},
    {"SLCOverride", tr("覆蓋方法"), tr("選擇您想要如何覆蓋當前速度限制.\n\n"), ""},
    {"SLCPriority", tr("限速源優先"), tr("設定限速資料來源的優先順序."), ""},
    {"SLCOffsets", tr("速度限制偏移"), tr("管理與「速度限制控制器」控制項相關的切換."), ""},
    {"Offset1", tr("速度限制偏移 (0-34 mph)"), tr("設定 0 到 34 速度的偏移."), ""},
    {"Offset2", tr("速度限制偏移 (35-54 mph)"), tr("設定 35 到 54 速度的偏移."), ""},
    {"Offset3", tr("速度限制偏移 (55-64 mph)"), tr("設定 55 到 64 速度的偏移."), ""},
    {"Offset4", tr("速度限制偏移 (65-99 mph)"), tr("設定 65 到 99 速度的偏移."), ""},
    {"SLCQOL", tr("改善駕駛體驗"), tr("各種以「速度限制控制器」為重點的功能，可改善您的整體開放駕駛體驗."), ""},
    {"ForceMPHDashboard", tr("強制從儀表板讀取 MPH 讀數"), tr("如果儀表板通常以 KPH 形式顯示速度限制讀數，則強制以 MPH 為單位."), ""},
    {"SLCLookaheadHigher", tr("為更高的速度限製做好準備"), tr("根據地圖資料設定前瞻值，為即將到來的更高速度限製做好準備."), ""},
    {"SLCLookaheadLower", tr("為較低的速度限製做好準備"), tr("根據地圖資料設定前瞻值，為即將到來的較低速度限製做好準備."), ""},
    {"SetSpeedLimit", tr("將速度設定為電流限制"), tr("啟用 openpilot 時設定最大速度以符合當前速度限制."), ""},
    {"SLCVisuals", tr("視覺設定"), tr("管理“限速控制器”的視覺設置'."), ""},
    {"UseVienna", tr("使用維也納風格的速度標誌"), tr("改用維也納式（歐盟）限速標誌而不是 MUTCD（美國）."), ""},
    {"ShowSLCOffset", tr("顯示速度限制偏移"), tr("使用速度限制控制器時，在道路 UI 中單獨顯示速度限制偏移."), ""},
  };

  for (const auto &[param, title, desc, icon] : longitudinalToggles) {
    AbstractControl *longitudinalToggle;

    if (param == "ConditionalExperimental") {
      FrogPilotParamManageControl *conditionalExperimentalToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(conditionalExperimentalToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(conditionalExperimentalKeys);
      });
      longitudinalToggle = conditionalExperimentalToggle;
    } else if (param == "CESpeed") {
      FrogPilotParamValueControl *CESpeed = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr("英里/小時"), std::map<int, QString>(), 1.0, true);
      FrogPilotParamValueControl *CESpeedLead = new FrogPilotParamValueControl("CESpeedLead", tr(" 有前車"), tr("當領先車輛低於設定速度行駛時切換到“實驗模式”."), icon, 0, 99, tr("mph"), std::map<int, QString>(), 1.0, true);
      FrogPilotDualParamControl *conditionalSpeeds = new FrogPilotDualParamControl(CESpeed, CESpeedLead);
      longitudinalToggle = reinterpret_cast<AbstractControl*>(conditionalSpeeds);
    } else if (param == "CECurves") {
      std::vector<QString> curveToggles{"CECurvesLead"};
      std::vector<QString> curveToggleNames{tr(" 有前車")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, curveToggles, curveToggleNames);
    } else if (param == "CELead") {
      std::vector<QString> leadToggles{"CESlowerLead", "CEStoppedLead"};
      std::vector<QString> leadToggleNames{tr("慢速車"), tr("停止車輛")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, leadToggles, leadToggleNames);
    } else if (param == "CENavigation") {
      std::vector<QString> navigationToggles{"CENavigationIntersections", "CENavigationTurns", "CENavigationLead"};
      std::vector<QString> navigationToggleNames{tr("交叉口"), tr("轉彎"), tr("含前車")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, navigationToggles, navigationToggleNames);
    } else if (param == "CEModelStopTime") {
      std::map<int, QString> modelStopTimeLabels;
      for (int i = 0; i <= 10; ++i) {
        modelStopTimeLabels[i] = (i == 0) ? tr("關閉") : QString::number(i) + " 秒";
      }
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, QString(), modelStopTimeLabels);
    } else if (param == "CESignalSpeed") {
      std::vector<QString> ceSignalToggles{"CESignalLaneDetection"};
      std::vector<QString> ceSignalToggleNames{"Lane Detection"};
      longitudinalToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 99, tr("英里/小時"), std::map<int, QString>(), 1.0, ceSignalToggles, ceSignalToggleNames);

    } else if (param == "CurveSpeedControl") {
      FrogPilotParamManageControl *curveControlToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(curveControlToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        curveDetectionBtn->setEnabledButtons(0, QDir("/data/media/0/osm/offline").exists());
        curveDetectionBtn->setCheckedButton(0, params.getBool("MTSCEnabled"));
        curveDetectionBtn->setCheckedButton(1, params.getBool("VisionTurnControl"));

        std::set<QString> modifiedCurveSpeedKeys = curveSpeedKeys;

        if (!params.getBool("MTSCEnabled")) {
          modifiedCurveSpeedKeys.erase("MTSCCurvatureCheck");
        }

        showToggles(modifiedCurveSpeedKeys);
      });
      longitudinalToggle = curveControlToggle;
    } else if (param == "CurveDetectionMethod") {
      curveDetectionBtn = new FrogPilotButtonsControl(title, desc, {tr("基於地圖"), tr("想像")}, true, false);
      QObject::connect(curveDetectionBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        bool mtscEnabled = params.getBool("MTSCEnabled");
        bool vtscEnabled = params.getBool("VisionTurnControl");

        if (id == 0) {
          if (mtscEnabled && !vtscEnabled) {
            curveDetectionBtn->setCheckedButton(0, true);
            return;
          }

          params.putBool("MTSCEnabled", !mtscEnabled);
          curveDetectionBtn->setCheckedButton(0, !mtscEnabled);

          std::set<QString> modifiedCurveSpeedKeys = curveSpeedKeys;

          if (mtscEnabled) {
            modifiedCurveSpeedKeys.erase("MTSCCurvatureCheck");
          }

          showToggles(modifiedCurveSpeedKeys);
        } else if (id == 1) {
          if (vtscEnabled && !mtscEnabled) {
            curveDetectionBtn->setCheckedButton(1, true);
            return;
          }

          params.putBool("VisionTurnControl", !vtscEnabled);
          curveDetectionBtn->setCheckedButton(1, !vtscEnabled);
        }
      });
      longitudinalToggle = curveDetectionBtn;
    } else if (param == "CurveSensitivity" || param == "TurnAggressiveness") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 200, "%");

    } else if (param == "ExperimentalModeActivation") {
      FrogPilotParamManageControl *experimentalModeActivationToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(experimentalModeActivationToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedExperimentalModeActivationKeys = experimentalModeActivationKeys;

        if (isSubaru || (params.getBool("AlwaysOnLateral") && params.getBool("AlwaysOnLateralLKAS"))) {
          modifiedExperimentalModeActivationKeys.erase("ExperimentalModeViaLKAS");
        }

        showToggles(modifiedExperimentalModeActivationKeys);
      });
      longitudinalToggle = experimentalModeActivationToggle;

    } else if (param == "LongitudinalTune") {
      FrogPilotParamManageControl *longitudinalTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(longitudinalTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(longitudinalTuneKeys);
      });
      longitudinalToggle = longitudinalTuneToggle;
    } else if (param == "AccelerationProfile") {
      std::vector<QString> profileOptions{tr("標準"), tr("節能"), tr("運動"), tr("跑車+")};
      ButtonParamControl *profileSelection = new ButtonParamControl(param, title, desc, icon, profileOptions);
      longitudinalToggle = profileSelection;
    } else if (param == "DecelerationProfile") {
      std::vector<QString> profileOptions{tr("標準"), tr("節能"), tr("運動")};
      ButtonParamControl *profileSelection = new ButtonParamControl(param, title, desc, icon, profileOptions);
      longitudinalToggle = profileSelection;
    } else if (param == "IncreasedStoppedDistance") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 15, tr(" feet"));

    } else if (param == "QOLLongitudinal") {
      FrogPilotParamManageControl *qolLongitudinalToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(qolLongitudinalToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedQolKeys = qolKeys;

        if (!hasPCMCruise) {
          modifiedQolKeys.erase("ReverseCruise");
        } else {
          modifiedQolKeys.erase("CustomCruise");
          modifiedQolKeys.erase("CustomCruiseLong");
        }

        if (!isToyota && !isGM && !isHKGCanFd) {
          modifiedQolKeys.erase("MapGears");
        }

        showToggles(modifiedQolKeys);
      });
      longitudinalToggle = qolLongitudinalToggle;
    } else if (param == "CustomCruise") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, tr("英里/小時"));
    } else if (param == "CustomCruiseLong") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, tr("英里/小時"));
    } else if (param == "MapGears") {
      std::vector<QString> mapGearsToggles{"MapAcceleration", "MapDeceleration"};
      std::vector<QString> mapGearsToggleNames{tr("加速"), tr("減速度")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, mapGearsToggles, mapGearsToggleNames);

    } else if (param == "SpeedLimitController") {
      FrogPilotParamManageControl *speedLimitControllerToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(speedLimitControllerToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        bool slcLower = params.getBool("SLCConfirmationLower");
        bool slcHigher = params.getBool("SLCConfirmationHigher");

        slcConfirmationBtn->setCheckedButton(0, slcLower);
        slcConfirmationBtn->setCheckedButton(1, slcHigher);
        slcConfirmationBtn->setCheckedButton(2, !(slcLower || slcHigher));

        slcOpen = true;
        showToggles(speedLimitControllerKeys);
      });
      longitudinalToggle = speedLimitControllerToggle;
    } else if (param == "SLCConfirmation") {
      slcConfirmationBtn = new FrogPilotButtonsControl(title, desc, {tr("最低的限制"), tr("更高的限制"), tr("無")}, true, false);
      QObject::connect(slcConfirmationBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        bool lowerEnabled = params.getBool("SLCConfirmationLower");
        bool higherEnabled = params.getBool("SLCConfirmationHigher");

        if (id == 0) {
          params.putBool("SLCConfirmationLower", !lowerEnabled);
          slcConfirmationBtn->setCheckedButton(0, !lowerEnabled);
          slcConfirmationBtn->setCheckedButton(2, false);

          if (lowerEnabled & !higherEnabled) {
            slcConfirmationBtn->setCheckedButton(2, true);
          }
        } else if (id == 1) {
          params.putBool("SLCConfirmationHigher", !higherEnabled);
          slcConfirmationBtn->setCheckedButton(1, !higherEnabled);
          slcConfirmationBtn->setCheckedButton(2, false);

          if (higherEnabled & !lowerEnabled) {
            slcConfirmationBtn->setCheckedButton(2, true);
          }
        } else {
          params.putBool("SLCConfirmationLower", false);
          params.putBool("SLCConfirmationHigher", false);
          slcConfirmationBtn->setCheckedButton(0, false);
          slcConfirmationBtn->setCheckedButton(1, false);
          slcConfirmationBtn->setCheckedButton(2, true);
        }
      });
      longitudinalToggle = slcConfirmationBtn;
    } else if (param == "SLCFallback") {
      std::vector<QString> fallbackOptions{tr("設定速度"), tr("實驗模式"), tr("先前限制")};
      ButtonParamControl *fallbackSelection = new ButtonParamControl(param, title, desc, icon, fallbackOptions);
      longitudinalToggle = fallbackSelection;
    } else if (param == "SLCOverride") {
      std::vector<QString> overrideOptions{tr("無"), tr("油門踏板壓機"), tr("巡航設定速度")};
      ButtonParamControl *overrideSelection = new ButtonParamControl(param, title, desc, icon, overrideOptions);
      longitudinalToggle = overrideSelection;
    } else if (param == "SLCPriority") {
      ButtonControl *slcPriorityButton = new ButtonControl(title, tr("選擇"), desc);
      QStringList primaryPriorities = {tr("無"), tr("儀表板"), tr("導航"), tr("離線地圖"), tr("最高"), tr("最低")};
      QStringList secondaryTertiaryPriorities = {tr("無"), tr("儀表板"), tr("導航"), tr("離線地圖")};
      QStringList priorityPrompts = {tr("選擇您的首要任務"), tr("選擇您的次要優先事項"), tr("選擇您的第三優先級")};

      QObject::connect(slcPriorityButton, &ButtonControl::clicked, [=]() {
        QStringList selectedPriorities;

        for (int i = 1; i <= 3; ++i) {
          QStringList currentPriorities = (i == 1) ? primaryPriorities : secondaryTertiaryPriorities;
          QStringList prioritiesToDisplay = currentPriorities;
          for (const auto &selectedPriority : qAsConst(selectedPriorities)) {
            prioritiesToDisplay.removeAll(selectedPriority);
          }

          if (!hasDashSpeedLimits) {
            prioritiesToDisplay.removeAll(tr("儀表板"));
          }

          if (prioritiesToDisplay.size() == 1 && prioritiesToDisplay.contains(tr("無"))) {
            break;
          }

          QString priorityKey = QString("SLCPriority%1").arg(i);
          QString selection = MultiOptionDialog::getSelection(priorityPrompts[i - 1], prioritiesToDisplay, "", this);

          if (selection.isEmpty()) break;

          params.putNonBlocking(priorityKey.toStdString(), selection.toStdString());
          selectedPriorities.append(selection);

          if (selection == tr("最低") || selection == tr("最高") || selection == tr("無")) break;

          updateFrogPilotToggles();
        }

        selectedPriorities.removeAll(tr("無"));
        slcPriorityButton->setValue(selectedPriorities.join(", "));
      });

      QStringList initialPriorities;
      for (int i = 1; i <= 3; ++i) {
        QString priorityKey = QString("SLCPriority%1").arg(i);
        QString priority = QString::fromStdString(params.get(priorityKey.toStdString()));

        if (!priority.isEmpty() && primaryPriorities.contains(priority) && priority != tr("無")) {
          initialPriorities.append(priority);
        }
      }
      slcPriorityButton->setValue(initialPriorities.join(", "));
      longitudinalToggle = slcPriorityButton;
    } else if (param == "SLCOffsets") {
      ButtonControl *manageSLCOffsetsBtn = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageSLCOffsetsBtn, &ButtonControl::clicked, [this]() {
        openSubParentToggle();
        showToggles(speedLimitControllerOffsetsKeys);
      });
      longitudinalToggle = reinterpret_cast<AbstractControl*>(manageSLCOffsetsBtn);
    } else if (param == "SLCQOL") {
      ButtonControl *manageSLCQOLBtn = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageSLCQOLBtn, &ButtonControl::clicked, [this]() {
        openSubParentToggle();
        std::set<QString> modifiedSpeedLimitControllerQOLKeys = speedLimitControllerQOLKeys;

        if (hasPCMCruise) {
          modifiedSpeedLimitControllerQOLKeys.erase("SetSpeedLimit");
        }

        if (!isToyota) {
          modifiedSpeedLimitControllerQOLKeys.erase("ForceMPHDashboard");
        }

        showToggles(modifiedSpeedLimitControllerQOLKeys);
      });
      longitudinalToggle = reinterpret_cast<AbstractControl*>(manageSLCQOLBtn);
    } else if (param == "SLCLookaheadHigher" || param == "SLCLookaheadLower") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 60, tr(" 秒"));
    } else if (param == "SLCVisuals") {
      ButtonControl *manageSLCVisualsBtn = new ButtonControl(title, tr("管理"), desc);
      QObject::connect(manageSLCVisualsBtn, &ButtonControl::clicked, [this]() {
        openSubParentToggle();
        showToggles(speedLimitControllerVisualsKeys);
      });
      longitudinalToggle = reinterpret_cast<AbstractControl*>(manageSLCVisualsBtn);
    } else if (param == "Offset1" || param == "Offset2" || param == "Offset3" || param == "Offset4") {
      longitudinalToggle = new FrogPilotParamValueControl(param, title, desc, icon, -99, 99, tr("英里/小時"));
    } else if (param == "ShowSLCOffset") {
      std::vector<QString> slcOffsetToggles{"ShowSLCOffsetUI"};
      std::vector<QString> slcOffsetToggleNames{tr("透過使用者介面控制")};
      longitudinalToggle = new FrogPilotButtonToggleControl(param, title, desc, slcOffsetToggles, slcOffsetToggleNames);

    } else {
      longitudinalToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(longitudinalToggle);
    toggles[param] = longitudinalToggle;

    makeConnections(longitudinalToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(longitudinalToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotLongitudinalPanel::openParentToggle);
    }

    QObject::connect(longitudinalToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["ExperimentalModeViaLKAS"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state && params.getBool("AlwaysOnLateralLKAS")) {
      params.putBoolNonBlocking("AlwaysOnLateralLKAS", false);
    }
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotLongitudinalPanel::hideToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubParentToggle, this, &FrogPilotLongitudinalPanel::hideSubToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateCarToggles, this, &FrogPilotLongitudinalPanel::updateCarToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotLongitudinalPanel::updateMetric);

  updateMetric();
}

void FrogPilotLongitudinalPanel::updateCarToggles() {
  hasDashSpeedLimits = parent->hasDashSpeedLimits;
  hasPCMCruise = parent->hasPCMCruise;
  isGM = parent->isGM;
  isHKGCanFd = parent->isHKGCanFd;
  isSubaru = parent->isSubaru;
  isToyota = parent->isToyota;

  hideToggles();
}

void FrogPilotLongitudinalPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;
    double speedConversion = isMetric ? MILE_TO_KM : KM_TO_MILE;

    params.putFloatNonBlocking("IncreasedStoppedDistance", params.getFloat("IncreasedStoppedDistance") * distanceConversion);

    params.putFloatNonBlocking("CESignalSpeed", params.getFloat("CESignalSpeed") * speedConversion);
    params.putFloatNonBlocking("CESpeed", params.getFloat("CESpeed") * speedConversion);
    params.putFloatNonBlocking("CESpeedLead", params.getFloat("CESpeedLead") * speedConversion);
    params.putFloatNonBlocking("CustomCruise", params.getFloat("CustomCruise") * speedConversion);
    params.putFloatNonBlocking("CustomCruiseLong", params.getFloat("CustomCruiseLong") * speedConversion);
    params.putFloatNonBlocking("Offset1", params.getFloat("Offset1") * speedConversion);
    params.putFloatNonBlocking("Offset2", params.getFloat("Offset2") * speedConversion);
    params.putFloatNonBlocking("Offset3", params.getFloat("Offset3") * speedConversion);
    params.putFloatNonBlocking("Offset4", params.getFloat("Offset4") * speedConversion);
  }

  FrogPilotDualParamControl *ceSpeedToggle = reinterpret_cast<FrogPilotDualParamControl*>(toggles["CESpeed"]);
  FrogPilotParamValueButtonControl *ceSignal = reinterpret_cast<FrogPilotParamValueButtonControl*>(toggles["CESignalSpeed"]);
  FrogPilotParamValueControl *customCruiseToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruise"]);
  FrogPilotParamValueControl *customCruiseLongToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruiseLong"]);
  FrogPilotParamValueControl *offset1Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset1"]);
  FrogPilotParamValueControl *offset2Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset2"]);
  FrogPilotParamValueControl *offset3Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset3"]);
  FrogPilotParamValueControl *offset4Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset4"]);
  FrogPilotParamValueControl *increasedStoppedDistanceToggle = static_cast<FrogPilotParamValueControl*>(toggles["IncreasedStoppedDistance"]);

  if (isMetric) {
    offset1Toggle->setTitle(tr("Speed Limit Offset (0-34 kph)"));
    offset2Toggle->setTitle(tr("Speed Limit Offset (35-54 kph)"));
    offset3Toggle->setTitle(tr("Speed Limit Offset (55-64 kph)"));
    offset4Toggle->setTitle(tr("Speed Limit Offset (65-99 kph)"));

    offset1Toggle->setDescription(tr("Set speed limit offset for limits between 0-34 kph."));
    offset2Toggle->setDescription(tr("Set speed limit offset for limits between 35-54 kph."));
    offset3Toggle->setDescription(tr("Set speed limit offset for limits between 55-64 kph."));
    offset4Toggle->setDescription(tr("Set speed limit offset for limits between 65-99 kph."));

    ceSignal->updateControl(0, 150, tr("kph"));
    ceSpeedToggle->updateControl(0, 150, tr("kph"));
    customCruiseToggle->updateControl(1, 150, tr("kph"));
    customCruiseLongToggle->updateControl(1, 150, tr("kph"));
    offset1Toggle->updateControl(-99, 99, tr("kph"));
    offset2Toggle->updateControl(-99, 99, tr("kph"));
    offset3Toggle->updateControl(-99, 99, tr("kph"));
    offset4Toggle->updateControl(-99, 99, tr("kph"));

    increasedStoppedDistanceToggle->updateControl(0, 5, tr(" meters"));
  } else {
    offset1Toggle->setTitle(tr("Speed Limit Offset (0-34 mph)"));
    offset2Toggle->setTitle(tr("Speed Limit Offset (35-54 mph)"));
    offset3Toggle->setTitle(tr("Speed Limit Offset (55-64 mph)"));
    offset4Toggle->setTitle(tr("Speed Limit Offset (65-99 mph)"));

    offset1Toggle->setDescription(tr("Set speed limit offset for limits between 0-34 mph."));
    offset2Toggle->setDescription(tr("Set speed limit offset for limits between 35-54 mph."));
    offset3Toggle->setDescription(tr("Set speed limit offset for limits between 55-64 mph."));
    offset4Toggle->setDescription(tr("Set speed limit offset for limits between 65-99 mph."));

    ceSignal->updateControl(0, 99, tr("mph"));
    ceSpeedToggle->updateControl(0, 99, tr("mph"));
    customCruiseToggle->updateControl(1, 99, tr("mph"));
    customCruiseLongToggle->updateControl(1, 99, tr("mph"));
    offset1Toggle->updateControl(-99, 99, tr("mph"));
    offset2Toggle->updateControl(-99, 99, tr("mph"));
    offset3Toggle->updateControl(-99, 99, tr("mph"));
    offset4Toggle->updateControl(-99, 99, tr("mph"));

    increasedStoppedDistanceToggle->updateControl(0, 15, tr(" feet"));
  }
}

void FrogPilotLongitudinalPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotLongitudinalPanel::hideToggles() {
  setUpdatesEnabled(false);

  slcOpen = false;

  for (auto &[key, toggle] : toggles) {
    bool subToggles = conditionalExperimentalKeys.find(key) != conditionalExperimentalKeys.end() ||
                      curveSpeedKeys.find(key) != curveSpeedKeys.end() ||
                      experimentalModeActivationKeys.find(key) != experimentalModeActivationKeys.end() ||
                      longitudinalTuneKeys.find(key) != longitudinalTuneKeys.end() ||
                      qolKeys.find(key) != qolKeys.end() ||
                      speedLimitControllerKeys.find(key) != speedLimitControllerKeys.end() ||
                      speedLimitControllerOffsetsKeys.find(key) != speedLimitControllerOffsetsKeys.end() ||
                      speedLimitControllerQOLKeys.find(key) != speedLimitControllerQOLKeys.end() ||
                      speedLimitControllerVisualsKeys.find(key) != speedLimitControllerVisualsKeys.end();

    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotLongitudinalPanel::hideSubToggles() {
  if (slcOpen) {
    showToggles(speedLimitControllerKeys);
  }
}
