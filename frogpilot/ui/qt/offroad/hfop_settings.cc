#include "frogpilot/ui/qt/offroad/hfop_settings.h"

FrogPilotHFOPPanel::FrogPilotHFOPPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *hfopLayout = new QStackedLayout();
  addItem(hfopLayout);

  FrogPilotListWidget *FuelpriceList = new FrogPilotListWidget(this);
  FrogPilotListWidget *TrafficModelList = new FrogPilotListWidget(this);
  FrogPilotListWidget *VagSpeedList = new FrogPilotListWidget(this);
  FrogPilotListWidget *AutoACCPList = new FrogPilotListWidget(this);
  FrogPilotListWidget *RoadtypeList = new FrogPilotListWidget(this);
  FrogPilotListWidget *DooropenList = new FrogPilotListWidget(this);



  ScrollView *FuelpricePanel = new ScrollView(FuelpriceList, this);
  ScrollView *TrafficModePanel = new ScrollView(TrafficModelList, this);
  ScrollView *VagSpeedPanel = new ScrollView(VagSpeedList, this);
  ScrollView *AutoACCPanel = new ScrollView(AutoACCPList, this);
  ScrollView *RoadtypePanel = new ScrollView(RoadtypeList, this);
  ScrollView *DooropenPanel = new ScrollView(DooropenList, this);


  hfopLayout->addWidget(FuelpricePanel);
  hfopLayout->addWidget(TrafficModePanel);
  hfopLayout->addWidget(VagSpeedPanel);
  hfopLayout->addWidget(AutoACCPanel);
  hfopLayout->addWidget(RoadtypePanel);
  hfopLayout->addWidget(DooropenPanel);


  const std::vector<std::tuple<QString, QString, QString, QString>> hfopToggles {
    // {"HFOPinf", "  訊息框", "主畫面左下方顯示訊息狀態.", "../assets/offroad/icon_custom.png"},
    {"AutoACC", "自動啟動ACC", "啟用後會自動啟動ACC.", "../assets/offroad/icon_conditional.png"},
    {"AutoACCspeed", "  自動啟動ACC時速設定", "設定自動啟動ACC的時速條件設定.", ""},
    {"AutoACCCarAway", "  前車遠離啟動", "啟用後當前方車輛遠離會自動啟動ACC.", ""},
    {"AutoACCGreenLight", "  綠燈啟動", "啟用後當偵測到綠燈時會自動啟動ACC.", ""},

    {"Roadtype", "道路種類設定", "開啟後可依道路種類在特定條件下預設時速", "../assets/offroad/icon_road.png"},
    {"AutoRoadtype", "自動道路種類設定", "開啟後可自動依道路種類在特定條件下預設時速", ""},
    {"RoadtypeProfile", "道路種類設定", "開啟後可依道路種類在特定條件下預設時速", ""},

    {"ChangeLaneReminder", "  變換車道語音", "開啟後在變換車道時會發出語音提醒.", "../assets/offroad/icon_warning.png"},
    {"AutoSpeeddistance", "車速調控跟車距離", "開啟後可依行車路線自動切換跟車距離， 1格 60公里 2格90公里 3格120公里.", "../assets/offroad/icon_distance.png"},

    {"Navspeed", "圖資速限", "開啟後可依當下所在道路的圖資速限自動更新.", "../assets/offroad/icon_map.png"},
    {"NavReminder", "  導航語音", "開啟後若使用道路導航時會播報轉彎語音訊息.", ""},
    {"speedoverreminder", "  超速提醒", "開啟後若當下速度高於圖資速限會發出提醒.", ""},
    {"speedreminderreset", "  超速重設速限", "開啟後若當下速度高於圖資速限會強制重設速限.", ""},

    {"Dooropen", "  車門開啟", "開啟後在引擎啟動狀態下駕駛車門開啟或後車箱未關閉時會發出提醒.", "../assets/offroad/icon_warning.png"},
    {"Dooropentype", "  車門類型", "選擇要監控的車門.", ""},
    {"DriverdoorOpen", "  駕駛車門", "開啟後在引擎啟動狀態下駕駛車門開啟時會發出提醒.", ""},
    {"CodriverdoorOpen", "  副駕駛車門", "開啟後在引擎啟動狀態下副駕駛車門開啟時會發出提醒.", ""},
    {"LpassengerdoorOpen", "  左乘客車門", "開啟後在引擎啟動狀態下左乘客車門開啟時會發出提醒.", ""},
    {"RpassengerdoorOpen", "  右乘客車門", "開啟後在引擎啟動狀態下右乘客車門開啟時會發出提醒.", ""},
    {"LuggagedoorOpen", "  後車門開啟", "開啟後在引擎啟動狀態下候車門開啟時會發出提醒.", ""},

    {"TrafficMode", tr("  塞車模式"), tr("按住「距離」按鈕 2.5 秒，可根據走走停停的交通狀況啟用更激進的駕駛行為."), ""},
    {"TrafficModespeed", "  塞車模式時速設定", "低於此速度將自動啟動塞車模式.", ""},

    {"Fuelprice", "油價計算", "啟動後會計算油費.", "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"Fuelcosts", "油價設定", "設定車輛使用油種與價格.", ""},

    {"VagSpeed", "時速差調整", "VAG專用。調整車錶速度與C3定速設定不同步的問題。", "../assets/offroad/icon_openpilot.png"},
    {"VagSpeedFactor", "  時速差調整", "請輸入OP定速為110時儀表板的速度差值.", ""},

    {"Disablestartstop", "取消怠速熄火", "開啟後將強制關閉怠速熄火功能.", "../assets/offroad/icon_warning.png"},
  };

  for (const auto &[param, title, desc, icon] : hfopToggles) {
    AbstractControl *hfopcontrolsToggle;

    if (param == "AutoACC") {
      FrogPilotManageControl *AutoACCToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(AutoACCToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, AutoACCPanel]() {
        hfopLayout->setCurrentWidget(AutoACCPanel);
      });
      hfopcontrolsToggle = AutoACCToggle;

    } else if (param == "AutoACCspeed") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 50, "公里");

    } else if (param == "Fuelprice") {
      FrogPilotManageControl *FuelpriceToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(FuelpriceToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, FuelpricePanel]() {
        hfopLayout->setCurrentWidget(FuelpricePanel);
      });
      hfopcontrolsToggle = FuelpriceToggle;

    } else if (param == "Fuelcosts") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 30.0, 36.0, "元", std::map<float, QString>(),0.1);


    } else if (param == "TrafficMode") {
      FrogPilotManageControl *TrafficModeToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(TrafficModeToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, TrafficModePanel]() {
        hfopLayout->setCurrentWidget(TrafficModePanel);
      });
      hfopcontrolsToggle = TrafficModeToggle;

    } else if (param == "TrafficModespeed") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 60, "公里");


    } else if (param == "VagSpeed") {
      FrogPilotManageControl *VagSpeedToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(VagSpeedToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, VagSpeedPanel]() {
        hfopLayout->setCurrentWidget(VagSpeedPanel);
      });
      hfopcontrolsToggle = VagSpeedToggle;
    } else if (param == "VagSpeedFactor") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 20, "公里");



    } else if (param == "Roadtype") {
      FrogPilotManageControl *RoadToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(RoadToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, RoadtypePanel]() {
        hfopLayout->setCurrentWidget(RoadtypePanel);
      });
      hfopcontrolsToggle = RoadToggle;
    } else if (param == "RoadtypeProfile") {
      std::vector<QString> profileOptions{tr("關閉"), tr("巷弄"),tr("平面"), tr("快速"), tr("高速")};
      ButtonParamControl *profileSelection = new ButtonParamControl(param, title, desc, icon, profileOptions);
      hfopcontrolsToggle = profileSelection;

    // } else if (param == "Navspeed") {
    //   FrogPilotManageControl *NavspeedToggle = new FrogPilotManageControl(param, title, desc, icon);
    //   QObject::connect(NavspeedToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, hfopManagementPanel]() {
    //     hfopLayout->setCurrentWidget(hfopManagementPanel);
    //   });
    //   hfopcontrolsToggle = NavspeedToggle;

    } else if(param == "Dooropen") {
      FrogPilotManageControl *DooropenToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(DooropenToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, DooropenPanel]() {
        hfopLayout->setCurrentWidget(DooropenPanel);
      });
      hfopcontrolsToggle = DooropenToggle;

    } else if(param == "Dooropentype") {
      std::vector<QString> adjustablePersonalitiesToggles{"DriverdoorOpen", "CodriverdoorOpen", "LpassengerdoorOpen", "RpassengerdoorOpen", "LuggagedoorOpen"};
      std::vector<QString> adjustablePersonalitiesNames{tr("駕駛"), tr("副駕"), tr("左乘客"), tr("右乘客"), tr("行李")};
      hfopcontrolsToggle = new FrogPilotButtonToggleControl(param, title, desc, icon, adjustablePersonalitiesToggles, adjustablePersonalitiesNames);

    } else {
      hfopcontrolsToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = hfopcontrolsToggle;

    if (AutoACCKeys.contains(param)) {
      AutoACCPList->addItem(hfopcontrolsToggle);
    } else if (FuelpriceKeys.contains(param)) {
      FuelpriceList->addItem(hfopcontrolsToggle);
    } else if (TrafficModeKeys.contains(param)) {
      TrafficModelList->addItem(hfopcontrolsToggle);
    } else if (VagSpeedKeys.contains(param)) {
      VagSpeedList->addItem(hfopcontrolsToggle);
    } else if (RoadKeys.contains(param)) {
      RoadtypeList->addItem(hfopcontrolsToggle);
    } else if (DooropenKeys.contains(param)) {
      DooropenList->addItem(hfopcontrolsToggle);
    } else {
      AutoACCPList->addItem(hfopcontrolsToggle);

      parentKeys.insert(param);
    }

    if (ButtonControl *buttonControl = qobject_cast<ButtonControl*>(hfopcontrolsToggle)) {
      QObject::connect(buttonControl, &ButtonControl::clicked, [this]() {
        emit openSubPanel();
      });
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(hfopcontrolsToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(hfopcontrolsToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(hfopcontrolsToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QSet<QString> forceUpdateKeys = {"AutoACC"};
  for (const QString &key : forceUpdateKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, this, &FrogPilotHFOPPanel::updateToggles);
  }

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [this] {
    openDescriptions(forceOpenDescriptions, toggles);
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubSubPanel, [this] {
    openDescriptions(forceOpenDescriptions, toggles);
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::updateToggles, this, &FrogPilotHFOPPanel::updateToggles);
}
void FrogPilotHFOPPanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  updateToggles();
}

void FrogPilotHFOPPanel::updateToggles() {
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

    if (key == "Fuelprice") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "TrafficMode") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "VagSpeed") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "AutoACC") {
      setVisible &= parent->hasRadar && !(params.getBool("AutoACC") && params.getBool("AutoACCCarAway")&& params.getBool("AutoACCGreenLight"));
    }

    else if (key == "Roadtype") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "Navspeed") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "Dooropen") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }
    else if (key == "AdjacentLeadsUI") {
      setVisible &= parent->hasRadar && !(params.getBool("AdvancedCustomUI") && params.getBool("HideLeadMarker"));
    }

    else if (key == "BlindSpotPath") {
      setVisible &= parent->hasBSM;
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (AutoACCKeys.contains(key)) {
        bool autoACCEnabled = params.getBool("AutoACC");
        toggles["AutoACCspeed"]->setVisible(autoACCEnabled);
        toggles["AutoACCCarAway"]->setVisible(autoACCEnabled);
        toggles["AutoACCGreenLight"]->setVisible(autoACCEnabled);

      } else if (FuelpriceKeys.contains(key)) {
        toggles["Fuelcosts"]->setVisible(true);
      } else if (TrafficModeKeys.contains(key)) {
        toggles["TrafficModespeed"]->setVisible(true);
      } else if (VagSpeedKeys.contains(key)) {
        toggles["VagSpeedFactor"]->setVisible(true);

      } else if (RoadKeys.contains(key)) {
        toggles["RoadtypeProfile"]->setVisible(true);
      } else if (NavspeedKeys.contains(key)) {
        toggles["Navspeed"]->setVisible(true);
      } else if (DooropenKeys.contains(key)) {
        toggles["Dooropen"]->setVisible(true);
      }
    }
  }

  // borderMetricsButton->setVisibleButton(0, parent->hasBSM);

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}



// void FrogPilotHFOPPanel::updateState(const UIState &s) {
//   started = s.scene.started;
// }

// void FrogPilotHFOPPanel::showToggles(const std::set<QString> &keys) {
//   setUpdatesEnabled(false);

//   for (auto &[key, toggle] : toggles) {
//     toggle->setVisible(keys.find(key) != keys.end());
//   }

//   setUpdatesEnabled(true);
//   update();
// }

// void FrogPilotHFOPPanel::hideToggles() {
//   setUpdatesEnabled(false);
//   for (auto &[key, toggle] : toggles) {
//     bool subToggles = FuelpriceKeys.find(key) != FuelpriceKeys.end() ||
//                       TrafficModeKeys.find(key) != TrafficModeKeys.end() ||
//                       VagSpeedKeys.find(key) != VagSpeedKeys.end() ||
//                       AutoACCKeys.find(key) != AutoACCKeys.end() ||
//                       RoadKeys.find(key) != RoadKeys.end() ||
//                       NavspeedKeys.find(key) != NavspeedKeys.end() ||
//                       DooropenKeys.find(key) != DooropenKeys.end() ;
//     toggle->setVisible(!subToggles);
//   }
//   setUpdatesEnabled(true);
//   update();
// }
