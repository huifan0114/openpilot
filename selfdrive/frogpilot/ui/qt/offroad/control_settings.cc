#include <filesystem>
#include <iostream>

#include "selfdrive/frogpilot/ui/qt/offroad/control_settings.h"

bool checkCommaNNFFSupport(const std::string &carFingerprint) {
  std::ifstream file("../car/torque_data/neural_ff_weights.json");
  for (std::string line; std::getline(file, line);) {
    if (line.find(carFingerprint) != std::string::npos) {
      std::cout << "comma's NNFF supports fingerprint: " << carFingerprint << std::endl;
      return true;
    }
  }
  return false;
}

bool checkNNFFLogFileExists(const std::string &carFingerprint) {
  for (const auto &entry : std::filesystem::directory_iterator("../car/torque_data/lat_models")) {
    if (entry.path().filename().string().find(carFingerprint) == 0) {
      std::cout << "NNFF supports fingerprint: " << entry.path().filename() << std::endl;
      return true;
    }
  }
  return false;
}

FrogPilotControlsPanel::FrogPilotControlsPanel(SettingsWindow *parent) : FrogPilotListWidget(parent) {
  std::string branch = params.get("GitBranch");
  isRelease = branch == "FrogPilot";

  const std::vector<std::tuple<QString, QString, QString, QString>> controlToggles {
    {"AlwaysOnLateral", tr("全時置中模式"), tr("使用剎車或油門踏板時仍保持橫向控制.只有停用“定速”後才能解除."), "../frogpilot/assets/toggle_icons/icon_always_on_lateral.png"},
    {"AlwaysOnLateralLKAS", tr("透過 LKAS 按鈕控制"), tr("點擊“LKAS”按鈕啟用或停用“全時置中”."), ""},
    {"AlwaysOnLateralMain", tr("及時啟用置中模式"), tr("只需打開“巡航控制”即可啟用“全時置中”."), ""},
    {"PauseAOLOnBrake", tr("踩煞車暫停"), tr("當踩下煞車踏板低於設定速度時暫停“全時置中”."), ""},
    {"HideAOLStatusBar", tr("隱藏狀態列"), tr("不要使用「始終橫向顯示」的狀態列'."), ""},

    {"ConditionalExperimental", tr("條件式的實驗模式"), tr("根據特定條件自動啟動實驗模式."), "../frogpilot/assets/toggle_icons/icon_conditional.png"},
    {"CESpeed", tr("  時速低於"), tr("當無前車時，低於此速度切換到“實驗模式”."), ""},
    {"CECurves", tr("  彎道"), tr("偵測到曲線時切換到“實驗模式”."), ""},
    {"CELead", tr("  前車"), tr("當偵測到前方車輛時切換到“實驗模式”."), ""},
    {"CENavigation", tr("  導航"), tr("根據導航十字路口、停車標誌等切換到“實驗模式"), ""},
    {"CEStopLights", tr("  停止標誌"), tr("當偵測到停車燈或停車標誌時切換到“實驗模式”."), ""},
    {"CESignal", tr("  方向燈"), tr("在低於高速公路速度時使用方向燈以幫助轉彎時切換到“實驗模式”."), ""},
    {"HideCEMStatusBar", tr("  隱藏狀態列"), tr("不要將狀態列用於“條件實驗模式”."), ""},

    {"DeviceManagement", tr("設備管理"), tr("根據您的個人喜好調整設備的行為."), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"DeviceShutdown", tr("  設備自動關機設定"), tr("設置設備在熄火後自動關閉的時間，以減少能源浪費並防止電池耗盡."), ""},
    {"NoLogging", tr("  停用日誌記錄"), tr("關閉所有數據追蹤以增強隱私或減少熱負荷."), ""},
    {"NoUploads", tr("  關閉上傳"), tr("關閉資料上傳comma伺服器."), ""},
    {"IncreaseThermalLimits", tr("  提高熱安全極限"), tr("允許設備在高於 comma 建議的熱限制的溫度下運行."), ""},
    {"LowVoltageShutdown", tr("  低電壓關斷閾值"), tr("當電池達到特定電壓等級時自動關閉設備，以防止電池耗盡."), ""},
    {"OfflineMode", tr("  離線模式"), tr("允許設備無限期離線."), ""},

    {"DrivingPersonalities", tr("設定駕駛模式"), tr("管理個人的駕駛行為'."), "../frogpilot/assets/toggle_icons/icon_personality.png"},
    {"CustomPersonalities", tr("客製化設定"), tr("根據您的駕駛風格客製化駕駛個性檔案."), ""},
    {"PersonalityInfo", tr("這些的作用"), tr("了解「自訂個性設定檔」中的所有數值對 openpilot 的駕駛行為有何影響."), ""}, 
    {"TrafficPersonalityProfile", tr("塞車模式"), tr("設定塞車模式行為."), "../frogpilot/assets/other_images/traffic.png"},
    {"TrafficFollow", tr("跟隨距離"), tr("設定使用「塞車模式」時的最小跟隨距離。當在 0 到 %1 之間行駛時，您的跟隨距離將在此距離和「激進」設定檔中的跟隨距離之間動態調整.\n\n例如:\n\n塞車模式: 0.5s\n積極模式: 1.0s\n\n0%2 = 0.5s\n%3 = 0.75s\n%1 = 1.0s"), ""},
    {"TrafficJerkAcceleration", tr("加速/減速度 反應調整"), tr("自訂使用「塞車模式」時的加速反應."), ""},
    {"TrafficJerkDanger", tr("危險區域  反應調整"), tr("使用「塞車模式」時自訂危險區域因素的反應."), ""},
    {"TrafficJerkSpeed", tr("速度控制 反應調整"), tr("自訂使用「塞車模式」時保持速度（包括煞車）的反應率."), ""},
    {"ResetTrafficPersonality", tr("重設塞車模式設定"), tr("將「塞車模式」設定值重設為預設."), ""},
    {"AggressivePersonalityProfile", tr("積極模式"), tr("設定積極模式行為."), "../frogpilot/assets/other_images/aggressive.png"},
    {"AggressiveFollow", tr("跟隨距離"), tr("設定「積極模式」的跟隨距離。代表跟隨前車的秒數.\n\n預設: 1.25 秒."), ""},
    {"AggressiveJerkAcceleration", tr("加速/減速度 反應調整"), tr("自訂使用「積極模式」時的加速反應."), ""},
    {"AggressiveJerkDanger", tr("危險區域  反應調整"), tr("使用「積極模式」時自訂危險區域因素的反應."), ""},
    {"AggressiveJerkSpeed", tr("速度控制 反應調整"), tr("自訂使用「積極模式」個性時保持速度（包括煞車）的反應率."), ""},
    {"ResetAggressivePersonality", tr("重設積極模式設定"), tr("將「積極模式」設定值重設為預設."), ""},
    {"StandardPersonalityProfile", tr("標準模式"), tr("設定標準模式行為."), "../frogpilot/assets/other_images/standard.png"},
    {"StandardFollow", tr("跟隨距離"), tr("設定「標準模式」的跟隨距離。代表跟隨前車的秒數.\n\n預設: 1.45 秒."), ""},
    {"StandardJerkAcceleration", tr("加速/減速度 反應調整"), tr("自訂使用「標準模式」時的加速反應."), ""},
    {"StandardJerkDanger", tr("危險區域  反應調整"), tr("使用「標準模式」時自訂危險區域因素的反應."), ""},
    {"StandardJerkSpeed", tr("速度控制 反應調整"), tr("自訂使用「積極模式」個性時保持速度（包括煞車）的反應率."), ""},
    {"ResetStandardPersonality", tr("重設標準模式設定"), tr("將「標準模式」設定值重設為預設."), ""},
    {"RelaxedPersonalityProfile", tr("放鬆模式"), tr("設定放鬆模式行為."), "../frogpilot/assets/other_images/relaxed.png"},
    {"RelaxedFollow", tr("跟隨距離"), tr("設定「放鬆模式」的跟隨距離。代表跟隨前車的秒數.\n\n預設: 1.75 秒."), ""},
    {"RelaxedJerkAcceleration", tr("加速/減速度 反應調整"), tr("自訂使用「放鬆模式」時的加速反應."), ""},
    {"RelaxedJerkDanger", tr("危險區域  反應調整"), tr("使用「輕鬆模式」時自訂危險區域因素的反應."), ""},
    {"RelaxedJerkSpeed", tr("速度控制 反應調整"), tr("自訂使用「積極模式」個性時保持速度（包括煞車）的反應率."), ""},
    {"ResetRelaxedPersonality", tr("重設放鬆模式設定"), tr("將「放鬆模式」設定值重設為預設."), ""},
    {"OnroadDistanceButton", tr("公路距離按鈕"), tr("透過道路 UI 模擬距離按鈕來控制個性、“實驗模式”和“交通模式”."), ""},

    {"ExperimentalModeActivation", tr("開啟實驗模式方式"), tr("通過雙擊方向盤上的“車道偏離”/LKAS 按鈕(Toyota/Lexus Only)以啟用或禁用實驗模式，或雙擊營幕覆蓋“條件實驗模式”'."), "../assets/img_experimental_white.svg"},
    {"ExperimentalModeViaLKAS", tr("  雙擊 LKAS 按鈕"), tr("雙擊方向盤上的“LKAS”按鈕啟用/停用“實驗模式”."), ""},
    {"ExperimentalModeViaTap", tr("  按兩下螢幕UI"), tr("透過在 0.5 秒的時間範圍內雙擊道路 UI 來啟用/停用“實驗模式”."), ""},
    {"ExperimentalModeViaDistance", tr("  長按距離按鈕"), tr("按住方向盤上的「距離」按鈕 0.5 秒，啟用/停用「實驗模式」."), ""},

    {"LaneChangeCustomizations", tr("變換車道設定"), tr("在 openpilot 中自訂變換車道行為."), "../frogpilot/assets/toggle_icons/icon_lane.png"},
    {"LaneChangeTime", tr("  自動變換車道延遲"), tr("設定自動變換車道延遲時間."), ""},
    {"LaneDetectionWidth", tr("  車道檢測"), tr("設定符合車道要求的車道寬度."), ""},
    {"MinimumLaneChangeSpeed", tr("  最小變換車道速度"), tr("自訂允許 openpilot 變換車道的最低行駛速度."), ""},
    {"NudgelessLaneChange", tr("  無助變換車道"), tr("無需手動轉向輸入即可實現車道變換."), ""},
    {"OneLaneChange", tr("  每次只變換一個車道"), tr("每次啟動方向燈時，僅執行一次自動變換車道."), ""},

    {"LateralTune", tr("橫向調整"), tr("改變 openpilot 的駕駛方式."), "../frogpilot/assets/toggle_icons/icon_lateral_tune.png"},
    {"ForceAutoTune", tr("強制自動控制"), tr("強制逗號對不支援的車輛進行自動橫向調整."), ""},
    {"NNFF", tr("NNFF"), tr("  使用Twilsonco's的神經網路前饋扭矩控制系統來獲得更精準的橫向控制."), ""},
    {"NNFFLite", tr("NNFF-Lite"), tr("使用 Twilsonco 的神經網路前饋提高沒有可用 NNFF 日誌的汽車橫向控制的精確度."), ""},
    {"SteerRatio", steerRatioStock != 0 ? QString(tr("Steer Ratio (Default: %1)")).arg(QString::number(steerRatioStock, 'f', 2)) : tr("  轉向比"), tr("為您的車輛控制設定自訂轉向比."), ""},
    {"TacoTune", tr("Taco Tune"), tr("使用逗號的“Taco Tune”，專為處理左轉和右轉而設計."), ""},
    {"TurnDesires", tr("轉彎預測"), tr("在低於最小變換車道速度的情況下使用轉彎期望以獲得更高的轉彎精度."), ""},

    {"LongitudinalTune", tr("縱向調整"), tr("改變 openpilot 加速和煞車方式."), "../frogpilot/assets/toggle_icons/icon_longitudinal_tune.png"},
    {"AccelerationProfile", tr("  加速曲線"), tr("將加速度改為運動型或環保型."), ""},
    {"DecelerationProfile", tr("  減速曲線"), tr("將減速度改為運動型或環保型."), ""},
    {"AggressiveAcceleration", tr("  積極跟車"), tr("當有前車可跟隨時起步更加積極的加速."), ""},
    {"StoppingDistance", tr("  增加跟車距離"), tr("增加停車距離，讓停車更舒適."), ""},
    {"LeadDetectionThreshold", tr("  前車偵測敏感度"), tr("增加或減少前車偵測敏感度，以更快地偵測到車輛，或提高模型置信度."), ""},
    {"SmoothBraking", tr("  平穩煞車"), tr("當接近速度較慢的車輛時，煞車行為更加自然."), ""},
    {"TrafficMode", tr("  塞車模式"), tr("按住「距離」按鈕 2.5 秒，可根據走走停停的交通狀況啟用更激進的駕駛行為."), ""},

    {"MTSCEnabled", tr("地圖彎道速度控制"), tr("根據下載地圖偵測到的預期曲線放慢速度."), "../frogpilot/assets/toggle_icons/icon_speed_map.png"},
    {"DisableMTSCSmoothing", tr("  禁用 MTSC 調整"), tr("在道路使用者介面中禁用速度的平滑調整."), ""},
    {"MTSCCurvatureCheck",  tr("  模型曲率檢測故障保護"), tr("僅當模型偵測到道路上有彎道時才觸發 MTSC。純粹用作故障保護以防止誤報。如果您從未遇到過誤報，請關閉此選項."), ""},
    {"MTSCAggressiveness", tr("  轉彎速度積極性"), tr("設定轉彎速度攻擊性.較高的數值會導致較快的轉彎，較低的數值會導致較平緩的轉彎."), ""},

    {"ModelManagement", tr("模型管理"), tr("管理 openpilot 的駕駛模型."), "../assets/offroad/icon_calibration.png"},
    {"AutomaticallyUpdateModels", tr("自動更新模型"), tr("當模型更新或新增到模型清單時自動下載模型."), ""},
    {"ModelRandomizer", tr("模型隨機產生器"), tr("為每個駕駛選擇一個隨機型號，可以在每個駕駛結束時進行查看以找到您喜歡的型號."), ""},
    {"ManageBlacklistedModels", tr("管理模型黑名單"), tr("管理黑名單上的型號."), ""},
    {"ResetScores", tr("重置模型分數"), tr("重置您對 openpilot 模型的評分."), ""},
    {"ReviewScores", tr("查看模型分數"), tr("查看 FrogPilot 和您自己對 openpilot 模型的評分."), ""},
    {"DeleteModel", tr("刪除模型"), "", ""},
    {"DownloadModel", tr("下載模型"), "", ""},
    {"DownloadAllModels", tr("下載所有型號"), "", ""},
    {"SelectModel", tr("選擇型號"), "", ""},
    {"ResetCalibrations", tr("重置模型校準"), tr("重置駕駛模型校準."), ""},

    {"QOLControls", tr("優化體驗"), tr("各種控制細項的調整可改善您的openpilot體驗."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"CustomCruise", tr("  巡航增加間隔"), tr("設定自訂間隔以增加最大設定速度."), ""},
    {"CustomCruiseLong", tr("  巡航增加間隔 (長按)"), tr("設定自訂間隔，以在按住巡航增加按鈕時增加最大設定速度."), ""},
    {"ForceStandstill", tr("  強制停止狀態"), tr("保持 openpilot 處於「靜止」狀態，直到踩下油門踏板."), ""},
    {"MapGears", tr("  匹配加速/減速控制"), tr("對應你的 加速/減速 設定到 '節能' 與 '運動' 控制."), ""},
    {"PauseLateralSpeed", tr("  暫停橫向控制時速"), tr("在低於設定速度的所有速度上暫停橫向控制."), ""},
    {"ReverseCruise", tr("  增加巡航速度"), tr("反轉「長按」功能邏輯，將最大設定速度增加 5 而不是 1. 有助於快速提高最大速度."), ""},
    {"SetSpeedOffset", tr("  設定速度偏移"), tr("為您所需的設定速度設定偏移量."), ""},

    {"SpeedLimitController", tr("限速控制器"), tr("使用「開放街道地圖」、「在 openpilot 上導航」或汽車儀表板（僅限 TSS2 豐田）自動調整車速以匹配速度限制."), "../assets/offroad/icon_speed_limit.png"},
    {"SLCControls", tr("控制設定"), tr("管理控制項的設定."), ""},
    {"Offset1", tr("速限微調 (0-34 mph)"), tr("速度介於 0-34 mph 的速限微調."), ""},
    {"Offset2", tr("速限微調 (35-54 mph)"), tr("速度介於 35-54 mph 的速限微調."), ""},
    {"Offset3", tr("速限微調 (55-64 mph)"), tr("速度介於 55-64 mph 的速限微調."), ""},
    {"Offset4", tr("速限微調 (65-99 mph)"), tr("速度介於 65-99 mph 的速限微調."), ""},
    {"SLCFallback", tr("備援設定"), tr("當沒有速度限制時選擇您的後備方法."), ""},
    {"SLCOverride", tr("覆蓋方法"), tr("選擇您喜歡的方法來覆蓋當前的速度限制."), ""},
    {"SLCPriority", tr("優先順序"), tr("配置限速優先順序."), ""},
    {"SLCQOL", tr("優化控制"), tr("管理與「限速控制器」生活品質功能相關的切換."), ""},
    {"SLCConfirmation", tr("確認新的速度限制"), tr("在手動確認之前，不要自動開始使用新的速度限制."), ""},
    {"ForceMPHDashboard", tr("從儀表板讀數強制 MPH"), tr("強制從儀表板讀取 MPH 讀數。僅當您居住的區域中儀表板的速度限制以 KPH 為單位，但您使用的是 MPH 時才使用此選項."), ""},
    {"SLCLookaheadHigher", tr("為更高的速度限製做好準備"), tr("使用「開放街道地圖」中儲存的資料設定「預測」值，以便為即將到來的高於當前速度限制的速度限製做好準備."), ""},
    {"SLCLookaheadLower", tr("為較低的速度限製做好準備"), tr("使用「開放街道地圖」中儲存的資料設定「預測」值，以便為即將到來的低於當前速度限制的速度限製做好準備."), ""},
    {"SetSpeedLimit", tr("使用當前速度限製作為設定速度"), tr("如果在您最初啟用 openpilot 時已填充，則將最大速度設定為當前速度限制."), ""},
    {"SLCVisuals", tr("視覺效果設定"), tr("管理與“速度限制控制器”視覺效果相關的切換."), ""},
    {"ShowSLCOffset", tr("顯示速度限制偏移"), tr("使用「速度限制控制器」時，在道路 UI 中顯示與速度限制分開的速度限制偏移."), ""},
    {"SpeedLimitChangedAlert", tr("速度限制更改警報"), tr("每當速度限制改變時觸發警報."), ""},
    {"UseVienna", tr("使用維也納限速標誌"), tr("使用維也納（歐盟）限速樣式標誌，而不是 MUTCD（美國）."), ""},

    {"VisionTurnControl", tr("視覺轉向速度控制器"), tr("偵測到道路彎道時減速."), "../frogpilot/assets/toggle_icons/icon_vtc.png"},
    {"DisableVTSCSmoothing", tr("禁用 VTSC UI 平滑"), tr("在道路使用者介面中關閉速度平滑切換模式."), ""},
    {"CurveSensitivity", tr("曲線檢測靈敏度"), tr("設定曲線檢測靈敏度。較高的值會導致較早的反應，較低的值會導致較平滑但較晚的反應."), ""},
    {"TurnAggressiveness", tr("轉彎速度積極性"), tr("設定轉彎速度積極性。較高的數值會導致較快的轉彎，較低的數值會導致較平緩的轉彎."), ""},
  };

  for (const auto &[param, title, desc, icon] : controlToggles) {
    AbstractControl *controlToggle;

    if (param == "AlwaysOnLateral") {
      FrogPilotParamManageControl *aolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(aolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(aolKeys.find(key.c_str()) != aolKeys.end());
        }
        openParentToggle();
      });
      controlToggle = aolToggle;
    } else if (param == "PauseAOLOnBrake") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr("mph"));

    } else if (param == "ConditionalExperimental") {
      FrogPilotParamManageControl *conditionalExperimentalToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(conditionalExperimentalToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(conditionalExperimentalKeys.find(key.c_str()) != conditionalExperimentalKeys.end());
        }
        openParentToggle();
      });
      controlToggle = conditionalExperimentalToggle;
    } else if (param == "CESpeed") {
      FrogPilotParamValueControl *CESpeed = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr("英里/小時"));
      FrogPilotParamValueControl *CESpeedLead = new FrogPilotParamValueControl("CESpeedLead", tr("  有車"), tr("有前方車輛時低於此速度切換到實驗模式."), icon, 0, 99, std::map<int, QString>(), this, false, tr("英里/小時"));
      FrogPilotDualParamControl *conditionalSpeeds = new FrogPilotDualParamControl(CESpeed, CESpeedLead, this);
      controlToggle = reinterpret_cast<AbstractControl*>(conditionalSpeeds);
    } else if (param == "CECurves") {
      std::vector<QString> curveToggles{"CECurvesLead"};
      std::vector<QString> curveToggleNames{tr("前車")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, curveToggles, curveToggleNames);
    } else if (param == "CELead") {
      std::vector<QString> leadToggles{"CESlowerLead", "CEStoppedLead"};
      std::vector<QString> leadToggleNames{tr("慢速車"), tr("靜止車")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, leadToggles, leadToggleNames);
    } else if (param == "CENavigation") {
      std::vector<QString> navigationToggles{"CENavigationIntersections", "CENavigationTurns", "CENavigationLead"};
      std::vector<QString> navigationToggleNames{tr("交叉口"), tr("轉彎"), tr("前車")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, navigationToggles, navigationToggleNames);
    } else if (param == "CEStopLights") {
      std::vector<QString> stopLightsToggles{"CEStopLightsLessSensitive"};
      std::vector<QString> stopLightsToggleNames{tr("降低靈敏度")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, stopLightsToggles, stopLightsToggleNames);

    } else if (param == "DeviceManagement") {
      FrogPilotParamManageControl *deviceManagementToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(deviceManagementToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(deviceManagementKeys.find(key.c_str()) != deviceManagementKeys.end());
        }
        openParentToggle();
      });
      controlToggle = deviceManagementToggle;
    } else if (param == "DeviceShutdown") {
      std::map<int, QString> shutdownLabels;
      for (int i = 0; i <= 33; ++i) {
        shutdownLabels[i] = i == 0 ? tr("5 分鐘") : i <= 3 ? QString::number(i * 15) + tr(" 分鐘") : QString::number(i - 3) + (i == 4 ? tr(" 小時") : tr(" 小時"));
      }
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 33, shutdownLabels, this, false);
    } else if (param == "NoUploads") {
      std::vector<QString> uploadsToggles{"DisableOnroadUploads"};
      std::vector<QString> uploadsToggleNames{tr("僅上路時")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, uploadsToggles, uploadsToggleNames);
    } else if (param == "LowVoltageShutdown") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 11.8, 12.5, std::map<int, QString>(), this, false, tr(" 伏特"), 1, 0.01);

    } else if (param == "DrivingPersonalities") {
      FrogPilotParamManageControl *drivingPersonalitiesToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(drivingPersonalitiesToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(drivingPersonalityKeys.find(key.c_str()) != drivingPersonalityKeys.end());
        }
        openParentToggle();
      });
      controlToggle = drivingPersonalitiesToggle;
    } else if (param == "CustomPersonalities") {
      FrogPilotParamManageControl *customPersonalitiesToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customPersonalitiesToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        customPersonalitiesOpen = true;
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(customdrivingPersonalityKeys.find(key.c_str()) != customdrivingPersonalityKeys.end());
        }
        openSubParentToggle();
      });
      controlToggle = customPersonalitiesToggle;
    } else if (param == "PersonalityInfo") {
      ButtonControl *personalitiesInfoBtn = new ButtonControl(title, tr("查看"), desc);
      QObject::connect(personalitiesInfoBtn, &ButtonControl::clicked, [=]() {
        const std::string txt = util::read_file("../frogpilot/ui/qt/offroad/personalities_info.txt");
        ConfirmationDialog::rich(QString::fromStdString(txt), this);
      });
      controlToggle = reinterpret_cast<AbstractControl*>(personalitiesInfoBtn);
    } else if (param == "ResetTrafficPersonality" || param == "ResetAggressivePersonality" || param == "ResetStandardPersonality" || param == "ResetRelaxedPersonality") {
      std::vector<QString> personalityOptions{tr("重設")};
      FrogPilotButtonsControl *profileBtn = new FrogPilotButtonsControl(title, desc, icon, personalityOptions);
      controlToggle = profileBtn;
    } else if (param == "TrafficPersonalityProfile") {
      FrogPilotParamManageControl *trafficPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(trafficPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(trafficPersonalityKeys.find(key.c_str()) != trafficPersonalityKeys.end());
        }
        openSubSubParentToggle();
      });
      controlToggle = trafficPersonalityToggle;
    } else if (param == "AggressivePersonalityProfile") {
      FrogPilotParamManageControl *aggressivePersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(aggressivePersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(aggressivePersonalityKeys.find(key.c_str()) != aggressivePersonalityKeys.end());
        }
        openSubSubParentToggle();
      });
      controlToggle = aggressivePersonalityToggle;
    } else if (param == "StandardPersonalityProfile") {
      FrogPilotParamManageControl *standardPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(standardPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(standardPersonalityKeys.find(key.c_str()) != standardPersonalityKeys.end());
        }
        openSubSubParentToggle();
      });
      controlToggle = standardPersonalityToggle;
    } else if (param == "RelaxedPersonalityProfile") {
      FrogPilotParamManageControl *relaxedPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(relaxedPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(relaxedPersonalityKeys.find(key.c_str()) != relaxedPersonalityKeys.end());
        }
        openSubSubParentToggle();
      });
      controlToggle = relaxedPersonalityToggle;
    } else if (trafficPersonalityKeys.find(param) != trafficPersonalityKeys.end() ||
               aggressivePersonalityKeys.find(param) != aggressivePersonalityKeys.end() ||
               standardPersonalityKeys.find(param) != standardPersonalityKeys.end() ||
               relaxedPersonalityKeys.find(param) != relaxedPersonalityKeys.end()) {
      if (param == "TrafficFollow" || param == "AggressiveFollow" || param == "StandardFollow" || param == "RelaxedFollow") {
        if (param == "TrafficFollow") {
          controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.5, 5, std::map<int, QString>(), this, false, tr(" 秒"), 1, 0.01);
        } else {
          controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 5, std::map<int, QString>(), this, false, tr(" 秒"), 1, 0.01);
        }
      } else {
        controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 500, std::map<int, QString>(), this, false, "%");
      }
    } else if (param == "OnroadDistanceButton") {
      std::vector<QString> onroadDistanceToggles{"KaofuiIcons"};
      std::vector<QString> onroadDistanceToggleNames{tr("Kaofui's 圖示")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, onroadDistanceToggles, onroadDistanceToggleNames);

    } else if (param == "ExperimentalModeActivation") {
      FrogPilotParamManageControl *experimentalModeActivationToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(experimentalModeActivationToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedExperimentalModeActivationKeys = experimentalModeActivationKeys;

          if (params.getBool("AlwaysOnLateralLKAS")) {
            modifiedExperimentalModeActivationKeys.erase("ExperimentalModeViaLKAS");
          }

          toggle->setVisible(modifiedExperimentalModeActivationKeys.find(key.c_str()) != modifiedExperimentalModeActivationKeys.end());
        }
        openParentToggle();
      });
      controlToggle = experimentalModeActivationToggle;

    } else if (param == "LateralTune") {
      FrogPilotParamManageControl *lateralTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(lateralTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedLateralTuneKeys = lateralTuneKeys;

          if (hasAutoTune || params.getBool("LateralTune") && params.getBool("NNFF")) {
            modifiedLateralTuneKeys.erase("ForceAutoTune");
          }

          if (hasCommaNNFFSupport || !hasNNFFLog) {
            modifiedLateralTuneKeys.erase("NNFF");
          } else {
            modifiedLateralTuneKeys.erase("NNFFLite");
          }

          toggle->setVisible(modifiedLateralTuneKeys.find(key.c_str()) != modifiedLateralTuneKeys.end());
        }
        openParentToggle();
      });
      controlToggle = lateralTuneToggle;
    } else if (param == "SteerRatio") {
      std::vector<QString> steerRatioToggles{"ResetSteerRatio"};
      std::vector<QString> steerRatioToggleNames{"重設"};
      controlToggle = new FrogPilotParamValueToggleControl(param, title, desc, icon, steerRatioStock * 0.75, steerRatioStock * 1.25, std::map<int, QString>(), this, false, "", 1, 0.01, steerRatioToggles, steerRatioToggleNames);

    } else if (param == "LongitudinalTune") {
      FrogPilotParamManageControl *longitudinalTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(longitudinalTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedLongitudinalTuneKeys = longitudinalTuneKeys;

          if (!isRelease && params.get("Model") == "radical-turtle") {
            modifiedLongitudinalTuneKeys.erase("LeadDetectionThreshold");
          }

          toggle->setVisible(modifiedLongitudinalTuneKeys.find(key.c_str()) != modifiedLongitudinalTuneKeys.end());
        }
        openParentToggle();
      });
      controlToggle = longitudinalTuneToggle;
    } else if (param == "AccelerationProfile") {
      std::vector<QString> profileOptions{tr("標準"), tr("節能"), tr("運動"), tr("超跑")};
      FrogPilotButtonParamControl *profileSelection = new FrogPilotButtonParamControl(param, title, desc, icon, profileOptions);
      controlToggle = profileSelection;
    } else if (param == "DecelerationProfile") {
      std::vector<QString> profileOptions{tr("標準"), tr("節能"), tr("運動")};
      FrogPilotButtonParamControl *profileSelection = new FrogPilotButtonParamControl(param, title, desc, icon, profileOptions);
      controlToggle = profileSelection;
    } else if (param == "StoppingDistance") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, std::map<int, QString>(), this, false, tr(" feet"));
    } else if (param == "LeadDetectionThreshold") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, std::map<int, QString>(), this, false, "%");

    } else if (param == "MTSCEnabled") {
      FrogPilotParamManageControl *mtscToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(mtscToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(mtscKeys.find(key.c_str()) != mtscKeys.end());
        }
        openParentToggle();
      });
      controlToggle = mtscToggle;
    } else if (param == "MTSCAggressiveness") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 200, std::map<int, QString>(), this, false, "%");

    } else if (param == "ModelManagement") {
      FrogPilotParamManageControl *modelManagementToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(modelManagementToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        availableModelNames = QString::fromStdString(params.get("AvailableModelsNames")).split(",");
        availableModels = QString::fromStdString(params.get("AvailableModels")).split(",");
        experimentalModels = QString::fromStdString(params.get("ExperimentalModels")).split(",");

        modelManagementOpen = true;
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(modelManagementKeys.find(key.c_str()) != modelManagementKeys.end());
        }

        std::string currentModel = params.get("Model") + ".thneed";
        QStringList modelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
        modelFiles.removeAll(QString::fromStdString(currentModel));
        haveModelsDownloaded = modelFiles.size() > 1;
        modelsDownloaded = params.getBool("ModelsDownloaded");

        openParentToggle();
      });
      controlToggle = modelManagementToggle;
    } else if (param == "ModelRandomizer") {
      FrogPilotParamManageControl *modelRandomizerToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(modelRandomizerToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(modelRandomizerKeys.find(key.c_str()) != modelRandomizerKeys.end());
        }
        openSubParentToggle();
      });
      controlToggle = modelRandomizerToggle;
    } else if (param == "ManageBlacklistedModels") {
      std::vector<QString> blacklistOptions{tr("增加"), tr("移除")};
      FrogPilotButtonsControl *manageModelsBlacklistBtn = new FrogPilotButtonsControl(title, desc, "", blacklistOptions);
      QObject::connect(manageModelsBlacklistBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QStringList blacklistedModels = QString::fromStdString(params.get("BlacklistedModels")).split(",", QString::SkipEmptyParts);
        QMap<QString, QString> labelToModelMap;
        QStringList selectableModels, deletableModels;

        for (int i = 0; i < availableModels.size(); ++i) {
          QString modelFileName = availableModels[i];
          QString readableName = availableModelNames[i];
          if (!blacklistedModels.contains(modelFileName)) {
            selectableModels.append(readableName);
          } else {
            deletableModels.append(readableName);
          }
          labelToModelMap[readableName] = modelFileName;
        }

        if (id == 0) {
          if (selectableModels.size() == 1) {
            QString onlyModel = selectableModels.first();
            FrogPilotConfirmationDialog::toggleAlert(
              tr("沒有可添加至黑名單的模型! 唯一可用的模型是 \"%1\"!").arg(onlyModel),
              tr("OK"), this);
          } else {
            QString selectedModel = MultiOptionDialog::getSelection(tr("選擇要加入黑名單的型號"), selectableModels, "", this);
            if (!selectedModel.isEmpty() && ConfirmationDialog::confirm(tr("您確定要將此型號新增至黑名單嗎?"), tr("增加"), this)) {
              QString modelToAdd = labelToModelMap[selectedModel];
              if (!blacklistedModels.contains(modelToAdd)) {
                blacklistedModels.append(modelToAdd);
                params.putNonBlocking("BlacklistedModels", blacklistedModels.join(",").toStdString());
              }
            }
          }
        } else if (id == 1) {
          QString selectedModel = MultiOptionDialog::getSelection(tr("選擇要從黑名單中刪除的型號"), deletableModels, "", this);
          if (!selectedModel.isEmpty() && ConfirmationDialog::confirm(tr("您確定要將此型號從黑名單中移除嗎?"), tr("移除"), this)) {
            QString modelToRemove = labelToModelMap[selectedModel];
            if (blacklistedModels.contains(modelToRemove)) {
              blacklistedModels.removeAll(modelToRemove);
              params.putNonBlocking("BlacklistedModels", blacklistedModels.join(",").toStdString());
              paramsStorage.put("BlacklistedModels", blacklistedModels.join(",").toStdString());
            }
          }
        }
      });
      controlToggle = reinterpret_cast<AbstractControl*>(manageModelsBlacklistBtn);
    } else if (param == "ResetScores") {
      ButtonControl *resetScoresBtn = new ButtonControl(title, tr("重置"), desc);
      QObject::connect(resetScoresBtn, &ButtonControl::clicked, [=]() {
        if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置所有模型分數嗎?"), this)) {
          for (QString model : availableModelNames) {
            QString cleanedModelName = processModelName(model);
            params.remove(QString("%1Drives").arg(cleanedModelName).toStdString());
            paramsStorage.remove(QString("%1Drives").arg(cleanedModelName).toStdString());
            params.remove(QString("%1Score").arg(cleanedModelName).toStdString());
            paramsStorage.remove(QString("%1Score").arg(cleanedModelName).toStdString());
          }

          updateModelLabels();
        }
      });
      controlToggle = reinterpret_cast<AbstractControl*>(resetScoresBtn);
    } else if (param == "ReviewScores") {
      ButtonControl *reviewScoresBtn = new ButtonControl(title, tr("查看"), desc);
      QObject::connect(reviewScoresBtn, &ButtonControl::clicked, [=]() {
        for (LabelControl *label : labelControls) {
          label->setVisible(true);
        }
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(false);
        }
        openSubSubParentToggle();
      });
      controlToggle = reinterpret_cast<AbstractControl*>(reviewScoresBtn);
    } else if (param == "DeleteModel") {
      deleteModelBtn = new ButtonControl(title, tr("刪除"), desc);
      QObject::connect(deleteModelBtn, &ButtonControl::clicked, [=]() {
        std::string currentModel = params.get("Model") + ".thneed";
        QMap<QString, QString> labelToFileMap;

        QStringList existingModelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
        QStringList deletableModelLabels;

        for (int i = 0; i < availableModels.size(); ++i) {
          QString modelFileName = availableModels[i] + ".thneed";
          if (existingModelFiles.contains(modelFileName) && modelFileName != QString::fromStdString(currentModel) && !availableModelNames[i].contains(" (Default)")) {
            deletableModelLabels.append(availableModelNames[i]);
            labelToFileMap[availableModelNames[i]] = modelFileName;
          }
        }

        QString selectedModel = MultiOptionDialog::getSelection(tr("選擇要刪除的模型"), deletableModelLabels, "", this);
        if (!selectedModel.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除該模型嗎?"), tr("刪除"), this)) {
          std::thread([=]() {
            modelDeleting = true;
            modelsDownloaded = false;
            update();

            params.putBoolNonBlocking("ModelsDownloaded", false);

            deleteModelBtn->setValue(tr("正在刪除..."));

            QFile::remove(modelDir.absoluteFilePath(labelToFileMap[selectedModel]));

            deleteModelBtn->setValue(tr("已刪除!"));

            std::this_thread::sleep_for(std::chrono::seconds(2));
            deleteModelBtn->setValue("");
            modelDeleting = false;

            std::string currentModel = params.get("Model") + ".thneed";
            QStringList modelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
            modelFiles.removeAll(QString::fromStdString(currentModel));

            haveModelsDownloaded = modelFiles.size() > 1;
            update();
          }).detach();
        }
      });
      controlToggle = reinterpret_cast<AbstractControl*>(deleteModelBtn);
    } else if (param == "DownloadModel") {
      downloadModelBtn = new ButtonControl(title, tr("下載"), desc);
      QObject::connect(downloadModelBtn, &ButtonControl::clicked, [=]() {
        if (downloadModelBtn->text() == tr("取消")) {
          paramsMemory.remove("ModelToDownload");
          paramsMemory.putBool("CancelModelDownload", true);
          cancellingDownload = true;
        } else {
          QMap<QString, QString> labelToModelMap;
          QStringList existingModelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
          QStringList downloadableModelLabels;

          for (int i = 0; i < availableModels.size(); ++i) {
            QString modelFileName = availableModels[i] + ".thneed";
            if (!existingModelFiles.contains(modelFileName) && !availableModelNames[i].contains("(Default)")) {
              downloadableModelLabels.append(availableModelNames[i]);
              labelToModelMap.insert(availableModelNames[i], availableModels[i]);
            }
          }

          QString modelToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的駕駛模型"), downloadableModelLabels, "", this);
          if (!modelToDownload.isEmpty()) {
            modelDownloading = true;
            paramsMemory.put("ModelToDownload", labelToModelMap.value(modelToDownload).toStdString());
            paramsMemory.put("ModelDownloadProgress", "0%");

            downloadModelBtn->setValue(tr("正在下載 %1...").arg(modelToDownload.remove(QRegularExpression("[🗺️👀📡]")).trimmed()));

            QTimer *progressTimer = new QTimer(this);
            progressTimer->setInterval(100);

            QObject::connect(progressTimer, &QTimer::timeout, this, [=]() {
              QString progress = QString::fromStdString(paramsMemory.get("ModelDownloadProgress"));
              bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|Failed|offline", QRegularExpression::CaseInsensitiveOption));

              if (progress != "0%") {
                downloadModelBtn->setValue(progress);
              }

              if (progress == "Downloaded!" || downloadFailed) {
                bool lastModelDownloaded = !downloadFailed;

                if (!downloadFailed) {
                  haveModelsDownloaded = true;
                  update();
                }

                if (lastModelDownloaded) {
                  for (const QString &model : availableModels) {
                    if (!QFile::exists(modelDir.filePath(model + ".thneed"))) {
                      lastModelDownloaded = false;
                      break;
                    }
                  }
                }

                downloadModelBtn->setValue(progress);

                paramsMemory.remove("ModelDownloadProgress");

                progressTimer->stop();
                progressTimer->deleteLater();

                QTimer::singleShot(2000, this, [=]() {
                  cancellingDownload = false;
                  modelDownloading = false;
                  downloadModelBtn->setValue("");
                  paramsMemory.remove("CancelModelDownload");

                  if (lastModelDownloaded) {
                    modelsDownloaded = true;
                    update();

                    params.putBoolNonBlocking("ModelsDownloaded", modelsDownloaded);
                  }
                });
              }
            });
            progressTimer->start();
          }
        }
      });
      controlToggle = reinterpret_cast<AbstractControl*>(downloadModelBtn);
    } else if (param == "DownloadAllModels") {
      downloadAllModelsBtn = new ButtonControl(title, tr("下載"), desc);
      QObject::connect(downloadAllModelsBtn, &ButtonControl::clicked, [=]() {
        if (downloadAllModelsBtn->text() == tr("取消")) {
          paramsMemory.remove("DownloadAllModels");
          paramsMemory.putBool("CancelModelDownload", true);
          cancellingDownload = true;
        } else {
          startDownloadAllModels();
        }
      });
      controlToggle = reinterpret_cast<AbstractControl*>(downloadAllModelsBtn);
    } else if (param == "SelectModel") {
      selectModelBtn = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(selectModelBtn, &ButtonControl::clicked, [=]() {
        QSet<QString> modelFilesBaseNames = QSet<QString>::fromList(
          modelDir.entryList({"*.thneed"}, QDir::Files).replaceInStrings(QRegExp("\\.thneed$"), "")
        );

        QStringList selectableModelLabels;
        for (int i = 0; i < availableModels.size(); ++i) {
          if (modelFilesBaseNames.contains(availableModels[i]) || availableModelNames[i].contains("(Default)")) {
            selectableModelLabels.append(availableModelNames[i]);
          }
        }

        QString modelToSelect = MultiOptionDialog::getSelection(tr("選擇模型 - 🗺️ = 導航 | 📡 = 雷達 | 👀 = VOACC"), selectableModelLabels, "", this);
        if (!modelToSelect.isEmpty()) {
          selectModelBtn->setValue(modelToSelect);
          int modelIndex = availableModelNames.indexOf(modelToSelect);

          params.putNonBlocking("Model", availableModels.at(modelIndex).toStdString());
          params.putNonBlocking("ModelName", modelToSelect.toStdString());

          if (experimentalModels.contains(availableModels.at(modelIndex))) {
            FrogPilotConfirmationDialog::toggleAlert(
              tr("警告：這是一個非常實驗性的模型，可能會造成危險駕駛!"),
              tr("我了解風險."), this);
          }

          QString model = availableModelNames.at(modelIndex);
          QString part_model_param = processModelName(model);

          if (!params.checkKey(part_model_param.toStdString() + "CalibrationParams") || !params.checkKey(part_model_param.toStdString() + "LiveTorqueParameters")) {
            if (FrogPilotConfirmationDialog::yesorno(tr("您想對新選擇的型號進行全新校準嗎?"), this)) {
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
      controlToggle = reinterpret_cast<AbstractControl*>(selectModelBtn);
    } else if (param == "ResetCalibrations") {
      std::vector<QString> resetOptions{tr("重設全部"), tr("重設選擇")};
      FrogPilotButtonsControl *resetCalibrationsBtn = new FrogPilotButtonsControl(title, desc, "", resetOptions);
      QObject::connect(resetCalibrationsBtn, &FrogPilotButtonsControl::showDescriptionEvent, this, &FrogPilotControlsPanel::updateCalibrationDescription);
      QObject::connect(resetCalibrationsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        if (id == 0) {
          if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重設所有模型校準嗎?"), this)) {
            for (QString model : availableModelNames) {
              QString cleanedModelName = processModelName(model);
              params.remove(QString("%1CalibrationParams").arg(cleanedModelName).toStdString());
              paramsStorage.remove(QString("%1CalibrationParams").arg(cleanedModelName).toStdString());
              params.remove(QString("%1LiveTorqueParameters").arg(cleanedModelName).toStdString());
              paramsStorage.remove(QString("%1LiveTorqueParameters").arg(cleanedModelName).toStdString());
            }
          }
        } else if (id == 1) {
          QStringList selectableModelLabels;
          for (int i = 0; i < availableModels.size(); ++i) {
            selectableModelLabels.append(availableModelNames[i]);
          }

          QString modelToReset = MultiOptionDialog::getSelection(tr("選擇要重置的模型"), selectableModelLabels, "", this);
          if (!modelToReset.isEmpty()) {
            if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重設該模型的校準嗎?"), this)) {
              QString cleanedModelName = processModelName(modelToReset);
              params.remove(QString("%1CalibrationParams").arg(cleanedModelName).toStdString());
              paramsStorage.remove(QString("%1CalibrationParams").arg(cleanedModelName).toStdString());
              params.remove(QString("%1LiveTorqueParameters").arg(cleanedModelName).toStdString());
              paramsStorage.remove(QString("%1LiveTorqueParameters").arg(cleanedModelName).toStdString());
            }
          }
        }
      });
      controlToggle = reinterpret_cast<AbstractControl*>(resetCalibrationsBtn);

    } else if (param == "QOLControls") {
      FrogPilotParamManageControl *qolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(qolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedQolKeys = qolKeys;

          if (!hasPCMCruise) {
            modifiedQolKeys.erase("ReverseCruise");
          } else {
            modifiedQolKeys.erase("CustomCruise");
            modifiedQolKeys.erase("CustomCruiseLong");
            modifiedQolKeys.erase("SetSpeedOffset");
          }

          if (!isToyota && !isGM && !isHKGCanFd) {
            modifiedQolKeys.erase("MapGears");
          }

          toggle->setVisible(modifiedQolKeys.find(key.c_str()) != modifiedQolKeys.end());
        }
        openParentToggle();
      });
      controlToggle = qolToggle;
    } else if (param == "CustomCruise") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, std::map<int, QString>(), this, false, tr("英里/小時"));
    } else if (param == "CustomCruiseLong") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, std::map<int, QString>(), this, false, tr("英里/小時"));
    } else if (param == "ForceStandstill") {
      std::vector<QString> forceStopToggles{"ForceStops"};
      std::vector<QString> forceStopToggleNames{tr("僅適用於停車燈/停車標誌")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, forceStopToggles, forceStopToggleNames);
    } else if (param == "MapGears") {
      std::vector<QString> mapGearsToggles{"MapAcceleration", "MapDeceleration"};
      std::vector<QString> mapGearsToggleNames{tr("加速"), tr("減速")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, mapGearsToggles, mapGearsToggleNames);
    } else if (param == "PauseLateralSpeed") {
      std::vector<QString> pauseLateralToggles{"PauseLateralOnSignal"};
      std::vector<QString> pauseLateralToggleNames{"Turn Signal Only"};
      controlToggle = new FrogPilotParamValueToggleControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr("英里/小時"), 1, 1, pauseLateralToggles, pauseLateralToggleNames);
    } else if (param == "PauseLateralOnSignal") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr("英里/小時"));
    } else if (param == "ReverseCruise") {
      std::vector<QString> reverseCruiseToggles{"ReverseCruiseUI"};
      std::vector<QString> reverseCruiseNames{tr("透過使用者介面控制")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, reverseCruiseToggles, reverseCruiseNames);
    } else if (param == "SetSpeedOffset") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr("英里/小時"));

    } else if (param == "LaneChangeCustomizations") {
      FrogPilotParamManageControl *laneChangeToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(laneChangeToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(laneChangeKeys.find(key.c_str()) != laneChangeKeys.end());
        }
        openParentToggle();
      });
      controlToggle = laneChangeToggle;
    } else if (param == "LaneChangeTime") {
      std::map<int, QString> laneChangeTimeLabels;
      for (int i = 0; i <= 10; ++i) {
        laneChangeTimeLabels[i] = i == 0 ? "立即的" : QString::number(i / 2.0) + " 秒";
      }
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, laneChangeTimeLabels, this, false);
    } else if (param == "LaneDetectionWidth") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, " feet", 10);
    } else if (param == "MinimumLaneChangeSpeed") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr("英里/小時"));

    } else if (param == "SpeedLimitController") {
      FrogPilotParamManageControl *speedLimitControllerToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(speedLimitControllerToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        slcOpen = true;
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(speedLimitControllerKeys.find(key.c_str()) != speedLimitControllerKeys.end());
        }
        openParentToggle();
      });
      controlToggle = speedLimitControllerToggle;
    } else if (param == "SLCControls") {
      FrogPilotParamManageControl *manageSLCControlsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this, true);
      QObject::connect(manageSLCControlsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(speedLimitControllerControlsKeys.find(key.c_str()) != speedLimitControllerControlsKeys.end());
        }
        openSubParentToggle();
      });
      controlToggle = manageSLCControlsToggle;
    } else if (param == "SLCQOL") {
      FrogPilotParamManageControl *manageSLCQOLToggle = new FrogPilotParamManageControl(param, title, desc, icon, this, true);
      QObject::connect(manageSLCQOLToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedSpeedLimitControllerQOLKeys = speedLimitControllerQOLKeys;

          if (hasPCMCruise) {
            modifiedSpeedLimitControllerQOLKeys.erase("SetSpeedLimit");
          }

          if (!isToyota) {
            modifiedSpeedLimitControllerQOLKeys.erase("ForceMPHDashboard");
          }

          toggle->setVisible(modifiedSpeedLimitControllerQOLKeys.find(key.c_str()) != modifiedSpeedLimitControllerQOLKeys.end());
        }
        openSubParentToggle();
      });
      controlToggle = manageSLCQOLToggle;
    } else if (param == "SLCConfirmation") {
      std::vector<QString> slcConfirmationToggles{"SLCConfirmationLower", "SLCConfirmationHigher"};
      std::vector<QString> slcConfirmationNames{tr("較低的限速"), tr("較高的限速")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, slcConfirmationToggles, slcConfirmationNames);
    } else if (param == "SLCLookaheadHigher" || param == "SLCLookaheadLower") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 60, std::map<int, QString>(), this, false, " seconds");
    } else if (param == "SLCVisuals") {
      FrogPilotParamManageControl *manageSLCVisualsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this, true);
      QObject::connect(manageSLCVisualsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(speedLimitControllerVisualsKeys.find(key.c_str()) != speedLimitControllerVisualsKeys.end());
        }
        openSubParentToggle();
      });
      controlToggle = manageSLCVisualsToggle;
    } else if (param == "Offset1" || param == "Offset2" || param == "Offset3" || param == "Offset4") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, -99, 99, std::map<int, QString>(), this, false, tr("mph"));
    } else if (param == "ShowSLCOffset") {
      std::vector<QString> slcOffsetToggles{"ShowSLCOffsetUI"};
      std::vector<QString> slcOffsetToggleNames{tr("透過螢幕控制")};
      controlToggle = new FrogPilotParamToggleControl(param, title, desc, icon, slcOffsetToggles, slcOffsetToggleNames);
    } else if (param == "SLCFallback") {
      std::vector<QString> fallbackOptions{tr("設定速度"), tr("實驗模式"), tr("先前速限")};
      FrogPilotButtonParamControl *fallbackSelection = new FrogPilotButtonParamControl(param, title, desc, icon, fallbackOptions);
      controlToggle = fallbackSelection;
    } else if (param == "SLCOverride") {
      std::vector<QString> overrideOptions{tr("無"), tr("手動設定"), tr("直接變更")};
      FrogPilotButtonParamControl *overrideSelection = new FrogPilotButtonParamControl(param, title, desc, icon, overrideOptions);
      controlToggle = overrideSelection;
    } else if (param == "SLCPriority") {
      ButtonControl *slcPriorityButton = new ButtonControl(title, tr("選擇"), desc);
      QStringList primaryPriorities = {tr("無"), tr("儀表"), tr("導航"), tr("離線地圖"), tr("最高"), tr("最低")};
      QStringList secondaryTertiaryPriorities = {tr("無"), tr("儀表"), tr("導航"), tr("離線地圖")};
      QStringList priorityPrompts = {tr("選擇第一優先"), tr("選擇第二優先"), tr("選擇第三優先")};

      QObject::connect(slcPriorityButton, &ButtonControl::clicked, [=]() {
        QStringList selectedPriorities;

        for (int i = 1; i <= 3; ++i) {
          QStringList currentPriorities = (i == 1) ? primaryPriorities : secondaryTertiaryPriorities;
          QStringList prioritiesToDisplay = currentPriorities;
          for (const auto &selectedPriority : qAsConst(selectedPriorities)) {
            prioritiesToDisplay.removeAll(selectedPriority);
          }

          if (!hasDashSpeedLimits) {
            prioritiesToDisplay.removeAll(tr("儀表"));
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
      controlToggle = slcPriorityButton;

    } else if (param == "VisionTurnControl") {
      FrogPilotParamManageControl *visionTurnControlToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(visionTurnControlToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(visionTurnControlKeys.find(key.c_str()) != visionTurnControlKeys.end());
        }
        openParentToggle();
      });
      controlToggle = visionTurnControlToggle;
    } else if (param == "CurveSensitivity" || param == "TurnAggressiveness") {
      controlToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 200, std::map<int, QString>(), this, false, "%");

    } else {
      controlToggle = new ParamControl(param, title, desc, icon, this);
    }

    addItem(controlToggle);
    toggles[param.toStdString()] = controlToggle;

    QObject::connect(static_cast<ToggleControl*>(controlToggle), &ToggleControl::toggleFlipped, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamToggleControl*>(controlToggle), &FrogPilotParamToggleControl::buttonTypeClicked, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamValueControl*>(controlToggle), &FrogPilotParamValueControl::valueChanged, &updateFrogPilotToggles);

    QObject::connect(controlToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });

    QObject::connect(static_cast<FrogPilotParamManageControl*>(controlToggle), &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
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
      tr("警告：這將阻止您的設備資料出現在逗號連接上，這可能會影響調試和支援!"),
      tr("我了解風險."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["TrafficMode"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("若要啟動“塞車模式”，請按住方向盤上的“距離”按鈕 2.5 秒."),
        tr("聽起來不錯!"), this);
    }
  });

  std::set<QString> rebootKeys = {"AlwaysOnLateral", "NNFF", "NNFFLite"};
  for (const QString &key : rebootKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key.toStdString().c_str()]), &ToggleControl::toggleFlipped, [this, key](bool state) {
      if (started) {
        if (key == "AlwaysOnLateral" && state) {
          if (FrogPilotConfirmationDialog::toggle(tr("需要開機才能生效."), tr("馬上重啟"), this)) {
            Hardware::reboot();
          }
        } else if (key != "AlwaysOnLateral") {
          if (FrogPilotConfirmationDialog::toggle(tr("需要開機才能生效."), tr("馬上重啟"), this)) {
            Hardware::reboot();
          }
        }
      }
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["ModelRandomizer"]), &ToggleControl::toggleFlipped, [this](bool state) {
    modelRandomizer = state;
    if (state && !modelsDownloaded) {
      if (FrogPilotConfirmationDialog::yesorno(tr("「模型隨機產生器」僅適用於下載的模型。您想下載所有駕駛模型嗎?"), this)) {
        startDownloadAllModels();
      }
    }
  });

  FrogPilotParamValueControl *trafficFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficFollow"]);
  FrogPilotParamValueControl *trafficAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkAcceleration"]);
  FrogPilotParamValueControl *trafficDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkDanger"]);
  FrogPilotParamValueControl *trafficSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkSpeed"]);
  FrogPilotButtonsControl *trafficResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetTrafficPersonality"]);

  QObject::connect(trafficResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「塞車模式」的設定嗎?"), this)) {
      params.putFloat("TrafficFollow", 0.5);
      params.putFloat("TrafficJerkAcceleration", 50);
      params.putFloat("TrafficJerkDanger", 100);
      params.putFloat("TrafficJerkSpeed", 50);
      trafficFollowToggle->refresh();
      trafficAccelerationToggle->refresh();
      trafficDangerToggle->refresh();
      trafficSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *aggressiveFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveFollow"]);
  FrogPilotParamValueControl *aggressiveAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkAcceleration"]);
  FrogPilotParamValueControl *aggressiveDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkDanger"]);
  FrogPilotParamValueControl *aggressiveSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkSpeed"]);
  FrogPilotButtonsControl *aggressiveResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetAggressivePersonality"]);

  QObject::connect(aggressiveResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「積極」模式的設定嗎?"), this)) {
      params.putFloat("AggressiveFollow", 1.25);
      params.putFloat("AggressiveJerkAcceleration", 50);
      params.putFloat("AggressiveJerkDanger", 100);
      params.putFloat("AggressiveJerkSpeed", 50);
      aggressiveFollowToggle->refresh();
      aggressiveAccelerationToggle->refresh();
      aggressiveDangerToggle->refresh();
      aggressiveSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *standardFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardFollow"]);
  FrogPilotParamValueControl *standardAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkAcceleration"]);
  FrogPilotParamValueControl *standardDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkDanger"]);
  FrogPilotParamValueControl *standardSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkSpeed"]);
  FrogPilotButtonsControl *standardResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetStandardPersonality"]);

  QObject::connect(standardResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「標準」模式的設定嗎?"), this)) {
      params.putFloat("StandardFollow", 1.45);
      params.putFloat("StandardJerkAcceleration", 100);
      params.putFloat("StandardJerkDanger", 100);
      params.putFloat("StandardJerkSpeed", 100);
      standardFollowToggle->refresh();
      standardAccelerationToggle->refresh();
      standardDangerToggle->refresh();
      standardSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *relaxedFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedFollow"]);
  FrogPilotParamValueControl *relaxedAccelerationToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkAcceleration"]);
  FrogPilotParamValueControl *relaxedDangerToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkDanger"]);
  FrogPilotParamValueControl *relaxedSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkSpeed"]);
  FrogPilotButtonsControl *relaxedResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetRelaxedPersonality"]);

  QObject::connect(relaxedResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("您確定要完全重置「輕鬆」模式的設定嗎?"), this)) {
      params.putFloat("RelaxedFollow", 1.75);
      params.putFloat("RelaxedJerkAcceleration", 100);
      params.putFloat("RelaxedJerkDanger", 100);
      params.putFloat("RelaxedJerkSpeed", 100);
      relaxedFollowToggle->refresh();
      relaxedAccelerationToggle->refresh();
      relaxedDangerToggle->refresh();
      relaxedSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  steerRatioToggle = static_cast<FrogPilotParamValueToggleControl*>(toggles["SteerRatio"]);

  QObject::connect(steerRatioToggle, &FrogPilotParamValueToggleControl::buttonClicked, this, [this]() {
    params.putFloat("SteerRatio", steerRatioStock);
    steerRatioToggle->refresh();
    updateFrogPilotToggles();
  });

  QObject::connect(parent, &SettingsWindow::closeParentToggle, this, &FrogPilotControlsPanel::hideToggles);
  QObject::connect(parent, &SettingsWindow::closeSubParentToggle, this, &FrogPilotControlsPanel::hideSubToggles);
  QObject::connect(parent, &SettingsWindow::closeSubSubParentToggle, this, &FrogPilotControlsPanel::hideSubSubToggles);
  QObject::connect(parent, &SettingsWindow::updateMetric, this, &FrogPilotControlsPanel::updateMetric);
  QObject::connect(uiState(), &UIState::driveRated, this, &FrogPilotControlsPanel::updateModelLabels);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &FrogPilotControlsPanel::updateCarToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotControlsPanel::updateState);

  updateMetric();
  updateModelLabels();
}

void FrogPilotControlsPanel::showEvent(QShowEvent *event) {
  disableOpenpilotLongitudinal = params.getBool("DisableOpenpilotLongitudinal");
  modelRandomizer = params.getBool("ModelRandomizer");
}

void FrogPilotControlsPanel::updateState(const UIState &s) {
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

void FrogPilotControlsPanel::updateCarToggles() {
  auto carParams = params.get("CarParamsPersistent");
  if (!carParams.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(carParams.data(), carParams.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();
    auto carFingerprint = CP.getCarFingerprint();
    auto carName = CP.getCarName();
    auto safetyConfigs = CP.getSafetyConfigs();
    auto safetyModel = safetyConfigs[0].getSafetyModel();

    hasAutoTune = (carName == "hyundai" || carName == "toyota") && CP.getLateralTuning().which() == cereal::CarParams::LateralTuning::TORQUE;
    bool forcingAutoTune = params.getBool("LateralTune") && params.getBool("ForceAutoTune");
    uiState()->scene.has_auto_tune = hasAutoTune || forcingAutoTune;
    hasCommaNNFFSupport = checkCommaNNFFSupport(carFingerprint);
    hasDashSpeedLimits = carName == "hyundai" || carName == "toyota";
    hasNNFFLog = checkNNFFLogFileExists(carFingerprint);
    hasOpenpilotLongitudinal = hasLongitudinalControl(CP);
    hasPCMCruise = CP.getPcmCruise();
    isGM = carName == "gm";
    isHKGCanFd = carName == "hyundai" && safetyModel == cereal::CarParams::SafetyModel::HYUNDAI_CANFD;
    isToyota = carName == "toyota";
    steerRatioStock = CP.getSteerRatio();

    steerRatioToggle->setTitle(QString(tr("轉向比 (Default: %1)")).arg(QString::number(steerRatioStock, 'f', 2)));
    steerRatioToggle->updateControl(steerRatioStock * 0.75, steerRatioStock * 1.25, "", 0.01);
    steerRatioToggle->refresh();
  } else {
    hasAutoTune = false;
    hasCommaNNFFSupport = false;
    hasDashSpeedLimits = true;
    hasNNFFLog = true;
    hasOpenpilotLongitudinal = true;
    hasPCMCruise = true;
    isGM = true;
    isHKGCanFd = true;
    isToyota = true;
  }

  hideToggles();
}

void FrogPilotControlsPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;
    double speedConversion = isMetric ? MILE_TO_KM : KM_TO_MILE;

    params.putIntNonBlocking("LaneDetectionWidth", std::nearbyint(params.getInt("LaneDetectionWidth") * distanceConversion));
    params.putIntNonBlocking("StoppingDistance", std::nearbyint(params.getInt("StoppingDistance") * distanceConversion));

    params.putIntNonBlocking("CESpeed", std::nearbyint(params.getInt("CESpeed") * speedConversion));
    params.putIntNonBlocking("CESpeedLead", std::nearbyint(params.getInt("CESpeedLead") * speedConversion));
    params.putIntNonBlocking("CustomCruise", std::nearbyint(params.getInt("CustomCruise") * speedConversion));
    params.putIntNonBlocking("CustomCruiseLong", std::nearbyint(params.getInt("CustomCruiseLong") * speedConversion));
    params.putIntNonBlocking("MinimumLaneChangeSpeed", std::nearbyint(params.getInt("MinimumLaneChangeSpeed") * speedConversion));
    params.putIntNonBlocking("Offset1", std::nearbyint(params.getInt("Offset1") * speedConversion));
    params.putIntNonBlocking("Offset2", std::nearbyint(params.getInt("Offset2") * speedConversion));
    params.putIntNonBlocking("Offset3", std::nearbyint(params.getInt("Offset3") * speedConversion));
    params.putIntNonBlocking("Offset4", std::nearbyint(params.getInt("Offset4") * speedConversion));
    params.putIntNonBlocking("PauseAOLOnBrake", std::nearbyint(params.getInt("PauseAOLOnBrake") * speedConversion));
    params.putIntNonBlocking("PauseLateralOnSignal", std::nearbyint(params.getInt("PauseLateralOnSignal") * speedConversion));
    params.putIntNonBlocking("PauseLateralSpeed", std::nearbyint(params.getInt("PauseLateralSpeed") * speedConversion));
    params.putIntNonBlocking("SetSpeedOffset", std::nearbyint(params.getInt("SetSpeedOffset") * speedConversion));
  }

  FrogPilotDualParamControl *ceSpeedToggle = reinterpret_cast<FrogPilotDualParamControl*>(toggles["CESpeed"]);
  FrogPilotParamValueControl *customCruiseToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruise"]);
  FrogPilotParamValueControl *customCruiseLongToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruiseLong"]);
  FrogPilotParamValueControl *laneWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneDetectionWidth"]);
  FrogPilotParamValueControl *minimumLaneChangeSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["MinimumLaneChangeSpeed"]);
  FrogPilotParamValueControl *offset1Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset1"]);
  FrogPilotParamValueControl *offset2Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset2"]);
  FrogPilotParamValueControl *offset3Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset3"]);
  FrogPilotParamValueControl *offset4Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset4"]);
  FrogPilotParamValueControl *pauseAOLOnBrakeToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseAOLOnBrake"]);
  FrogPilotParamValueControl *pauseLateralToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseLateralSpeed"]);
  FrogPilotParamValueControl *setSpeedOffsetToggle = static_cast<FrogPilotParamValueControl*>(toggles["SetSpeedOffset"]);
  FrogPilotParamValueControl *stoppingDistanceToggle = static_cast<FrogPilotParamValueControl*>(toggles["StoppingDistance"]);

  if (isMetric) {
    offset1Toggle->setTitle(tr("Speed Limit Offset (0-34 kph)"));
    offset2Toggle->setTitle(tr("Speed Limit Offset (35-54 kph)"));
    offset3Toggle->setTitle(tr("Speed Limit Offset (55-64 kph)"));
    offset4Toggle->setTitle(tr("Speed Limit Offset (65-99 kph)"));

    offset1Toggle->setDescription(tr("Set speed limit offset for limits between 0-34 kph."));
    offset2Toggle->setDescription(tr("Set speed limit offset for limits between 35-54 kph."));
    offset3Toggle->setDescription(tr("Set speed limit offset for limits between 55-64 kph."));
    offset4Toggle->setDescription(tr("Set speed limit offset for limits between 65-99 kph."));

    ceSpeedToggle->updateControl(0, 150, tr(" 公里"));
    customCruiseToggle->updateControl(1, 150, tr(" 公里"));
    customCruiseLongToggle->updateControl(1, 150, tr(" 公里"));
    minimumLaneChangeSpeedToggle->updateControl(0, 150, tr(" 公里"));
    offset1Toggle->updateControl(-99, 99, tr(" 公里"));
    offset2Toggle->updateControl(-99, 99, tr(" 公里"));
    offset3Toggle->updateControl(-99, 99, tr(" 公里"));
    offset4Toggle->updateControl(-99, 99, tr(" 公里"));
    pauseAOLOnBrakeToggle->updateControl(0, 99, tr(" 公里"));
    pauseLateralToggle->updateControl(0, 99, tr(" 公里"));
    setSpeedOffsetToggle->updateControl(0, 150, tr(" 公里"));

    laneWidthToggle->updateControl(0, 30, tr(" 公尺"), 10);
    stoppingDistanceToggle->updateControl(0, 5, tr(" 公尺"));
  } else {
    offset1Toggle->setTitle(tr("Speed Limit Offset (0-34 mph)"));
    offset2Toggle->setTitle(tr("Speed Limit Offset (35-54 mph)"));
    offset3Toggle->setTitle(tr("Speed Limit Offset (55-64 mph)"));
    offset4Toggle->setTitle(tr("Speed Limit Offset (65-99 mph)"));

    offset1Toggle->setDescription(tr("Set speed limit offset for limits between 0-34 mph."));
    offset2Toggle->setDescription(tr("Set speed limit offset for limits between 35-54 mph."));
    offset3Toggle->setDescription(tr("Set speed limit offset for limits between 55-64 mph."));
    offset4Toggle->setDescription(tr("Set speed limit offset for limits between 65-99 mph."));

    ceSpeedToggle->updateControl(0, 99, tr("mph"));
    customCruiseToggle->updateControl(1, 99, tr("mph"));
    customCruiseLongToggle->updateControl(1, 99, tr("mph"));
    minimumLaneChangeSpeedToggle->updateControl(0, 99, tr("mph"));
    offset1Toggle->updateControl(-99, 99, tr("mph"));
    offset2Toggle->updateControl(-99, 99, tr("mph"));
    offset3Toggle->updateControl(-99, 99, tr("mph"));
    offset4Toggle->updateControl(-99, 99, tr("mph"));
    pauseAOLOnBrakeToggle->updateControl(0, 99, tr("mph"));
    pauseLateralToggle->updateControl(0, 99, tr("mph"));
    setSpeedOffsetToggle->updateControl(0, 99, tr("mph"));

    laneWidthToggle->updateControl(0, 100, tr(" feet"), 10);
    stoppingDistanceToggle->updateControl(0, 10, tr(" feet"));
  }

  ceSpeedToggle->refresh();
  customCruiseToggle->refresh();
  customCruiseLongToggle->refresh();
  laneWidthToggle->refresh();
  minimumLaneChangeSpeedToggle->refresh();
  offset1Toggle->refresh();
  offset2Toggle->refresh();
  offset3Toggle->refresh();
  offset4Toggle->refresh();
  pauseAOLOnBrakeToggle->refresh();
  pauseLateralToggle->refresh();
  setSpeedOffsetToggle->refresh();
  stoppingDistanceToggle->refresh();
}

void FrogPilotControlsPanel::startDownloadAllModels() {
  allModelsDownloading = true;
  modelDownloading = true;

  paramsMemory.putBool("DownloadAllModels", true);

  downloadAllModelsBtn->setValue(tr("下載模型..."));

  QTimer *checkDownloadTimer = new QTimer(this);
  checkDownloadTimer->setInterval(100);

  QObject::connect(checkDownloadTimer, &QTimer::timeout, this, [=]() {
    QString progress = QString::fromStdString(paramsMemory.get("ModelDownloadProgress"));
    bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|Failed|offline", QRegularExpression::CaseInsensitiveOption));

    if (!progress.isEmpty() && progress != "0%") {
      downloadAllModelsBtn->setValue(progress);
    }

    if (progress == "All models downloaded!" || downloadFailed) {
      if (!downloadFailed) {
        haveModelsDownloaded = true;
        update();
      }

      QTimer::singleShot(2000, this, [=]() {
        allModelsDownloading = false;
        cancellingDownload = false;
        modelDownloading = false;
        downloadAllModelsBtn->setValue("");
        modelsDownloaded = params.getBool("ModelsDownloaded");
        paramsMemory.remove("CancelModelDownload");
        update();
      });

      paramsMemory.remove("ModelDownloadProgress");

      checkDownloadTimer->stop();
      checkDownloadTimer->deleteLater();
    }
  });

  checkDownloadTimer->start();
}

QString FrogPilotControlsPanel::processModelName(const QString &modelName) {
  QString modelCleaned = modelName;
  modelCleaned = modelCleaned.remove(QRegularExpression("[🗺️👀📡]")).simplified();
  QString scoreParam = modelCleaned.remove(QRegularExpression("[^a-zA-Z0-9()-]")).replace(" ", "").simplified();
  scoreParam = scoreParam.replace("(Default)", "").replace("-", "");
  return scoreParam;
}

void FrogPilotControlsPanel::updateCalibrationDescription() {
  QString model = QString::fromStdString(params.get("ModelName"));
  QString part_model_param = processModelName(model);

  QString desc =
      tr("openpilot 要求設備安裝在左或右 4° 以內  向上 5° 或向下 9° 以內"
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
        desc += tr(" 您的裝置已指向 %1° %2 and %3° %4.")
                    .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? tr("向下") : tr("向上"),
                         QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? tr("向左") : tr("向右"));
      }
    } catch (kj::Exception) {
      qInfo() << "invalid CalibrationParams";
    }
  }
  qobject_cast<FrogPilotButtonsControl *>(sender())->setDescription(desc);
}

void FrogPilotControlsPanel::updateModelLabels() {
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
    LabelControl *labelControl = new LabelControl(pair.first, scoreDisplay, "", this);
    addItem(labelControl);
    labelControls.append(labelControl);
  }

  for (LabelControl *label : labelControls) {
    label->setVisible(false);
  }
}

void FrogPilotControlsPanel::hideToggles() {
  customPersonalitiesOpen = false;
  modelManagementOpen = false;
  slcOpen = false;

  for (LabelControl *label : labelControls) {
    label->setVisible(false);
  }

  std::set<QString> longitudinalKeys = {"ConditionalExperimental", "DrivingPersonalities", "ExperimentalModeActivation",
                                        "LongitudinalTune", "MTSCEnabled", "SpeedLimitController", "VisionTurnControl"};

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(false);

    if ((!hasOpenpilotLongitudinal || disableOpenpilotLongitudinal) && longitudinalKeys.find(key.c_str()) != longitudinalKeys.end()) {
      continue;
    }

    bool subToggles = aggressivePersonalityKeys.find(key.c_str()) != aggressivePersonalityKeys.end() ||
                      aolKeys.find(key.c_str()) != aolKeys.end() ||
                      conditionalExperimentalKeys.find(key.c_str()) != conditionalExperimentalKeys.end() ||
                      customdrivingPersonalityKeys.find(key.c_str()) != customdrivingPersonalityKeys.end() ||
                      deviceManagementKeys.find(key.c_str()) != deviceManagementKeys.end() ||
                      drivingPersonalityKeys.find(key.c_str()) != drivingPersonalityKeys.end() ||
                      experimentalModeActivationKeys.find(key.c_str()) != experimentalModeActivationKeys.end() ||
                      laneChangeKeys.find(key.c_str()) != laneChangeKeys.end() ||
                      lateralTuneKeys.find(key.c_str()) != lateralTuneKeys.end() ||
                      longitudinalTuneKeys.find(key.c_str()) != longitudinalTuneKeys.end() ||
                      modelManagementKeys.find(key.c_str()) != modelManagementKeys.end() ||
                      modelRandomizerKeys.find(key.c_str()) != modelRandomizerKeys.end() ||
                      mtscKeys.find(key.c_str()) != mtscKeys.end() ||
                      qolKeys.find(key.c_str()) != qolKeys.end() ||
                      relaxedPersonalityKeys.find(key.c_str()) != relaxedPersonalityKeys.end() ||
                      speedLimitControllerKeys.find(key.c_str()) != speedLimitControllerKeys.end() ||
                      speedLimitControllerControlsKeys.find(key.c_str()) != speedLimitControllerControlsKeys.end() ||
                      speedLimitControllerQOLKeys.find(key.c_str()) != speedLimitControllerQOLKeys.end() ||
                      speedLimitControllerVisualsKeys.find(key.c_str()) != speedLimitControllerVisualsKeys.end() ||
                      standardPersonalityKeys.find(key.c_str()) != standardPersonalityKeys.end() ||
                      trafficPersonalityKeys.find(key.c_str()) != trafficPersonalityKeys.end() ||
                      visionTurnControlKeys.find(key.c_str()) != visionTurnControlKeys.end();
    toggle->setVisible(!subToggles);
  }

  update();
}

void FrogPilotControlsPanel::hideSubToggles() {
  if (customPersonalitiesOpen) {
    for (auto &[key, toggle] : toggles) {
      bool isVisible = drivingPersonalityKeys.find(key.c_str()) != drivingPersonalityKeys.end();
      toggle->setVisible(isVisible);
    }
  } else if (slcOpen) {
    for (auto &[key, toggle] : toggles) {
      bool isVisible = speedLimitControllerKeys.find(key.c_str()) != speedLimitControllerKeys.end();
      toggle->setVisible(isVisible);
    }
  } else if (modelManagementOpen) {
    for (LabelControl *label : labelControls) {
      label->setVisible(false);
    }

    for (auto &[key, toggle] : toggles) {
      bool isVisible = modelManagementKeys.find(key.c_str()) != modelManagementKeys.end();
      toggle->setVisible(isVisible);
    }
  }

  update();
}

void FrogPilotControlsPanel::hideSubSubToggles() {
  if (customPersonalitiesOpen) {
    for (auto &[key, toggle] : toggles) {
      bool isVisible = customdrivingPersonalityKeys.find(key.c_str()) != customdrivingPersonalityKeys.end();
      toggle->setVisible(isVisible);
    }
  } else if (modelManagementOpen) {
    for (LabelControl *label : labelControls) {
      label->setVisible(false);
    }

    for (auto &[key, toggle] : toggles) {
      bool isVisible = modelRandomizerKeys.find(key.c_str()) != modelRandomizerKeys.end();
      toggle->setVisible(isVisible);
    }
  }

  update();
}
