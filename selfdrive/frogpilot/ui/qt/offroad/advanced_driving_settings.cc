#include "selfdrive/frogpilot/ui/qt/offroad/advanced_driving_settings.h"

FrogPilotAdvancedDrivingPanel::FrogPilotAdvancedDrivingPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> advancedToggles {
    {"AdvancedLateralTune", tr("進階橫向調整"), tr("控制 openpilot 如何管理轉向的高階設置."), "../frogpilot/assets/toggle_icons/icon_advanced_lateral_tune.png"},
    {"SteerFriction", steerFrictionStock != 0 ? QString(tr("轉向阻力 (Default: %1)")).arg(QString::number(steerFrictionStock, 'f', 2)) : tr("轉向阻力"), tr("轉向時的阻力。較高的值提供更穩定的轉向，但可能會讓人感覺沉重，而較低的值允許更輕的轉向，但可能感覺太敏感."), ""},
    {"SteerKP", steerKPStock != 0 ? QString(tr("校正速度 (Default: %1)")).arg(QString::number(steerKPStock, 'f', 2)) : tr("校正速度"), tr("汽車糾正轉向的正面程度。較高的值提供更快的校正，但可能會感覺不穩定，而較低的值使轉向更平穩，但響應較慢."), ""},
    {"SteerLatAccel", steerLatAccelStock != 0 ? QString(tr("橫向加速 (Default: %1)")).arg(QString::number(steerLatAccelStock, 'f', 2)) : tr("橫向加速"), tr("調整汽車左右轉向的速度。較高的值可以更快地變換車道，但可能會感覺不穩定，而較低的值可以提供更平穩的轉向，但可能會感覺遲緩."), ""},
    {"SteerRatio", steerRatioStock != 0 ? QString(tr("轉向比 (Default: %1)")).arg(QString::number(steerRatioStock, 'f', 2)) : tr("轉向比"), tr("調整 openpilot 需要轉動方向盤多少度才能轉向。較高的數值感覺像駕駛卡車，高速時較穩定，但低速時較難快速轉向，而較低的數值感覺像卡丁車，較容易在狹窄的地方轉向，但高速時較敏感且穩定性較差."), ""},
    {"TacoTune", tr("comma's 2022 Taco Bell Turn Hack"), tr("Use comma's hack they used to help handle left and right turns more precisely during their 2022 'Taco Bell' drive."), ""},
    {"ForceAutoTune", tr("強制自動橫向 On"), tr("強制逗號對不支援的車輛進行自動橫向調整."), ""},
    {"ForceAutoTuneOff", tr("強制自動橫向 Off"), tr("強制逗號對受支援車輛的自動橫向調整."), ""},
    {"TurnDesires", tr("轉彎預測"), tr("強制模型在低於最小變換車道速度時使用轉彎期望，以幫助更精確地進行左轉和右轉."), ""},

    {"AdvancedLongitudinalTune", tr("進階縱向調整"), tr("控制 openpilot 如何管理速度和加速度的高階設置."), "../frogpilot/assets/toggle_icons/icon_advanced_longitudinal_tune.png"},
    {"LeadDetectionThreshold", tr("前車偵測敏感度"), tr("openpilot 對偵測前方車輛的敏感度如何。較低的值有助於更快、更遠地偵測到車輛，但有時可能會將其他物體誤認為車輛."), ""},
    {"MaxDesiredAcceleration", tr("最大加速度"), tr("設定 openpilot 加速速度上限，以防止低速時出現高加速度."), ""},

    {"AdvancedQOLDriving", tr("進階設定"), tr("各種高級功能可改善您的整體開放駕駛體驗."), "../frogpilot/assets/toggle_icons/advanced_quality_of_life.png"},
    {"ForceStandstill", tr("強制保持 openpilot 處於靜止狀態"), tr("將 openpilot 保持在「靜止」狀態，直到按下油門踏板或「恢復」按鈕."), ""},
    {"ForceStops", tr("「偵測到」停車燈/標誌時強制停車"), tr("每當 openpilot 「偵測到」潛在的停車燈/停車標誌時，請強制在最初偵測到的位置停車，以防止闖入潛在的紅燈/停車標誌."), ""},
    {"SetSpeedOffset", tr("設定速度偏移"), tr("與目前設定速度相比，設定速度應高或低多少。例如，如果您希望以高於速度限制 5 英里/小時的速度行駛，則當您調整設定速度時，此設定會自動添加該差異."), ""},

    {"CustomPersonalities", tr("客製化駕駛個性"), tr("自訂個性檔案以滿足您的喜好."), "../frogpilot/assets/toggle_icons/icon_advanced_personality.png"},
    {"TrafficPersonalityProfile", tr("塞車駕駛"), tr("自訂「塞車」個性檔案，專為塞車導航而定制."), "../frogpilot/assets/stock_theme/distance_icons/traffic.png"},
    {"TrafficFollow", tr("跟隨距離"), tr("「塞車模式」下的最小跟隨距離。 openpilot 將根據您的速度在此值和「積極」設定檔距離之間動態調整."), ""},
    {"TrafficJerkAcceleration", tr("加速靈敏度"), tr("openpilot 對「塞車模式」下加速度變化的敏感度如何。較高的值會導致更平滑、更漸進的加速和減速，而較低的值允許更快的變化，可能會感覺更突然."), ""},
    {"TrafficJerkDeceleration", tr("減速度靈敏度"), tr("控制 openpilot 對「交通模式」中減速度變化的敏感度。較高的值會導致更平穩、更漸進的製動，而較低的值則允許更快、更靈敏的製動，但可能會感覺突然."), ""},
    {"TrafficJerkDanger", tr("安全距離靈敏度"), tr("調整「塞車模式」下 openpilot 對其他車輛或障礙物的謹慎程度。較高的值會增加跟隨距離並優先考慮安全，從而導致更謹慎的駕駛，而較低的值允許更緊密的跟隨，但可能會減少反應時間."), ""},
    {"TrafficJerkSpeed", tr("速度提高反應能力"), tr("控制 openpilot 在「塞車模式」下調整速度的速度。較高的值可確保更平滑、更漸進的速度變化，而較低的值可實現更快的調整，但可能會感覺更尖銳或不太平滑."), ""},
    {"TrafficJerkSpeedDecrease", tr("速度降低反應能力"), tr("設定 openpilot 在「塞車模式」下調整速度降低的速度。較高的值可確保減速時過渡更平滑，而較低的值可實現更快、更靈敏的減速，可能會感覺更銳利."), ""},
    {"ResetTrafficPersonality", tr("恢復設定"), tr("將“塞車模式”設定恢復為預設值."), ""},

    {"AggressivePersonalityProfile", tr("積極駕駛"), tr("客製化「積極」個性檔案，專為更自信的駕駛風格而設計."), "../frogpilot/assets/stock_theme/distance_icons/aggressive.png"},
    {"AggressiveFollow", tr("跟隨距離"), tr("將跟隨距離設定為“積極” 模式. 這大致決定了您將跟隨前方車輛的秒數.\n\n預設: 1.25 秒數."), ""},
    {"AggressiveJerkAcceleration", tr("加速靈敏度"), tr("控制 openpilot 對「積極​​」模式下加速度變化的敏感度。較高的值使加速和減速更平滑但較慢，而較低的值允許更快的變化，但可能會感覺不穩定.\n\n預設: 0.5."), ""},
    {"AggressiveJerkDeceleration", tr("減速度靈敏度"), tr("控制 openpilot 在「積極」模式下對減速的敏感度。較高的值會導致更平穩的製動，而較低的值允許更立即的製動，但可能會感覺突然.\n\n預設: 0.5."), ""},
    {"AggressiveJerkDanger", tr("安全距離靈敏度"), tr("調整 openpilot 在「積極」模式下對車輛或障礙物的謹慎程度。較高的值使其更加謹慎，而較低的值允許更緊密的跟隨，增加突然煞車的風險.\n\n預設: 1.0."), ""},
    {"AggressiveJerkSpeed", tr("速度提高反應能力"), tr("控制 openpilot 在「積極」模式下調整速度的速度。較高的值會導致更平滑但較慢的速度變化，而較低的值會使速度調整更快但可能更突然.\n\n預設: 0.5."), ""},
    {"AggressiveJerkSpeedDecrease", tr("速度降低反應能力"), tr("設定 openpilot 在「積極」模式下調整速度降低的速度。較高的數值可確保減速時過渡更平滑，而較低的數值可實現更快、更靈敏的速度降低（可能會讓人感覺急劇）.\n\n預設: 0.5."), ""},
    {"ResetAggressivePersonality", tr("重新設定"), tr("將“激進”設定恢復為其預設值."), ""},

    {"StandardPersonalityProfile", tr("標準駕駛"), tr("客製化「標準」個性檔案，針對平衡駕駛進行最佳化."), "../frogpilot/assets/stock_theme/distance_icons/standard.png"},
    {"StandardFollow", tr("跟隨距離"), tr("設定“標準”模式的跟隨距離。這大致決定了您將跟隨前方車輛的秒數.\n\n預設: 1.45 秒."), ""},
    {"StandardJerkAcceleration", tr("加速靈敏度"), tr("控制 openpilot 對「標準」模式下加速度變化的敏感度。較高的值使加速和減速更平滑但較慢，而較低的值允許更快的變化，但可能會感覺不穩定.\n\n預設: 1.0."), ""},
    {"StandardJerkDeceleration", tr("減速度靈敏度"), tr("控制 openpilot 在「標準」模式下對減速的敏感度。較高的值會導致更平穩的製動，而較低的值則允許更快、更立即的製動，但可能會感覺突然.\n\n預設: 1.0."), ""},
    {"StandardJerkDanger", tr("安全距離靈敏度"), tr("調整「標準」模式下 openpilot 在車輛或障礙物周圍的謹慎程度。較高的值使其更加謹慎，而較低的值允許更緊密的跟隨，增加突然煞車的風險.\n\n預設: 1.0."), ""},
    {"StandardJerkSpeed", tr("速度提高反應能力"), tr("控制 openpilot 在「標準」模式下調整速度的速度。較高的值會導致更平滑但較慢的速度變化，而較低的值會使速度調整更快但可能更突然.\n\n預設: 1.0."), ""},
    {"StandardJerkSpeedDecrease", tr("速度降低反應能力"), tr("設定 openpilot 在「標準」模式下調整速度降低的速度。較高的數值可確保減速時過渡更平滑，而較低的數值可實現更快、更靈敏的速度降低（可能會讓人感覺急劇）.\n\n預設: 1.0."), ""},
    {"ResetStandardPersonality", tr("重新設定"), tr("將“標準”設定恢復為預設值."), ""},

    {"RelaxedPersonalityProfile", tr("性格駕駛"), tr("客製化「輕鬆」個性檔案，非常適合更悠閒的駕駛風格."), "../frogpilot/assets/stock_theme/distance_icons/relaxed.png"},
    {"RelaxedFollow", tr("跟隨距離"), tr("設定“放鬆”模式的跟隨距離。這大致決定了您將跟隨前方車輛的秒數.\n\n預設: 1.75 seconds."), ""},
    {"RelaxedJerkAcceleration", tr("加速靈敏度"), tr("控制 openpilot 對「放鬆」模式下加速度變化的敏感度。較高的值使加速和減速更平滑但較慢，而較低的值允許更快的變化，但可能會感覺不穩定.\n\n預設: 1.0."), ""},
    {"RelaxedJerkDeceleration", tr("減速度靈敏度"), tr("控制 openpilot 在「放鬆」模式下對減速的敏感度。較高的值會導致更平穩的製動，而較低的值則允許更快、更立即的製動，但可能會感覺突然.\n\n預設: 1.0."), ""},
    {"RelaxedJerkDanger", tr("安全距離靈敏度"), tr("調整 openpilot 在「放鬆」模式下對車輛或障礙物的謹慎程度。較高的值使其更加謹慎，而較低的值允許更緊密的跟隨，增加突然煞車的風險.\n\n預設: 1.0."), ""},
    {"RelaxedJerkSpeed", tr("速度提高反應能力"), tr("控制 openpilot 在「放鬆」模式下調整速度的速度。較高的值會導致更平滑但較慢的速度變化，而較低的值會使速度調整更快但可能更突然.\n\n預設: 1.0."), ""},
    {"RelaxedJerkSpeedDecrease", tr("速度降低反應能力"), tr("設定 openpilot 在「輕鬆」模式下調整速度降低的速度。較高的數值可確保減速時過渡更平滑，而較低的數值可實現更快、更靈敏的速度降低（可能會讓人感覺急劇）.\n\n預設: 1.0."), ""},
    {"ResetRelaxedPersonality", tr("重新設定"), tr("將“寬鬆”設定恢復為預設值."), ""},

    {"ModelManagement", tr("模型管理"), tr("管理 openpilot 所使用的駕駛模型."), "../frogpilot/assets/toggle_icons/icon_advanced_model.png"},
    {"AutomaticallyUpdateModels", tr("自動更新和下載模型"), tr("自動下載新的或更新的駕駛模型."), ""},
    {"ModelRandomizer", tr("模型隨機產生器"), tr("隨機選擇一個型號，如果時間超過 15 分鐘，可以在每次駕駛結束時進行查看，以幫助找到您喜歡的型號."), ""},
    {"ManageBlacklistedModels", tr("管理模型黑名單"), tr("控制哪些型號被列入黑名單並且不會用於未來的驅動器."), ""},
    {"ResetScores", tr("重置模型分數"), tr("清除您對駕駛車型的評分."), ""},
    {"ReviewScores", tr("查看模型分數"), tr("查看您為駕駛模型分配的評分."), ""},
    {"DeleteModel", tr("刪除模型"), tr("從您的裝置中刪除選定的駕駛模型."), ""},
    {"DownloadModel", tr("下載模型"), tr("下載未下載的駕駛模型."), ""},
    {"DownloadAllModels", tr("下載所有型號"), tr("下載所有未下載的駕駛模型."), ""},
    {"SelectModel", tr("選擇模型"), tr("選擇 openpilot 使用哪種模型驅動."), ""},
    {"ResetCalibrations", tr("重置模型校準"), tr("重置駕駛模型的校準設置."), ""},
  };

  for (const auto &[param, title, desc, icon] : advancedToggles) {
    AbstractControl *advancedDrivingToggle;

    if (param == "AdvancedLateralTune") {
      FrogPilotParamManageControl *advancedLateralTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(advancedLateralTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedLateralTuneKeys = lateralTuneKeys;

        bool usingNNFF = hasNNFFLog && params.getBool("LateralTune") && params.getBool("NNFF");
        if (usingNNFF) {
          modifiedLateralTuneKeys.erase("ForceAutoTune");
          modifiedLateralTuneKeys.erase("ForceAutoTuneOff");
        } else {
          if (hasAutoTune) {
            modifiedLateralTuneKeys.erase("ForceAutoTune");
          } else {
            modifiedLateralTuneKeys.erase("ForceAutoTuneOff");
          }
        }

        if (!liveValid || usingNNFF) {
          modifiedLateralTuneKeys.erase("SteerFriction");
          modifiedLateralTuneKeys.erase("SteerLatAccel");
        }

        showToggles(modifiedLateralTuneKeys);
      });
      advancedDrivingToggle = advancedLateralTuneToggle;
    } else if (param == "SteerFriction") {
      std::vector<QString> steerFrictionToggleNames{"Reset"};
      advancedDrivingToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0.01, 0.25, QString(), std::map<int, QString>(), 0.01, {}, steerFrictionToggleNames, false);
    } else if (param == "SteerKP") {
      std::vector<QString> steerKPToggleNames{"Reset"};
      advancedDrivingToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, steerKPStock * 0.50, steerKPStock * 1.50, QString(), std::map<int, QString>(), 0.01, {}, steerKPToggleNames, false);
    } else if (param == "SteerLatAccel") {
      std::vector<QString> steerLatAccelToggleNames{"Reset"};
      advancedDrivingToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, steerLatAccelStock * 0.25, steerLatAccelStock * 1.25, QString(), std::map<int, QString>(), 0.01, {}, steerLatAccelToggleNames, false);
    } else if (param == "SteerRatio") {
      std::vector<QString> steerRatioToggleNames{"Reset"};
      advancedDrivingToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, steerRatioStock * 0.75, steerRatioStock * 1.25, QString(), std::map<int, QString>(), 0.01, {}, steerRatioToggleNames, false);

    } else if (param == "AdvancedLongitudinalTune") {
      FrogPilotParamManageControl *advancedLongitudinalTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(advancedLongitudinalTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedLongitudinalTuneKeys = longitudinalTuneKeys;

        bool radarlessModel = QString::fromStdString(params.get("RadarlessModels")).split(",").contains(QString::fromStdString(params.get("Model")));
        if (radarlessModel) {
          modifiedLongitudinalTuneKeys.erase("LeadDetectionThreshold");
        }

        showToggles(modifiedLongitudinalTuneKeys);
      });
      advancedDrivingToggle = advancedLongitudinalTuneToggle;
    } else if (param == "LeadDetectionThreshold") {
      advancedDrivingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, "%");
    } else if (param == "MaxDesiredAcceleration") {
      advancedDrivingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.1, 4.0, "m/s", std::map<int, QString>(), 0.1);

    } else if (param == "AdvancedQOLDriving") {
      FrogPilotParamManageControl *advancedQOLToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(advancedQOLToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedQolKeys = qolKeys;

        if (hasPCMCruise) {
          modifiedQolKeys.erase("SetSpeedOffset");
        }

        showToggles(modifiedQolKeys);
      });
      advancedDrivingToggle = advancedQOLToggle;
    } else if (param == "SetSpeedOffset") {
      advancedDrivingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, tr("英里/小時"));

    } else if (param == "CustomPersonalities") {
      FrogPilotParamManageControl *customPersonalitiesToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(customPersonalitiesToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(customDrivingPersonalityKeys);
      });
      advancedDrivingToggle = customPersonalitiesToggle;
    } else if (param == "ResetTrafficPersonality" || param == "ResetAggressivePersonality" || param == "ResetStandardPersonality" || param == "ResetRelaxedPersonality") {
      FrogPilotButtonsControl *profileBtn = new FrogPilotButtonsControl(title, desc, {tr("重設")});
      advancedDrivingToggle = profileBtn;
    } else if (param == "TrafficPersonalityProfile") {
      FrogPilotParamManageControl *trafficPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(trafficPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        customPersonalityOpen = true;
        openSubParentToggle();
        showToggles(trafficPersonalityKeys);
      });
      advancedDrivingToggle = trafficPersonalityToggle;
    } else if (param == "AggressivePersonalityProfile") {
      FrogPilotParamManageControl *aggressivePersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(aggressivePersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        customPersonalityOpen = true;
        openSubParentToggle();
        showToggles(aggressivePersonalityKeys);
      });
      advancedDrivingToggle = aggressivePersonalityToggle;
    } else if (param == "StandardPersonalityProfile") {
      FrogPilotParamManageControl *standardPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(standardPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        customPersonalityOpen = true;
        openSubParentToggle();
        showToggles(standardPersonalityKeys);
      });
      advancedDrivingToggle = standardPersonalityToggle;
    } else if (param == "RelaxedPersonalityProfile") {
      FrogPilotParamManageControl *relaxedPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(relaxedPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        customPersonalityOpen = true;
        openSubParentToggle();
        showToggles(relaxedPersonalityKeys);
      });
      advancedDrivingToggle = relaxedPersonalityToggle;
    } else if (trafficPersonalityKeys.find(param) != trafficPersonalityKeys.end() ||
               aggressivePersonalityKeys.find(param) != aggressivePersonalityKeys.end() ||
               standardPersonalityKeys.find(param) != standardPersonalityKeys.end() ||
               relaxedPersonalityKeys.find(param) != relaxedPersonalityKeys.end()) {
      if (param == "TrafficFollow" || param == "AggressiveFollow" || param == "StandardFollow" || param == "RelaxedFollow") {
        if (param == "TrafficFollow") {
          advancedDrivingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.5, 5, tr(" 秒"), std::map<int, QString>(), 0.01);
        } else {
          advancedDrivingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 5, tr(" 秒"), std::map<int, QString>(), 0.01);
        }
      } else {
        advancedDrivingToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 500, "%");
      }

    } else if (param == "ModelManagement") {
      FrogPilotParamManageControl *modelManagementToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(modelManagementToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        availableModelNames = QString::fromStdString(params.get("AvailableModelsNames")).split(",");
        availableModels = QString::fromStdString(params.get("AvailableModels")).split(",");
        experimentalModels = QString::fromStdString(params.get("ExperimentalModels")).split(",");

        modelManagementOpen = true;

        QString currentModel = QString::fromStdString(params.get("Model")) + ".thneed";
        QStringList modelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
        modelFiles.removeAll(currentModel);
        haveModelsDownloaded = modelFiles.size() > 1;
        modelsDownloaded = params.getBool("ModelsDownloaded");

        showToggles(modelManagementKeys);
      });
      advancedDrivingToggle = modelManagementToggle;
    } else if (param == "ModelRandomizer") {
      FrogPilotParamManageControl *modelRandomizerToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(modelRandomizerToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        openSubParentToggle();
        showToggles(modelRandomizerKeys);
      });
      advancedDrivingToggle = modelRandomizerToggle;
    } else if (param == "ManageBlacklistedModels") {
      FrogPilotButtonsControl *blacklistBtn = new FrogPilotButtonsControl(title, desc, {tr("增加"), tr("移除")});
      QObject::connect(blacklistBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QStringList blacklistedModels = QString::fromStdString(params.get("BlacklistedModels")).split(",", QString::SkipEmptyParts);
        QMap<QString, QString> labelToModelMap;
        QStringList selectableModels, deletableModels;

        for (int i = 0; i < availableModels.size(); ++i) {
          QString model = availableModels[i];
          if (blacklistedModels.contains(model)) {
            deletableModels.append(availableModelNames[i]);
          } else {
            selectableModels.append(availableModelNames[i]);
          }
          labelToModelMap[availableModelNames[i]] = model;
        }

        if (id == 0) {
          if (selectableModels.size() == 1) {
            FrogPilotConfirmationDialog::toggleAlert(tr("沒有更多型號可列入黑名單！唯一可用的型號是 \"%1\"!").arg(selectableModels.first()), tr("確認"), this);
          } else {
            QString selectedModel = MultiOptionDialog::getSelection(tr("選擇要加入黑名單的型號"), selectableModels, "", this);
            if (!selectedModel.isEmpty()) {
              if (ConfirmationDialog::confirm(tr("您確定要新增 '%1' 型號加入黑名單?").arg(selectedModel), tr("添加"), this)) {
                QString modelToAdd = labelToModelMap[selectedModel];
                if (!blacklistedModels.contains(modelToAdd)) {
                  blacklistedModels.append(modelToAdd);
                  params.putNonBlocking("BlacklistedModels", blacklistedModels.join(",").toStdString());
                }
              }
            }
          }
        } else if (id == 1) {
          QString selectedModel = MultiOptionDialog::getSelection(tr("選擇要從黑名單中刪除的型號"), deletableModels, "", this);
          if (!selectedModel.isEmpty()) {
            if (ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 黑名單中的型號?").arg(selectedModel), tr("消除"), this)) {
              QString modelToRemove = labelToModelMap[selectedModel];
              if (blacklistedModels.contains(modelToRemove)) {
                blacklistedModels.removeAll(modelToRemove);
                params.putNonBlocking("BlacklistedModels", blacklistedModels.join(",").toStdString());
                paramsStorage.put("BlacklistedModels", blacklistedModels.join(",").toStdString());
              }
            }
          }
        }
      });
      advancedDrivingToggle = blacklistBtn;
    } else if (param == "ResetScores") {
      ButtonControl *resetCalibrationsBtn = new ButtonControl(title, tr("重置"), desc);
      QObject::connect(resetCalibrationsBtn, &ButtonControl::clicked, [this]() {
        if (FrogPilotConfirmationDialog::yesorno(tr("重置所有模型分數?"), this)) {
          for (const QString &model : availableModelNames) {
            QString cleanedModel = processModelName(model);
            params.remove(QString("%1Drives").arg(cleanedModel).toStdString());
            paramsStorage.remove(QString("%1Drives").arg(cleanedModel).toStdString());
            params.remove(QString("%1Score").arg(cleanedModel).toStdString());
            paramsStorage.remove(QString("%1Score").arg(cleanedModel).toStdString());
          }
          updateModelLabels();
        }
      });
      advancedDrivingToggle = reinterpret_cast<AbstractControl*>(resetCalibrationsBtn);
    } else if (param == "ReviewScores") {
      ButtonControl *reviewScoresBtn = new ButtonControl(title, tr("查看"), desc);
      QObject::connect(reviewScoresBtn, &ButtonControl::clicked, [this]() {
        openSubSubParentToggle();

        for (LabelControl *label : labelControls) {
          label->setVisible(true);
        }

        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(false);
        }
      });
      advancedDrivingToggle = reinterpret_cast<AbstractControl*>(reviewScoresBtn);
    } else if (param == "DeleteModel") {
      deleteModelBtn = new ButtonControl(title, tr("刪除"), desc);
      QObject::connect(deleteModelBtn, &ButtonControl::clicked, [this]() {
        QStringList deletableModels, existingModels = modelDir.entryList({"*.thneed"}, QDir::Files);
        QMap<QString, QString> labelToFileMap;
        QString currentModel = QString::fromStdString(params.get("Model")) + ".thneed";

        for (int i = 0; i < availableModels.size(); ++i) {
          QString modelFile = availableModels[i] + ".thneed";
          if (existingModels.contains(modelFile) && modelFile != currentModel && !availableModelNames[i].contains("(Default)")) {
            deletableModels.append(availableModelNames[i]);
            labelToFileMap[availableModelNames[i]] = modelFile;
          }
        }

        QString selectedModel = MultiOptionDialog::getSelection(tr("選擇要刪除的模型"), deletableModels, "", this);
        if (!selectedModel.isEmpty()) {
          if (ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 模型?").arg(selectedModel), tr("刪除"), this)) {
            std::thread([=]() {
              modelDeleting = true;
              modelsDownloaded = false;
              update();

              params.putBoolNonBlocking("ModelsDownloaded", false);
              deleteModelBtn->setValue(tr("正在刪除..."));

              QFile::remove(modelDir.absoluteFilePath(labelToFileMap[selectedModel]));
              deleteModelBtn->setValue(tr("已刪除!"));

              util::sleep_for(1000);
              deleteModelBtn->setValue("");
              modelDeleting = false;

              QStringList modelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
              modelFiles.removeAll(currentModel);

              haveModelsDownloaded = modelFiles.size() > 1;
              update();
            }).detach();
          }
        }
      });
      advancedDrivingToggle = reinterpret_cast<AbstractControl*>(deleteModelBtn);
    } else if (param == "DownloadModel") {
      downloadModelBtn = new ButtonControl(title, tr("下載"), desc);
      QObject::connect(downloadModelBtn, &ButtonControl::clicked, [this]() {
        if (downloadModelBtn->text() == tr("取消")) {
          paramsMemory.remove("ModelToDownload");
          paramsMemory.putBool("CancelModelDownload", true);
          cancellingDownload = true;

          device()->resetInteractiveTimeout(30);
        } else {
          QMap<QString, QString> labelToModelMap;
          QStringList existingModels = modelDir.entryList({"*.thneed"}, QDir::Files);
          QStringList downloadableModels;

          for (int i = 0; i < availableModels.size(); ++i) {
            QString modelFile = availableModels[i] + ".thneed";
            if (!existingModels.contains(modelFile) && !availableModelNames[i].contains("(Default)")) {
              downloadableModels.append(availableModelNames[i]);
              labelToModelMap.insert(availableModelNames[i], availableModels[i]);
            }
          }

          QString modelToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的駕駛模型"), downloadableModels, "", this);
          if (!modelToDownload.isEmpty()) {
            device()->resetInteractiveTimeout(300);

            modelDownloading = true;
            paramsMemory.put("ModelToDownload", labelToModelMap.value(modelToDownload).toStdString());
            paramsMemory.put("ModelDownloadProgress", "0%");

            downloadModelBtn->setValue(tr("正在下載 %1...").arg(modelToDownload.remove(QRegularExpression("[🗺️👀📡]")).trimmed()));

            QTimer *progressTimer = new QTimer(this);
            progressTimer->setInterval(100);

            QObject::connect(progressTimer, &QTimer::timeout, this, [=]() {
              QString progress = QString::fromStdString(paramsMemory.get("ModelDownloadProgress"));
              bool downloadComplete = progress.contains(QRegularExpression("downloaded", QRegularExpression::CaseInsensitiveOption));
              bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|failed|offline", QRegularExpression::CaseInsensitiveOption));

              if (!progress.isEmpty() && progress != "0%") {
                downloadModelBtn->setValue(progress);
              }

              if (downloadComplete || downloadFailed) {
                bool lastModelDownloaded = downloadComplete;

                if (downloadComplete) {
                  haveModelsDownloaded = true;
                  update();
                }

                if (downloadComplete) {
                  for (const QString &model : availableModels) {
                    if (!QFile::exists(modelDir.filePath(model + ".thneed"))) {
                      lastModelDownloaded = false;
                      break;
                    }
                  }
                }

                downloadModelBtn->setValue(progress);

                paramsMemory.remove("CancelModelDownload");
                paramsMemory.remove("ModelDownloadProgress");

                progressTimer->stop();
                progressTimer->deleteLater();

                QTimer::singleShot(2000, this, [=]() {
                  cancellingDownload = false;
                  modelDownloading = false;

                  downloadModelBtn->setValue("");

                  if (lastModelDownloaded) {
                    modelsDownloaded = true;
                    update();

                    params.putBoolNonBlocking("ModelsDownloaded", modelsDownloaded);
                  }

                  device()->resetInteractiveTimeout(30);
                });
              }
            });
            progressTimer->start();
          }
        }
      });
      advancedDrivingToggle = reinterpret_cast<AbstractControl*>(downloadModelBtn);
    } else if (param == "DownloadAllModels") {
      downloadAllModelsBtn = new ButtonControl(title, tr("下載"), desc);
      QObject::connect(downloadAllModelsBtn, &ButtonControl::clicked, [this]() {
        if (downloadAllModelsBtn->text() == tr("取消")) {
          paramsMemory.remove("DownloadAllModels");
          paramsMemory.putBool("CancelModelDownload", true);
          cancellingDownload = true;

          device()->resetInteractiveTimeout(30);
        } else {
          device()->resetInteractiveTimeout(300);

          startDownloadAllModels();
        }
      });
      advancedDrivingToggle = reinterpret_cast<AbstractControl*>(downloadAllModelsBtn);
    } else if (param == "SelectModel") {
      selectModelBtn = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(selectModelBtn, &ButtonControl::clicked, [this]() {
        QSet<QString> modelFilesBaseNames = QSet<QString>::fromList(modelDir.entryList({"*.thneed"}, QDir::Files).replaceInStrings(QRegExp("\\.thneed$"), ""));
        QStringList selectableModels;

        for (int i = 0; i < availableModels.size(); ++i) {
          if (modelFilesBaseNames.contains(availableModels[i]) || availableModelNames[i].contains("(Default)")) {
            selectableModels.append(availableModelNames[i]);
          }
        }

        QString modelToSelect = MultiOptionDialog::getSelection(tr("選擇型號 - 🗺️ = Navigation | 📡 = Radar | 👀 = VOACC"), selectableModels, "", this);
        if (!modelToSelect.isEmpty()) {
          selectModelBtn->setValue(modelToSelect);
          int modelIndex = availableModelNames.indexOf(modelToSelect);

          params.putNonBlocking("Model", availableModels.at(modelIndex).toStdString());
          params.putNonBlocking("ModelName", modelToSelect.toStdString());

          if (experimentalModels.contains(availableModels.at(modelIndex))) {
            FrogPilotConfirmationDialog::toggleAlert(tr("警告：這是一個非常實驗性的模型，可能會導致危險駕駛!"), tr("我了解風險."), this);
          }

          QString model = availableModelNames.at(modelIndex);
          QString part_model_param = processModelName(model);

          if (!params.checkKey(part_model_param.toStdString() + "CalibrationParams") || !params.checkKey(part_model_param.toStdString() + "LiveTorqueParameters")) {
            if (FrogPilotConfirmationDialog::yesorno(tr("對新選擇的模型進行全新校準?"), this)) {
              params.remove("CalibrationParams");
              params.remove("LiveTorqueParameters");
            }
          }

          if (started) {
            if (FrogPilotConfirmationDialog::toggle(tr("需要重新啟動才能生效."), tr("馬上重啟"), this)) {
              Hardware::reboot();
            }
          }
        }
      });
      selectModelBtn->setValue(QString::fromStdString(params.get("ModelName")));
      advancedDrivingToggle = reinterpret_cast<AbstractControl*>(selectModelBtn);
    } else if (param == "ResetCalibrations") {
      FrogPilotButtonsControl *resetCalibrationsBtn = new FrogPilotButtonsControl(title, desc, {tr("全部重置"), tr("重置選擇")});
      QObject::connect(resetCalibrationsBtn, &FrogPilotButtonsControl::showDescriptionEvent, this, &FrogPilotAdvancedDrivingPanel::updateCalibrationDescription);
      QObject::connect(resetCalibrationsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        if (id == 0) {
          if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重設所有模型校準嗎?"), this)) {
            for (const QString &model : availableModelNames) {
              QString cleanedModel = processModelName(model);
              params.remove(QString("%1CalibrationParams").arg(cleanedModel).toStdString());
              paramsStorage.remove(QString("%1CalibrationParams").arg(cleanedModel).toStdString());
              params.remove(QString("%1LiveTorqueParameters").arg(cleanedModel).toStdString());
              paramsStorage.remove(QString("%1LiveTorqueParameters").arg(cleanedModel).toStdString());
            }
          }
        } else if (id == 1) {
          QStringList selectableModelLabels;
          for (int i = 0; i < availableModels.size(); ++i) {
            selectableModelLabels.append(availableModelNames[i]);
          }

          QString modelToReset = MultiOptionDialog::getSelection(tr("選擇要重置的型號"), selectableModelLabels, "", this);
          if (!modelToReset.isEmpty()) {
            if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重設該模型的校準嗎?"), this)) {
              QString cleanedModel = processModelName(modelToReset);
              params.remove(QString("%1CalibrationParams").arg(cleanedModel).toStdString());
              paramsStorage.remove(QString("%1CalibrationParams").arg(cleanedModel).toStdString());
              params.remove(QString("%1LiveTorqueParameters").arg(cleanedModel).toStdString());
              paramsStorage.remove(QString("%1LiveTorqueParameters").arg(cleanedModel).toStdString());
            }
          }
        }
      });
      advancedDrivingToggle = resetCalibrationsBtn;

    } else {
      advancedDrivingToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(advancedDrivingToggle);
    toggles[param] = advancedDrivingToggle;

    makeConnections(advancedDrivingToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(advancedDrivingToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotAdvancedDrivingPanel::openParentToggle);
    }

    QObject::connect(advancedDrivingToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["ModelManagement"]), &ToggleControl::toggleFlipped, [this](bool state) {
    modelManagement = state;
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["ModelRandomizer"]), &ToggleControl::toggleFlipped, [this](bool state) {
    modelRandomizer = state;
    if (state && !modelsDownloaded) {
      if (FrogPilotConfirmationDialog::yesorno(tr("「模型隨機產生器」僅適用於下載的模型。您想下載所有駕駛模型嗎?"), this)) {
        startDownloadAllModels();
      }
    }
  });

  steerFrictionToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerFriction"]);
  QObject::connect(steerFrictionToggle, &FrogPilotParamValueButtonControl::buttonClicked, [this]() {
    params.putFloat("SteerFriction", steerFrictionStock);
    steerFrictionToggle->refresh();
    updateFrogPilotToggles();
  });

  steerKPToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerKP"]);
  QObject::connect(steerKPToggle, &FrogPilotParamValueButtonControl::buttonClicked, [this]() {
    params.putFloat("SteerKP", steerKPStock);
    steerKPToggle->refresh();
    updateFrogPilotToggles();
  });

  steerLatAccelToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerLatAccel"]);
  QObject::connect(steerLatAccelToggle, &FrogPilotParamValueButtonControl::buttonClicked, [this]() {
    params.putFloat("SteerLatAccel", steerLatAccelStock);
    steerLatAccelToggle->refresh();
    updateFrogPilotToggles();
  });

  steerRatioToggle = static_cast<FrogPilotParamValueButtonControl*>(toggles["SteerRatio"]);
  QObject::connect(steerRatioToggle, &FrogPilotParamValueButtonControl::buttonClicked, [this]() {
    params.putFloat("SteerRatio", steerRatioStock);
    steerRatioToggle->refresh();
    updateFrogPilotToggles();
  });

  FrogPilotParamValueControl *trafficFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficFollow"]);
  FrogPilotParamValueControl *trafficAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkAcceleration"]);
  FrogPilotParamValueControl *trafficDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkDeceleration"]);
  FrogPilotParamValueControl *trafficDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkDanger"]);
  FrogPilotParamValueControl *trafficSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkSpeed"]);
  FrogPilotParamValueControl *trafficSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkSpeedDecrease"]);
  FrogPilotButtonsControl *trafficResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetTrafficPersonality"]);
  QObject::connect(trafficResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「塞車模式」個性設定嗎?"), this)) {
      params.putFloat("TrafficFollow", 0.5);
      params.putFloat("TrafficJerkAcceleration", 50);
      params.putFloat("TrafficJerkDeceleration", 50);
      params.putFloat("TrafficJerkDanger", 100);
      params.putFloat("TrafficJerkSpeed", 50);
      params.putFloat("TrafficJerkSpeedDecrease", 50);
      trafficFollowToggle->refresh();
      trafficAccelerationToggle->refresh();
      trafficDecelerationToggle->refresh();
      trafficDangerToggle->refresh();
      trafficSpeedToggle->refresh();
      trafficSpeedDecreaseToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *aggressiveFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveFollow"]);
  FrogPilotParamValueControl *aggressiveAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkAcceleration"]);
  FrogPilotParamValueControl *aggressiveDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkDeceleration"]);
  FrogPilotParamValueControl *aggressiveDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkDanger"]);
  FrogPilotParamValueControl *aggressiveSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkSpeed"]);
  FrogPilotParamValueControl *aggressiveSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkSpeedDecrease"]);
  FrogPilotButtonsControl *aggressiveResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetAggressivePersonality"]);
  QObject::connect(aggressiveResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「積極駕駛」個性的設定嗎?"), this)) {
      params.putFloat("AggressiveFollow", 1.25);
      params.putFloat("AggressiveJerkAcceleration", 50);
      params.putFloat("AggressiveJerkDeceleration", 50);
      params.putFloat("AggressiveJerkDanger", 100);
      params.putFloat("AggressiveJerkSpeed", 50);
      params.putFloat("AggressiveJerkSpeedDecrease", 50);
      aggressiveFollowToggle->refresh();
      aggressiveAccelerationToggle->refresh();
      aggressiveDecelerationToggle->refresh();
      aggressiveDangerToggle->refresh();
      aggressiveSpeedToggle->refresh();
      aggressiveSpeedDecreaseToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *standardFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardFollow"]);
  FrogPilotParamValueControl *standardAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkAcceleration"]);
  FrogPilotParamValueControl *standardDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkDeceleration"]);
  FrogPilotParamValueControl *standardDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkDanger"]);
  FrogPilotParamValueControl *standardSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkSpeed"]);
  FrogPilotParamValueControl *standardSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkSpeedDecrease"]);
  FrogPilotButtonsControl *standardResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetStandardPersonality"]);
  QObject::connect(standardResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置“標準”個性設定嗎?"), this)) {
      params.putFloat("StandardFollow", 1.45);
      params.putFloat("StandardJerkAcceleration", 100);
      params.putFloat("StandardJerkDeceleration", 100);
      params.putFloat("StandardJerkDanger", 100);
      params.putFloat("StandardJerkSpeed", 100);
      params.putFloat("StandardJerkSpeedDecrease", 100);
      standardFollowToggle->refresh();
      standardAccelerationToggle->refresh();
      standardDecelerationToggle->refresh();
      standardDangerToggle->refresh();
      standardSpeedToggle->refresh();
      standardSpeedDecreaseToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *relaxedFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedFollow"]);
  FrogPilotParamValueControl *relaxedAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkAcceleration"]);
  FrogPilotParamValueControl *relaxedDecelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkDeceleration"]);
  FrogPilotParamValueControl *relaxedDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkDanger"]);
  FrogPilotParamValueControl *relaxedSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkSpeed"]);
  FrogPilotParamValueControl *relaxedSpeedDecreaseToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkSpeedDecrease"]);
  FrogPilotButtonsControl *relaxedResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetRelaxedPersonality"]);
  QObject::connect(relaxedResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「輕鬆」個性的設定嗎?"), this)) {
      params.putFloat("RelaxedFollow", 1.75);
      params.putFloat("RelaxedJerkAcceleration", 100);
      params.putFloat("RelaxedJerkDeceleration", 100);
      params.putFloat("RelaxedJerkDanger", 100);
      params.putFloat("RelaxedJerkSpeed", 100);
      params.putFloat("RelaxedJerkSpeedDecrease", 100);
      relaxedFollowToggle->refresh();
      relaxedAccelerationToggle->refresh();
      relaxedDecelerationToggle->refresh();
      relaxedDangerToggle->refresh();
      relaxedSpeedToggle->refresh();
      relaxedSpeedDecreaseToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotAdvancedDrivingPanel::hideToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubParentToggle, this, &FrogPilotAdvancedDrivingPanel::hideSubToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubSubParentToggle, this, &FrogPilotAdvancedDrivingPanel::hideSubSubToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateCarToggles, this, &FrogPilotAdvancedDrivingPanel::updateCarToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotAdvancedDrivingPanel::updateMetric);
  QObject::connect(uiState(), &UIState::driveRated, this, &FrogPilotAdvancedDrivingPanel::updateModelLabels);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotAdvancedDrivingPanel::updateState);

  updateMetric();
  updateModelLabels();
}

void FrogPilotAdvancedDrivingPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double speedConversion = isMetric ? MILE_TO_KM : KM_TO_MILE;

    params.putFloatNonBlocking("SetSpeedOffset", params.getFloat("SetSpeedOffset") * speedConversion);
  }

  FrogPilotParamValueControl *setSpeedOffsetToggle = static_cast<FrogPilotParamValueControl*>(toggles["SetSpeedOffset"]);

  if (isMetric) {
    setSpeedOffsetToggle->updateControl(0, 150, tr("公里/小時"));
  } else {
    setSpeedOffsetToggle->updateControl(0, 99, tr("英里/小時"));
  }
}

void FrogPilotAdvancedDrivingPanel::showEvent(QShowEvent *event) {
  modelManagement = params.getBool("ModelManagement");
  modelRandomizer = params.getBool("ModelRandomizer");
}

void FrogPilotAdvancedDrivingPanel::updateCarToggles() {
  disableOpenpilotLongitudinal = parent->disableOpenpilotLongitudinal;
  hasAutoTune = parent->hasAutoTune;
  hasNNFFLog = parent->hasNNFFLog;
  hasOpenpilotLongitudinal = parent->hasOpenpilotLongitudinal;
  hasPCMCruise = parent->hasPCMCruise;
  liveValid = parent->liveValid;
  steerFrictionStock = parent->steerFrictionStock;
  steerKPStock = parent->steerKPStock;
  steerLatAccelStock = parent->steerLatAccelStock;
  steerRatioStock = parent->steerRatioStock;

  steerFrictionToggle->setTitle(QString(tr("Friction (Default: %1)")).arg(QString::number(steerFrictionStock, 'f', 2)));
  steerKPToggle->setTitle(QString(tr("Kp Factor (Default: %1)")).arg(QString::number(steerKPStock, 'f', 2)));
  steerKPToggle->updateControl(steerKPStock * 0.50, steerKPStock * 1.50);
  steerLatAccelToggle->setTitle(QString(tr("橫向加速 (Default: %1)")).arg(QString::number(steerLatAccelStock, 'f', 2)));
  steerLatAccelToggle->updateControl(steerLatAccelStock * 0.75, steerLatAccelStock * 1.25);
  steerRatioToggle->setTitle(QString(tr("轉向比 (Default: %1)")).arg(QString::number(steerRatioStock, 'f', 2)));
  steerRatioToggle->updateControl(steerRatioStock * 0.75, steerRatioStock * 1.25);

  hideToggles();
}

void FrogPilotAdvancedDrivingPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  if (modelManagementOpen) {
    downloadAllModelsBtn->setText(modelDownloading && allModelsDownloading ? tr("取消") : tr("下載"));
    downloadModelBtn->setText(modelDownloading && !allModelsDownloading ? tr("取消") : tr("下載"));

    deleteModelBtn->setEnabled(!modelDeleting && !modelDownloading);
    downloadAllModelsBtn->setEnabled(s.scene.online && !cancellingDownload && !modelDeleting && (!modelDownloading || allModelsDownloading) && !modelsDownloaded);
    downloadModelBtn->setEnabled(s.scene.online && !cancellingDownload && !modelDeleting && !allModelsDownloading && !modelsDownloaded);
    selectModelBtn->setEnabled(!modelDeleting && !modelDownloading && !modelRandomizer);
  }

  started = s.scene.started;
}

void FrogPilotAdvancedDrivingPanel::startDownloadAllModels() {
  allModelsDownloading = true;
  modelDownloading = true;

  paramsMemory.putBool("DownloadAllModels", true);

  downloadAllModelsBtn->setValue(tr("下載模型..."));

  QTimer *progressTimer = new QTimer(this);
  progressTimer->setInterval(100);

  QObject::connect(progressTimer, &QTimer::timeout, this, [=]() {
    QString progress = QString::fromStdString(paramsMemory.get("ModelDownloadProgress"));
    bool downloadComplete = progress.contains(QRegularExpression("All models downloaded!", QRegularExpression::CaseInsensitiveOption));
    bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|failed|offline", QRegularExpression::CaseInsensitiveOption));

    if (!progress.isEmpty() && progress != "0%") {
      downloadAllModelsBtn->setValue(progress);
    }

    if (downloadComplete || downloadFailed) {
      if (downloadComplete) {
        haveModelsDownloaded = true;
        update();
      }

      downloadAllModelsBtn->setValue(progress);

      paramsMemory.remove("CancelModelDownload");
      paramsMemory.remove("ModelDownloadProgress");

      progressTimer->stop();
      progressTimer->deleteLater();

      QTimer::singleShot(2000, this, [=]() {
        cancellingDownload = false;
        modelDownloading = false;

        paramsMemory.remove("DownloadAllModels");

        downloadAllModelsBtn->setValue("");

        device()->resetInteractiveTimeout(30);
      });
    }
  });
  progressTimer->start();
}

void FrogPilotAdvancedDrivingPanel::updateCalibrationDescription() {
  QString model = QString::fromStdString(params.get("ModelName"));
  QString part_model_param = processModelName(model);

  QString desc =
      tr("openpilot 要求設備安裝在左側或右側 4° 以內，並且“「向上 5° 或向下 9° 以內."
          "openpilot 持續校準，很少需要重置.");
  std::string calib_bytes = params.get(part_model_param.toStdString() + "CalibrationParams");
  if (!calib_bytes.empty()) {
    try {
      AlignedBuffer aligned_buf;
      capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
      auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
      if (calib.getCalStatus() != cereal::LiveCalibrationData::Status::UNCALIBRATED) {
        double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
        double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
        desc += tr(" 您的裝置已指向 %1° %2 和 %3° %4.")
                    .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? tr("下") : tr("上"),
                         QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? tr("左") : tr("右"));
      }
    } catch (kj::Exception) {
      qInfo() << "invalid CalibrationParams";
    }
  }
  qobject_cast<FrogPilotButtonsControl *>(sender())->setDescription(desc);
}

void FrogPilotAdvancedDrivingPanel::updateModelLabels() {
  QVector<QPair<QString, int>> modelScores;
  availableModelNames = QString::fromStdString(params.get("AvailableModelsNames")).split(",");

  for (const QString &model : availableModelNames) {
    QString cleanedModel = processModelName(model);
    int score = params.getInt((cleanedModel + "Score").toStdString());

    if (model.contains("(Default)")) {
      modelScores.prepend(qMakePair(model, score));
    } else {
      modelScores.append(qMakePair(model, score));
    }
  }

  labelControls.clear();

  for (const auto &pair : modelScores) {
    QString scoreDisplay = pair.second == 0 ? "N/A" : QString::number(pair.second) + "%";
    LabelControl *labelControl = new LabelControl(pair.first, scoreDisplay, "");
    addItem(labelControl);
    labelControls.append(labelControl);
  }

  for (LabelControl *label : labelControls) {
    label->setVisible(false);
  }
}

void FrogPilotAdvancedDrivingPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotAdvancedDrivingPanel::hideToggles() {
  setUpdatesEnabled(false);

  customPersonalityOpen = false;
  modelManagementOpen = false;

  for (LabelControl *label : labelControls) {
    label->setVisible(false);
  }

  std::set<QString> longitudinalKeys = {"AdvancedLongitudinalTune", "CustomPersonalities"};
  for (auto &[key, toggle] : toggles) {
    bool subToggles = aggressivePersonalityKeys.find(key) != aggressivePersonalityKeys.end() ||
                      customDrivingPersonalityKeys.find(key) != customDrivingPersonalityKeys.end() ||
                      lateralTuneKeys.find(key) != lateralTuneKeys.end() ||
                      longitudinalTuneKeys.find(key) != longitudinalTuneKeys.end() ||
                      modelManagementKeys.find(key) != modelManagementKeys.end() ||
                      modelRandomizerKeys.find(key) != modelRandomizerKeys.end() ||
                      qolKeys.find(key) != qolKeys.end() ||
                      relaxedPersonalityKeys.find(key) != relaxedPersonalityKeys.end() ||
                      standardPersonalityKeys.find(key) != standardPersonalityKeys.end() ||
                      trafficPersonalityKeys.find(key) != trafficPersonalityKeys.end();

    if (disableOpenpilotLongitudinal || !hasOpenpilotLongitudinal) {
      if (longitudinalKeys.find(key) != longitudinalKeys.end()) {
        toggle->setVisible(false);
        continue;
      }
    }

    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotAdvancedDrivingPanel::hideSubToggles() {
  if (customPersonalityOpen) {
    customPersonalityOpen = false;
    showToggles(customDrivingPersonalityKeys);
  } else if (modelManagementOpen) {
    for (LabelControl *label : labelControls) {
      label->setVisible(false);
    }
    showToggles(modelManagementKeys);
  }
}

void FrogPilotAdvancedDrivingPanel::hideSubSubToggles() {
  if (modelManagementOpen) {
    for (LabelControl *label : labelControls) {
      label->setVisible(false);
    }
    showToggles(modelRandomizerKeys);
  }
}
