#include "selfdrive/frogpilot/ui/qt/offroad/advanced_visual_settings.h"

FrogPilotAdvancedVisualsPanel::FrogPilotAdvancedVisualsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> advancedToggles {
    {"AdvancedCustomUI", tr("進階 Onroad UI 小工具"), tr("Onroad UI 的進階使用者自訂."), "../frogpilot/assets/toggle_icons/icon_advanced_road.png"},
    {"CameraView", tr("鏡頭角度"), tr("道路 UI 的攝影機視圖。這純粹是視覺上的變化，不會影響 openpilot 的駕駛方式."), ""},
    {"ShowStoppingPoint", tr("顯示停止點"), tr("在螢幕上顯示 openpilot 偵測到潛在紅燈/停車標誌的影像."), ""},
    {"HideLeadMarker", tr("隱藏引導標記"), tr("在螢幕上隱藏前方車輛的標記."), ""},
    {"HideSpeed", tr("隱藏速度"), tr("隱藏道路使用者介面中的速度指示器。附加切換允許透過點擊速度本身來隱藏/顯示."), ""},
    {"HideUIElements", tr("隱藏使用者介面元素"), tr("在道路畫面上隱藏選定的 UI 元素."), ""},
    {"WheelSpeed", tr("使用輪速"), tr("在道路使用者介面中使用車輪速度而不是叢集速度."), ""},

    {"DeveloperUI", tr("開發者介面"), tr("顯示openpilot內部操作的詳細信息."), "../frogpilot/assets/toggle_icons/icon_advanced_device.png"},
    {"BorderMetrics", tr("邊界指標"), tr("駕駛時在螢幕邊緣顯示性能指標."), ""},
    {"FPSCounter", tr("FPS顯示"), tr("駕駛時在螢幕底部顯示「每秒影格數」(FPS)."), ""},
    {"LateralMetrics", tr("橫向指標"), tr("駕駛時在螢幕頂部顯示與轉向控制相關的指標."), ""},
    {"LongitudinalMetrics", tr("縱向指標"), tr("駕駛時在螢幕頂部顯示與加速、速度和所需跟隨距離相關的指標."), ""},
    {"NumericalTemp", tr("數位溫度計"), tr("在側邊欄中顯示準確的溫度讀數，而不是“良好”、“正常”或“高”等一般狀態標籤."), ""},
    {"SidebarMetrics", tr("側邊欄"), tr("在側邊欄中顯示 CPU、GPU、RAM 使用情況、IP 位址和儲存空間等系統資訊."), ""},
    {"UseSI", tr("使用國際單位制"), tr("使用「國際單位制」(SI) 顯示測量值."), ""},

    {"ModelUI", tr("使用者介面模型"), tr("自訂螢幕上的模型視覺化."), "../frogpilot/assets/toggle_icons/icon_advanced_calibration.png"},
    {"LaneLinesWidth", tr("車道線寬度"), tr("顯示幕上顯示的車道線有多粗.\n\n預設符合MUTCD標準4英寸."), ""},
    {"PathEdgeWidth", tr("路徑邊緣寬度"), tr("代表不同模式和狀態的行駛路徑邊緣的寬度.\n\n預設為 20% 總路徑寬度.\n\nColor Guide:\n- Blue: Navigation\n- Light Blue: 'Always On Lateral'\n- Green: Default\n- Orange: 'Experimental Mode'\n- Red: 'Traffic Mode'\n- Yellow: 'Conditional Experimental Mode' Overridden"), ""},
    {"PathWidth", tr("路徑寬度"), tr("螢幕上顯示的行駛路徑有多寬.\n\n預設值（6.1 英尺/1.9 公尺）與 2019 年雷克薩斯 ES 350 的寬度相符."), ""},
    {"RoadEdgesWidth", tr("道路邊緣寬度"), tr("顯示幕上顯示的道路邊緣有多厚.\n\n預設匹配 MUTCD 標準車道線寬度 4 英吋的一半."), ""},
    {"UnlimitedLength", tr("無限的道路使用者介面"), tr("將路徑、車道線和道路邊緣的顯示擴展到模型可以看到的範圍."), ""},
  };

  for (const auto &[param, title, desc, icon] : advancedToggles) {
    AbstractControl *advancedVisualToggle;

    if (param == "AdvancedCustomUI") {
      FrogPilotParamManageControl *advancedCustomUIToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(advancedCustomUIToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(advancedCustomOnroadUIKeys);
      });
      advancedVisualToggle = advancedCustomUIToggle;
    } else if (param == "CameraView") {
      std::vector<QString> cameraOptions{tr("自動"), tr("駕駛"), tr("標準"), tr("廣角")};
      ButtonParamControl *preferredCamera = new ButtonParamControl(param, title, desc, icon, cameraOptions);
      advancedVisualToggle = preferredCamera;
    } else if (param == "HideSpeed") {
      std::vector<QString> hideSpeedToggles{"HideSpeedUI"};
      std::vector<QString> hideSpeedToggleNames{tr("透過使用者介面控制")};
      advancedVisualToggle = new FrogPilotButtonToggleControl(param, title, desc, hideSpeedToggles, hideSpeedToggleNames);
    } else if (param == "HideUIElements") {
      std::vector<QString> uiElementsToggles{"HideAlerts", "HideMapIcon", "HideMaxSpeed"};
      std::vector<QString> uiElementsToggleNames{tr("警報"), tr("地圖圖示"), tr("最大速度")};
      advancedVisualToggle = new FrogPilotButtonToggleControl(param, title, desc, uiElementsToggles, uiElementsToggleNames);
    } else if (param == "ShowStoppingPoint") {
      std::vector<QString> stoppingPointToggles{"ShowStoppingPointMetrics"};
      std::vector<QString> stoppingPointToggleNames{tr("顯示距離")};
      advancedVisualToggle = new FrogPilotButtonToggleControl(param, title, desc, stoppingPointToggles, stoppingPointToggleNames);

    } else if (param == "DeveloperUI") {
      FrogPilotParamManageControl *developerUIToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(developerUIToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        borderMetricsBtn->setVisibleButton(0, hasBSM);
        lateralMetricsBtn->setVisibleButton(1, hasAutoTune);

        std::set<QString> modifiedDeveloperUIKeys = developerUIKeys;

        if (disableOpenpilotLongitudinal || !hasOpenpilotLongitudinal) {
          modifiedDeveloperUIKeys.erase("LongitudinalMetrics");
        }

        showToggles(modifiedDeveloperUIKeys);
      });
      advancedVisualToggle = developerUIToggle;
    } else if (param == "BorderMetrics") {
      std::vector<QString> borderToggles{"BlindSpotMetrics", "ShowSteering", "SignalMetrics"};
      std::vector<QString> borderToggleNames{tr("盲點"), tr("轉向扭矩"), tr("轉向訊號")};
      borderMetricsBtn = new FrogPilotButtonToggleControl(param, title, desc, borderToggles, borderToggleNames);
      advancedVisualToggle = borderMetricsBtn;
    } else if (param == "LateralMetrics") {
      std::vector<QString> lateralToggles{"AdjacentPathMetrics", "TuningInfo"};
      std::vector<QString> lateralToggleNames{tr("相鄰路徑距離"), tr("Auto Tune")};
      lateralMetricsBtn = new FrogPilotButtonToggleControl(param, title, desc, lateralToggles, lateralToggleNames);
      advancedVisualToggle = lateralMetricsBtn;
    } else if (param == "LongitudinalMetrics") {
      std::vector<QString> longitudinalToggles{"LeadInfo", "JerkInfo"};
      std::vector<QString> longitudinalToggleNames{tr("前車訊息"), tr("縱向加加速度")};
      advancedVisualToggle = new FrogPilotButtonToggleControl(param, title, desc, longitudinalToggles, longitudinalToggleNames);
    } else if (param == "NumericalTemp") {
      std::vector<QString> temperatureToggles{"Fahrenheit"};
      std::vector<QString> temperatureToggleNames{tr("華氏度")};
      advancedVisualToggle = new FrogPilotButtonToggleControl(param, title, desc, temperatureToggles, temperatureToggleNames);
    } else if (param == "SidebarMetrics") {
      std::vector<QString> sidebarMetricsToggles{"ShowCPU", "ShowGPU", "ShowIP", "ShowMemoryUsage", "ShowStorageLeft", "ShowStorageUsed"};
      std::vector<QString> sidebarMetricsToggleNames{tr("CPU"), tr("GPU"), tr("IP"), tr("RAM"), tr("SSD Left"), tr("SSD Used")};
      FrogPilotButtonToggleControl *sidebarMetricsToggle = new FrogPilotButtonToggleControl(param, title, desc, sidebarMetricsToggles, sidebarMetricsToggleNames, false, 150);
      QObject::connect(sidebarMetricsToggle, &FrogPilotButtonToggleControl::buttonClicked, [this](int index) {
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
      });
      advancedVisualToggle = sidebarMetricsToggle;

    } else if (param == "ModelUI") {
      FrogPilotParamManageControl *modelUIToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(modelUIToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        std::set<QString> modifiedModelUIKeysKeys = modelUIKeys;

        if (disableOpenpilotLongitudinal || !hasOpenpilotLongitudinal) {
          modifiedModelUIKeysKeys.erase("HideLeadMarker");
        }

        showToggles(modifiedModelUIKeysKeys);
      });
      advancedVisualToggle = modelUIToggle;
    } else if (param == "LaneLinesWidth" || param == "RoadEdgesWidth") {
      advancedVisualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 24, tr(" 英吋"));
    } else if (param == "PathEdgeWidth") {
      advancedVisualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, tr("%"));
    } else if (param == "PathWidth") {
      advancedVisualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, tr(" feet"), std::map<int, QString>(), 0.1);

    } else {
      advancedVisualToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(advancedVisualToggle);
    toggles[param] = advancedVisualToggle;

    makeConnections(advancedVisualToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(advancedVisualToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotAdvancedVisualsPanel::openParentToggle);
    }

    QObject::connect(advancedVisualToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotAdvancedVisualsPanel::hideToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateCarToggles, this, &FrogPilotAdvancedVisualsPanel::updateCarToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotAdvancedVisualsPanel::updateMetric);

  updateMetric();
}

void FrogPilotAdvancedVisualsPanel::updateCarToggles() {
  disableOpenpilotLongitudinal = parent->disableOpenpilotLongitudinal;
  hasAutoTune = parent->hasAutoTune;
  hasBSM = parent->hasBSM;
  hasOpenpilotLongitudinal = parent->hasOpenpilotLongitudinal;

  hideToggles();
}

void FrogPilotAdvancedVisualsPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double smallDistanceConversion = isMetric ? INCH_TO_CM : CM_TO_INCH;
    double distanceConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;

    params.putFloatNonBlocking("LaneLinesWidth", params.getFloat("LaneLinesWidth") * smallDistanceConversion);
    params.putFloatNonBlocking("RoadEdgesWidth", params.getFloat("RoadEdgesWidth") * smallDistanceConversion);

    params.putFloatNonBlocking("PathWidth", params.getFloat("PathWidth") * distanceConversion);
  }

  FrogPilotParamValueControl *laneLinesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneLinesWidth"]);
  FrogPilotParamValueControl *pathWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["PathWidth"]);
  FrogPilotParamValueControl *roadEdgesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["RoadEdgesWidth"]);

  if (isMetric) {
    laneLinesWidthToggle->setDescription(tr("調整顯示器上車道線的粗細程度.\n\n預設符合維也納標準 10 厘米."));
    roadEdgesWidthToggle->setDescription(tr("調整道路邊緣在顯示幕上顯示的厚度.\n\n預設值符合維也納標準 10 公分的一半."));

    laneLinesWidthToggle->updateControl(0, 60, tr(" 公分"));
    roadEdgesWidthToggle->updateControl(0, 60, tr(" 公分"));

    pathWidthToggle->updateControl(0, 3, tr(" 公尺"));
  } else {
    laneLinesWidthToggle->setDescription(tr("調整顯示器上車道線的粗細程度.\n\n預設符合MUTCD標準4英寸."));
    roadEdgesWidthToggle->setDescription(tr("調整道路邊緣在顯示幕上顯示的厚度.\n\n預設匹配 4 英寸 MUTCD 標準的一半."));

    laneLinesWidthToggle->updateControl(0, 24, tr(" 英吋"));
    roadEdgesWidthToggle->updateControl(0, 24, tr(" 英吋"));

    pathWidthToggle->updateControl(0, 10, tr(" feet"));
  }
}

void FrogPilotAdvancedVisualsPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotAdvancedVisualsPanel::hideToggles() {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    bool subToggles = advancedCustomOnroadUIKeys.find(key) != advancedCustomOnroadUIKeys.end() ||
                      developerUIKeys.find(key) != developerUIKeys.end() ||
                      modelUIKeys.find(key) != modelUIKeys.end();

    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}
