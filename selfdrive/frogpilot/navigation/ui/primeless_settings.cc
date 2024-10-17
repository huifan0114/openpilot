#include "selfdrive/frogpilot/navigation/ui/primeless_settings.h"

FrogPilotPrimelessPanel::FrogPilotPrimelessPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent) {
  addItem(ipLabel = new LabelControl(tr("管理您的設置"), tr("裝置離線")));

  std::vector<QString> searchOptions{tr("MapBox"), tr("Amap"), tr("Google")};
  searchInput = new ButtonParamControl("SearchInput", tr("目的地搜尋方式"),
                                    tr("在 Navigate on Openpilot 中為目的地查詢選擇搜尋提供者。選項包括 MapBox（建議）、Amap 和 Google 地圖."),
                                       "", searchOptions);
  addItem(searchInput);

  createMapboxKeyControl(publicMapboxKeyControl, tr("Public Mapbox Key"), "MapboxPublicKey", "pk.");
  createMapboxKeyControl(secretMapboxKeyControl, tr("Secret Mapbox Key"), "MapboxSecretKey", "sk.");

  setupButton = new ButtonControl(tr("Mapbox 設定說明"), tr("查看"), tr("查看為「Primeless 導航」設定 MapBox 的說明."), this);
  QObject::connect(setupButton, &ButtonControl::clicked, [this]() {
    displayMapboxInstructions(true);
    openMapBoxInstructions();
    updateStep();
  });
  addItem(setupButton);

  imageLabel = new QLabel(this);
  addItem(imageLabel);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeMapBoxInstructions, [this]() {displayMapboxInstructions(false);});
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotPrimelessPanel::updateState);

  displayMapboxInstructions(false);
}

void FrogPilotPrimelessPanel::showEvent(QShowEvent *event) {
  WifiManager *wifi = new WifiManager(this);
  QString ipAddress = wifi->getIp4Address();
  ipLabel->setText(ipAddress.isEmpty() ? tr("裝置離線") : QString("%1:8082").arg(ipAddress));

  mapboxPublicKeySet = !params.get("MapboxPublicKey").empty();
  mapboxSecretKeySet = !params.get("MapboxSecretKey").empty();
  setupCompleted = mapboxPublicKeySet && mapboxSecretKeySet;

  publicMapboxKeyControl->setText(mapboxPublicKeySet ? tr("移除") : tr("增加"));
  secretMapboxKeyControl->setText(mapboxSecretKeySet ? tr("移除") : tr("增加"));
}

void FrogPilotPrimelessPanel::updateState() {
  if (!isVisible()) {
    return;
  }

  if (imageLabel->isVisible()) {
    mapboxPublicKeySet = !params.get("MapboxPublicKey").empty();
    mapboxSecretKeySet = !params.get("MapboxSecretKey").empty();

    updateStep();
  }
}

void FrogPilotPrimelessPanel::createMapboxKeyControl(ButtonControl *&control, const QString &label, const std::string &paramKey, const QString &prefix) {
  control = new ButtonControl(label, "", tr("管理你的 %1.").arg(label));

  QObject::connect(control, &ButtonControl::clicked, [=] {
    if (control->text() == tr("增加")) {
      QString key = InputDialog::getText(tr("輸入您的 %1").arg(label), this);

      if (!key.startsWith(prefix)) {
        key = prefix + key;
      }
      if (key.length() >= 80) {
        params.putNonBlocking(paramKey, key.toStdString());
      } else {
        FrogPilotConfirmationDialog::toggleAlert(tr("輸入的金鑰無效或太短!"), tr("完成"), this);
      }
    } else {
      if (FrogPilotConfirmationDialog::yesorno(tr("您確定要刪除您的 %1?").arg(label), this)) {
        control->setText(tr("增加"));

        params.remove(paramKey);

        setupCompleted = false;
      }
    }
  });

  control->setText(params.get(paramKey).empty() ? tr("增加") : tr("移除"));
  addItem(control);
}

void FrogPilotPrimelessPanel::hideEvent(QHideEvent *event) {
  displayMapboxInstructions(false);
}

void FrogPilotPrimelessPanel::mousePressEvent(QMouseEvent *event) {
  displayMapboxInstructions(false);
}

void FrogPilotPrimelessPanel::displayMapboxInstructions(bool visible) {
  setUpdatesEnabled(false);

  imageLabel->setVisible(visible);
  ipLabel->setVisible(!visible);
  publicMapboxKeyControl->setVisible(!visible);
  searchInput->setVisible(!visible);
  secretMapboxKeyControl->setVisible(!visible);
  setupButton->setVisible(!visible);

  setUpdatesEnabled(true);
  update();
}

void FrogPilotPrimelessPanel::updateStep() {
  QString currentStep;

  if (setupCompleted) {
    currentStep = "../frogpilot/navigation/navigation_training/setup_completed.png";
  } else if (mapboxPublicKeySet && mapboxSecretKeySet) {
    currentStep = "../frogpilot/navigation/navigation_training/both_keys_set.png";
  } else if (mapboxPublicKeySet) {
    currentStep = "../frogpilot/navigation/navigation_training/public_key_set.png";
  } else {
    currentStep = "../frogpilot/navigation/navigation_training/no_keys_set.png";
  }

  QPixmap pixmap;
  pixmap.load(currentStep);

  imageLabel->setPixmap(pixmap.scaledToWidth(1500, Qt::SmoothTransformation));
  update();
}
