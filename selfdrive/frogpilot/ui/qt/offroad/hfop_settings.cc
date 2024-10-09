#include "selfdrive/frogpilot/ui/qt/offroad/hfop_settings.h"

FrogPilotHFOPPanel::FrogPilotHFOPPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  const std::vector<std::tuple<QString, QString, QString, QString>> hfopToggles {
    {"HFOPinf", "  訊息框", "主畫面左下方顯示訊息狀態.", "../assets/offroad/icon_custom.png"},
    {"TrafficMode", tr("  塞車模式"), tr("按住「距離」按鈕 2.5 秒，可根據走走停停的交通狀況啟用更激進的駕駛行為."), ""},
    // {"Faststart", "快速開機", "啟動後開機會加速.", "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"Fuelprice", "油價計算", "啟動後會計算油費.", "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"Fuelcosts", "油價設定", "設定車輛使用油種與價格.", ""},

    {"VagSpeed", "時速差調整", "VAG專用。調整車錶速度與C3定速設定不同步的問題。", "../assets/offroad/icon_openpilot.png"},
    {"VagSpeedFactor", "  時速差調整", "請輸入OP定速為110時儀表板的速度差值.", ""},
    {"Disablestartstop", "取消怠速熄火", "開啟後將強制關閉怠速熄火功能.", "../assets/offroad/icon_warning.png"},

    {"AutoACC", "自動啟動ACC", "啟用後會自動啟動ACC.", "../assets/offroad/icon_conditional.png"},
    {"AutoACCspeed", "  自動啟動ACC時速設定", "設定自動啟動ACC的時速條件設定.", ""},
    {"AutoACCCarAway", "  前車遠離啟動", "啟用後當前方車輛遠離會自動啟動ACC.", ""},
    {"AutoACCGreenLight", "  綠燈啟動", "啟用後當偵測到綠燈時會自動啟動ACC.", ""},
    {"TrafficModespeed", "  塞車模式時速設定", "低於此速度將自動啟動塞車模式.", ""},

    {"Roadtype", "道路種類設定", "開啟後可依道路種類在特定條件下預設時速", "../assets/offroad/icon_road.png"},
    {"AutoRoadtype", "自動道路種類設定", "開啟後可自動依道路種類在特定條件下預設時速", "../assets/offroad/icon_road.png"},
    {"RoadtypeProfile", "道路種類設定", "開啟後可依道路種類在特定條件下預設時速", "../assets/offroad/icon_road.png"},
    {"ChangeLaneReminder", "  變換車道語音", "開啟後在變換車道時會發出語音提醒.", "../assets/offroad/icon_warning.png"},
    {"Speeddistance", "車速調控跟車距離", "開啟後可依行車路線自動切換跟車距離， 1格 60公里 2格90公里 3格120公里.", "../assets/offroad/icon_distance.png"},

    {"Navspeed", "圖資速限", "開啟後可依當下所在道路的圖資速限自動更新.", "../assets/offroad/icon_map.png"},
    {"NavReminder", "  導航語音", "開啟後若使用道路導航時會播報轉彎語音訊息.", ""},
    {"speedoverreminder", "  超速提醒", "開啟後若當下速度高於圖資速限會發出提醒.", ""},
    {"speedreminderreset", "  超速重設速限", "開啟後若當下速度高於圖資速限會強制重設速限.", ""},

    {"Dooropen", "  車門開啟", "開啟後在引擎啟動狀態下駕駛車門開啟或後車箱未關閉時會發出提醒.", "../assets/offroad/icon_warning.png"},
    {"DriverdoorOpen", "  駕駛車門", "開啟後在引擎啟動狀態下駕駛車門開啟時會發出提醒.", ""},
    {"CodriverdoorOpen", "  副駕駛車門", "開啟後在引擎啟動狀態下副駕駛車門開啟時會發出提醒.", ""},
    {"LpassengerdoorOpen", "  左乘客車門", "開啟後在引擎啟動狀態下左乘客車門開啟時會發出提醒.", ""},
    {"RpassengerdoorOpen", "  右乘客車門", "開啟後在引擎啟動狀態下右乘客車門開啟時會發出提醒.", ""},
    {"LuggagedoorOpen", "  後車門開啟", "開啟後在引擎啟動狀態下候車門開啟時會發出提醒.", ""},

  };

  for (const auto &[param, title, desc, icon] : hfopToggles) {
    AbstractControl *hfopcontrolsToggle;

    if (param == "Fuelprice") {
      FrogPilotParamManageControl *FuelpriceToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(FuelpriceToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(FuelpriceKeys);
      });
      hfopcontrolsToggle = FuelpriceToggle;

    } else if (param == "Fuelcosts") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 30.0, 36.0, "元", std::map<int, QString>(),0.1);

    } else if (param == "VagSpeed") {
      FrogPilotParamManageControl *VagSpeedToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(VagSpeedToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(VagSpeedKeys);
      });
      hfopcontrolsToggle = VagSpeedToggle;
    } else if (param == "VagSpeedFactor") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 20, "公里");

    } else if (param == "AutoACC") {
      FrogPilotParamManageControl *AutoACCToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(AutoACCToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(AutoACCKeys);
      });
      hfopcontrolsToggle = AutoACCToggle;

    } else if (param == "AutoACCspeed") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 50, "公里");

    } else if (param == "TrafficModespeed") {
      hfopcontrolsToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 60, "公里");

    } else if (param == "Roadtype") {
      FrogPilotParamManageControl *RoadToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(RoadToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(RoadKeys);
      });
      hfopcontrolsToggle = RoadToggle;
    } else if (param == "RoadtypeProfile") {
      std::vector<QString> profileOptions{tr("關閉"), tr("巷弄"),tr("平面"), tr("快速"), tr("高速")};
      ButtonParamControl *profileSelection = new ButtonParamControl(param, title, desc, icon, profileOptions);
      hfopcontrolsToggle = profileSelection;

    } else if (param == "Navspeed") {
      FrogPilotParamManageControl *NavspeedToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(NavspeedToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(NavspeedKeys);
      });
      hfopcontrolsToggle = NavspeedToggle;

    } else if(param == "Dooropen") {
      FrogPilotParamManageControl *DooropenToggle = new FrogPilotParamManageControl(param, title, desc, icon);
      QObject::connect(DooropenToggle, &FrogPilotParamManageControl::manageButtonClicked, [this]() {
        showToggles(DooropenKeys);
      });
      hfopcontrolsToggle = DooropenToggle;

    } else if(param == "Dooropentype") {
      std::vector<QString> adjustablePersonalitiesToggles{"DriverdoorOpen", "CodriverdoorOpen", "LpassengerdoorOpen", "RpassengerdoorOpen", "LuggagedoorOpen"};
      std::vector<QString> adjustablePersonalitiesNames{tr("駕駛"), tr("副駕"), tr("左乘客"), tr("右乘客"), tr("行李")};
      hfopcontrolsToggle = new FrogPilotButtonToggleControl(param, title, desc, adjustablePersonalitiesToggles, adjustablePersonalitiesNames);

    } else {
      hfopcontrolsToggle = new ParamControl(param, title, desc, icon);
    }

    addItem(hfopcontrolsToggle);
    toggles[param] = hfopcontrolsToggle;

    makeConnections(hfopcontrolsToggle);

    if (FrogPilotParamManageControl *frogPilotManageToggle = qobject_cast<FrogPilotParamManageControl*>(hfopcontrolsToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotParamManageControl::manageButtonClicked, this, &FrogPilotHFOPPanel::openParentToggle);
    }

    QObject::connect(hfopcontrolsToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, this, &FrogPilotHFOPPanel::hideToggles);
  // QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotHFOPPanel::updateState);
  // hideToggles();
}

void FrogPilotHFOPPanel::showToggles(const std::set<QString> &keys) {
  setUpdatesEnabled(false);

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(keys.find(key) != keys.end());
  }

  setUpdatesEnabled(true);
  update();
}

void FrogPilotHFOPPanel::hideToggles() {
  setUpdatesEnabled(false);
  for (auto &[key, toggle] : toggles) {
    bool subToggles = FuelpriceKeys.find(key) != FuelpriceKeys.end() ||
                      VagSpeedKeys.find(key) != VagSpeedKeys.end() ||
                      AutoACCKeys.find(key) != AutoACCKeys.end() ||
                      RoadKeys.find(key) != RoadKeys.end() ||
                      NavspeedKeys.find(key) != NavspeedKeys.end() ||
                      DooropenKeys.find(key) != DooropenKeys.end() ;
    toggle->setVisible(!subToggles);
  }
  setUpdatesEnabled(true);
  update();
}
