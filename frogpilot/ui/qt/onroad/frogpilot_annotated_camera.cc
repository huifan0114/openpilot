#include <QMovie>
/////////////////////////////////////////////////////
#include <fstream>
#include <sstream>
#include <map>
/////////////////////////////////////////////////////

#include "frogpilot/ui/qt/onroad/frogpilot_annotated_camera.h"
/////////////////////////////////////////////////////
#include "selfdrive/ui/qt/maps/map_helpers.h"
/////////////////////////////////////////////////////

FrogPilotAnnotatedCameraWidget::FrogPilotAnnotatedCameraWidget(QWidget *parent) : QWidget(parent) {
  animationTimer = new QTimer(this);

  brakePedalImg = loadPixmap("../../frogpilot/assets/other_images/brake_pedal.png", {btn_size, btn_size});
  curveSpeedIcon = loadPixmap("../../frogpilot/assets/other_images/curve_speed.png", {btn_size, btn_size});
  dashboardIcon = loadPixmap("../../frogpilot/assets/other_images/dashboard_icon.png", {btn_size / 2, btn_size / 2});
  gasPedalImg = loadPixmap("../../frogpilot/assets/other_images/gas_pedal.png", {btn_size, btn_size});
  mapDataIcon = loadPixmap("../../frogpilot/assets/other_images/offline_maps_icon.png", {btn_size / 2, btn_size / 2});
  navigationIcon = loadPixmap("../../frogpilot/assets/other_images/navigation_icon.png", {btn_size / 2, btn_size / 2});
  nextMapsIcon = loadPixmap("../../frogpilot/assets/other_images/next_maps_icon.png", {btn_size / 2, btn_size / 2});
  pausedIcon = loadPixmap("../../frogpilot/assets/other_images/paused_icon.png", {widget_size, widget_size});
  speedIcon = loadPixmap("../../frogpilot/assets/other_images/speed_icon.png", {widget_size, widget_size});
  stopSignImg = loadPixmap("../../frogpilot/assets/other_images/stop_sign.png", {btn_size, btn_size});
  turnIcon = loadPixmap("../../frogpilot/assets/other_images/turn_icon.png", {widget_size, widget_size});

  loadGif("../../frogpilot/assets/other_images/curve_icon.gif", cemCurveIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/lead_icon.gif", cemLeadIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/speed_icon.gif", cemSpeedIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/light_icon.gif", cemStopIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/turn_icon.gif", cemTurnIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/chill_mode_icon.gif", chillModeIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/experimental_mode_icon.gif", experimentalModeIcon, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/weather_clear_day.gif", weatherClearDay, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/weather_clear_night.gif", weatherClearNight, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/weather_low_visibility.gif", weatherLowVisibility, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/weather_rain.gif", weatherRain, QSize(widget_size, widget_size), this);
  loadGif("../../frogpilot/assets/other_images/weather_snow.gif", weatherSnow, QSize(widget_size, widget_size), this);

  QObject::connect(animationTimer, &QTimer::timeout, [this] {
    animationFrameIndex = (animationFrameIndex + 1) % totalFrames;
  });
  QObject::connect(frogpilotUIState(), &FrogPilotUIState::themeUpdated, this, &FrogPilotAnnotatedCameraWidget::updateSignals);
  QObject::connect(uiState(), &UIState::offroadTransition, [this] {
    standstillTimer.invalidate();

    QJsonObject stats = QJsonDocument::fromJson(QString::fromStdString(params.get("FrogPilotStats")).toUtf8()).object();
    stats["FrogHops"] = stats.value("FrogHops").toInt(0) + frogHopCount;
    params.putNonBlocking("FrogPilotStats", QJsonDocument(stats).toJson(QJsonDocument::Compact).toStdString());

    frogHopCount = 0;
  });
}

void FrogPilotAnnotatedCameraWidget::showEvent(QShowEvent *event) {
  updateSignals();
}

void FrogPilotAnnotatedCameraWidget::updateSignals() {
  QVector<QPixmap>().swap(blindspotImages);
  QVector<QPixmap>().swap(signalImages);

  bool isGif = false;

  QFileInfoList files = QDir("../../frogpilot/assets/active_theme/signals/").entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
  for (const QFileInfo &fileInfo : files) {
    QString fileName = fileInfo.fileName();
    QString filePath = fileInfo.absoluteFilePath();

    if (fileName.endsWith(".gif", Qt::CaseInsensitive)) {
      isGif = true;

      QMovie movie(filePath);
      movie.setCacheMode(QMovie::CacheNone);
      movie.start();

      int frameCount = movie.frameCount();
      signalImages.reserve(frameCount);

      for (int i = 0; i < frameCount; ++i) {
        movie.jumpToFrame(i);

        QImage image = movie.currentPixmap().toImage().convertToFormat(QImage::Format_Indexed8);
        QPixmap frame = QPixmap::fromImage(image);
        signalImages.append(frame);
      }

      movie.stop();
    } else if (fileName.endsWith(".png", Qt::CaseInsensitive)) {
      QVector<QPixmap> &targetList = fileName.contains("blindspot", Qt::CaseInsensitive) ? blindspotImages : signalImages;
      targetList.append(QPixmap::fromImage(QImage(filePath).convertToFormat(QImage::Format_Indexed8)));
    } else {
      QStringList parts = fileName.split('_');
      if (parts.size() == 2) {
        signalStyle = parts[0];
        signalAnimationLength = parts[1].toInt();
      }
    }
  }

  if (!signalImages.isEmpty()) {
    QPixmap &firstImage = signalImages.front();
    signalHeight = firstImage.height();
    signalWidth = firstImage.width();
    totalFrames = signalImages.size();

    if (isGif && signalStyle == "traditional") {
      signalMovement = (width() + signalWidth * 2) / totalFrames;

      signalStyle = "traditional_gif";
    } else {
      signalMovement = 0;
    }
  } else {
    signalAnimationLength = 0;
    signalHeight = 0;
    signalMovement = 0;
    signalWidth = 0;
    totalFrames = 0;

    signalStyle = "None";
  }
}

void FrogPilotAnnotatedCameraWidget::updateState(const FrogPilotUIState &fs, const QJsonObject &frogpilot_toggles) {
  const UIState &s = *uiState();
  const UIScene &scene = s.scene;

  const FrogPilotUIScene &frogpilot_scene = fs.frogpilot_scene;
  const SubMaster &fpsm = *(fs.sm);

  const cereal::FrogPilotPlan::Reader &frogpilotPlan = fpsm["frogpilotPlan"].getFrogpilotPlan();

  if (scene.is_metric || frogpilot_toggles.value("use_si_metrics").toBool()) {
    leadDistanceUnit = tr(" 米");
    leadSpeedUnit = frogpilot_toggles.value("use_si_metrics").toBool() ? tr(" m/s") : tr(" 公里/小時");

    distanceConversion = 1.0f;
    speedConversion = scene.is_metric ? MS_TO_KPH : MS_TO_MPH;
    speedConversionMetrics = frogpilot_toggles.value("use_si_metrics").toBool() ? 1.0f : MS_TO_KPH;
  } else {
    leadDistanceUnit = tr(" feet");
    leadSpeedUnit = tr(" mph");

    distanceConversion = METER_TO_FOOT;
    speedConversion = MS_TO_MPH;
    speedConversionMetrics = MS_TO_MPH;
  }

  float speedLimitOffset = frogpilotPlan.getSlcSpeedLimitOffset() * speedConversion;

  cscSpeedStr = QString::number(std::nearbyint(fmin(speed, frogpilotPlan.getCscSpeed() * speedConversion))) + speedUnit;
  speedLimitOffsetStr = (speedLimitOffset != 0) ? QString::number(speedLimitOffset, 'f', 0).prepend((speedLimitOffset > 0) ? "+" : "-") : "–";

  if (frogpilot_scene.standstill && frogpilot_toggles.value("stopped_timer").toBool()) {
    if (!standstillTimer.isValid()) {
      standstillTimer.start();
    } else {
      standstillDuration = frogpilot_scene.map_open || frogpilot_scene.started_timer / UI_FREQ < 60 ? 0 : standstillTimer.elapsed() / 1000;
    }
  } else {
    standstillDuration = 0;

    standstillTimer.invalidate();
  }

  static int lastFrameIndex;
  if (lastFrameIndex > animationFrameIndex && frogpilot_toggles.value("signal_icons").toString() == "frog") {
    frogHopCount++;
  }
  lastFrameIndex = animationFrameIndex;

  update();
}

void FrogPilotAnnotatedCameraWidget::paintFrogPilotWidgets(QPainter &p, UIState &s, FrogPilotUIState &fs, SubMaster &sm, SubMaster &fpsm, QJsonObject &frogpilot_toggles) {
  FrogPilotUIScene &frogpilot_scene = fs.frogpilot_scene;
  UIScene &scene = s.scene;

  const cereal::CarState::Reader &carState = fpsm["carState"].getCarState();
  const cereal::FrogPilotCarState::Reader &frogpilotCarState = fpsm["frogpilotCarState"].getFrogpilotCarState();
  const cereal::FrogPilotNavigation::Reader &frogpilotNavigation = fpsm["frogpilotNavigation"].getFrogpilotNavigation();
  const cereal::FrogPilotPlan::Reader &frogpilotPlan = fpsm["frogpilotPlan"].getFrogpilotPlan();
  const cereal::ModelDataV2::Reader &model = sm["modelV2"].getModelV2();

  if (!hideBottomIcons && frogpilot_toggles.value("cem_status").toBool()) {
    paintCEMStatus(p, frogpilotPlan, frogpilot_scene, sm);
  } else {
    cemStatusPosition.setX(0);
    cemStatusPosition.setY(0);
  }

  if (!frogpilot_scene.map_open && !hideBottomIcons && frogpilot_toggles.value("compass").toBool()) {
    paintCompass(p, frogpilot_toggles);
  } else {
    compassPosition.setX(0);
    compassPosition.setY(0);
  }

  if (!frogpilot_scene.map_open && !frogpilotPlan.getSpeedLimitChanged() && !(signalStyle == "static" && carState.getLeftBlinker()) && frogpilot_toggles.value("csc_status").toBool()) {
    if (frogpilotPlan.getCscTraining()) {
      paintSmartControllerTraining(p, frogpilotPlan);
    } else {
      glowTimer.invalidate();

      if (isCruiseSet && frogpilotPlan.getCscControllingSpeed()) {
        paintCurveSpeedControl(p, frogpilotPlan);
      }
    }
  } else {
    glowTimer.invalidate();
  }

  if (!frogpilot_scene.map_open && frogpilotCarState.getPauseLateral() && !hideBottomIcons) {
    paintLateralPaused(p, frogpilot_scene);
  } else {
    lateralPausedPosition.setX(0);
    lateralPausedPosition.setY(0);
  }

  if (!frogpilot_scene.map_open && (frogpilotCarState.getForceCoast() || frogpilotCarState.getPauseLongitudinal()) && !hideBottomIcons) {
    paintLongitudinalPaused(p, frogpilot_scene);
  }

  if (!bigMapOpen && frogpilot_toggles.value("pedals_on_ui").toBool()) {
    paintPedalIcons(p, carState, frogpilotCarState, frogpilot_scene, frogpilot_toggles);
  }

  if (frogpilotPlan.getSpeedLimitChanged()) {
    paintPendingSpeedLimit(p, frogpilotPlan);
  } else {
    pendingLimitTimer.invalidate();
  }

  if (frogpilot_toggles.value("radar_tracks").toBool()) {
    paintRadarTracks(p, model, s, frogpilot_scene, sm, fpsm);
  }

  if (frogpilot_toggles.value("road_name_ui").toBool()) {
    paintRoadName(p);
  }

  if (!bigMapOpen && (mutcdSpeedLimit || viennaSpeedLimit) && frogpilot_toggles.value("speed_limit_sources").toBool()) {
    paintSpeedLimitSources(p, frogpilotCarState, frogpilotNavigation, frogpilotPlan);
  }

  if (!frogpilot_scene.map_open && standstillDuration != 0 && frogpilot_scene.started_timer / UI_FREQ >= 60) {
    paintStandstillTimer(p);
  }
/////////////////停車標記//////////////////
  // 停車點顯示（僅 UI，與降速功能解耦）
  if (scene.track_vertices.length() >= 1 && frogpilotPlan.getRedLight() && frogpilot_toggles.value("show_stopping_point").toBool()) {
    paintStoppingPoint(p, scene, frogpilot_scene, frogpilot_toggles);
  }

  // 停車降速邏輯（防抖機制：快速反應 + 距離過濾）
  const bool redLightDetected = scene.track_vertices.length() >= 1 && frogpilotPlan.getRedLight();
  // Always keep StopmarkDistance updated when a stop point exists (independent of UI toggle).
  if (redLightDetected) {
    int stopDistance = static_cast<int>(std::nearbyint(frogpilot_scene.model_length));
    params_memory.putInt("StopmarkDistance", stopDistance);
  } else {
    params_memory.putInt("StopmarkDistance", 0);
  }
  const int DEBOUNCE_TIME_MS = 150;  // 🔧 優化：從250ms減到150ms，加快初始反應

  if (redLightDetected) {
    // 紅燈偵測到：啟動或繼續計時
    if (!redLightDetectedTimer.isValid()) {
      redLightDetectedTimer.start();
    }
    redLightDisappearedTimer.invalidate();  // 清除消失計時器

    // 穩定偵測超過防抖時間才啟用
    if (redLightDetectedTimer.elapsed() >= DEBOUNCE_TIME_MS) {
      int roadProfile = params.getInt("RoadtypeProfile");
      const bool stopmarkslowsdown = params.getBool("Stopmarkslowsdown");
      const bool is_high_speed_profile = roadProfile == 3 || roadProfile == 4;

      // 🔧 優化：增加距離過濾，避免過遠誤觸發（>600m 不啟動）
      int stopDistance = params_memory.getInt("StopmarkDistance");
      const bool distance_valid = stopDistance > 0 && stopDistance <= 600;

      // 僅在開啟 stopmarkslowsdown 且非高速/快速道路時啟用
      if (stopmarkslowsdown && !is_high_speed_profile && distance_valid) {
        params_memory.putBool("StopmarkActive", true);
      } else {
        params_memory.putBool("StopmarkActive", false);
      }
    }
  } else {
    // 紅燈消失：啟動消失計時器
    if (!redLightDisappearedTimer.isValid()) {
      redLightDisappearedTimer.start();
    }
    redLightDetectedTimer.invalidate();  // 清除偵測計時器

    // 穩定消失超過防抖時間才清除狀態
    if (redLightDisappearedTimer.elapsed() >= DEBOUNCE_TIME_MS) {
      params_memory.putBool("StopmarkActive", false);
    }
  }
////////////////////////////////////////
  if (!bigMapOpen && (carState.getLeftBlinker() || carState.getRightBlinker()) && signalStyle != "None") {
    if (!animationTimer->isActive()) {
      animationTimer->start(signalAnimationLength);
    }
    paintTurnSignals(p, carState);
  } else if (animationTimer->isActive()) {
    animationTimer->stop();
  }

  if (!frogpilot_scene.map_open && !hideBottomIcons) {
    paintWeather(p, frogpilotPlan, frogpilot_scene);
  }

/////////////////////////////////////////////////////
  // 車輛資訊面板 (HFOP Info Panel)
  if (!frogpilot_scene.map_open && !hideBottomIcons && frogpilot_toggles.value("hfop_inf").toBool()) {
    paintVehicleInfoPanel(p, carState, frogpilot_toggles);
  }
/////////////////////////////////////////////////////
}

void FrogPilotAnnotatedCameraWidget::paintAdjacentPaths(QPainter &p, const cereal::CarState::Reader &carState, const FrogPilotUIScene &frogpilot_scene, const QJsonObject &frogpilot_toggles) {
  std::function<void(bool, float, float, const QPolygonF &)> drawAdjacentPath = [&p, &frogpilot_toggles, this](bool isBlindSpot, float width, float requirement, const QPolygonF &polygon) {
    QLinearGradient gradient(0, height(), 0, 0);
    if (isBlindSpot && frogpilot_toggles.value("blind_spot_path").toBool()) {
      gradient.setColorAt(0.0f, QColor::fromHslF(0 / 360.0f, 0.75f, 0.5f, 0.6f));
      gradient.setColorAt(0.5f, QColor::fromHslF(0 / 360.0f, 0.75f, 0.5f, 0.4f));
      gradient.setColorAt(1.0f, QColor::fromHslF(0 / 360.0f, 0.75f, 0.5f, 0.2f));
    } else {
      float ratio = std::clamp(width / requirement, 0.0f, 1.0f);
      float hue = ratio * (120.0f / 360.0f);

      gradient.setColorAt(0.0f, QColor::fromHslF(hue, 0.75f, 0.5f, 0.6f));
      gradient.setColorAt(0.5f, QColor::fromHslF(hue, 0.75f, 0.5f, 0.4f));
      gradient.setColorAt(1.0f, QColor::fromHslF(hue, 0.75f, 0.5f, 0.2f));
    }

    p.setBrush(gradient);
    p.drawPolygon(polygon);
  };

  std::function<void(bool, float, const QPolygonF &)> drawAdjacentPathMetric = [&p, &frogpilot_toggles, this](bool isBlindSpot, float width, const QPolygonF &polygon) {
    QString text = isBlindSpot && frogpilot_toggles.value("blind_spot_path").toBool() ? tr("車輛在盲點中") : QString::number(width * distanceConversion, 'f', 2) + leadDistanceUnit;

    p.setFont(InterFont(40, QFont::Normal));
    p.setPen(QPen(whiteColor()));
    p.drawText(polygon.boundingRect(), Qt::AlignCenter, text);
  };

  if (frogpilot_scene.lane_width_left >= frogpilot_toggles.value("lane_detection_width").toDouble()) {
    p.save();

    drawAdjacentPath(carState.getLeftBlindspot(), frogpilot_scene.lane_width_left, frogpilot_toggles.value("lane_detection_width").toDouble(), frogpilot_scene.track_adjacent_vertices[0]);

    if (frogpilot_toggles.value("adjacent_path_metrics").toBool()) {
      drawAdjacentPathMetric(carState.getLeftBlindspot(), frogpilot_scene.lane_width_left, frogpilot_scene.track_adjacent_vertices[0]);
    }

    p.restore();
  }

  if (frogpilot_scene.lane_width_right >= frogpilot_toggles.value("lane_detection_width").toDouble()) {
    p.save();

    drawAdjacentPath(carState.getRightBlindspot(), frogpilot_scene.lane_width_right, frogpilot_toggles.value("lane_detection_width").toDouble(), frogpilot_scene.track_adjacent_vertices[1]);

    if (frogpilot_toggles.value("adjacent_path_metrics").toBool()) {
      drawAdjacentPathMetric(carState.getRightBlindspot(), frogpilot_scene.lane_width_right, frogpilot_scene.track_adjacent_vertices[1]);
    }

    p.restore();
  }
}

void FrogPilotAnnotatedCameraWidget::paintBlindSpotPath(QPainter &p, const cereal::CarState::Reader &carState, const FrogPilotUIScene &frogpilot_scene) {
  p.save();

  QLinearGradient bs(0, height(), 0, 0);
  bs.setColorAt(0.0f, QColor::fromHslF(0 / 360.0f, 0.75f, 0.5f, 0.6f));
  bs.setColorAt(0.5f, QColor::fromHslF(0 / 360.0f, 0.75f, 0.5f, 0.4f));
  bs.setColorAt(1.0f, QColor::fromHslF(0 / 360.0f, 0.75f, 0.5f, 0.2f));

  p.setBrush(bs);
  if (frogpilot_scene.lane_width_left != 0 && carState.getLeftBlindspot()) {
    p.drawPolygon(frogpilot_scene.track_adjacent_vertices[0]);
  }
  if (frogpilot_scene.lane_width_right != 0 && carState.getRightBlindspot()) {
    p.drawPolygon(frogpilot_scene.track_adjacent_vertices[1]);
  }

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintCEMStatus(QPainter &p, const cereal::FrogPilotPlan::Reader &frogpilotPlan, FrogPilotUIScene &frogpilot_scene, SubMaster &sm) {
  if (dmIconPosition == QPoint(0, 0)) {
    return;
  }

  p.save();

  cemStatusPosition.rx() = dmIconPosition.x();
  cemStatusPosition.ry() = dmIconPosition.y() - widget_size / 2;
  cemStatusPosition.rx() += (rightHandDM ? -img_size - widget_size : widget_size) / (frogpilot_scene.map_open ? 1.25 : 1);

  QRect cemWidget(cemStatusPosition, QSize(widget_size, widget_size));

  p.setBrush(blackColor(166));
  if (frogpilot_scene.conditional_status == 1) {
    p.setPen(QPen(QColor(bg_colors[STATUS_CONDITIONAL_OVERRIDDEN]), 10));
  } else if (frogpilot_scene.enabled && sm["controlsState"].getControlsState().getExperimentalMode()) {
    p.setPen(QPen(QColor(bg_colors[STATUS_EXPERIMENTAL_MODE_ENABLED]), 10));
  } else {
    p.setPen(QPen(blackColor(), 10));
  }
  p.drawRoundedRect(cemWidget, 24, 24);

  QSharedPointer<QMovie> icon = chillModeIcon;
  if (frogpilot_scene.enabled && sm["controlsState"].getControlsState().getExperimentalMode()) {
    if (frogpilot_scene.conditional_status == 1) {
      icon = chillModeIcon;
    } else if (frogpilot_scene.conditional_status == 2) {
      icon = experimentalModeIcon;
    } else if (frogpilot_scene.conditional_status == 3 || frogpilot_scene.conditional_status == 4) {
      icon = cemSpeedIcon;
    } else if (frogpilot_scene.conditional_status == 5 || frogpilot_scene.conditional_status == 7) {
      icon = cemTurnIcon;
    } else if (frogpilot_scene.conditional_status == 6 || frogpilot_scene.conditional_status == 11 || frogpilot_scene.conditional_status == 12) {
      icon = cemStopIcon;
    } else if (frogpilot_scene.conditional_status == 8) {
      icon = cemCurveIcon;
    } else if (frogpilot_scene.conditional_status == 9 || frogpilot_scene.conditional_status == 10) {
      icon = cemLeadIcon;
    } else {
      icon = experimentalModeIcon;
    }
  }
  p.drawPixmap(cemWidget, icon->currentPixmap());

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintCompass(QPainter &p, QJsonObject &frogpilot_toggles) {
  p.save();

  compassPosition.rx() = rightHandDM ? UI_BORDER_SIZE + widget_size / 2 : width() - UI_BORDER_SIZE - btn_size;
  if (mapButtonVisible) {
    if (rightHandDM) {
      compassPosition.rx() += btn_size - UI_BORDER_SIZE;
    } else {
      compassPosition.rx() -= btn_size + UI_BORDER_SIZE;
    }
  }
  compassPosition.ry() = dmIconPosition.y() - widget_size / 2;

  QRect compassWidget(compassPosition, QSize(widget_size, widget_size));

  p.setBrush(blackColor(166));
  p.setPen(QPen(blackColor(), 10));
  p.drawRoundedRect(compassWidget, 24, 24);

  QPainterPath clipPath;
  clipPath.addRoundedRect(compassWidget.adjusted(5, 5, -5, -5), 24, 24);
  p.setClipPath(clipPath);

  QFont font = InterFont(65, QFont::Normal);
  QFontMetrics fm(font);
  p.setFont(font);
  p.setPen(QPen(whiteColor()));

  const double pixelsPerDegree = 2.5;

  const int baseRibbonWidth = qRound(360 * pixelsPerDegree);

  static QPixmap ribbonPixmap;
  if (!ribbonPixmap) {
    int ribbonHeight = compassWidget.height();
    int ribbonWidth = baseRibbonWidth * 2;

    ribbonPixmap = QPixmap(ribbonWidth, ribbonHeight);
    ribbonPixmap.fill(Qt::transparent);

    QPainter ribbonPainter(&ribbonPixmap);
    ribbonPainter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    ribbonPainter.setFont(font);

    QMap<int, QString> directionLabels = {{0, "N"}, {45, "NE"}, {90, "E"}, {135, "SE"}, {180, "S"}, {225, "SW"}, {270, "W"}, {315, "NW"}, {360, "N"}};

    for (int i = 0; i < 2; ++i) {
      for (int degree = 0; degree < 360; ++degree) {
        int x = qRound((i * 360 + degree) * pixelsPerDegree);

        if (directionLabels.contains(degree)) {
          const QString &label = directionLabels[degree];
          int textX = x - fm.horizontalAdvance(label) / 2;

          ribbonPainter.setPen(whiteColor());
          ribbonPainter.drawText(textX, fm.ascent(), label);
        }

        int notchHeight = (degree % 45 == 0) ? 35 : (degree % 15 == 0) ? 25 : 15;
        int notchWidth = (degree % 45 == 0) ? 5 : (degree % 15 == 0) ? 4 : 3;

        ribbonPainter.setPen(QPen(whiteColor(), notchWidth));
        ribbonPainter.drawLine(x, ribbonHeight - notchHeight - 5, x, ribbonHeight);
      }
    }
  }

  double rawBearing = QJsonDocument::fromJson(QString::fromStdString(params_memory.get("LastGPSPosition")).toUtf8()).object().value("bearing").toDouble(0.0);

  int bearing = qRound(fmod(rawBearing + 360.0, 360.0));
  int offset = qRound(bearing * pixelsPerDegree) % baseRibbonWidth;
  int drawX = compassWidget.center().x() - offset;

  p.drawPixmap(drawX - baseRibbonWidth, compassWidget.top() + 5, ribbonPixmap);
  p.drawPixmap(drawX, compassWidget.top() + 5, ribbonPixmap);

  int triangleSize = 40;
  int triangleX = compassWidget.center().x();
  int triangleY = compassWidget.bottom() - triangleSize;

  QPolygon triangle(QVector<QPoint>{
    QPoint(triangleX, triangleY - triangleSize),
    QPoint(triangleX - triangleSize / 1.5, triangleY),
    QPoint(triangleX + triangleSize / 1.5, triangleY)
  });

  p.setBrush(whiteColor());
  p.setPen(Qt::NoPen);
  p.drawPolygon(triangle);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintCurveSpeedControl(QPainter &p, const cereal::FrogPilotPlan::Reader &frogpilotPlan) {
  p.save();

  QRect curveSpeedRect(QPoint(setSpeedRect.right() + UI_BORDER_SIZE, setSpeedRect.top()), QSize(defaultSize.width() * 1.25, defaultSize.width() * 1.25));

  QPixmap curveSpeedImage = frogpilotPlan.getRoadCurvature() < 0 ? curveSpeedIcon : curveSpeedIcon.transformed(QTransform().scale(-1, 1));
  QSize curveSpeedSize = curveSpeedImage.size();
  QPoint curveSpeedPoint(curveSpeedRect.x() + (curveSpeedRect.width()  - curveSpeedSize.width())  / 2, curveSpeedRect.y() + (curveSpeedRect.height() - curveSpeedSize.height()) / 2);

  p.setOpacity(1.0);

  QRect cscRect(curveSpeedRect.topLeft() + QPoint(0, curveSpeedRect.height() + 10), QSize(curveSpeedRect.width(), 100));

  p.setBrush(blueColor(166));
  p.setFont(InterFont(45, QFont::Normal));
  p.setPen(QPen(blueColor(), 10));

  p.drawRoundedRect(cscRect, 24, 24);
  p.setPen(QPen(whiteColor(), 6));
  p.drawText(cscRect.adjusted(20, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, cscSpeedStr);

  p.drawPixmap(curveSpeedPoint, curveSpeedImage);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintLateralPaused(QPainter &p, FrogPilotUIScene &frogpilot_scene) {
  if (dmIconPosition == QPoint(0, 0)) {
    return;
  }

  p.save();

  if (cemStatusPosition != QPoint(0, 0)) {
    lateralPausedPosition = cemStatusPosition;
  } else {
    lateralPausedPosition.rx() = dmIconPosition.x();
    lateralPausedPosition.ry() = dmIconPosition.y() - widget_size / 2;
  }
  lateralPausedPosition.rx() += (rightHandDM ? -UI_BORDER_SIZE - widget_size - UI_BORDER_SIZE : UI_BORDER_SIZE + widget_size + UI_BORDER_SIZE) / (frogpilot_scene.map_open ? 1.25 : 1);

  QRect lateralWidget(lateralPausedPosition, QSize(widget_size, widget_size));

  p.setBrush(blackColor(166));
  p.setPen(QPen(QColor(bg_colors[STATUS_TRAFFIC_MODE_ENABLED]), 10));
  p.drawRoundedRect(lateralWidget, 24, 24);

  p.setOpacity(0.5);
  p.drawPixmap(lateralWidget, turnIcon);
  p.setOpacity(0.75);
  p.drawPixmap(lateralWidget, pausedIcon);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintLeadMetrics(QPainter &p, bool adjacent, QPointF *chevron, const cereal::FrogPilotPlan::Reader &frogpilotPlan, const cereal::RadarState::LeadData::Reader &lead_data) {
  float leadDistance = lead_data.getDRel() + (adjacent ? fabs(lead_data.getYRel()) : 0);
  float leadSpeed = std::max(lead_data.getVLead(), 0.0f);
///////////////////////////////////////////
  // 計算速度差並寫入 leadspeeddiffProfile
  if (!adjacent) {
    float speedDiff = (leadSpeed - speed / speedConversion) * (speedConversion == KM_TO_MILE ? 3.6f : 1.0f);
    params_memory.putInt("leadspeeddiffProfile", (int)std::round(speedDiff));
  }
///////////////////////////////////////////

  p.setFont(InterFont(40, QFont::Normal));
  p.setPen(QPen(whiteColor()));

  QString text;
  if (adjacent) {
    text = QString("%1 %2 | %3 %4")
              .arg(qRound(leadDistance * distanceConversion))
              .arg(leadDistanceUnit)
              .arg(qRound(leadSpeed * speedConversionMetrics))
              .arg(leadSpeedUnit);
  } else {
    text = QString("%1 %2 (%3) | %4 %5 | %6 %7")
              .arg(qRound(leadDistance * distanceConversion))
              .arg(leadDistanceUnit)
              .arg(QString(tr("Desired: %1")).arg(frogpilotPlan.getDesiredFollowDistance() * distanceConversion))
              .arg(qRound(leadSpeed * speedConversionMetrics))
              .arg(leadSpeedUnit)
              .arg(QString::number(leadDistance / std::max(speed / speedConversion, 1.0f), 'f', 2))
              .arg(tr("s"));
  }

  QFontMetrics metrics(p.font());
  int textHeight = metrics.height();
  int textWidth = metrics.horizontalAdvance(text);

  int textX = ((chevron[2].x() + chevron[0].x()) / 2) - textWidth / 2;
  int textY = chevron[0].y() + textHeight + 5;

  if (!adjacent) {
    int xMargin = textWidth * 0.25;
    int yMargin = textHeight * 0.25;

    leadTextRect = QRect(textX, textY - textHeight, textWidth, textHeight).adjusted(-xMargin, -yMargin, xMargin, yMargin);
    p.drawText(textX, textY, text);
  } else {
    QRect adjacentTextRect(textX, textY - textHeight, textWidth, textHeight);
    if (!adjacentTextRect.intersects(leadTextRect)) {
      p.drawText(textX, textY, text);
    }
  }
}

void FrogPilotAnnotatedCameraWidget::paintLongitudinalPaused(QPainter &p, FrogPilotUIScene &frogpilot_scene) {
  if (dmIconPosition == QPoint(0, 0)) {
    return;
  }

  p.save();

  QPoint longitudinalIconPosition;
  if (lateralPausedPosition != QPoint(0, 0)) {
    longitudinalIconPosition = lateralPausedPosition;
  } else if (cemStatusPosition != QPoint(0, 0)) {
    longitudinalIconPosition = cemStatusPosition;
  } else {
    longitudinalIconPosition.rx() = dmIconPosition.x();
    longitudinalIconPosition.ry() = dmIconPosition.y() - widget_size / 2;
  }
  longitudinalIconPosition.rx() += (rightHandDM ? -UI_BORDER_SIZE - widget_size - UI_BORDER_SIZE : UI_BORDER_SIZE + widget_size + UI_BORDER_SIZE) / (frogpilot_scene.map_open ? 1.25 : 1);

  QRect longitudinalWidget(longitudinalIconPosition, QSize(widget_size, widget_size));

  p.setBrush(blackColor(166));
  p.setPen(QPen(QColor(bg_colors[STATUS_TRAFFIC_MODE_ENABLED]), 10));
  p.drawRoundedRect(longitudinalWidget, 24, 24);

  p.setOpacity(0.5);
  p.drawPixmap(longitudinalWidget, speedIcon);
  p.setOpacity(0.75);
  p.drawPixmap(longitudinalWidget, pausedIcon);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintPathEdges(QPainter &p, const cereal::NavInstruction::Reader &navInstruction, const UIScene &scene, const FrogPilotUIScene &frogpilot_scene, SubMaster &sm) {
  p.save();

  std::function<void(QLinearGradient&, const QColor&)> setPathEdgeColors = [&](QLinearGradient &gradient, QColor baseColor) {
    baseColor.setAlphaF(1.0f); gradient.setColorAt(0.0f, baseColor);
    baseColor.setAlphaF(0.5f); gradient.setColorAt(0.5f, baseColor);
    baseColor.setAlphaF(0.1f); gradient.setColorAt(1.0f, baseColor);
  };

  QLinearGradient pe(0, height(), 0, 0);
  if (frogpilot_scene.always_on_lateral_active) {
    setPathEdgeColors(pe, bg_colors[STATUS_ALWAYS_ON_LATERAL_ACTIVE]);
  } else if (frogpilot_scene.conditional_status == 1) {
    setPathEdgeColors(pe, bg_colors[STATUS_CONDITIONAL_OVERRIDDEN]);
  } else if (sm["controlsState"].getControlsState().getExperimentalMode()) {
    setPathEdgeColors(pe, bg_colors[STATUS_EXPERIMENTAL_MODE_ENABLED]);
  } else if (frogpilot_scene.traffic_mode_enabled) {
    setPathEdgeColors(pe, bg_colors[STATUS_TRAFFIC_MODE_ENABLED]);
  } else if (frogpilot_scene.model_length > navInstruction.getManeuverDistance() && navInstruction.getManeuverDistance() >= 1) {
    setPathEdgeColors(pe, bg_colors[STATUS_NAVIGATION_ACTIVE]);
  } else if (!frogpilot_scene.use_stock_colors) {
    setPathEdgeColors(pe, frogpilot_scene.path_edges_color);
  } else {
    pe.setColorAt(0.0f, QColor::fromHslF(148 / 360.0f, 0.94f, 0.51f, 1.0f));
    pe.setColorAt(0.5f, QColor::fromHslF(112 / 360.0f, 1.00f, 0.68f, 0.5f));
    pe.setColorAt(1.0f, QColor::fromHslF(112 / 360.0f, 1.00f, 0.68f, 0.1f));
  }

  QPainterPath path;
  path.addPolygon(scene.track_vertices);
  path.addPolygon(frogpilot_scene.track_edge_vertices);
  p.setBrush(pe);
  p.drawPath(path);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintPedalIcons(QPainter &p, const cereal::CarState::Reader &carState, const cereal::FrogPilotCarState::Reader &frogpilotCarState, FrogPilotUIScene &frogpilot_scene, QJsonObject &frogpilot_toggles) {
  p.save();

  float brakeOpacity = 1.0f;
  float gasOpacity = 1.0f;

  if (frogpilot_toggles.value("dynamic_pedals_on_ui").toBool()) {
    brakeOpacity = frogpilot_scene.standstill ? 1.0f : carState.getAEgo() < -0.25f ? std::max(0.25f, std::abs(carState.getAEgo())) : 0.25f;
    gasOpacity = std::max(0.25f, carState.getAEgo());
  } else if (frogpilot_toggles.value("static_pedals_on_ui").toBool()) {
    brakeOpacity = frogpilot_scene.standstill || frogpilotCarState.getBrakeLights() || carState.getAEgo() < -0.25f ? 1.0f : 0.25f;
    gasOpacity = carState.getAEgo() > 0.25 ? 1.0f : 0.25f;
  }

  int startX = experimentalButtonPosition.x();
  int startY = experimentalButtonPosition.y() + btn_size + UI_BORDER_SIZE;

  p.setOpacity(brakeOpacity);
  p.drawPixmap(startX, startY, brakePedalImg);

  p.setOpacity(gasOpacity);
  p.drawPixmap(startX + btn_size / 2, startY, gasPedalImg);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintPendingSpeedLimit(QPainter &p, const cereal::FrogPilotPlan::Reader &frogpilotPlan) {
  p.save();

  if (!pendingLimitTimer.isValid()) {
    pendingLimitTimer.start();
  }

  QString newSpeedLimitStr = (frogpilotPlan.getUnconfirmedSlcSpeedLimit() > 1) ? QString::number(std::nearbyint(frogpilotPlan.getUnconfirmedSlcSpeedLimit() * speedConversion)) : "–";
  newSpeedLimitRect = speedLimitRect.translated(speedLimitRect.width() + UI_BORDER_SIZE, 0);

  if (!viennaSpeedLimit) {
    newSpeedLimitRect.setWidth(newSpeedLimitStr.size() >= 3 ? 200 : 175);

    p.setBrush(whiteColor());
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(newSpeedLimitRect, 24, 24);
    p.setPen(pendingLimitTimer.elapsed() % 1000 < 500 ? QPen(blackColor(), 6) : QPen(redColor(), 6));
    p.drawRoundedRect(newSpeedLimitRect.adjusted(9, 9, -9, -9), 16, 16);

    p.setFont(InterFont(28, QFont::Normal));
    p.drawText(newSpeedLimitRect.adjusted(0, 22, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("下一段"));
    p.drawText(newSpeedLimitRect.adjusted(0, 51, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("速限"));
    p.setFont(InterFont(70, QFont::Normal));
    p.drawText(newSpeedLimitRect.adjusted(0, 85, 0, 0), Qt::AlignTop | Qt::AlignHCenter, newSpeedLimitStr);
  } else {
    p.setBrush(whiteColor());
    p.setPen(Qt::NoPen);
    p.drawEllipse(newSpeedLimitRect);
    p.setPen(QPen(Qt::red, 20));
    p.drawEllipse(newSpeedLimitRect.adjusted(16, 16, -16, -16));

    p.setPen(pendingLimitTimer.elapsed() % 1000 < 500 ? QPen(blackColor(), 6) : QPen(redColor(), 6));
    p.setFont(InterFont((newSpeedLimitStr.size() >= 3) ? 60 : 70, QFont::Normal));
    p.drawText(newSpeedLimitRect, Qt::AlignCenter, newSpeedLimitStr);
  }

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintRadarTracks(QPainter &p, const cereal::ModelDataV2::Reader &model, UIState &s, FrogPilotUIScene &frogpilot_scene, SubMaster &sm, SubMaster &fpsm) {
  p.save();

  capnp::List<cereal::LiveTracks>::Reader liveTracks = fpsm["liveTracks"].getLiveTracks();
  update_radar_tracks(liveTracks, model.getPosition(), s, sm);

  int diameter = 25;

  QRect viewport = p.viewport();

  for (std::size_t i = 0; i < frogpilot_scene.live_radar_tracks.size(); ++i) {
    const RadarTrackData &track = frogpilot_scene.live_radar_tracks[i];

    float x = std::clamp(static_cast<float>(track.calibrated_point.x()), 0.0f, float(viewport.width() - diameter));
    float y = std::clamp(static_cast<float>(track.calibrated_point.y()), 0.0f, float(viewport.height() - diameter));

    p.setBrush(redColor());
    p.drawEllipse(QPointF(x + diameter / 2.0f, y + diameter / 2.0f), diameter / 2.0f, diameter / 2.0f);
  }

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintRainbowPath(QPainter &p, QLinearGradient &bg, float lin_grad_point, SubMaster &sm) {
  p.save();

  static float hueOffset = 0.0;
  if (sm["carState"].getCarState().getVEgo() > 0) {
    hueOffset += powf(sm["carState"].getCarState().getVEgo(), 0.5f) / sqrtf(145.0f / MS_TO_KPH);
  }

  float alpha = util::map_val(lin_grad_point, 0.0f, 1.0f, 0.5f, 0.1f);
  float pathHue = fmodf((lin_grad_point * 360.0f) + hueOffset, 360.0f);

  bg.setColorAt(lin_grad_point, QColor::fromHslF(pathHue / 360.0f, 1.0f, 0.5f, alpha));
  bg.setSpread(QGradient::RepeatSpread);

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintRoadName(QPainter &p) {
  QString roadName = QString::fromStdString(params_memory.get("RoadName"));
  if (roadName.isEmpty()) {
    return;
  }

  alertHeight = std::max(50, alertHeight);

  p.save();

  QFont font = InterFont(70, QFont::Normal);
  int textWidth = QFontMetrics(font).horizontalAdvance(roadName);

/////////////////////////////////////////////////////
  QRect roadNameRect((width() - textWidth * 1.25) / 2, rect().bottom() - 100 + 1, textWidth * 1.25, 70);
/////////////////////////////////////////////////////
  // 背景框（可選）
  // p.setBrush(blackColor(166));
  // p.setOpacity(1.0);
  // p.setPen(QPen(blackColor(), 10));
  // p.drawRoundedRect(roadNameRect, 24, 24);
/////////////////////////////////////////////////////

  p.setFont(font);
  p.setPen(QPen(whiteColor(), 6));
  p.drawText(roadNameRect, Qt::AlignCenter, roadName);

/////////////////////////////////////////////////////
  // 自動道路類型識別 + 速限優先序設定
  bool autoRoadtype = params.getBool("AutoRoadtype");
  if (autoRoadtype) {
    int previousRoadProfile = params.getInt("RoadtypeProfile");
    int newRoadProfile = 2; // 默認：一般平面
    QString priority1 = "Map Data";
    QString priority2 = "Navigation";
    QString priority3 = "Lowest";

    // 讀取右邊路徑寬度（單位：公尺）- 需要傳入 frogpilotPlan 參數
    // 暫時從 params_memory 讀取，需確保 Python 端有寫入
    // float laneWidthRight = params_memory.getFloat("LaneWidthRight");

    // 根據道路名稱分類 + 同時設定優先序
    if (roadName.contains("高速") || roadName.contains("國道")) {
      newRoadProfile = 4; // 高速公路
      priority1 = "Map Data";
      priority2 = "Navigation";
      priority3 = "Highest";
    } else if (roadName.contains("快速") || roadName.contains("省道")) {
      newRoadProfile = 3; // 快速道路
      priority1 = "Map Data";
      priority2 = "Navigation";
      priority3 = "Highest";
    } else if (roadName.contains("交流道")) {
      newRoadProfile = 2; // 一般平面
      priority1 = "Map Data";
      priority2 = "Navigation";
      priority3 = "Dashboard";
    } else if (roadName.contains("街") || roadName.contains("巷") || roadName.contains("弄") ) {
    // } else if (roadName.contains("街") || roadName.contains("巷") || roadName.contains("弄") || laneWidthRight < 0.5f) {
      // 加入路徑寬度判斷：右邊路徑寬度小於 1 公尺時也判定為街道巷弄
      newRoadProfile = 1; // 街道巷弄
      priority1 = "Map Data";
      priority2 = "Navigation";
      priority3 = "Lowest";
    } else {
      newRoadProfile = 2; // 一般平面
      priority1 = "Map Data";
      priority2 = "Navigation";
      priority3 = "Dashboard";
    }

    // 只有當 roadProfile 真的變更時才更新
    if (newRoadProfile != previousRoadProfile) {
      params.putInt("RoadtypeProfile", newRoadProfile);
      // 同時更新速限優先序
      params.put("SLCPriority1", priority1.toStdString());
      params.put("SLCPriority2", priority2.toStdString());
      params.put("SLCPriority3", priority3.toStdString());
      // 觸發 FrogPilot toggles 更新
      params.putBool("FrogPilotTogglesUpdated", true);
    }
  }
/////////////////////////////////////////////////////
  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintSmartControllerTraining(QPainter &p, const cereal::FrogPilotPlan::Reader &frogpilotPlan) {
  p.save();

  if (!glowTimer.isValid()) {
    glowTimer.start();
  }

  QRect curveSpeedRect(QPoint(setSpeedRect.right() + UI_BORDER_SIZE, setSpeedRect.top()), QSize(defaultSize.width() * 1.25, defaultSize.width() * 1.25));
  QPixmap curveSpeedImage = frogpilotPlan.getRoadCurvature() < 0 ? curveSpeedIcon : curveSpeedIcon.transformed(QTransform().scale(-1, 1));

  qreal phase = (glowTimer.elapsed() % 2000) / 2000.0 * 2 * M_PI;
  qreal alphaFactor = 0.5 + 0.5 * sin(phase);

  QColor glowColor = blueColor();
  glowColor.setAlphaF(0.3 + 0.7 * alphaFactor);

  int glowWidth = 8 + static_cast<int>(2 * alphaFactor);

  p.setOpacity(1.0);

  p.setBrush(blackColor(166));
  p.setPen(QPen(glowColor, glowWidth));
  p.drawRoundedRect(curveSpeedRect, 24, 24);

  QSize curveSpeedSize = curveSpeedImage.size();
  QPoint curveSpeedPoint(curveSpeedRect.x() + (curveSpeedRect.width()  - curveSpeedSize.width())  / 2, curveSpeedRect.y() + (curveSpeedRect.height() - curveSpeedSize.height()) / 2);
  p.drawPixmap(curveSpeedPoint, curveSpeedImage);

  QRect textRect(curveSpeedRect.topLeft() + QPoint(0, curveSpeedRect.height() + 10), QSize(curveSpeedRect.width(), 50));
  p.setBrush(blackColor(166));
  p.setPen(QPen(blackColor(), 10));
  p.drawRoundedRect(textRect, 24, 24);

  p.setFont(InterFont(35, QFont::Normal));
  p.setPen(QPen(whiteColor(), 6));
  p.drawText(textRect.adjusted(20, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, "訓練...");

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintSpeedLimitSources(QPainter &p, const cereal::FrogPilotCarState::Reader &frogpilotCarState, const cereal::FrogPilotNavigation::Reader &frogpilotNavigation, const cereal::FrogPilotPlan::Reader &frogpilotPlan) {
  p.save();

  std::function<void(QRect&, QPixmap&, const QString&, const double)> drawSource = [&](QRect &rect, QPixmap &icon, QString title, double speedLimitValue) {
    if (QString::fromUtf8(frogpilotPlan.getSlcSpeedLimitSource().cStr()) == "Mapbox" && title == "Navigation") {
      speedLimitValue = frogpilotPlan.getSlcMapboxSpeedLimit() * speedConversion;

      title = "Mapbox";
    }

    if (QString::fromUtf8(frogpilotPlan.getSlcSpeedLimitSource().cStr()) == title && speedLimitValue != 0) {
      p.setBrush(redColor(166));
      p.setFont(InterFont(35, QFont::Normal));
      p.setPen(QPen(redColor(), 10));
    } else {
      p.setBrush(blackColor(166));
      p.setFont(InterFont(35, QFont::Normal));
      p.setPen(QPen(blackColor(), 10));
    }

    QRect iconRect(rect.x() + 20, rect.y() + (rect.height() - img_size / 4) / 2, img_size / 4, img_size / 4);
    QPixmap scaledIcon = icon.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QString speedText;
    if (speedLimitValue != 0) {
      speedText = QString::number(std::nearbyint(speedLimitValue)) + speedUnit;
    } else {
      speedText = "N/A";
    }

    QString fullText = tr(title.toUtf8().constData()) + " - " + speedText;

    p.setOpacity(1.0);
    p.drawRoundedRect(rect, 24, 24);
    p.drawPixmap(iconRect, scaledIcon);

    p.setPen(QPen(whiteColor(), 6));
    QRect textRect(iconRect.right() + 10, rect.y(), rect.width() - iconRect.width() - 30, rect.height());
    p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, fullText);
  };
/////////////////////////////////////////////////////////////////////
const int rect_height = 60;
const int spacing = UI_BORDER_SIZE / 2;
const int info_panel_top = 20;  // 距離畫面頂部 20 像素
const int info_panel_left = speedLimitRect.right() + 20;  // 速限右邊 20 像素

// 初始化繪製位置
int current_y = info_panel_top;

// 繪製有效項目
if (frogpilotCarState.getDashboardSpeedLimit() > 0) {
  QRect dashboardRect(info_panel_left, current_y, 450, rect_height);
  drawSource(dashboardRect, dashboardIcon, tr("儀表"), frogpilotCarState.getDashboardSpeedLimit() * speedConversion);
  current_y += rect_height + spacing;
}

if (frogpilotPlan.getSlcMapSpeedLimit() > 0) {
  QRect mapDataRect(info_panel_left, current_y, 450, rect_height);
  drawSource(mapDataRect, mapDataIcon, tr("地圖"), frogpilotPlan.getSlcMapSpeedLimit() * speedConversion);
  current_y += rect_height + spacing;
}

if (frogpilotNavigation.getNavigationSpeedLimit() > 0) {
  QRect navigationRect(info_panel_left, current_y, 450, rect_height);
  drawSource(navigationRect, navigationIcon, tr("導航"), frogpilotNavigation.getNavigationSpeedLimit() * speedConversion);
  current_y += rect_height + spacing;
}

if (frogpilotPlan.getSlcNextSpeedLimit() > 0) {
  QRect nextLimitRect(info_panel_left, current_y, 450, rect_height);
  drawSource(nextLimitRect, nextMapsIcon, tr("下一段"), frogpilotPlan.getSlcNextSpeedLimit() * speedConversion);
  current_y += rect_height + spacing;
}

  // Controller status (VSC/CSC) for quick diagnostics
  const QString vscStatus = frogpilotPlan.getVscActive() ? tr("視覺安全(VSC): 開") : tr("視覺安全(VSC): 關");
  const QString cscStatus = frogpilotPlan.getCscControllingSpeed() ? tr("彎道控速(CSC): 開") : tr("彎道控速(CSC): 關");
  QRect vscRect(info_panel_left, current_y, 450, rect_height);
  p.setBrush(blackColor(166));
  p.setFont(InterFont(35, QFont::Normal));
  p.setPen(QPen(blackColor(), 10));
  p.drawRoundedRect(vscRect, 24, 24);
  p.setPen(QPen(whiteColor(), 6));
  p.drawText(vscRect.adjusted(20, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, vscStatus);
  current_y += rect_height + spacing;

  QRect cscRect(info_panel_left, current_y, 450, rect_height);
  p.setBrush(blackColor(166));
  p.setFont(InterFont(35, QFont::Normal));
  p.setPen(QPen(blackColor(), 10));
  p.drawRoundedRect(cscRect, 24, 24);
  p.setPen(QPen(whiteColor(), 6));
  p.drawText(cscRect.adjusted(20, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, cscStatus);
  current_y += rect_height + spacing;
/////////////////////////////////////////////////////////////////////
  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintStandstillTimer(QPainter &p) {
  p.save();

  float transition = 0.0f;

  QColor startColor, endColor;
  if (standstillDuration < 60) {
    startColor = endColor = bg_colors[STATUS_ENGAGED];
  } else if (standstillDuration < 150) {
    startColor = bg_colors[STATUS_ENGAGED];
    endColor = bg_colors[STATUS_CONDITIONAL_OVERRIDDEN];

    transition = (standstillDuration - 60) / 150.0f;
  } else if (standstillDuration < 300) {
    startColor = bg_colors[STATUS_CONDITIONAL_OVERRIDDEN];
    endColor = bg_colors[STATUS_TRAFFIC_MODE_ENABLED];

    transition = (standstillDuration - 150) / 150.0f;
  } else {
    startColor = endColor = bg_colors[STATUS_TRAFFIC_MODE_ENABLED];

    transition = 0.0f;
  }

  QColor blendedColor(
    startColor.red() + transition * (endColor.red() - startColor.red()),
    startColor.green() + transition * (endColor.green() - startColor.green()),
    startColor.blue() + transition * (endColor.blue() - startColor.blue())
  );

  int minutes = standstillDuration / 60;
  int seconds = standstillDuration % 60;

  p.setFont(InterFont(176, QFont::Normal));
  {
    QString minuteStr = (minutes == 1) ? tr("1 分鐘") : QString(tr("%1 分鐘")).arg(minutes);
    QRect textRect = p.fontMetrics().boundingRect(minuteStr);
    textRect.moveCenter({rect().center().x(), 210 - textRect.height() / 2});
    p.setPen(QPen(blendedColor));
    p.drawText(textRect.x(), textRect.bottom(), minuteStr);
  }

  p.setFont(InterFont(66));
  {
    QString secondStr = (seconds == 1) ? tr("1 秒") : QString(tr("%1 秒")).arg(seconds);
    QRect textRect = p.fontMetrics().boundingRect(secondStr);
    textRect.moveCenter({rect().center().x(), 290 - textRect.height() / 2});
    p.setPen(QPen(whiteColor()));
    p.drawText(textRect.x(), textRect.bottom(), secondStr);
  }

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintStoppingPoint(QPainter &p, UIScene &scene, FrogPilotUIScene &frogpilot_scene, QJsonObject &frogpilot_toggles) {
  p.save();

  QPointF centerPoint = (scene.track_vertices.first() + scene.track_vertices.last()) / 2.0;
  QPointF adjustedPoint = centerPoint - QPointF(stopSignImg.width() / 2, stopSignImg.height());
  p.drawPixmap(adjustedPoint, stopSignImg);

  if (frogpilot_toggles.value("show_stopping_point_metrics").toBool()) {
    QFont font = InterFont(35, QFont::Normal);
    QString text = QString::number(std::nearbyint(frogpilot_scene.model_length * distanceConversion)) + leadDistanceUnit;
    QPointF textPosition = centerPoint - QPointF(QFontMetrics(font).horizontalAdvance(text) / 2, stopSignImg.height() + 35);

    p.setFont(font);
    p.setPen(QPen(whiteColor()));
    p.drawText(textPosition, text);
  }

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintTurnSignals(QPainter &p, const cereal::CarState::Reader &carState) {
  p.save();

  bool leftBlinker = carState.getLeftBlinker();
  bool blindspotActive = leftBlinker ? carState.getLeftBlindspot() : carState.getRightBlindspot();

  if (signalStyle == "static") {
    int signalXPosition = leftBlinker ? (rect().center().x() * 0.75) - signalWidth : rect().center().x() * 1.25;
    int signalYPosition = signalHeight / 2;

    if (blindspotActive && !blindspotImages.empty()) {
      p.drawPixmap(signalXPosition, signalYPosition, signalWidth, signalHeight, blindspotImages[0].transformed(QTransform().scale(leftBlinker ? 1 : -1, 1)));
    } else {
      p.drawPixmap(signalXPosition, signalYPosition, signalWidth, signalHeight, signalImages[animationFrameIndex].transformed(QTransform().scale(leftBlinker ? 1 : -1, 1)));
    }
  } else {
    int signalXPosition;
    if (signalStyle == "traditional_gif") {
      signalXPosition = leftBlinker ? width() - (animationFrameIndex * signalMovement) + signalWidth : (animationFrameIndex * signalMovement) - signalWidth;
    } else {
      signalXPosition = leftBlinker ? width() - ((animationFrameIndex + 1) * signalWidth) : animationFrameIndex * signalWidth;
    }

    int signalYPosition = height() - signalHeight - alertHeight;

    if (blindspotActive && !blindspotImages.empty()) {
      p.drawPixmap(leftBlinker ? width() - signalWidth : 0, signalYPosition, signalWidth, signalHeight, blindspotImages[0].transformed(QTransform().scale(leftBlinker ? 1 : -1, 1)));
    } else {
      p.drawPixmap(signalXPosition, signalYPosition, signalWidth, signalHeight, signalImages[animationFrameIndex].transformed(QTransform().scale(leftBlinker ? 1 : -1, 1)));
    }
  }

  p.restore();
}

void FrogPilotAnnotatedCameraWidget::paintWeather(QPainter &p, const cereal::FrogPilotPlan::Reader &frogpilotPlan, FrogPilotUIScene &frogpilot_scene) {
  int weatherId = frogpilotPlan.getWeatherId();
  if (weatherId == 0) {
    return;
  }

  p.save();

  QPoint weatherIconPosition;
  if (compassPosition != QPoint(0, 0)) {
    weatherIconPosition = compassPosition;
    weatherIconPosition.rx() += (rightHandDM ? UI_BORDER_SIZE + widget_size + UI_BORDER_SIZE : -UI_BORDER_SIZE - widget_size - UI_BORDER_SIZE) / (frogpilot_scene.map_open ? 1.25 : 1);
  } else {
    weatherIconPosition.rx() = rightHandDM ? UI_BORDER_SIZE + widget_size / 2 : width() - UI_BORDER_SIZE - btn_size;
    if (mapButtonVisible) {
      if (rightHandDM) {
        weatherIconPosition.rx() += btn_size - UI_BORDER_SIZE;
      } else {
        weatherIconPosition.rx() -= btn_size + UI_BORDER_SIZE;
      }
    }
    weatherIconPosition.ry() = dmIconPosition.y() - widget_size / 2;
  }

  QRect weatherRect(weatherIconPosition, QSize(widget_size, widget_size));

  p.setBrush(blackColor(166));
  p.setPen(QPen(blackColor(), 10));
  p.drawRoundedRect(weatherRect, 24, 24);

  QSharedPointer<QMovie> icon = weatherClearDay;
  if ((weatherId >= 200 && weatherId <= 232) || (weatherId >= 300 && weatherId <= 321) || (weatherId >= 500 && weatherId <= 531)) {
    icon = weatherRain;
  } else if (weatherId >= 600 && weatherId <= 622) {
    icon = weatherSnow;
  } else if (weatherId >= 701 && weatherId <= 762) {
    icon = weatherLowVisibility;
  } else if (weatherId == 800) {
    icon = frogpilotPlan.getWeatherDaytime() ? weatherClearDay : weatherClearNight;
  }

  p.drawPixmap(weatherRect, icon->currentPixmap());

  p.restore();
}

/////////////////////////////////////////////////////
QString FrogPilotAnnotatedCameraWidget::translateNavigationText(const cereal::NavInstruction::Reader &nav_instruction, bool is_metric) {
  QString primary_str = QString::fromStdString(nav_instruction.getManeuverPrimaryText());
  QString secondary_str = QString::fromStdString(nav_instruction.getManeuverSecondaryText());
  auto distance_str_pair = map_format_distance(nav_instruction.getManeuverDistance(), is_metric);
  QString type = QString::fromStdString(nav_instruction.getManeuverType());
  QString modifier = QString::fromStdString(nav_instruction.getManeuverModifier());
  QString distance_str = distance_str_pair.first;
  QString distance_unit = distance_str_pair.second;

  QString fn;
  fn += "於" + distance_str + distance_unit + "後  ";

  if (!modifier.isEmpty()) {
    QString moditext;
    if (modifier == "uturn") {
      moditext = "迴轉";
    } else if (modifier == "sharp right") {
      moditext = "向右急";
    } else if (modifier == "right") {
      moditext = "向右";
    } else if (modifier == "slight right") {
      moditext = "靠右";
    } else if (modifier == "straight") {
      moditext = "直行";
    } else if (modifier == "slight left") {
      moditext = "靠左";
    } else if (modifier == "left") {
      moditext = "向左";
    } else if (modifier == "sharp left") {
      moditext = "向左急";
    } else {
      moditext = modifier;
    }
    fn += moditext;
  }

  type = type.trimmed();
  if (!type.isEmpty()) {
    QString typetext;
    if (type == "turn") {
      typetext = "轉彎";
    } else if (type == "new name") {
      typetext = "新路";
    } else if (type == "depart") {
      typetext = "出發";
    } else if (type == "arrive") {
      typetext = "抵達";
    } else if (type == "merge") {
      typetext = "合併";
    } else if (type == "on ramp") {
      typetext = "進入交流道";
    } else if (type == "off ramp") {
      typetext = "駛出交流道";
    } else if (type == "fork") {
      typetext = "換道";
    } else if (type == "use lane") {
      typetext = "線道";
    } else if (type == "end off road") {
      typetext = "抵達終點";
    } else if (type == "continue") {
      typetext = "直行";
    } else if (type == "roundabout") {
      typetext = "進入圓環";
    } else if (type == "takeRoundabout") {
      typetext = "圓環轉彎";
    } else if (type == "exit roundabout") {
      typetext = "駛出圓環";
    } else if (type == "exit rotary") {
      typetext = "駛出圓環";
    } else if (type == "rotary") {
      typetext = "進入圓環";
    } else if (type == "notification") {
      typetext = "注意";
    } else if (type == "roundabout turn") {
      typetext = "圓環轉彎";
    } else {
      typetext = type;
    }
    fn += typetext;
  }

  return fn + "\n" + primary_str + " " + secondary_str;
}

void FrogPilotAnnotatedCameraWidget::paintVehicleInfoPanel(QPainter &p, const cereal::CarState::Reader &carState, const QJsonObject &frogpilot_toggles) {
  p.save();

  // 繪製資訊面板背景
  int leadspeed_diffProfile = params_memory.getInt("leadspeeddiffProfile");
  const QRect info_rect(rect().left() + 20, rect().bottom() - 560, 220, 500);
  p.setPen(Qt::NoPen);
  if (leadspeed_diffProfile < -20) {
    p.setBrush(Qt::red);
  } else if (leadspeed_diffProfile < 0 && leadspeed_diffProfile > -20) {
    p.setBrush(QColor(255, 165, 0));
  } else {
    p.setBrush(whiteColor());
  }
  p.drawRoundedRect(info_rect, 24, 24);

  // 根據 AutoRoadtype 狀態改變邊框顏色
  bool autoRoadtype = params.getBool("AutoRoadtype");
  if (autoRoadtype) {
    p.setPen(QPen(QColor(255, 165, 0), 6));
  } else {
    p.setPen(QPen(blackColor(), 6));
  }
  p.drawRoundedRect(info_rect.adjusted(9, 9, -9, -9), 16, 16);

  // 道路類型
  int roadProfile = params.getInt("RoadtypeProfile");
  std::map<int, QString> roadProfileMap = {
    {0, "未選道路"},
    {1, "街道巷弄"},
    {2, "一般平面"},
    {3, "快速道路"},
    {4, "高速公路"},
  };
  QString roadProfileText = roadProfileMap[roadProfile];
  p.setFont(InterFont(45, QFont::Normal));
  p.setPen(QPen(blackColor(), 6));
  p.drawText(info_rect.adjusted(20, 10, 0, 0), Qt::AlignTop | Qt::AlignLeft, roadProfileText);

  // 駕駛模式
  int accProfile = params_memory.getInt("AccelerationProfile");
  std::map<int, QString> accelerationProfileMap = {
    {0, "標準"},
    {1, "節能"},
    {2, "運動"},
    {3, "超跑"},
  };
  QString accProfileText = "駕駛  " + accelerationProfileMap[accProfile];
  p.setFont(InterFont(40, QFont::Normal));
  p.drawText(info_rect.adjusted(20, 65, 0, 0), Qt::AlignTop | Qt::AlignLeft, accProfileText);

  // 車距設定
  std::map<int, QString> personalityProfileMap = {
    {0, "接近"},
    {1, "普通"},
    {2, "遠離"},
  };
  int personalityProfile = params.getInt("LongitudinalPersonality");
  QString profileText = "車距  " + personalityProfileMap[personalityProfile];
  p.setFont(InterFont(40, QFont::Normal));
  p.drawText(info_rect.adjusted(20, 110, 0, 0), Qt::AlignTop | Qt::AlignLeft, profileText);

  //速差顯示
  QString vr_text = "速差  " + QString::number(leadspeed_diffProfile);
  p.drawText(info_rect.adjusted(20, 155, 0, 0), Qt::AlignTop | Qt::AlignLeft, vr_text);


  // 油量顯示
  float tankVolume = carState.getTankvol();
  QString tankVolStr = QString::number(tankVolume);
  if (tankVolume > 30) {
    p.setPen(QPen(blackColor(), 6));
  } else if (tankVolume > 10 && tankVolume <= 30) {
    p.setPen(QPen(QColor(128, 0, 128), 6));
  } else if (tankVolume <= 10) {
    p.setPen(QPen(QColor(255, 0, 0), 6));
  }
  p.drawText(info_rect.adjusted(20, 290, 0, 0), Qt::AlignTop | Qt::AlignLeft, "油量  " + tankVolStr);

  // 油溫顯示
  float oilTemp = carState.getOiltemperature();
  QString oilTempStr = QString::number(oilTemp);
  if (oilTemp < 90) {
    p.setPen(QPen(blackColor(), 6));
  } else if (oilTemp > 90 && oilTemp <= 110) {
    p.setPen(QPen(QColor(128, 0, 128), 6));
  } else if (oilTemp > 110) {
    p.setPen(QPen(QColor(255, 0, 0), 6));
  }
  p.drawText(info_rect.adjusted(20, 340, 0, 0), Qt::AlignTop | Qt::AlignLeft, "油溫  " + oilTempStr);

  // 電壓顯示
  std::stringstream buffer;
  buffer << std::ifstream("/sys/class/hwmon/hwmon1/in1_input").rdbuf();
  float voltage = (float)std::atoi(buffer.str().c_str()) / 1000.0f;
  p.setPen(QPen(blackColor(), 6));
  p.setFont(InterFont(40, QFont::Normal));
  QString batteryVolStr = (voltage > 1) ? QString::number(voltage, 'f', 1) : "–";
  p.drawText(info_rect.adjusted(20, 385, 0, 0), Qt::AlignTop | Qt::AlignLeft, "電壓  " + batteryVolStr);

  // AutoACC 狀態
  bool autoAcc = params.getBool("AutoACC");
  std::map<int, QString> autoAccProfileMap = {
    {0, "手動"},
    {1, "自動"},
  };
  p.setFont(InterFont(40, QFont::Normal));
  if (autoAcc) {
    p.setPen(QPen(redColor(), 6));
  } else {
    p.setPen(QPen(blackColor(), 6));
  }
  QString autoAccProfileText = autoAccProfileMap[autoAcc ? 1 : 0] + " ACC";
  p.drawText(info_rect.adjusted(20, 430, 0, 0), Qt::AlignTop | Qt::AlignLeft, autoAccProfileText);

  // 油價計算（如果啟用）
  bool fuelpriceEnabled = params.getBool("Fuelprice");
  if (fuelpriceEnabled) {
    int fuelCosts = params.getInt("Fuelcosts") / 10;
    int fuelCostsNow = params.getInt("Fuelcostsnow");
    int fuelCostsPre = params.getInt("Fuelcostspre");

    if (fuelCostsNow != 0 && fuelCostsPre == 0) {
      fuelCostsPre = fuelCostsPre + fuelCostsNow / 100;
      params.putInt("Fuelcostsnow", 0);
    }

    float fuelTotal = carState.getFueltotal();
    if (fuelTotal > 0) {
      params.putInt("Fuelcostsnow", fuelCostsPre + (std::round(fuelTotal * 10) / 10 * fuelCosts) * 100);
    }

    int fuelConsumptionNow = params.getInt("Fuelconsumptionnow");
    int fuelConsumptionPre = params.getInt("Fuelconsumptionpre");
    if (fuelConsumptionNow != 0 && fuelConsumptionPre == 0) {
      fuelConsumptionPre = fuelConsumptionPre + fuelConsumptionNow / 100;
      params.putInt("Fuelconsumptionnow", 0);
    }

    if (fuelTotal > 0) {
      params.putInt("Fuelconsumptionnow", fuelConsumptionPre + (std::round(fuelTotal * 100) / 100) * 100);
    }

    p.setPen(QPen(blackColor(), 6));
    QString tankUsedText = "油資  " + QString::number(std::round(fuelTotal * 10) / 10 * fuelCosts);
    p.drawText(info_rect.adjusted(20, 200, 0, 0), Qt::AlignTop | Qt::AlignLeft, tankUsedText);

    QString fuelTotalStr = (fuelTotal > 0) ? QString::number(std::round(fuelTotal * 100) / 100) : "–";
    p.drawText(info_rect.adjusted(20, 245, 0, 0), Qt::AlignTop | Qt::AlignLeft, "已用  " + fuelTotalStr);
  }

  p.restore();
}
/////////////////////////////////////////////////////

