#include "frogpilot/ui/qt/offroad/navigation_settings.h"

FrogPilotNavigationPanel::FrogPilotNavigationPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  primelessLayout = new QStackedLayout();
  addItem(primelessLayout);

  FrogPilotListWidget *settingsList = new FrogPilotListWidget(this);
  ipLabel = new LabelControl(tr("在此管理您的設定"), tr("離線..."));
  settingsList->addItem(ipLabel);

  std::vector<QString> searchOptions{tr("Mapbox"), tr("Amap")};
  searchInput = new FrogPilotButtonsControl(tr("目的地搜尋服務提供者"),
                                            tr("<b>用於「在 Openpilot 上導航」目的地查詢的搜尋服務提供者</b>。選項包括 Mapbox（推薦）和 Amap。"),
                                               "", searchOptions, true);
  QObject::connect(searchInput, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
    amapKeyControl1->setVisible(id == 1);
    amapKeyControl2->setVisible(id == 1);

    params.putInt("SearchInput", id);

    update();
  });
  searchInput->setCheckedButton(params.getInt("SearchInput"));
  settingsList->addItem(searchInput);

  createKeyControl(amapKeyControl1, tr("Amap 金鑰 #1"), "AMapKey1", "", 39, settingsList);
  createKeyControl(amapKeyControl2, tr("Amap 金鑰 #2"), "AMapKey2", "", 39, settingsList);

  createKeyControl(publicMapboxKeyControl, tr("Mapbox 公開金鑰"), "MapboxPublicKey", "pk.", 80, settingsList);
  createKeyControl(secretMapboxKeyControl, tr("Mapbox 私密金鑰"), "MapboxSecretKey", "sk.", 80, settingsList);

  // NavBridge 設定區塊
  navBridgeToggle = new ParamControl("NavBridgeEnabled", tr("NavBridge（Google 地圖）"),
    tr("<b>啟用 NavBridge 以透過 NavBridge Android 應用接收來自 Google 地圖的導航資料</b>。這可讓依據即將到來的轉彎與交叉路口啟用條件性實驗模式。"), "", this);
  settingsList->addItem(navBridgeToggle);

  navBridgeHostControl = new ButtonControl(tr("NavBridge 主機 IP"), "", tr("<b>設定 NavBridge Android 裝置的 IP 位址。</b> 預設若為同一裝置為 \"localhost\"。"));
  QObject::connect(navBridgeHostControl, &ButtonControl::clicked, [this]() {
    QString currentHost = QString::fromStdString(params.get("NavBridgeHost"));
    if (currentHost.isEmpty()) {
      currentHost = "localhost";
    }
    QString newHost = InputDialog::getText(tr("輸入 NavBridge IP"), this, tr("目前：%1").arg(currentHost), false, 1, currentHost).trimmed();
    if (!newHost.isEmpty()) {
      params.put("NavBridgeHost", newHost.toStdString());
      updateNavBridgeHostLabel();
    }
  });
  settingsList->addItem(navBridgeHostControl);

  setupButton = new ButtonControl(tr("Mapbox 設定說明"), tr("檢視"), tr("<b>設定 Mapbox（用於「無儀表導航」）的操作說明</b>。"), this);
  QObject::connect(setupButton, &ButtonControl::clicked, [this]() {
    openSubPanel();

    updateStep();

    primelessLayout->setCurrentIndex(1);
  });
  settingsList->addItem(setupButton);

    std::vector<QString> filterButtonNames{tr("取消"), tr("手動更新速限")};
    updateSpeedLimitsToggle = new FrogPilotButtonControl("SpeedLimitFiller", tr("速限補全"),
                                     tr("<b>在您行駛時自動收集遺失或錯誤的速限資料</b>，資料來源包含車儀（若支援）、Mapbox 與「在 Openpilot 上導航」。<br><br>"
                                       "當您停好車並連上 Wi‑Fi 時，FrogPilot 會自動將這些資料處理成檔案，供位於 \"SpeedLimitFiller.frogpilot.download\" 的工具使用。<br><br>"
                                       "您可以在「下載速限」選單的 \"The Pond\" 下載此檔案。<br><br>"
                                       "需要步驟教學？請至 FrogPilot Discord 的 <b>#speed-limit-filler</b> 頻道查看！"),
                                       "", filterButtonNames);
  QObject::connect(updateSpeedLimitsToggle, &FrogPilotButtonControl::buttonClicked, [this](int id) {
    if (id == 0) {
      if (FrogPilotConfirmationDialog::yesorno(tr("取消速限更新嗎？"), this)) {
        updatingLimits = false;

        updateSpeedLimitsToggle->setEnabledButton(0, false);
        updateSpeedLimitsToggle->setValue(tr("已取消..."));

        params_memory.remove("UpdateSpeedLimits");

        QTimer::singleShot(2500, [this]() {
          updateSpeedLimitsToggle->clearCheckedButtons(true);
          updateSpeedLimitsToggle->setEnabledButton(0, true);
          updateSpeedLimitsToggle->setValue("");
          updateSpeedLimitsToggle->setVisibleButton(0, false);
          updateSpeedLimitsToggle->setVisibleButton(1, true);

          params_memory.remove("UpdateSpeedLimitsStatus");
        });
      }
    } else if (id == 1) {
      QJsonObject overpassRequests = QJsonDocument::fromJson(QString::fromStdString(params.get("OverpassRequests")).toUtf8()).object();

      int totalRequests = overpassRequests.value("total_requests").toInt(0);
      int maxRequests = overpassRequests.value("max_requests").toInt(10000);
      int savedDay = overpassRequests.value("day").toInt(QDate::currentDate().day());

      int currentDay = QDate::currentDate().day();

      if (savedDay != currentDay) {
        totalRequests = 0;
      }

      if (totalRequests >= maxRequests) {
        QTime now = QTime::currentTime();

        int secondsUntilMidnight = (24 * 3600) - (now.hour() * 3600 + now.minute() * 60 + now.second());
        int hours = secondsUntilMidnight / 3600;
        int minutes = (secondsUntilMidnight % 3600) / 60;

        ConfirmationDialog::alert(QString(tr("您已達到今日的請求上限。\n\n將在 %1 小時 %2 分鐘後重置。")).arg(hours).arg(minutes), this);

        updateSpeedLimitsToggle->clearCheckedButtons(true);
        return;
      }

      updateSpeedLimitsToggle->setVisibleButton(0, true);
      updateSpeedLimitsToggle->setVisibleButton(1, false);

      if (FrogPilotConfirmationDialog::yesorno(tr("此過程需要一些時間，建議在行程結束且連上穩定 Wi‑Fi 時開始。是否繼續？"), this)) {
        updatingLimits = true;

        updateSpeedLimitsToggle->setValue("Calculating...");

        params_memory.put("UpdateSpeedLimitsStatus", "Calculating...");
        params_memory.putBool("UpdateSpeedLimits", true);
      } else {
        updateSpeedLimitsToggle->setVisibleButton(0, false);
        updateSpeedLimitsToggle->setVisibleButton(1, true);

        updateSpeedLimitsToggle->clearCheckedButtons(true);
      }
    }
  });
  updateSpeedLimitsToggle->setVisibleButton(0, false);
  settingsList->addItem(updateSpeedLimitsToggle);

  ScrollView *settingsPanel = new ScrollView(settingsList, this);
  primelessLayout->addWidget(settingsPanel);

  imageLabel = new QLabel(this);

  ScrollView *instructionsPanel = new ScrollView(imageLabel, this);
  primelessLayout->addWidget(instructionsPanel);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [this]() {
    primelessLayout->setCurrentIndex(0);

    if (forceOpenDescriptions) {
      amapKeyControl1->showDescription();
      amapKeyControl2->showDescription();
      publicMapboxKeyControl->showDescription();
      searchInput->showDescription();
      secretMapboxKeyControl->showDescription();
      setupButton->showDescription();
      updateSpeedLimitsToggle->showDescription();
      navBridgeToggle->showDescription();
      navBridgeHostControl->showDescription();
    }
  });
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotNavigationPanel::updateState);
}

void FrogPilotNavigationPanel::updateNavBridgeHostLabel() {
  QString host = QString::fromStdString(params.get("NavBridgeHost"));
  if (host.isEmpty()) {
    host = "localhost";
  }
  navBridgeHostControl->setText(host);
}

void FrogPilotNavigationPanel::showEvent(QShowEvent *event) {
  if (forceOpenDescriptions) {
    amapKeyControl1->showDescription();
    amapKeyControl2->showDescription();
    publicMapboxKeyControl->showDescription();
    searchInput->showDescription();
    secretMapboxKeyControl->showDescription();
    setupButton->showDescription();
    updateSpeedLimitsToggle->showDescription();
    navBridgeToggle->showDescription();
    navBridgeHostControl->showDescription();
  }

  // 更新 NavBridge Host 顯示
  updateNavBridgeHostLabel();

  FrogPilotUIState &fs = *frogpilotUIState();
  UIState &s = *uiState();

  FrogPilotUIScene &frogpilot_scene = fs.frogpilot_scene;

  QString ipAddress = fs.wifi->getIp4Address();
  ipLabel->setText(ipAddress.isEmpty() ? tr("離線...") : QString("%1:8082").arg(ipAddress));

  updateButtons();

  setupCompleted = mapboxPublicKeySet && mapboxSecretKeySet;
  updatingLimits = !params_memory.get("UpdateSpeedLimitsStatus").empty() && QString::fromStdString(params_memory.get("UpdateSpeedLimitsStatus")) != "Completed!";

  bool parked = !s.scene.started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  int selectedSearchInput = params.getInt("SearchInput");

  amapKeyControl1->setVisible(selectedSearchInput == 1);
  amapKeyControl2->setVisible(selectedSearchInput == 1);

  updateSpeedLimitsToggle->setVisibleButton(0, updatingLimits);
  updateSpeedLimitsToggle->setVisibleButton(1, !updatingLimits);

  if (updatingLimits) {
    updateSpeedLimitsToggle->setValue(QString::fromStdString(params_memory.get("UpdateSpeedLimitsStatus")));
  } else {
    updateSpeedLimitsToggle->setEnabledButton(1, frogpilot_scene.online && util::system_time_valid() && parked);
    updateSpeedLimitsToggle->setValue(frogpilot_scene.online ? (parked ? "" : "Not parked") : tr("離線..."));
    updateSpeedLimitsToggle->setVisible(parent->tuningLevel >= parent->frogpilotToggleLevels["SpeedLimitFiller"].toDouble());
  }
}

void FrogPilotNavigationPanel::hideEvent(QHideEvent *event) {
  primelessLayout->setCurrentIndex(0);
}

void FrogPilotNavigationPanel::mousePressEvent(QMouseEvent *event) {
  if (primelessLayout->currentIndex() == 1) {
    closeSubPanel();

    primelessLayout->setCurrentIndex(0);

    if (forceOpenDescriptions) {
      amapKeyControl1->showDescription();
      amapKeyControl2->showDescription();
      publicMapboxKeyControl->showDescription();
      searchInput->showDescription();
      secretMapboxKeyControl->showDescription();
      setupButton->showDescription();
      updateSpeedLimitsToggle->showDescription();
      navBridgeToggle->showDescription();
      navBridgeHostControl->showDescription();
    }
  }
}

void FrogPilotNavigationPanel::createKeyControl(ButtonControl *&control, const QString &label, const std::string &paramKey, const QString &prefix, const int &minLength, FrogPilotListWidget *list) {
  control = new ButtonControl(label, "", tr("<b>管理您的「%1」。</b>").arg(label));
  QObject::connect(control, &ButtonControl::clicked, [=] {
    if (control->text() == tr("加入")) {
      QString key = InputDialog::getText(tr("Enter your %1").arg(label), this).trimmed();

      if (!key.startsWith(prefix)) {
        key = prefix + key;
      }

      if (key.length() >= minLength) {
        params.put(paramKey, key.toStdString());
      } else {
        ConfirmationDialog::alert(tr("輸入的金鑰無效或太短！"), this);
      }
    } else {
      if (FrogPilotConfirmationDialog::yesorno(tr("您要移除 %1 嗎？").arg(label), this)) {
        control->setText(tr("增加"));

        params.remove(paramKey);
        params_cache.remove(paramKey);

        setupCompleted = false;
      }
    }
  });
  control->setText(QString::fromStdString(params.get(paramKey)).startsWith(prefix) ? tr("移除") : tr("加入"));
  list->addItem(control);
}

void FrogPilotNavigationPanel::updateButtons() {
  amapKeyControl1->setText(params.get("AMapKey1").empty() ? tr("增加") : tr("移除"));
  amapKeyControl2->setText(params.get("AMapKey2").empty() ? tr("增加") : tr("移除"));

  mapboxPublicKeySet = QString::fromStdString(params.get("MapboxPublicKey")).startsWith("pk");
  mapboxSecretKeySet = QString::fromStdString(params.get("MapboxSecretKey")).startsWith("sk");

  publicMapboxKeyControl->setText(mapboxPublicKeySet ? tr("移除") : tr("增加"));
  secretMapboxKeyControl->setText(mapboxSecretKeySet ? tr("移除") : tr("增加"));
}

void FrogPilotNavigationPanel::updateState(const UIState &s, const FrogPilotUIState &fs) {
  if (!isVisible() || s.sm->frame % (UI_FREQ / 2) != 0) {
    return;
  }

  updateButtons();
  updateStep();

  bool parked = !s.scene.started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  if (updatingLimits) {
    if (QString::fromStdString(params_memory.get("UpdateSpeedLimitsStatus")) == "Completed!") {
      updatingLimits = false;

      updateSpeedLimitsToggle->setValue(tr("完成!"));

      QTimer::singleShot(2500, [this]() {
        updateSpeedLimitsToggle->clearCheckedButtons(true);
        updateSpeedLimitsToggle->setValue("");
        updateSpeedLimitsToggle->setVisibleButton(0, false);
        updateSpeedLimitsToggle->setVisibleButton(1, true);

        params_memory.remove("UpdateSpeedLimitsStatus");
      });
    } else {
      updateSpeedLimitsToggle->setValue(QString::fromStdString(params_memory.get("UpdateSpeedLimitsStatus")));
    }
  } else {
    updateSpeedLimitsToggle->setEnabledButton(1, fs.frogpilot_scene.online && util::system_time_valid() && parked);
    updateSpeedLimitsToggle->setValue(fs.frogpilot_scene.online ? (parked ? "" : "Not parked") : tr("離線..."));
  }

  parent->keepScreenOn = primelessLayout->currentIndex() == 1 || updatingLimits;
}

void FrogPilotNavigationPanel::updateStep() {
  QString currentStep;
  if (setupCompleted) {
    currentStep = "../../frogpilot/navigation/navigation_training/setup_completed.png";
  } else if (mapboxPublicKeySet && mapboxSecretKeySet) {
    currentStep = "../../frogpilot/navigation/navigation_training/both_keys_set.png";
  } else if (mapboxPublicKeySet) {
    currentStep = "../../frogpilot/navigation/navigation_training/public_key_set.png";
  } else {
    currentStep = "../../frogpilot/navigation/navigation_training/no_keys_set.png";
  }

  QPixmap pixmap;
  pixmap.load(currentStep);
  imageLabel->setPixmap(pixmap.scaledToWidth(1500, Qt::SmoothTransformation));

  update();
}
