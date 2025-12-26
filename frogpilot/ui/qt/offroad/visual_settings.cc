#include "frogpilot/ui/qt/offroad/visual_settings.h"

FrogPilotVisualsPanel::FrogPilotVisualsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *visualsLayout = new QStackedLayout();
  addItem(visualsLayout);

  FrogPilotListWidget *visualsList = new FrogPilotListWidget(this);

  ScrollView *visualsPanel = new ScrollView(visualsList, this);

  visualsLayout->addWidget(visualsPanel);

  FrogPilotListWidget *advancedCustomList = new FrogPilotListWidget(this);
  FrogPilotListWidget *customUIList = new FrogPilotListWidget(this);
  FrogPilotListWidget *developerMetricList = new FrogPilotListWidget(this);
  FrogPilotListWidget *developerSidebarList = new FrogPilotListWidget(this);
  FrogPilotListWidget *developerUIList = new FrogPilotListWidget(this);
  FrogPilotListWidget *developerWidgetList = new FrogPilotListWidget(this);
  FrogPilotListWidget *modelUIList = new FrogPilotListWidget(this);
  FrogPilotListWidget *navigationUIList = new FrogPilotListWidget(this);
  FrogPilotListWidget *qualityOfLifeList = new FrogPilotListWidget(this);

  ScrollView *advancedCustomPanel = new ScrollView(advancedCustomList, this);
  ScrollView *customUIPanel = new ScrollView(customUIList, this);
  ScrollView *developerMetricPanel = new ScrollView(developerMetricList, this);
  ScrollView *developerSidebarPanel = new ScrollView(developerSidebarList, this);
  ScrollView *developerUIPanel = new ScrollView(developerUIList, this);
  ScrollView *developerWidgetPanel = new ScrollView(developerWidgetList, this);
  ScrollView *modelUIPanel = new ScrollView(modelUIList, this);
  ScrollView *navigationUIPanel = new ScrollView(navigationUIList, this);
  ScrollView *qualityOfLifePanel = new ScrollView(qualityOfLifeList, this);

  visualsLayout->addWidget(advancedCustomPanel);
  visualsLayout->addWidget(customUIPanel);
  visualsLayout->addWidget(developerMetricPanel);
  visualsLayout->addWidget(developerSidebarPanel);
  visualsLayout->addWidget(developerUIPanel);
  visualsLayout->addWidget(developerWidgetPanel);
  visualsLayout->addWidget(modelUIPanel);
  visualsLayout->addWidget(navigationUIPanel);
  visualsLayout->addWidget(qualityOfLifePanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> visualToggles {
    {"AdvancedCustomUI", tr("進階 UI 控制"), tr("<b>進階視覺設定</b> 用以微調駕駛畫面外觀。"), "../../frogpilot/assets/toggle_icons/icon_advanced_device.png"},
    {"HideSpeed", tr("隱藏當前速度"), tr("<b>從駕駛畫面隱藏當前速度</b>。"), ""},
    {"HideLeadMarker", tr("隱藏前車標記"), tr("<b>從駕駛畫面隱藏前車標記</b>。"), ""},
    {"HideMapIcon", tr("隱藏地圖設定按鈕"), tr("<b>從駕駛畫面隱藏地圖或地圖設定按鈕</b>。"), ""},
    {"HideMaxSpeed", tr("隱藏最高速度"), tr("<b>從駕駛畫面隱藏最高速顯示</b>。"), ""},
    {"HideAlerts", tr("隱藏非關鍵警示"), tr("<b>從駕駛畫面隱藏非關鍵警示</b>。"), ""},
    {"HideSpeedLimit", tr("隱藏速限標示"), tr("<b>從駕駛畫面隱藏路邊速限</b>。"), ""},
    {"WheelSpeed", tr("使用輪速"), tr("<b>使用車輛的輪速</b> 取代儀表速度。這只是視覺顯示，不會影響 openpilot 的行車行為！"), ""},

    {"DeveloperUI", tr("開發者介面"), tr("<b>顯示 openpilot 內部運作的詳細資訊。</b>"), "../assets/offroad/icon_shell.png"},
    {"AdjacentPathMetrics", tr("鄰道寬度"), tr("<b>顯示鄰側車道的寬度</b>。"), ""},
    {"DeveloperMetrics", tr("開發者度量"), tr("<b>性能資料、感測器讀數與系統度量</b>，用於偵錯與優化 openpilot。"), ""},
    {"BorderMetrics", tr("邊框度量"), tr("<b>在畫面邊框顯示狀態。</b><br><br><b>盲點</b>：當有車輛在盲點時邊框會變紅<br><b>轉向力矩</b>：邊框會依使用的轉向力矩從綠到紅變化<br><b>方向燈</b>：方向燈開啟時邊框會閃爍黃色"), ""},
    {"LeadInfo", tr("前車資訊"), tr("<b>在標記下方顯示每輛被追蹤車輛的距離與速度</b>。"), ""},
    {"FPSCounter", tr("FPS 顯示"), tr("<b>在駕駛畫面底部顯示每秒影格數 (FPS)</b>。"), ""},
    {"NumericalTemp", tr("數值溫度計"), tr("<b>在側欄顯示數值溫度</b>，取代狀態標籤。"), ""},
    {"SidebarMetrics", tr("側欄度量"), tr("<b>在側欄顯示系統資訊</b>（CPU、GPU、記憶體使用、IP 位址、儲存空間）。"), ""},
    {"UseSI", tr("使用國際單位制"), tr("<b>使用「國際單位制 (SI)」</b> 顯示測量值。"), ""},
    {"DeveloperSidebar", tr("開發者側欄"), tr("<b>在畫面右側的專用側欄顯示偵錯資訊與度量</b>。"), ""},
    {"DeveloperSidebarMetric1", tr("度量 #1"), tr("<b>選擇顯示在第一個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperSidebarMetric2", tr("度量 #2"), tr("<b>選擇顯示在第二個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperSidebarMetric3", tr("度量 #3"), tr("<b>選擇顯示在第三個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperSidebarMetric4", tr("度量 #4"), tr("<b>選擇顯示在第四個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperSidebarMetric5", tr("度量 #5"), tr("<b>選擇顯示在第五個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperSidebarMetric6", tr("度量 #6"), tr("<b>選擇顯示在第六個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperSidebarMetric7", tr("度量 #7"), tr("<b>選擇顯示在第七個「開發者側欄」元件中的度量。</b>"), ""},
    {"DeveloperWidgets", tr("開發者小工具"), tr("<b>在駕駛畫面上顯示偵錯視覺、內部狀態和模型預測的覆蓋圖層</b>。"), ""},
    {"AdjacentLeadsUI", tr("鄰車追蹤"), tr("<b>在當前行駛路徑左右顯示車輛雷達偵測到的鄰側前車</b>。"), ""},
    {"ShowStoppingPoint", tr("模型停車點"), tr("<b>顯示模型預計停車的位置（停止標記）</b>。"), ""},
    {"RadarTracksUI", tr("雷達追蹤點"), tr("<b>顯示車用雷達產生的所有雷達點</b>。"), ""},

    {"CustomUI", tr("駕駛畫面小工具"), tr("<b>用於駕駛畫面的自訂 FrogPilot 小工具</b>。"), "../assets/offroad/icon_road.png"},
    {"AccelerationPath", tr("加速度路徑"), tr("<b>依據規劃的加減速為行駛路徑著色</b>。"), ""},
    {"AdjacentPath", tr("鄰道"), tr("<b>顯示左右車道的行駛路徑</b>。"), ""},
    {"BlindSpotPath", tr("盲點路徑"), tr("<b>當車輛位​​於該車道盲點時顯示紅色路徑。</b>"), ""},
    {"Compass", tr("Compass"), tr("使用簡單的屏幕指南針<b>顯示當前行駛方向</b>。"), ""},
    {"OnroadDistanceButton", tr("駕駛個性按鈕"), tr("通過駕駛屏幕小部件<b>控制和查看當前駕駛個性</b>。"), ""},
    {"PedalsOnUI", tr("油門/制動踏板指示器"), tr("<b>屏幕上的油門和製動指示器。</b><br><br><b>動態</b>：不透明度根據 openpilot 加速或製動的程度而變化<br><b>靜態</b>：活動時滿，不活動時暗"), ""},
    {"RotatingWheel", tr("旋轉方向盤"), tr("用實體方向盤<b>旋轉駕駛屏幕輪</b>。"), ""},

    {"ModelUI", tr("模型介面"), tr("<b>駕駛路徑、車道線、路徑邊緣與道路邊緣的模型視覺化</b>。"), "../../frogpilot/assets/toggle_icons/icon_road.png"},
    {"DynamicPathWidth", tr("動態路徑寬度"), tr("<b>依據接管狀態調整路徑寬度</b><br><br><b>完全接管</b>：100%<br><b>側向恆開</b>：75%<br><b>未接管</b>：50%"), ""},
    {"LaneLinesWidth", tr("車道線寬度"), tr("<b>設定車道線厚度。</b><br><br>預設符合 MUTCD 車道線 4 吋 標準。"), ""},
    {"PathEdgeWidth", tr("路徑邊緣寬度"), tr("<b>設定行駛路徑邊緣寬度</b>，用以表示不同的行駛模式與狀態。<br><br>預設為總路徑寬度的 20%。<br><br>顏色對照：<br><br>- <b>藍色</b>：導航<br>- <b>淺藍</b>：側向恆開<br>- <b>綠色</b>：預設<br>- <b>橙色</b>：實驗模式<br>- <b>紅色</b>：交通模式<br>- <b>黃色</b>：條件性實驗模式被覆蓋"), ""},
    {"PathWidth", tr("路徑寬度"), tr("<b>設定行駛路徑寬度。</b><br><br>預設（6.1 英尺）與 2019 Lexus ES 350 寬度相符。"), ""},
    {"RoadEdgesWidth", tr("道路邊緣寬度"), tr("<b>設定道路邊緣厚度。</b><br><br>預設為 MUTCD 車道線 4 吋 標準的一半。"), ""},
    {"UnlimitedLength", tr("\"無限\" 道路 UI"), tr("<b>將行駛路徑、車道線和道路邊緣延伸至模型能看到的最遠範圍</b>。"), ""},

    {"NavigationUI", tr("導航小工具"), tr("<b>地圖樣式、速限與其他導航小工具</b>。"), "../../frogpilot/assets/toggle_icons/icon_map.png"},
    {"BigMap", tr("擴大地圖顯示"), tr("<b>放大地圖</b>，方便閱讀導航資訊。"), ""},
    {"MapStyle", tr("地圖樣式"), tr("<b>為「Navigate on openpilot」(NOO) 選擇地圖樣式</b>：<br><br><b>Stock openpilot</b>：comma.ai 預設樣式<br><b>FrogPilot</b>：官方 FrogPilot 地圖樣式<br><b>Mapbox Streets</b>：以街道為主的標準檢視<br><b>Mapbox Outdoors</b>：強調戶外與地形特徵<br><b>Mapbox Light</b>：極簡且明亮主題<br><b>Mapbox Dark</b>：極簡且深色主題<br><b>Mapbox Navigation Day</b>：最佳化的日間導航顯示<br><b>Mapbox Navigation Night</b>：最佳化的夜間導航顯示<br><b>Mapbox Satellite</b>：衛星影像<br><b>Mapbox Satellite Streets</b>：衛星影像與街道標籤混合<br><b>Mapbox Traffic Night</b>：強調路況的深色主題<br><b>Mike 的個人化樣式</b>：自訂混合衛星檢視"), ""},
    {"RoadNameUI", tr("路名"), tr("<b>在駕駛畫面底部顯示路名</b>，使用「OpenStreetMap (OSM)」資料。"), ""},
    {"ShowSpeedLimits", tr("顯示速限"), tr("<b>在駕駛畫面左上角顯示速限</b>，使用車輛儀錶板（若支援）與「OpenStreetMap (OSM)」資料。"), ""},
    {"SLCMapboxFiller", tr("從 Mapbox 顯示速限"), tr("<b>在沒有其他來源時使用 Mapbox 的速限資料</b>。"), ""},
    {"UseVienna", tr("使用維也納樣式速限標誌"), tr("<b>顯示維也納風格（EU）速限標誌</b>，替代 MUTCD（US）樣式。"), ""},

    {"QOLVisuals", tr("體驗優化"), tr("<b>雜項視覺調整</b>，用以微調駕駛畫面外觀。"), "../../frogpilot/assets/toggle_icons/icon_quality_of_life.png"},
    {"CameraView", tr("相機檢視"), tr("<b>選擇啟用的相機檢視。</b> 這只是視覺變更，不會影響 openpilot 的行車。"), ""},
    {"DriverCamera", tr("倒車時顯示駕駛側相機"), tr("<b>在車輛倒車時顯示駕駛側相機影像</b>。"), ""},
    {"SimpleDashServer", tr("Simple Dash Server"), tr("<b>Enable/Disable the Simple Dash web server.</b><br><br>When disabled, the server won't start - useful when using the native NavDashN app that connects directly to WebRTC.<br><br>Server runs on port 8000."), ""},
    {"SimpleDashTheme", tr("Simple Dash Theme"), tr("<b>Select the Simple Dash theme.</b><br><br>Simple Dash is a web-based dashboard that can be accessed at port 8000."), ""},
    {"StoppedTimer", tr("停車計時"), tr("<b>停車時顯示計時器</b>，取代目前車速，顯示停車時長。"), ""}
  };

  for (const auto &[param, title, desc, icon] : visualToggles) {
    AbstractControl *visualToggle;

    if (param == "AdvancedCustomUI") {
      FrogPilotManageControl *advancedCustomUIToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(advancedCustomUIToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, advancedCustomPanel]() {
        visualsLayout->setCurrentWidget(advancedCustomPanel);
      });
      visualToggle = advancedCustomUIToggle;
    } else if (param == "HideMapIcon") {
      std::vector<QString> mapIconToggles{"HideMap"};
      std::vector<QString> mapIconToggleNames{tr("隱藏地圖")};
      visualToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, mapIconToggles, mapIconToggleNames);

    } else if (param == "DeveloperUI") {
      FrogPilotManageControl *developerUIToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(developerUIToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, developerUIPanel]() {
        visualsLayout->setCurrentWidget(developerUIPanel);
      });
      visualToggle = developerUIToggle;
    } else if (param == "DeveloperMetrics") {
      FrogPilotManageControl *developerMetricsToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(developerMetricsToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, developerMetricPanel, this]() {
        openSubSubPanel();

        visualsLayout->setCurrentWidget(developerMetricPanel);

        developerUIOpen = true;
      });
      visualToggle = developerMetricsToggle;
    } else if (param == "BorderMetrics") {
      std::vector<QString> borderToggles{"BlindSpotMetrics", "ShowSteering", "SignalMetrics"};
      std::vector<QString> borderToggleNames{tr("盲點"), tr("轉向力矩"), tr("方向燈")};
      borderMetricsButton = new FrogPilotButtonToggleControl(param, title, desc, icon, borderToggles, borderToggleNames);
      visualToggle = borderMetricsButton;
    } else if (param == "NumericalTemp") {
      std::vector<QString> temperatureToggles{"Fahrenheit"};
      std::vector<QString> temperatureToggleNames{tr("華氏")};
      visualToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, temperatureToggles, temperatureToggleNames);
    } else if (param == "SidebarMetrics") {
      sidebarMetricsToggles = {"ShowCPU", "ShowGPU", "ShowIP", "ShowMemoryUsage", "ShowStorageLeft", "ShowStorageUsed"};
      std::vector<QString> sidebarMetricsToggleNames{tr("CPU"), tr("GPU"), tr("IP"), tr("記憶體"), tr("SSD 剩餘"), tr("SSD 使用量")};
      sidebarMetricsToggle = new FrogPilotButtonsControl(title, desc, icon, sidebarMetricsToggleNames, true, false, 150);
      for (int i = 0; i < sidebarMetricsToggles.size(); ++i) {
        if (params.getBool(sidebarMetricsToggles[i].toStdString())) {
          sidebarMetricsToggle->setCheckedButton(i);
        }
      }
      QObject::connect(sidebarMetricsToggle, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        params.putBool(sidebarMetricsToggles[id].toStdString(), !params.getBool(sidebarMetricsToggles[id].toStdString()));

        if (id == 0) {
          params.putBool("ShowGPU", false);
        } else if (id == 1) {
          params.putBool("ShowCPU", false);
        } else if (id == 3) {
          params.putBool("ShowStorageLeft", false);
          params.putBool("ShowStorageUsed", false);
        } else if (id == 4) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageUsed", false);
        } else if (id == 5) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageLeft", false);
        }

        sidebarMetricsToggle->clearCheckedButtons();
        for (int i = 0; i < sidebarMetricsToggles.size(); ++i) {
          if (params.getBool(sidebarMetricsToggles[i].toStdString())) {
            sidebarMetricsToggle->setCheckedButton(i);
          }
        }
      });
      visualToggle = sidebarMetricsToggle;
    } else if (param == "DeveloperSidebar") {
      FrogPilotManageControl *developerSidebarToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(developerSidebarToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, developerSidebarPanel, this]() {
        openSubSubPanel();

        visualsLayout->setCurrentWidget(developerSidebarPanel);

        developerUIOpen = true;
      });
      visualToggle = developerSidebarToggle;
    } else if (developerSidebarKeys.contains(param)) {
      QMap<int, QString> developerSidebarMetricOptions {
        {0，tr（“無”）}，
        {1, tr("加速度：當前")},
        {2, tr("加速度：最大")},
        {3, tr("自動調諧：執行器延遲")},
        {4, tr("自動調諧：摩擦力")},
        {5, tr("自動調諧：橫向加速度")},
        {6, tr("自動調諧：轉向比")},
        {7, tr("自動調諧：剛度係數")},
        {8，tr（“參與％：橫向”）}，
        {9, tr("接合百分比：縱向")},
        {10, tr("橫向控制：轉向角度")},
        {11, tr("橫向控制：使用的扭矩%")},
        {12, tr("縱向控制：執行器加速度輸出")},
        {13, tr("縱向 MPC：危險因素")},
        {14, tr("縱向 MPC 加加速度：加速度")},
        {15, tr("縱向 MPC 加加速度：危險區域")},
        {16, tr("縱向 MPC 加加速度：速度控制")},
      };

      ButtonControl *metricToggle = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(metricToggle, &ButtonControl::clicked, [metricToggle, key = param, developerSidebarMetricOptions, this]() mutable {
        QString current = developerSidebarMetricOptions.value(params.getInt(key.toStdString()), tr("None"));
        QString selection = MultiOptionDialog::getSelection(tr("選擇欲顯示的度量"), developerSidebarMetricOptions.values(), current, this);

        if (!selection.isEmpty()) {
          int selectedMetric = developerSidebarMetricOptions.key(selection);

          params.putInt(key.toStdString(), selectedMetric);

          metricToggle->setValue(selection);
        }
      });
      metricToggle->setValue(developerSidebarMetricOptions.value(params.getInt(param.toStdString()), tr("無")));
      visualToggle = metricToggle;
    } else if (param == "DeveloperWidgets") {
      FrogPilotManageControl *developerWidgetsToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(developerWidgetsToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, developerWidgetPanel, this]() {
        openSubSubPanel();

        visualsLayout->setCurrentWidget(developerWidgetPanel);

        developerUIOpen = true;
      });
      visualToggle = developerWidgetsToggle;
    } else if (param == "ShowStoppingPoint") {
      std::vector<QString> stoppingPointToggles{"ShowStoppingPointMetrics"};
      std::vector<QString> stoppingPointToggleNames{tr("顯示距離")};
      visualToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, stoppingPointToggles, stoppingPointToggleNames);

    } else if (param == "CustomUI") {
      FrogPilotManageControl *customUIToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(customUIToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, customUIPanel]() {
        visualsLayout->setCurrentWidget(customUIPanel);
      });
      visualToggle = customUIToggle;
    } else if (param == "PedalsOnUI") {
      std::vector<QString> pedalsToggles{"DynamicPedalsOnUI", "StaticPedalsOnUI"};
      std::vector<QString> pedalsToggleNames{tr("動態"), tr("靜態")};
      FrogPilotButtonToggleControl *pedalsToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, pedalsToggles, pedalsToggleNames, true);
      QObject::connect(pedalsToggle, &FrogPilotButtonToggleControl::buttonClicked, [this](int id) {
        if (id == 0) {
          params.putBool("StaticPedalsOnUI", false);
        } else if (id == 1) {
          params.putBool("DynamicPedalsOnUI", false);
        }
      });
      visualToggle = pedalsToggle;

    } else if (param == "ModelUI") {
      FrogPilotManageControl *modelUIToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(modelUIToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, modelUIPanel]() {
        visualsLayout->setCurrentWidget(modelUIPanel);
      });
      visualToggle = modelUIToggle;
    } else if (param == "LaneLinesWidth" || param == "RoadEdgesWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 24, tr(" 英寸"));
    } else if (param == "PathEdgeWidth") {
      std::map<float, QString> pathEdgeLabels;
      for (int i = 0; i <= 100; ++i) {
        pathEdgeLabels[i] = i == 0 ? tr("關閉") : QString::number(i) + "%";
      }
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, QString(), pathEdgeLabels);
    } else if (param == "PathWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, tr(" 英尺"), std::map<float, QString>(), 0.1);

    } else if (param == "NavigationUI") {
      FrogPilotManageControl *navigationUIToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(navigationUIToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, navigationUIPanel]() {
        visualsLayout->setCurrentWidget(navigationUIPanel);
      });
      visualToggle = navigationUIToggle;
    } else if (param == "BigMap") {
      std::vector<QString> mapToggles{"FullMap"};
      std::vector<QString> mapToggleNames{tr("全地圖")};
      visualToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, mapToggles, mapToggleNames);
    } else if (param == "MapStyle") {
      QMap<int, QString> styleMap {
        {0, tr("Stock openpilot")},
        {1, tr("FrogPilot")},
        {2, tr("Mapbox Streets")},
        {3, tr("Mapbox Outdoors")},
        {4, tr("Mapbox Light")},
        {5, tr("Mapbox Dark")},
        {6, tr("Mapbox Navigation Day")},
        {7, tr("Mapbox Navigation Night")},
        {8, tr("Mapbox Satellite")},
        {9, tr("Mapbox Satellite Streets")},
        {10, tr("Mapbox Traffic Night")},
        {11, tr("Mike's Personalized Style")}
      };

      ButtonControl *mapStyleButton = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(mapStyleButton, &ButtonControl::clicked, [mapStyleButton, styleMap, this]() {
        QString selection = MultiOptionDialog::getSelection(tr("選擇地圖樣式"), styleMap.values(), "", this);
        if (!selection.isEmpty()) {
          int selectedStyle = styleMap.key(selection);

          params.putInt("MapStyle", selectedStyle);

          mapStyleButton->setValue(selection);
        }
      });
      int currentStyle = params.getInt("MapStyle");
      mapStyleButton->setValue(styleMap[currentStyle]);

      visualToggle = mapStyleButton;

    } else if (param == "QOLVisuals") {
      FrogPilotManageControl *qolToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(qolToggle, &FrogPilotManageControl::manageButtonClicked, [visualsLayout, qualityOfLifePanel]() {
        visualsLayout->setCurrentWidget(qualityOfLifePanel);
      });
      visualToggle = qolToggle;
    } else if (param == "CameraView") {
      std::vector<QString> cameraOptions{tr("自動"), tr("駕駛"), tr("標準"), tr("廣角")};
      ButtonParamControl *cameraSelection = new ButtonParamControl(param, title, desc, icon, cameraOptions);
      visualToggle = cameraSelection;

      } else if (param == "SimpleDashTheme") {
      // Simple Dash 主題選擇器 - 硬編碼主題列表（新增主題時需更新此處）
      QStringList themeList{"HFOP-GTI-NAVDASH", "SIMPLE-DASH"};

      ButtonControl *themeButton = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(themeButton, &ButtonControl::clicked, [themeButton, themeList, this]() {
        QString currentTheme = QString::fromStdString(params.get("SimpleDashTheme"));
        if (currentTheme.isEmpty()) {
          currentTheme = "HFOP-GTI-NAVDASH";
        }
        QString selection = MultiOptionDialog::getSelection(tr("選擇 Simple Dash 主題"), themeList, currentTheme, this);
        if (!selection.isEmpty()) {
          params.put("SimpleDashTheme", selection.toStdString());
          themeButton->setValue(selection);
        }
      });

      QString currentTheme = QString::fromStdString(params.get("SimpleDashTheme"));
      themeButton->setValue(currentTheme.isEmpty() ? "HFOP-GTI-NAVDASH" : currentTheme);
      visualToggle = themeButton;

    } else {
      visualToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = visualToggle;

    if (advancedCustomOnroadUIKeys.contains(param)) {
      advancedCustomList->addItem(visualToggle);
    } else if (customOnroadUIKeys.contains(param)) {
      customUIList->addItem(visualToggle);
    } else if (developerMetricKeys.contains(param)) {
      developerMetricList->addItem(visualToggle);
    } else if (developerSidebarKeys.contains(param)) {
      developerSidebarList->addItem(visualToggle);
    } else if (developerUIKeys.contains(param)) {
      developerUIList->addItem(visualToggle);
    } else if (developerWidgetKeys.contains(param)) {
      developerWidgetList->addItem(visualToggle);
    } else if (modelUIKeys.contains(param)) {
      modelUIList->addItem(visualToggle);
    } else if (navigationUIKeys.contains(param)) {
      navigationUIList->addItem(visualToggle);
    } else if (qualityOfLifeKeys.contains(param)) {
      qualityOfLifeList->addItem(visualToggle);
    } else {
      visualsList->addItem(visualToggle);

      parentKeys.insert(param);
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(visualToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(visualToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(visualToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QSet<QString> forceUpdateKeys = {"HideLeadMarker", "ShowSpeedLimits", "SimpleDashServer"};
  for (const QString &key : forceUpdateKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, this, &FrogPilotVisualsPanel::updateToggles);
  }

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [visualsLayout, visualsPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    visualsLayout->setCurrentWidget(visualsPanel);
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubSubPanel, [visualsLayout, developerUIPanel, this]() {
    openDescriptions(forceOpenDescriptions, toggles);

    if (developerUIOpen) {
      visualsLayout->setCurrentWidget(developerUIPanel);

      developerUIOpen = false;
    }
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, this, &FrogPilotVisualsPanel::updateMetric);
}

void FrogPilotVisualsPanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  for (int i = 0; i < sidebarMetricsToggles.size(); ++i) {
    if (params.getBool(sidebarMetricsToggles[i].toStdString())) {
      sidebarMetricsToggle->setCheckedButton(i);
    }
  }

  updateToggles();
}

void FrogPilotVisualsPanel::updateMetric(bool metric, bool bootRun) {
  static bool previousMetric;
  if (metric != previousMetric && !bootRun) {
    double distanceConversion = metric ? FOOT_TO_METER : METER_TO_FOOT;
    double smallDistanceConversion = metric ? INCH_TO_CM : CM_TO_INCH;

    params.putIntNonBlocking("LaneLinesWidth", params.getInt("LaneLinesWidth") * smallDistanceConversion);
    params.putIntNonBlocking("RoadEdgesWidth", params.getInt("RoadEdgesWidth") * smallDistanceConversion);

    params.putFloatNonBlocking("PathWidth", params.getFloat("PathWidth") * distanceConversion);
  }
  previousMetric = metric;

  static std::map<float, QString> imperialDistanceLabels;
  static std::map<float, QString> imperialSmallDistanceLabels;
  static std::map<float, QString> metricDistanceLabels;
  static std::map<float, QString> metricSmallDistanceLabels;

  static bool labelsInitialized = false;
  if (!labelsInitialized) {
    for (int i = 0; i <= 10; ++i) {
      imperialDistanceLabels[i] = i == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" 英尺") : QString::number(i) + tr(" 英尺");
    }

    for (int i = 0; i <= 24; ++i) {
      imperialSmallDistanceLabels[i] = i == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" 英吋") : QString::number(i) + tr(" 英吋");
    }

    for (float i = 0.0f; i <= 3.0f; i += 0.1f) {
      metricDistanceLabels[i] = i == 0.0f ? tr("關閉") : i == 1.0 ? QString::number(i) + tr(" 公尺") : QString::number(i, 'f', 1) + tr(" 公尺");
    }

    for (int i = 0; i <= 60; ++i) {
      metricSmallDistanceLabels[i] = i == 0 ? tr("關閉") : i == 1 ? QString::number(i) + tr(" 公分") : QString::number(i) + tr(" 公分");
    }

    labelsInitialized = true;
  }

  FrogPilotParamValueControl *laneLinesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneLinesWidth"]);
  FrogPilotParamValueControl *pathWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["PathWidth"]);
  FrogPilotParamValueControl *roadEdgesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["RoadEdgesWidth"]);

  if (metric) {
    laneLinesWidthToggle->setDescription(tr("<b>設定車道線厚度。</b><br><br>預設符合 MUTCD 車道線 10 公分 標準。"));
    pathWidthToggle->setDescription(tr("<b>設定行駛路徑寬度。</b><br><br>預設（1.9 公尺）與 2019 Lexus ES 350 寬度相符。"));
    roadEdgesWidthToggle->setDescription(tr("<b>設定道路邊緣厚度。</b><br><br>預設為 MUTCD 車道線 10 公分 標準的一半。"));

    laneLinesWidthToggle->updateControl(0, 60, metricSmallDistanceLabels);
    roadEdgesWidthToggle->updateControl(0, 60, metricSmallDistanceLabels);

    pathWidthToggle->updateControl(0, 3, metricDistanceLabels);
  } else {
    laneLinesWidthToggle->setDescription(tr("<b>設定車道線厚度。</b><br><br>預設符合 MUTCD 車道線 4 英吋 標準。"));
    pathWidthToggle->setDescription(tr("<b>設定行駛路徑寬度。</b><br><br>預設（6.1 英尺）與 2019 Lexus ES 350 寬度相符。"));
    roadEdgesWidthToggle->setDescription(tr("<b>設定道路邊緣厚度。</b><br><br>預設為 MUTCD 車道線 4 英吋 標準的一半。"));

    laneLinesWidthToggle->updateControl(0, 24, imperialSmallDistanceLabels);
    roadEdgesWidthToggle->updateControl(0, 24, imperialSmallDistanceLabels);

    pathWidthToggle->updateControl(0, 10, imperialDistanceLabels);
  }
}

void FrogPilotVisualsPanel::updateToggles() {
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

    if (key == "AccelerationPath") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "AdjacentLeadsUI") {
      setVisible &= parent->hasRadar && !(params.getBool("AdvancedCustomUI") && params.getBool("HideLeadMarker"));
    }

    else if (key == "BlindSpotPath") {
      setVisible &= parent->hasBSM;
    }

    else if (key == "HideLeadMarker") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "LeadInfo") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "OnroadDistanceButton") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "PedalsOnUI") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "RadarTracksUI") {
      setVisible &= parent->hasRadar;
    }

    else if (key == "ShowSpeedLimits") {
      setVisible &= !params.getBool("SpeedLimitController") || !parent->hasOpenpilotLongitudinal;
    }

    else if (key == "ShowStoppingPoint") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "SLCMapboxFiller") {
      setVisible &= params.getBool("ShowSpeedLimits") && !(parent->hasOpenpilotLongitudinal && params.getBool("SpeedLimitController"));
      setVisible &= !params.get("MapboxSecretKey").empty();
    }

    else if (key == "SimpleDashTheme") {
      setVisible &= params.getBool("SimpleDashServer");
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (advancedCustomOnroadUIKeys.contains(key)) {
        toggles["AdvancedCustomUI"]->setVisible(true);
      } else if (customOnroadUIKeys.contains(key)) {
        toggles["CustomUI"]->setVisible(true);
      } else if (developerMetricKeys.contains(key)) {
        toggles["DeveloperMetrics"]->setVisible(true);
      } else if (developerUIKeys.contains(key)) {
        toggles["DeveloperUI"]->setVisible(true);
      } else if (developerWidgetKeys.contains(key)) {
        toggles["DeveloperWidgets"]->setVisible(true);
      } else if (modelUIKeys.contains(key)) {
        toggles["ModelUI"]->setVisible(true);
      } else if (navigationUIKeys.contains(key)) {
        toggles["NavigationUI"]->setVisible(true);
      } else if (qualityOfLifeKeys.contains(key)) {
        toggles["QOLVisuals"]->setVisible(true);
      }
    }
  }

  borderMetricsButton->setVisibleButton(0, parent->hasBSM);

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
