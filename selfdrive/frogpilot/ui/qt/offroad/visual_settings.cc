#include "selfdrive/frogpilot/ui/qt/offroad/visual_settings.h"

FrogPilotVisualsPanel::FrogPilotVisualsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> visualToggles {
    {"CustomUI", tr("自定義道路畫面"), tr("Custom定義自己喜歡的道路介面."), "../assets/offroad/icon_road.png"},
    {"Compass", tr("  羅盤"), tr("將指南針加入道路使用者介面."), ""},
    {"DynamicPathWidth", tr("  動態路徑寬度"), tr("Automatically adjust the width of the driving path display based on the current engagement state:\n\nFully engaged = 100%\nAlways On Lateral Active = 75%\nFully disengaged = 50%"), ""},
    {"PedalsOnUI", tr("  踏板"), tr("Pedal indicators in the onroad UI that change opacity based on the pressure applied."), ""},
    {"CustomPaths", tr("  路徑"), tr("Projected acceleration path, detected lanes, and vehicles in the blind spot."), ""},
    {"RoadNameUI", tr("  道路名稱"), tr("The current road name is displayed at the bottom of the screen using data from 'OpenStreetMap'."), ""},
    {"RotatingWheel", tr("  旋轉方向盤"), tr("The steering wheel in the onroad UI rotates along with your steering wheel movements."), ""},

    {"QOLVisuals", tr("Quality of Life Improvements"), tr("Miscellaneous visual focused features to improve your overall openpilot experience."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"BigMap", tr("Larger Map Display"), tr("A larger size of the map in the onroad UI for easier navigation readings."), ""},
    {"MapStyle", tr("Map Style"), tr("Custom map styles for the map used during navigation."), ""},
    {"StandbyMode", tr("Screen Standby Mode"), tr("The screen is turned off after it times out when driving, but it automatically wakes up if engagement state changes or important alerts occur."), ""},
    {"DriverCamera", tr("Show Driver Camera When In Reverse"), tr("The driver camera feed is displayed when the vehicle is in reverse."), ""},
    {"StoppedTimer", tr("Stopped Timer"), tr("A timer on the onroad UI to indicate how long the vehicle has been stopped."), ""}
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
        {12, tr("huifan's 導航(夜晚)")},
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
