#include "selfdrive/ui/qt/widgets/wifi.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>

WiFiPromptWidget::WiFiPromptWidget(QWidget *parent) : QFrame(parent) {
  stack = new QStackedLayout(this);

  // Setup Wi-Fi
  QFrame *setup = new QFrame;
  QVBoxLayout *setup_layout = new QVBoxLayout(setup);
  setup_layout->setContentsMargins(56, 40, 56, 40);
  setup_layout->setSpacing(20);
  {
    QHBoxLayout *title_layout = new QHBoxLayout;
    title_layout->setSpacing(32);
    {
      QLabel *icon = new QLabel;
      QPixmap pixmap("../assets/offroad/icon_wifi_strength_full.svg");
      icon->setPixmap(pixmap.scaledToWidth(80, Qt::SmoothTransformation));
      title_layout->addWidget(icon);

      QLabel *title = new QLabel(tr("設置 Wi-Fi 連接"));
      title->setStyleSheet("font-size: 64px; font-weight: 600;");
      title_layout->addWidget(title);
      title_layout->addStretch();
    }
    setup_layout->addLayout(title_layout);

    QLabel *desc = new QLabel(tr("請連接至 Wi-Fi 上傳駕駛數據，並協助改進 openpilot"));
    desc->setStyleSheet("font-size: 40px; font-weight: 400;");
    desc->setWordWrap(true);
    setup_layout->addWidget(desc);

    QPushButton *settings_btn = new QPushButton(tr("開啟設定"));
    connect(settings_btn, &QPushButton::clicked, [=]() { emit openSettings(1); });
    settings_btn->setStyleSheet(R"(
      QPushButton {
        font-size: 48px;
        font-weight: 500;
        border-radius: 10px;
        background-color: #465BEA;
        padding: 32px;
      }
      QPushButton:pressed {
        background-color: #3049F4;
      }
    )");
    setup_layout->addWidget(settings_btn);
  }
  stack->addWidget(setup);

  // Uploading data
  QWidget *uploading = new QWidget;
  QVBoxLayout *uploading_layout = new QVBoxLayout(uploading);
  uploading_layout->setContentsMargins(64, 56, 64, 56);
  uploading_layout->setSpacing(36);
  {
    QHBoxLayout *title_layout = new QHBoxLayout;
    {
      QLabel *title = new QLabel(tr("準備上傳"));
      title->setStyleSheet("font-size: 64px; font-weight: 600;");
      title->setWordWrap(true);
      title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      title_layout->addWidget(title);
      title_layout->addStretch();

      QLabel *icon = new QLabel;
      QPixmap pixmap("../assets/offroad/icon_wifi_uploading.svg");
      icon->setPixmap(pixmap.scaledToWidth(120, Qt::SmoothTransformation));
      title_layout->addWidget(icon);
    }
    uploading_layout->addLayout(title_layout);

    QLabel *desc = new QLabel(tr("當您的設備連接 Wi-Fi 時，將定期提取訓練數據"));
    desc->setStyleSheet("font-size: 48px; font-weight: 400;");
    desc->setWordWrap(true);
    uploading_layout->addWidget(desc);
  }
  stack->addWidget(uploading);

  // not uploading data
  QWidget *notUploading = new QWidget;
  QVBoxLayout *not_uploading_layout = new QVBoxLayout(notUploading);
  not_uploading_layout->setContentsMargins(64, 56, 64, 56);
  not_uploading_layout->setSpacing(36);
  {
    QHBoxLayout *title_layout = new QHBoxLayout;
    {
      QLabel *title = new QLabel(tr("Uploading disabled"));
      title->setStyleSheet("font-size: 64px; font-weight: 600;");
      title->setWordWrap(true);
      title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      title_layout->addWidget(title);
      title_layout->addStretch();

      QLabel *icon = new QLabel;
      QPixmap pixmap("../frogpilot/assets/other_images/icon_wifi_uploading_disabled.svg");
      icon->setPixmap(pixmap.scaledToWidth(120, Qt::SmoothTransformation));
      title_layout->addWidget(icon);
    }
    not_uploading_layout->addLayout(title_layout);

    QLabel *desc = new QLabel(tr("Toggle off the 'Disable Uploading' toggle to enable uploads."));
    desc->setStyleSheet("font-size: 48px; font-weight: 400;");
    desc->setWordWrap(true);
    not_uploading_layout->addWidget(desc);
  }
  stack->addWidget(notUploading);

  setStyleSheet(R"(
    WiFiPromptWidget {
      background-color: #333333;
      border-radius: 10px;
    }
  )");

  QObject::connect(uiState(), &UIState::uiUpdate, this, &WiFiPromptWidget::updateState);
}

void WiFiPromptWidget::updateState(const UIState &s) {
  if (!isVisible()) return;

  auto &sm = *(s.sm);

  auto network_type = sm["deviceState"].getDeviceState().getNetworkType();
  auto uploading = network_type == cereal::DeviceState::NetworkType::WIFI ||
      network_type == cereal::DeviceState::NetworkType::ETHERNET;
  bool uploading_disabled = params.getBool("DeviceManagement") && params.getBool("NoUploads");
  stack->setCurrentIndex(uploading_disabled ? 2 : uploading ? 1 : 0);
}
