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

  FrogPilotListWidget *hfopList = new FrogPilotListWidget(this);
  ScrollView *hfopPanel = new ScrollView(hfopList, this);
  hfopLayout->addWidget(hfopPanel);

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
    {"HFOPinf", "  訊息框", "主畫面左下方顯示訊息狀態.", ""},
    {"AutoACC", "  自動啟動ACC", "啟用後會自動啟動ACC.", ""},
    {"AutoACCspeed", "  自動啟動ACC時速設定", "設定自動啟動ACC的時速條件設定.", ""},
    {"AutoACCCarAway", "  前車遠離啟動", "啟用後當前方車輛遠離會自動啟動ACC.", ""},
    {"AutoACCGreenLight", "  綠燈啟動", "啟用後當偵測到綠燈時會自動啟動ACC.", ""},

    {"Roadtype", "  道路種類設定", "開啟後可依道路種類在特定條件下預設時速", ""},
    {"AutoRoadtype", "  自動道路種類設定", "開啟後可自動依道路種類在特定條件下預設時速", ""},
    {"RoadtypeProfile", "  道路種類設定", "開啟後可依道路種類在特定條件下預設時速", ""},

    {"TrafficMode", "  塞車模式", "按住「距離」按鈕 2.5 秒，可根據走走停停的交通狀況啟用更激進的駕駛行為.", ""},
    {"TrafficModespeed", "  塞車模式時速設定", "低於此速度將自動啟動塞車模式.", ""},

    {"VagSpeed", "  時速差調整", "VAG專用。調整車錶速度與C3定速設定不同步的問題。", ""},
    {"VagSpeedFactor", "  時速差調整", "請輸入OP定速為110時儀表板的速度差值.", ""},
    {"Disablestartstop", "取消怠速熄火", "開啟後將強制關閉怠速熄火功能.", ""},

    {"ChangeLaneReminder", "  變換車道語音", "開啟後在變換車道時會發出語音提醒.", ""},
    {"AutoSpeeddistance", "  車速調控跟車距離", "開啟後可依行車路線自動切換跟車距離， 1格 60公里 2格90公里 3格120公里.", ""},

    {"Navspeed", "  圖資速限", "開啟後可依當下所在道路的圖資速限自動更新.", ""},
    {"NavReminder", "  導航語音", "開啟後若使用道路導航時會播報轉彎語音訊息.", ""},
    {"speedoverreminder", "  超速提醒", "開啟後若當下速度高於圖資速限會發出提醒.", ""},
    {"speedreminderreset", "  超速重設速限", "開啟後若當下速度高於圖資速限會強制重設速限.", ""},

    {"Dooropen", "  車門開啟", "開啟後在引擎啟動狀態下駕駛車門開啟或後車箱未關閉時會發出提醒.", ""},
    {"DriverdoorOpen", "  駕駛車門", "開啟後在引擎啟動狀態下駕駛車門開啟時會發出提醒.", ""},
    {"CodriverdoorOpen", "  副駕駛車門", "開啟後在引擎啟動狀態下副駕駛車門開啟時會發出提醒.", ""},
    {"LpassengerdoorOpen", "  左乘客車門", "開啟後在引擎啟動狀態下左乘客車門開啟時會發出提醒.", ""},
    {"RpassengerdoorOpen", "  右乘客車門", "開啟後在引擎啟動狀態下右乘客車門開啟時會發出提醒.", ""},
    {"LuggagedoorOpen", "  後車門開啟", "開啟後在引擎啟動狀態下候車門開啟時會發出提醒.", ""},

    {"Fuelprice", "  油價計算", "啟動後會計算油費.", ""},
    {"Fuelcosts", "油價設定", "設定車輛使用油種與價格.", ""},


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

    } else if (param == "Navspeed") {
      hfopcontrolsToggle = new ParamControl(param, title, desc, icon);

    } else if (param == "Dooropen") {
      FrogPilotManageControl *DooropenToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(DooropenToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, DooropenPanel]() {
        hfopLayout->setCurrentWidget(DooropenPanel);
      });
      hfopcontrolsToggle = DooropenToggle;

    } else if (param == "Fuelprice") {
      FrogPilotManageControl *FuelpriceToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(FuelpriceToggle, &FrogPilotManageControl::manageButtonClicked, [hfopLayout, FuelpricePanel]() {
        hfopLayout->setCurrentWidget(FuelpricePanel);
      });
      hfopcontrolsToggle = FuelpriceToggle;

    } else if (param == "Fuelcosts") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 30.0, 36.0, "元", std::map<float, QString>(),0.1);

    } else {
      hfopcontrolsToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = hfopcontrolsToggle;

    if (AutoACCKeys.contains(param)) {
      AutoACCPList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else if (FuelpriceKeys.contains(param)) {
      FuelpriceList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else if (TrafficModeKeys.contains(param)) {
      TrafficModelList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else if (VagSpeedKeys.contains(param)) {
      VagSpeedList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else if (RoadKeys.contains(param)) {
      RoadtypeList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else if (DooropenKeys.contains(param)) {
      DooropenList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else if (NavspeedKeys.contains(param)) {
      hfopList->addItem(hfopcontrolsToggle);
      parentKeys.insert(param);
    } else {
      hfopList->addItem(hfopcontrolsToggle);
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

  QSet<QString> forceUpdateKeys = {"AutoACC", "Roadtype", "TrafficMode", "VagSpeed", "Navspeed", "Dooropen", "Fuelprice"};
  for (const QString &key : forceUpdateKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key]), &ToggleControl::toggleFlipped, this, &FrogPilotHFOPPanel::updateToggles);
  }

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [hfopLayout, hfopPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    hfopLayout->setCurrentWidget(hfopPanel);
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubSubPanel, [this] {
    openDescriptions(forceOpenDescriptions, toggles);
  });
  // QObject::connect(parent, &FrogPilotSettingsWindow::updateToggles, this, &FrogPilotHFOPPanel::updateToggles);
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

    if (key == "Roadtype") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "TrafficMode") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "VagSpeed") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "Navspeed") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "Dooropen") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    else if (key == "Fuelprice") {
      setVisible &= parent->hasOpenpilotLongitudinal;
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (key == "AutoACC") {
        bool autoACCEnabled = params.getBool("AutoACC");
        toggles["AutoACCspeed"]->setVisible(autoACCEnabled);
        toggles["AutoACCCarAway"]->setVisible(autoACCEnabled);
        toggles["AutoACCGreenLight"]->setVisible(autoACCEnabled);

      } else if (key == "Roadtype") {
        toggles["AutoRoadtype"]->setVisible(true);
        toggles["RoadtypeProfile"]->setVisible(true);

      } else if (key == "TrafficMode") {
        toggles["TrafficModespeed"]->setVisible(true);

      } else if (key == "VagSpeed") {
        toggles["VagSpeedFactor"]->setVisible(true);

      } else if (key == "Navspeed") {
        toggles["NavReminder"]->setVisible(true);
        toggles["speedoverreminder"]->setVisible(true);
        toggles["speedreminderreset"]->setVisible(true);

      } else if (key == "Dooropen") {
        toggles["DriverdoorOpen"]->setVisible(true);
        toggles["CodriverdoorOpen"]->setVisible(true);
        toggles["LpassengerdoorOpen"]->setVisible(true);
        toggles["RpassengerdoorOpen"]->setVisible(true);
        toggles["LuggagedoorOpen"]->setVisible(true);

      } else if (key == "Fuelprice") {
        toggles["Fuelcosts"]->setVisible(true);
      }
    }
  }

  // borderMetricsButton->setVisibleButton(0, parent->hasBSM);

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}