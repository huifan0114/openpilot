#include "selfdrive/ui/qt/onroad/onroad_home.h"

/////
#include <QApplication>
/////
#include <QPainter>
#include <QStackedLayout>
/////
#include "frogpilot/ui/qt/widgets/frogpilot_controls.h"
/////
#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_helpers.h"
#include "selfdrive/ui/qt/maps/map_panel.h"
#endif

#include "selfdrive/ui/qt/util.h"

OnroadWindow::OnroadWindow(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  QStackedLayout *stacked_layout = new QStackedLayout;
  stacked_layout->setStackingMode(QStackedLayout::StackAll);
  main_layout->addLayout(stacked_layout);

  nvg = new AnnotatedCameraWidget(VISION_STREAM_ROAD, this);

  QWidget * split_wrapper = new QWidget;
  split = new QHBoxLayout(split_wrapper);
  split->setContentsMargins(0, 0, 0, 0);
  split->setSpacing(0);
  split->addWidget(nvg);

  if (getenv("DUAL_CAMERA_VIEW")) {
    CameraWidget *arCam = new CameraWidget("camerad", VISION_STREAM_ROAD, true, this);
    split->insertWidget(0, arCam);
  }

  if (getenv("MAP_RENDER_VIEW")) {
    CameraWidget *map_render = new CameraWidget("navd", VISION_STREAM_MAP, false, this);
    split->insertWidget(0, map_render);
  }

  stacked_layout->addWidget(split_wrapper);

  alerts = new OnroadAlerts(this);
  alerts->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  stacked_layout->addWidget(alerts);

  // setup stacking order
  alerts->raise();

  setAttribute(Qt::WA_OpaquePaintEvent);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &OnroadWindow::updateState);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &OnroadWindow::offroadTransition);
  QObject::connect(uiState(), &UIState::primeChanged, this, &OnroadWindow::primeChanged);

  // FrogPilot variables
  frogpilot_onroad = new FrogPilotOnroadWindow(this);
  frogpilot_onroad->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void OnroadWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  frogpilot_onroad->setGeometry(rect());
}

void OnroadWindow::updateState(const UIState &s, const FrogPilotUIState &fs) {
  if (!s.scene.started) {
    return;
  }

  if (s.scene.map_on_left) {
    split->setDirection(QBoxLayout::LeftToRight);
  } else {
    split->setDirection(QBoxLayout::RightToLeft);
  }

  alerts->updateState(s, fs);
  nvg->updateState(s, fs);

  QColor bgColor = bg_colors[s.status];
  if (bg != bgColor) {
    // repaint border
    bg = bgColor;
    update();
  }

  // FrogPilot variables
  frogpilot_onroad->bg = bg;
  frogpilot_onroad->fps = nvg->fps;

  nvg->frogpilot_nvg->alertHeight = alerts->alertHeight;

  frogpilot_onroad->updateState(s, fs);
}

void OnroadWindow::mousePressEvent(QMouseEvent* e) {
  FrogPilotUIState &fs = *frogpilotUIState();
  QJsonObject &frogpilot_toggles = fs.frogpilot_toggles;
  SubMaster &fpsm = *(fs.sm);
/////
  Params params;  // 創建持久化參數對象
  QPoint pos = e->pos();

  // 定義觸控區域
  QRect hideSpeedRect(rect().center().x() - 175, 50, 350, 350);
  QRect maxSpeedRect(7, 25, 225, 225);
  QRect speedLimitRect(7, 250, 225, 225);
  QRect autoRoadtypeRect(20, 560, 225, 225);
  QRect roadtypeProfileRect(20, 800, 225, 225);

  if (fpsm["frogpilotPlan"].getFrogpilotPlan().getSpeedLimitChanged() && nvg->frogpilot_nvg->newSpeedLimitRect.contains(pos)) {
    fs.params_memory.putBool("SpeedLimitAccepted", true);
    return;
  }

  // 最大速度區 - 切換自動 ACC 模式
  if (maxSpeedRect.contains(pos)) {
    bool currentAutoACC = params.getBool("AutoACC");
    params.putBool("AutoACC", !currentAutoACC);
    updateFrogPilotToggles();
    return;
  }

  // 隱藏速度區 - 切換速度顯示和螢幕亮度
  if (hideSpeedRect.contains(pos)) {
    bool hide_speed = !frogpilot_toggles.value("hide_speed").toBool();
    params.putBool("HideSpeed", hide_speed);

    int ScreenBrightnessOnroadpre = fs.params_memory.getInt("ScreenBrightnessOnroadpre");
    if (ScreenBrightnessOnroadpre == 0) {
      fs.params_memory.putInt("ScreenBrightnessOnroadpre", params.getInt("ScreenBrightnessOnroad"));
      params.putInt("ScreenBrightnessOnroad", 0);
    } else {
      params.putInt("ScreenBrightnessOnroad", ScreenBrightnessOnroadpre);
      fs.params_memory.putInt("ScreenBrightnessOnroadpre", 0);
    }
    updateFrogPilotToggles();
    return;
  }

  // 速限區 - 切換交通模式
  if (speedLimitRect.contains(pos)) {
    bool currentTrafficMode = params.getBool("TrafficMode");
    params.putBool("TrafficMode", !currentTrafficMode);
    updateFrogPilotToggles();
    return;
  }

  // 自動道路類型區 - 切換自動道路類型
  if (autoRoadtypeRect.contains(pos)) {
    bool currentAutoRoadtype = params.getBool("AutoRoadtype");
    params.putBool("AutoRoadtype", !currentAutoRoadtype);
    updateFrogPilotToggles();
    return;
  }

  // 道路類型配置檔區 - 輪換配置檔
  if (roadtypeProfileRect.contains(pos)) {
    bool isAutoRoadtype = params.getBool("AutoRoadtype");
    if (isAutoRoadtype) {
      // 如果是自動模式，切換為手動模式
      params.putBool("AutoRoadtype", false);
    } else {
      // 如果是手動模式，輪換配置檔
      int currentProfile = params.getInt("RoadtypeProfile");
      int nextProfile = (currentProfile + 1) % 5;  // 0-4 循環
      params.putInt("RoadtypeProfile", nextProfile);
    }
    updateFrogPilotToggles();
/////
    return;
  }

#ifdef ENABLE_MAPS
  if (map != nullptr) {
    bool sidebarVisible = geometry().x() > 0;
    bool show_map = !sidebarVisible && !frogpilot_toggles.value("hide_map").toBool();
    map->setVisible(show_map && !map->isVisible());
    if (map->isVisible() && frogpilot_toggles.value("full_map").toBool()) {
      nvg->frogpilot_nvg->bigMapOpen = false;

      map->setFixedSize(this->size());

      alerts->setVisible(false);
      nvg->setVisible(false);
    } else if (map->isVisible() && frogpilot_toggles.value("big_map").toBool()) {
      nvg->frogpilot_nvg->bigMapOpen = true;

      map->setFixedWidth(topWidget(this)->width() * 3 / 4 - UI_BORDER_SIZE);

      alerts->setVisible(true);
      nvg->setVisible(true);
    } else {
      nvg->frogpilot_nvg->bigMapOpen = false;

      map->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);

      alerts->setVisible(true);
      nvg->setVisible(true);
    }
    nvg->screen_recorder->setVisible(!map->isVisible() && frogpilot_toggles.value("screen_recorder").toBool());
  }
#endif
  // propagation event to parent(HomeWindow)
  QWidget::mousePressEvent(e);
}

void OnroadWindow::createMapWidget() {
  FrogPilotUIState &fs = *frogpilotUIState();
  QJsonObject &frogpilot_toggles = fs.frogpilot_toggles;

  if (frogpilot_toggles.value("hide_map").toBool()) {
    return;
  }

#ifdef ENABLE_MAPS
  auto m = new MapPanel(get_mapbox_settings());
  map = m;
  QObject::connect(m, &MapPanel::mapPanelRequested, this, &OnroadWindow::mapPanelRequested);
  QObject::connect(nvg->map_settings_btn, &MapSettingsButton::clicked, m, &MapPanel::toggleMapSettings);
  nvg->map_settings_btn->setEnabled(true);

  m->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);
  split->insertWidget(0, m);
  // hidden by default, made visible when navRoute is published
  m->setVisible(false);
#endif
}

void OnroadWindow::offroadTransition(bool offroad) {
#ifdef ENABLE_MAPS
  if (!offroad) {
    if (map == nullptr && !MAPBOX_TOKEN.isEmpty()) {
      createMapWidget();
    }
  }
#endif
  alerts->clear();
  if (!offroad) {
    alerts->enableFerg = util::random_int(0, 1) == 1;
  } else {
    alerts->displayFerg = false;
  }
}

void OnroadWindow::primeChanged(bool prime) {
#ifdef ENABLE_MAPS
  if (map && (!prime && MAPBOX_TOKEN.isEmpty())) {
    nvg->map_settings_btn->setEnabled(false);
    nvg->map_settings_btn->setVisible(false);
    map->deleteLater();
    map = nullptr;
  } else if (!map && (prime || !MAPBOX_TOKEN.isEmpty())) {
    createMapWidget();
  }
#endif
}

void OnroadWindow::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.fillRect(rect(), QColor(bg.red(), bg.green(), bg.blue(), 255));
}
