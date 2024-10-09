#include "selfdrive/frogpilot/ui/qt/offroad/visual_settings.h"

FrogPilotVisualsPanel::FrogPilotVisualsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> visualToggles {
    {"CustomUI", tr("自定義道路畫面"), tr("Custom定義自己喜歡的道路介面."), "../assets/offroad/icon_road.png"},
    {"Compass", tr("  羅盤"), tr("將指南針加入道路使用者介面."), ""},
    {"DynamicPathWidth", tr("  動態路徑寬度"), tr("根據目前接合狀態自動調整行駛路徑顯示的寬度:\n\nFully engaged = 100%\nAlways On Lateral Active = 75%\nFully disengaged = 50%"), ""},
    {"PedalsOnUI", tr("  踏板"), tr("公路使用者介面中的踏板指示器可根據施加的壓力改變不透明度."), ""},
    {"CustomPaths", tr("  路徑"), tr("預計的加速路徑、偵測到的車道以及盲點中的車輛."), ""},
    {"RoadNameUI", tr("  道路名稱"), tr("目前道路名稱使用來自「OpenStreetMap」的資料顯示在螢幕底部'."), ""},
    {"RotatingWheel", tr("  旋轉方向盤"), tr("onroad UI 中的方向盤會隨著方向盤的移動而旋轉."), ""},

    {"QOLVisuals", tr("進階設定"), tr("整合各種視覺功能可改善您的駕駛體驗."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"BigMap", tr("全螢幕地圖顯示"), tr("道路使用者介面中的地圖尺寸更大，更易於導航閱讀."), ""},
    {"MapStyle", tr("地圖樣式"), tr("導航期間使用的地圖的自訂地圖樣式."), ""},
    {"StandbyMode", tr("螢幕待機模式"), tr("開車時超時後螢幕會關閉，但如果交戰狀態發生變化或發生重要警報，螢幕會自動喚醒."), ""},
    {"DriverCamera", tr("倒車時顯示駕駛員攝影機"), tr("車輛倒車時顯示駕駛攝影機畫面."), ""},
    {"StoppedTimer", tr("停止計時器"), tr("道路使用者介面上的計時器可指示車輛停止了多長時間."), ""}
  };

  for (const auto &[param, title, desc, icon] : visualToggles) {
    AbstractControl *visualToggle;

    if (param == "CustomUI") {
      FrogPilotParamManageControl *customUIToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(customUIToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        customPathsBtn->setVisibleButton(0, hasBSM);

        std::set<QString> modifiedCustomOnroadUIKeys = customOnroadUIKeys;

        showToggles(modifiedCustomOnroadUIKeys);
      });
      visualToggle = customUIToggle;
    } else if (param == "CustomPaths") {
      std::vector<QString> pathToggles{"AccelerationPath", "AdjacentPath", "BlindSpotPath"};
      std::vector<QString> pathToggleNames{tr("加速"), tr("Adjacent"), tr("盲點")};
      customPathsBtn = new FrogPilotButtonToggleControl(param, title, desc, pathToggles, pathToggleNames);
      visualToggle = customPathsBtn;
    } else if (param == "PedalsOnUI") {
      std::vector<QString> pedalsToggles{"DynamicPedalsOnUI", "StaticPedalsOnUI"};
      std::vector<QString> pedalsToggleNames{tr("動態的"), tr("靜止的")};
      FrogPilotButtonToggleControl *pedalsToggle = new FrogPilotButtonToggleControl(param, title, desc, pedalsToggles, pedalsToggleNames, true);
      QObject::connect(pedalsToggle, &FrogPilotButtonToggleControl::buttonClicked, [this](int index) {
        if (index == 0) {
          params.putBool("StaticPedalsOnUI", false);
        } else if (index == 1) {
          params.putBool("DynamicPedalsOnUI", false);
        }
      });
      visualToggle = pedalsToggle;

    } else if (param == "QOLVisuals") {
      FrogPilotParamManageControl *qolToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(qolToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(qolKeys);
      });
      visualToggle = qolToggle;
    } else if (param == "BigMap") {
      std::vector<QString> mapToggles{"FullMap"};
      std::vector<QString> mapToggleNames{tr("全螢幕")};
      visualToggle = new FrogPilotButtonToggleControl(param, title, desc, mapToggles, mapToggleNames);
    } else if (param == "MapStyle") {
      QMap<int, QString> styleMap = {
        {0, tr("Stock openpilot")},
        {1, tr("Mapbox Streets")},
        {2, tr("Mapbox Outdoors")},
        {3, tr("Mapbox Light")},
        {4, tr("Mapbox Dark")},
        {5, tr("Mapbox Satellite")},
        {6, tr("Mapbox Satellite Streets")},
        {7, tr("Mapbox Navigation Day")},
        {8, tr("Mapbox Navigation Night")},
        {9, tr("Mapbox Traffic Night")},
        {10, tr("mike854's (Satellite hybrid)")},
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

    } else {
      visualToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(visualToggle);
    toggles[param] = visualToggle;

    makeConnections(visualToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(visualToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotVisualsPanel::openParentToggle);
    }

    QObject::connect(visualToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotVisualsPanel::hideToggles);
  QObject::connect(parent, &FrogPilotSettingsWindow::updateCarToggles, this, &FrogPilotVisualsPanel::updateCarToggles);
}

void FrogPilotVisualsPanel::updateCarToggles() {
  hasBSM = parent->hasBSM;

  hideToggles();
}

void FrogPilotVisualsPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotVisualsPanel::hideToggles() {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    bool subToggles = customOnroadUIKeys.find(key) != customOnroadUIKeys.end() ||
                      qolKeys.find(key) != qolKeys.end();

    toggle->setVisible(!subToggles);
  }

  setUpdatesEnabled(true);
  update();
}
