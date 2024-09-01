#include "selfdrive/frogpilot/ui/qt/offroad/visual_settings.h"

FrogPilotVisualsPanel::FrogPilotVisualsPanel(SettingsWindow *parent) : FrogPilotListWidget(parent) {
  std::string branch = params.get("GitBranch");
  isRelease = branch == "FrogPilot";

  const std::vector<std::tuple<QString, QString, QString, QString>> visualToggles {
    {"AlertVolumeControl", tr("警報音量控制"), tr("控制 openpilot 中每個聲音的音量級別."), "../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"DisengageVolume", tr("  失效音量"), tr("相關提醒:\n\n巡航停用\n手煞車\n煞車\n速度太低"), ""},
    {"EngageVolume", tr("  啟用音量"), tr("相關提醒:\n\nNNFF 扭矩控制器已加載\nopenpilot 啟用"), ""},
    {"PromptVolume", tr("  提示音量"), tr("相關提醒:\n\n盲點偵測到汽車\n綠燈提醒\n速度太低\n轉向低於“X”時不可用\n控制，轉彎超出轉向極限"), ""},
    {"PromptDistractedVolume", tr("  注意力分散的音量"), tr("相關提醒:\n\n請注意，司機分心\n觸摸方向盤，駕駛反應遲鈍"), ""},
    {"RefuseVolume", tr("  拒絕音量"), tr("相關提醒:\n\nopenpilot 不可用"), ""},
    {"WarningSoftVolume", tr("  警告音量"), tr("相關提醒:\n\n煞車！，碰撞危險\n立即控制"), ""},
    {"WarningImmediateVolume", tr("  警告即時音量"), tr("相關提醒:\n\n立即脫離，駕駛分心\n立即脫離，駕駛員沒有反應"), ""},

    {"CustomAlerts", tr("自訂警報"), tr("針對各種邏輯或情況變化啟用自訂警報."), "../frogpilot/assets/toggle_icons/icon_green_light.png"},
    {"GreenLightAlert", tr("  綠燈提醒"), tr("當交通燈由紅變綠時收到警報."), ""},
    {"LeadDepartingAlert", tr("  前車遠離警告"), tr("當您處於靜止狀態時前方車輛開始出發時收到警報."), ""},
    {"LoudBlindspotAlert", tr("  大聲盲點警報"), tr("當嘗試變換車道時在盲點偵測到車輛時，啟用更響亮的警報."), ""},

    {"CustomUI", tr("自定義道路畫面"), tr("定義自己喜歡的道路介面."), "../assets/offroad/icon_road.png"},
    {"Compass", tr("  羅盤"), tr("將指南針加入道路使用者介面."), ""},
    {"CustomPaths", tr("  路徑"), tr("顯示您在行駛路徑上的預期加速度、偵測到的相鄰車道或在盲點中偵測到車輛時的加速度."), ""},
    {"PedalsOnUI", tr("  踏板"), tr("在方向盤圖示下方的道路使用者介面上顯示煞車踏板和油門踏板."), ""},
    {"RoadNameUI", tr("  道路名稱"), tr("在螢幕底部顯示目前道路的名稱。來源自 OpenStreetMap."), ""},
    {"WheelIcon", tr("  方向盤圖示"), tr("將預設方向盤圖示替換為自訂圖標."), ""},
    {"ShowStoppingPoint", tr("  停止點"), tr("顯示 openpilot 因紅燈或停車標誌而停止的位子."), ""},

    {"CustomTheme", tr("自訂外觀主題"), tr("啟動後使用自訂外觀."), "../frogpilot/assets/wheel_images/frog.png"},
    {"CustomColors", tr("  顏色"), tr("使用自訂配色方案替換庫存 openpilot 顏色"), ""},
    {"CustomIcons", tr("  圖示"), tr("用自訂圖標包替換庫存 openpilot 圖標"), ""},
    {"CustomSounds", tr("  聲音"), tr("用自訂聲音包替換庫存 openpilot 聲音"), ""},
    {"CustomSignals", tr("  訊號"), tr("啟用自訂方向燈動畫"), ""},
    {"HolidayThemes", tr("  節日主題"), tr("openpilot 主題會根據當前/即將到來的假期而變化。小假期持續一天，大假期（復活節、聖誕節、萬聖節等）持續一周."), ""},
    {"RandomEvents", tr("  隨機事件"), tr("享受在某些駕駛條件下可能發生的隨機事件的一些不可預測性。這純粹是裝飾性的，對駕駛控制沒有影響!"), ""},

    {"DeveloperUI", tr("開發者介面"), tr("獲取 openpilot 在幕後所做的各種詳細信息."), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"BorderMetrics", tr("  邊界指標"), tr("在道路 UI 邊框中顯示指標."), ""},
    {"FPSCounter", tr("  FPS 計數器"), tr("顯示道路 UI 的「每秒幀數」(FPS)，以監控系統效能."), ""},
    {"LateralMetrics", tr("  橫向指標"), tr("顯示與 openpilot 橫向性能相關的各種指標."), ""},
    {"LongitudinalMetrics", tr("  縱向指標"), tr("顯示與 openpilot 縱向效能相關的各種指標."), ""},
    {"NumericalTemp", tr("  數位溫度計"), tr("將「GOOD」、「OK」和「HIGH」溫度狀態替換為基於記憶體、CPU 和 GPU 之間最高溫度的數位溫度計."), ""},
    {"SidebarMetrics", tr("  側邊欄"), tr("在側邊欄上顯示 CPU、GPU、RAM、IP 和已使用/剩餘儲存的各種自訂指標."), ""},
    {"UseSI", tr("  使用國際單位制"), tr("以 SI 格式顯示相關指標."), ""},

    {"ModelUI", tr("路徑外觀"), tr("個性化模型的可視化在螢幕上的顯示方式."), "../assets/offroad/icon_calibration.png"},
    {"DynamicPathWidth", tr("  動態路徑寬度"), tr("根據 openpilot 目前的接合狀態動態調整路徑寬度."), ""},
    {"HideLeadMarker", tr("  隱藏引導標記"), tr("從道路 UI 中隱藏領先標記."), ""},
    {"LaneLinesWidth", tr("  車道寬"), tr("調整顯示器上車道線的視覺粗細.\n\n預設值為 MUTCD 平均車道線寬度 4 英寸."), ""},
    {"PathEdgeWidth", tr("  路徑邊寬"), tr("自定義顯示當前駕駛狀態的路徑邊緣寬度。預設為總路徑的 20%.\n\n藍色 = 導航\n淺藍色=“全時置中”\n綠色 = 預設\n橙色=“實驗模式”\n紅色=“塞車模式”\n黃色=“條件實驗模式”被覆蓋"), ""},
    {"PathWidth", tr("  路徑寬"), tr("自定義路徑寬度。\n\n預設為 skoda kodiaq 的寬度."), ""},
    {"RoadEdgesWidth", tr("  道路邊寬"), tr("自定義道路邊緣寬度。\n\n預設值為 MUTCD 平均車道線寬度 4 英寸."), ""},
    {"UnlimitedLength", tr("  無限道路"), tr("將路徑、車道線和道路邊緣的顯示器擴展到系統可以偵測的範圍內，提供更廣闊的前方道路視野."), ""},

    {"QOLVisuals", tr("優化體驗"), tr("各種控制細項的調整可改善您的openpilot體驗."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"BigMap", tr("  大地圖"), tr("增加道路使用者介面中地圖的大小."), ""},
    {"CameraView", tr("  相機視圖"), tr("為道路使用者介面選擇您喜歡的攝影機視圖。這純粹是視覺上的變化，不會影響 openpilot 的駕駛方式."), ""},
    {"DriverCamera", tr("  倒車駕駛員攝影機"), tr("倒車時顯示駕駛攝影機畫面."), ""},
    {"HideSpeed", tr("  隱藏速度"), tr("隱藏道路使用者介面中的速度指示器。附加切換允許透過點擊速度本身來隱藏/顯示."), ""},
    {"MapStyle", tr("  地圖樣式"), tr("選擇用於導航的地圖樣式."), ""},
    {"StoppedTimer", tr("Stopped Timer"), tr("Display a timer in the onroad UI that indicates how long you've been stopped for."), ""},
    {"WheelSpeed", tr("  使用輪速"), tr("在道路使用者介面中使用車輪速度而不是人工速度."), ""},

    {"ScreenManagement", tr("螢幕管理"), tr("管理螢幕亮度、超時設定並隱藏道路 UI 元素."), "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"HideUIElements", tr("  隱藏使用者介面元素"), tr("在道路畫面上隱藏選定的 UI 元素."), ""},
    {"ScreenBrightness", tr("  螢幕亮度"), tr("自訂停止時的螢幕亮度."), ""},
    {"ScreenBrightnessOnroad", tr("  螢幕亮度 (行進間)"), tr("自訂行進時的螢幕亮度."), ""},
    {"ScreenRecorder", tr("  螢幕錄影機"), tr("啟用在路上錄製螢幕的功能."), ""},
    {"ScreenTimeout", tr("  螢幕待機"), tr("自訂螢幕關閉所需的時間."), ""},
    {"ScreenTimeoutOnroad", tr("  螢幕待機 (行進間)"), tr("自訂行進間螢幕關閉所需的時間."), ""},
    {"StandbyMode", tr("  待機模式"), tr("在路上螢幕超時後關閉螢幕，但在參與狀態變更或觸發重要警報時將其喚醒."), ""},
    {"GooffScreen", tr("  啟動關閉螢幕"), tr("車輛起動後自動關閉螢幕，熄火後恢復螢幕."), ""},
  };

  for (const auto &[param, title, desc, icon] : visualToggles) {
    AbstractControl *visualToggle;

    if (param == "AlertVolumeControl") {
      FrogPilotParamManageControl *alertVolumeControlToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(alertVolumeControlToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(alertVolumeControlKeys.find(key.c_str()) != alertVolumeControlKeys.end());
        }
      });
      visualToggle = alertVolumeControlToggle;
    } else if (alertVolumeControlKeys.find(param) != alertVolumeControlKeys.end()) {
      if (param == "WarningImmediateVolume") {
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 25, 100, std::map<int, QString>(), this, false, "%");
      } else {
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, "%");
      }

    } else if (param == "CustomAlerts") {
      FrogPilotParamManageControl *customAlertsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customAlertsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedCustomAlertsKeys = customAlertsKeys;

          if (!hasBSM) {
            modifiedCustomAlertsKeys.erase("LoudBlindspotAlert");
          }

          toggle->setVisible(modifiedCustomAlertsKeys.find(key.c_str()) != modifiedCustomAlertsKeys.end());
        }
      });
      visualToggle = customAlertsToggle;

    } else if (param == "CustomTheme") {
      FrogPilotParamManageControl *customThemeToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customThemeToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(customThemeKeys.find(key.c_str()) != customThemeKeys.end());
        }
      });
      visualToggle = customThemeToggle;
    } else if (param == "CustomColors" || param == "CustomIcons" || param == "CustomSignals" || param == "CustomSounds") {
      std::vector<QString> themeOptions{tr("原始"), tr("Frog"), tr("Tesla"), tr("Stalin")};
      FrogPilotButtonParamControl *themeSelection = new FrogPilotButtonParamControl(param, title, desc, icon, themeOptions);
      visualToggle = themeSelection;

      if (param == "CustomSounds") {
        QObject::connect(themeSelection, &FrogPilotButtonParamControl::buttonClicked, [this](int id) {
          if (id == 1) {
            if (FrogPilotConfirmationDialog::yesorno(tr("您想啟用額外的“山羊”音效嗎?"), this)) {
              params.putBoolNonBlocking("GoatScream", true);
            } else {
              params.putBoolNonBlocking("GoatScream", false);
            }
          }
        });
      }

    } else if (param == "CustomUI") {
      FrogPilotParamManageControl *customUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(customOnroadUIKeys.find(key.c_str()) != customOnroadUIKeys.end());
        }
      });
      visualToggle = customUIToggle;
    } else if (param == "CustomPaths") {
      std::vector<QString> pathToggles{"AccelerationPath", "AdjacentPath", "BlindSpotPath", "AdjacentPathMetrics"};
      std::vector<QString> pathToggleNames{tr("加速"), tr("鄰近的"), tr("盲點"), tr("指標")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, pathToggles, pathToggleNames);
    } else if (param == "PedalsOnUI") {
      std::vector<QString> pedalsToggles{"DynamicPedalsOnUI", "StaticPedalsOnUI"};
      std::vector<QString> pedalsToggleNames{tr("動態的"), tr("靜止的")};
      FrogPilotParamToggleControl *pedalsToggle = new FrogPilotParamToggleControl(param, title, desc, icon, pedalsToggles, pedalsToggleNames);
      QObject::connect(pedalsToggle, &FrogPilotParamToggleControl::buttonTypeClicked, this, [this, pedalsToggle](int index) {
        if (index == 0) {
          params.putBool("StaticPedalsOnUI", false);
        } else if (index == 1) {
          params.putBool("DynamicPedalsOnUI", false);
        }

        pedalsToggle->updateButtonStates();
      });
      visualToggle = pedalsToggle;
    } else if (param == "WheelIcon") {
      std::vector<QString> wheelToggles{"RotatingWheel"};
      std::vector<QString> wheelToggleNames{"即時旋轉"};
      std::map<int, QString> steeringWheelLabels = {{-1, tr("無")}, {0, tr("Stock")}, {1, tr("Lexus")}, {2, tr("Toyota")}, {3, tr("Frog")}, {4, tr("Rocket")}, {5, tr("Hyundai")}, {6, tr("Stalin")}};
      visualToggle = new FrogPilotParamValueToggleControl(param, title, desc, icon, -1, 6, steeringWheelLabels, this, true, "", 1, 1, wheelToggles, wheelToggleNames);
    } else if (param == "ShowStoppingPoint") {
      std::vector<QString> stoppingPointToggles{"ShowStoppingPointMetrics"};
      std::vector<QString> stoppingPointToggleNames{tr("顯示距離")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, stoppingPointToggles, stoppingPointToggleNames);

    } else if (param == "DeveloperUI") {
      FrogPilotParamManageControl *developerUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(developerUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedDeveloperUIKeys  = developerUIKeys ;

          toggle->setVisible(modifiedDeveloperUIKeys.find(key.c_str()) != modifiedDeveloperUIKeys.end());
        }
      });
      visualToggle = developerUIToggle;
    } else if (param == "BorderMetrics") {
      std::vector<QString> borderToggles{"BlindSpotMetrics", "ShowSteering", "SignalMetrics"};
      std::vector<QString> borderToggleNames{tr("盲點"), tr("轉向扭矩"), tr("轉彎訊號")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, borderToggles, borderToggleNames);
    } else if (param == "LateralMetrics") {
      std::vector<QString> lateralToggles{"TuningInfo"};
      std::vector<QString> lateralToggleNames{tr("自動 Tune")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, lateralToggles, lateralToggleNames);
    } else if (param == "LongitudinalMetrics") {
      std::vector<QString> longitudinalToggles{"LeadInfo", "JerkInfo"};
      std::vector<QString> longitudinalToggleNames{tr("前車訊息"), tr("Longitudinal Jerk")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, longitudinalToggles, longitudinalToggleNames);
    } else if (param == "NumericalTemp") {
      std::vector<QString> temperatureToggles{"Fahrenheit"};
      std::vector<QString> temperatureToggleNames{tr("華氏")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, temperatureToggles, temperatureToggleNames);
    } else if (param == "SidebarMetrics") {
      std::vector<QString> sidebarMetricsToggles{"ShowCPU", "ShowGPU", "ShowIP", "ShowMemoryUsage", "ShowStorageLeft", "ShowStorageUsed"};
      std::vector<QString> sidebarMetricsToggleNames{tr("CPU"), tr("GPU"), tr("IP"), tr("RAM"), tr("SSD 剩餘量"), tr("SSD 使用量")};
      FrogPilotParamToggleControl *sidebarMetricsToggle = new FrogPilotParamToggleControl(param, title, desc, icon, sidebarMetricsToggles, sidebarMetricsToggleNames, this, 125);
      QObject::connect(sidebarMetricsToggle, &FrogPilotParamToggleControl::buttonTypeClicked, this, [this, sidebarMetricsToggle](int index) {
        if (index == 0) {
          params.putBool("ShowGPU", false);
        } else if (index == 1) {
          params.putBool("ShowCPU", false);
        } else if (index == 3) {
          params.putBool("ShowStorageLeft", false);
          params.putBool("ShowStorageUsed", false);
        } else if (index == 4) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageUsed", false);
        } else if (index == 5) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageLeft", false);
        }

        sidebarMetricsToggle->updateButtonStates();
      });
      visualToggle = sidebarMetricsToggle;

    } else if (param == "ModelUI") {
      FrogPilotParamManageControl *modelUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(modelUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedModelUIKeysKeys = modelUIKeys;

          if (!hasOpenpilotLongitudinal || disableOpenpilotLongitudinal) {
            modifiedModelUIKeysKeys.erase("HideLeadMarker");
          }

          toggle->setVisible(modifiedModelUIKeysKeys.find(key.c_str()) != modifiedModelUIKeysKeys.end());
        }
      });
      visualToggle = modelUIToggle;
    } else if (param == "LaneLinesWidth" || param == "RoadEdgesWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 24, std::map<int, QString>(), this, false, tr(" inches"));
    } else if (param == "PathEdgeWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, tr("%"));
    } else if (param == "PathWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, tr(" feet"), 10);

    } else if (param == "QOLVisuals") {
      FrogPilotParamManageControl *qolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(qolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(qolKeys.find(key.c_str()) != qolKeys.end());
        }
      });
      visualToggle = qolToggle;
    } else if (param == "CameraView") {
      std::vector<QString> cameraOptions{tr("自動"), tr("駕駛"), tr("標準"), tr("廣角")};
      FrogPilotButtonParamControl *preferredCamera = new FrogPilotButtonParamControl(param, title, desc, icon, cameraOptions);
      visualToggle = preferredCamera;
    } else if (param == "BigMap") {
      std::vector<QString> mapToggles{"FullMap"};
      std::vector<QString> mapToggleNames{tr("全螢幕")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, mapToggles, mapToggleNames);
    } else if (param == "HideSpeed") {
      std::vector<QString> hideSpeedToggles{"HideSpeedUI"};
      std::vector<QString> hideSpeedToggleNames{tr("透過螢幕控制")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, hideSpeedToggles, hideSpeedToggleNames);
    } else if (param == "MapStyle") {
      QMap<int, QString> styleMap = {
        {0, tr("原始 openpilot")},
        {1, tr("Mapbox 街道")},
        {2, tr("Mapbox 戶外活動")},
        {3, tr("Mapbox 白天")},
        {4, tr("Mapbox 夜晚")},
        {5, tr("Mapbox 衛星")},
        {6, tr("Mapbox 衛星街道")},
        {7, tr("Mapbox 導航(白天)")},
        {8, tr("Mapbox 導航(夜晚)")},
        {9, tr("Mapbox 交通(夜晚)")},
        {10, tr("mike854's (Satellite hybrid)")},
        {11, tr("huifan's 白天")},
        {12, tr("huifan's 交通")},
      };

      QStringList styles = styleMap.values();
      ButtonControl *mapStyleButton = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(mapStyleButton, &ButtonControl::clicked, [=]() {
        QStringList styles = styleMap.values();
        QString selection = MultiOptionDialog::getSelection(tr("選擇地圖樣式"), styles, "", this);
        if (!selection.isEmpty()) {
          int selectedStyle = styleMap.key(selection);
          params.putIntNonBlocking("MapStyle", selectedStyle);
          mapStyleButton->setValue(selection);
          updateFrogPilotToggles();
        }
      });

      int currentStyle = params.getInt("MapStyle");
      mapStyleButton->setValue(styleMap[currentStyle]);

      visualToggle = mapStyleButton;

    } else if (param == "ScreenManagement") {
      FrogPilotParamManageControl *screenToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(screenToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(screenKeys.find(key.c_str()) != screenKeys.end());
        }
      });
      visualToggle = screenToggle;
    } else if (param == "HideUIElements") {
      std::vector<QString> uiElementsToggles{"HideAlerts", "HideMapIcon", "HideMaxSpeed"};
      std::vector<QString> uiElementsToggleNames{tr("警告"), tr("地圖"), tr("最高速")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, uiElementsToggles, uiElementsToggleNames);
    } else if (param == "ScreenBrightness" || param == "ScreenBrightnessOnroad") {
      std::map<int, QString> brightnessLabels;
      if (param == "ScreenBrightnessOnroad") {
        for (int i = 0; i <= 101; i++) {
          brightnessLabels[i] = (i == 0) ? tr("螢幕關閉") : (i == 101) ? tr("自動") : QString::number(i) + "%";
        }
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 101, brightnessLabels, this, false);
      } else {
        for (int i = 1; i <= 101; i++) {
          brightnessLabels[i] = (i == 101) ? tr("自動") : QString::number(i) + "%";
        }
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 101, brightnessLabels, this, false);
      }
    } else if (param == "ScreenTimeout" || param == "ScreenTimeoutOnroad") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 5, 60, std::map<int, QString>(), this, false, tr(" 秒"));

    } else {
      visualToggle = new ParamControl(param, title, desc, icon, this);
    }

    addItem(visualToggle);
    toggles[param.toStdString()] = visualToggle;

    QObject::connect(static_cast<ToggleControl*>(visualToggle), &ToggleControl::toggleFlipped, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamToggleControl*>(visualToggle), &FrogPilotParamToggleControl::buttonTypeClicked, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamValueControl*>(visualToggle), &FrogPilotParamValueControl::valueChanged, [this]() {
      bool screen_management = params.getBool("ScreenManagement");
      if (!started) {
        uiState()->scene.screen_brightness = screen_management ? params.getInt("ScreenBrightness") : 101;
      } else {
        uiState()->scene.screen_brightness_onroad = screen_management ? params.getInt("ScreenBrightnessOnroad") : 101;
      }
      updateFrogPilotToggles();
    });

    QObject::connect(visualToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });

    QObject::connect(static_cast<FrogPilotParamManageControl*>(visualToggle), &FrogPilotParamManageControl::manageButtonClicked, [this]() {
      update();
    });
  }

  QObject::connect(parent, &SettingsWindow::closeParentToggle, this, &FrogPilotVisualsPanel::hideToggles);
  QObject::connect(parent, &SettingsWindow::updateMetric, this, &FrogPilotVisualsPanel::updateMetric);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &FrogPilotVisualsPanel::updateCarToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotVisualsPanel::updateState);

  updateMetric();
}

void FrogPilotVisualsPanel::showEvent(QShowEvent *event) {
  disableOpenpilotLongitudinal = params.getBool("DisableOpenpilotLongitudinal");
}

void FrogPilotVisualsPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  started = s.scene.started;
}

void FrogPilotVisualsPanel::updateCarToggles() {
  auto carParams = params.get("CarParamsPersistent");
  if (!carParams.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(carParams.data(), carParams.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();
    auto carName = CP.getCarName();

    hasAutoTune = (carName == "hyundai" || carName == "toyota") && CP.getLateralTuning().which() == cereal::CarParams::LateralTuning::TORQUE;
    hasBSM = CP.getEnableBsm();
    hasOpenpilotLongitudinal = hasLongitudinalControl(CP);
  } else {
    hasAutoTune = true;
    hasBSM = true;
    hasOpenpilotLongitudinal = true;
  }

  hideToggles();
}

void FrogPilotVisualsPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? INCH_TO_CM : CM_TO_INCH;
    double speedConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;

    params.putIntNonBlocking("LaneLinesWidth", std::nearbyint(params.getInt("LaneLinesWidth") * distanceConversion));
    params.putIntNonBlocking("RoadEdgesWidth", std::nearbyint(params.getInt("RoadEdgesWidth") * distanceConversion));

    params.putIntNonBlocking("PathWidth", std::nearbyint(params.getInt("PathWidth") * speedConversion));
  }

  FrogPilotParamValueControl *laneLinesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneLinesWidth"]);
  FrogPilotParamValueControl *roadEdgesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["RoadEdgesWidth"]);
  FrogPilotParamValueControl *pathWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["PathWidth"]);

  if (isMetric) {
    laneLinesWidthToggle->setDescription(tr("自訂車道線寬度.\n\n預設值符合維也納平均 10 厘米."));
    roadEdgesWidthToggle->setDescription(tr("自訂道路邊緣寬度.\n\n預設為維也納平均車道線寬度 10 公分的 1/2."));

    laneLinesWidthToggle->updateControl(0, 60, tr(" 公分"));
    roadEdgesWidthToggle->updateControl(0, 60, tr(" 公分"));
    pathWidthToggle->updateControl(0, 30, tr(" 公尺"), 10);
  } else {
    laneLinesWidthToggle->setDescription(tr("Customize the lane line width.\n\nDefault matches the MUTCD average of 4 inches."));
    roadEdgesWidthToggle->setDescription(tr("Customize the road edges width.\n\nDefault is 1/2 of the MUTCD average lane line width of 4 inches."));

    laneLinesWidthToggle->updateControl(0, 24, tr(" inches"));
    roadEdgesWidthToggle->updateControl(0, 24, tr(" inches"));

    pathWidthToggle->updateControl(0, 100, tr(" feet"), 10);
  }

  laneLinesWidthToggle->refresh();
  roadEdgesWidthToggle->refresh();
}

void FrogPilotVisualsPanel::hideToggles() {
  for (auto &[key, toggle] : toggles) {
    bool subToggles = alertVolumeControlKeys.find(key.c_str()) != alertVolumeControlKeys.end() ||
                      customAlertsKeys.find(key.c_str()) != customAlertsKeys.end() ||
                      customOnroadUIKeys.find(key.c_str()) != customOnroadUIKeys.end() ||
                      customThemeKeys.find(key.c_str()) != customThemeKeys.end() ||
                      developerUIKeys.find(key.c_str()) != developerUIKeys.end() ||
                      modelUIKeys.find(key.c_str()) != modelUIKeys.end() ||
                      qolKeys.find(key.c_str()) != qolKeys.end() ||
                      screenKeys.find(key.c_str()) != screenKeys.end();
    toggle->setVisible(!subToggles);
  }

  update();
}
