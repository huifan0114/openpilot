#include <regex>

#include <QtConcurrent>

#include "frogpilot/ui/qt/offroad/maps_settings.h"

FrogPilotMapsPanel::FrogPilotMapsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *mapsLayout = new QStackedLayout();
  addItem(mapsLayout);

  FrogPilotListWidget *settingsList = new FrogPilotListWidget(this);

    std::vector<QString> scheduleOptions{tr("手動"), tr("每週"), tr("每月")};
    preferredSchedule = new ButtonParamControl("PreferredSchedule", tr("自動更新地圖"),
                              tr("<b>從 \"OpenStreetMap (OSM)\" 更新地圖頻率</b>，以取得最新的速限資訊。"
                                "每週更新於每週日；每月更新於每月 1 日。"),
                                "",
                                scheduleOptions);
  settingsList->addItem(preferredSchedule);

  downloadMapsButton = new ButtonControl(tr("下載地圖"), tr("下載"), tr("<b>手動更新您選取的地圖來源</b>，以讓\"速限控制器\"擁有最新的速限資訊。"));
  QObject::connect(downloadMapsButton, &ButtonControl::clicked, [this] {
    if (downloadMapsButton->text() == tr("取消")) {
      if (FrogPilotConfirmationDialog::yesorno(tr("是否取消下載？"), this)) {
        cancelDownload();
      }
    } else {
      startDownload();
    }
  });
  settingsList->addItem(downloadMapsButton);

  settingsList->addItem(lastMapsDownload = new LabelControl(tr("最後更新"), params.get("LastMapsUpdate").empty() ? "Never" : QString::fromStdString(params.get("LastMapsUpdate"))));

    selectMaps = new FrogPilotButtonsControl(tr("地圖來源"),
                               tr("<b>選擇要與\"速限控制器\"一同使用的國家或美國州別。</b>"),
                                 "", {tr("國家"), tr("州")});
  QObject::connect(selectMaps, &FrogPilotButtonsControl::buttonClicked, [mapsLayout, this](int id) {
    mapsLayout->setCurrentIndex(id + 1);

    openSubPanel();
  });
  settingsList->addItem(selectMaps);

  settingsList->addItem(downloadStatus = new LabelControl(tr("進度")));
  settingsList->addItem(downloadTimeElapsed = new LabelControl(tr("已用時間")));
  settingsList->addItem(downloadETA = new LabelControl(tr("剩餘時間")));

  downloadETA->setVisible(false);
  downloadStatus->setVisible(false);
  downloadTimeElapsed->setVisible(false);

  removeMapsButton = new ButtonControl(tr("移除地圖"), tr("移除"), tr("<b>刪除已下載的地圖資料</b>以釋放儲存空間。"));
  QObject::connect(removeMapsButton, &ButtonControl::clicked, [this] {
    if (FrogPilotConfirmationDialog::yesorno(tr("是否刪除所有已下載的地圖？"), this)) {
      std::thread([this] {
        mapsSize->setText("0 MB");

        mapsFolderPath.removeRecursively();
      }).detach();
    }
  });
  settingsList->addItem(removeMapsButton);

  resetMapdButton = new ButtonControl(tr("重置下載器"), tr("重置"),
                                   tr("<b>重置地圖下載器。</b>當下載卡住或失敗時使用。"));
  QObject::connect(resetMapdButton, &ButtonControl::clicked, [parent, this]() {
    if (ConfirmationDialog::confirm(tr("是否重置地圖下載器？裝置將會重新啟動。"), tr("重置"), this)) {
      std::thread([parent, this]() {
        parent->keepScreenOn = true;

        resetMapdButton->setEnabled(false);
        resetMapdButton->setValue(tr("重置中..."));

        std::system("pkill mapd");

        QDir("/data/media/0/osm").removeRecursively();

        resetMapdButton->setValue(tr("已重置!"));

        util::sleep_for(2500);

        resetMapdButton->setValue(tr("重新啟動中..."));

        util::sleep_for(2500);

        Hardware::reboot();
      }).detach();
    }
  });
  settingsList->addItem(resetMapdButton);

  settingsList->addItem(mapsSize = new LabelControl(tr("已使用儲存"), calculateDirectorySize(mapsFolderPath)));

  ScrollView *settingsPanel = new ScrollView(settingsList, this);
  mapsLayout->addWidget(settingsPanel);

  FrogPilotListWidget *countriesList = new FrogPilotListWidget(this);
    std::vector<std::pair<QString, QMap<QString, QString>>> countries = {
    {tr("非洲"), africaMap},
    {tr("南極洲"), antarcticaMap},
    {tr("亞洲"), asiaMap},
    {tr("歐洲"), europeMap},
    {tr("北美洲"), northAmericaMap},
    {tr("大洋洲"), oceaniaMap},
    {tr("南美洲"), southAmericaMap}
  };

  for (std::pair<QString, QMap<QString, QString>> country : countries) {
    countriesList->addItem(new LabelControl(country.first, ""));
    countriesList->addItem(new MapSelectionControl(country.second, true));
  }

  ScrollView *countryMapsPanel = new ScrollView(countriesList, this);
  mapsLayout->addWidget(countryMapsPanel);

  FrogPilotListWidget *statesList = new FrogPilotListWidget(this);
  std::vector<std::pair<QString, QMap<QString, QString>>> states = {
    {tr("美國 - 中西部"), midwestMap},
    {tr("美國 - 東北部"), northeastMap},
    {tr("美國 - 南部"), southMap},
    {tr("美國 - 西部"), westMap},
    {tr("美國 - 屬地"), territoriesMap}
  };

  for (std::pair<QString, QMap<QString, QString>> state : states) {
    statesList->addItem(new LabelControl(state.first, ""));
    statesList->addItem(new MapSelectionControl(state.second));
  }

  ScrollView *stateMapsPanel = new ScrollView(statesList, this);
  mapsLayout->addWidget(stateMapsPanel);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [mapsLayout, settingsPanel, this] {
    if (forceOpenDescriptions) {
      downloadMapsButton->showDescription();
      preferredSchedule->showDescription();
      removeMapsButton->showDescription();
      resetMapdButton->showDescription();
      selectMaps->showDescription();
    }

    std::string mapsSelected = params.get("MapsSelected");
    hasMapsSelected = !QJsonDocument::fromJson(QByteArray::fromStdString(mapsSelected)).object().value("nations").toArray().isEmpty();
    hasMapsSelected |= !QJsonDocument::fromJson(QByteArray::fromStdString(mapsSelected)).object().value("states").toArray().isEmpty();

    mapsLayout->setCurrentWidget(settingsPanel);
  });
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotMapsPanel::updateState);
}

void FrogPilotMapsPanel::showEvent(QShowEvent *event) {
  if (forceOpenDescriptions) {
    downloadMapsButton->showDescription();
    preferredSchedule->showDescription();
    removeMapsButton->showDescription();
    resetMapdButton->showDescription();
    selectMaps->showDescription();
  }

  FrogPilotUIState &fs = *frogpilotUIState();
  UIState &s = *uiState();

  std::string mapsSelected = params.get("MapsSelected");
  hasMapsSelected = !QJsonDocument::fromJson(QByteArray::fromStdString(mapsSelected)).object().value("nations").toArray().isEmpty();
  hasMapsSelected |= !QJsonDocument::fromJson(QByteArray::fromStdString(mapsSelected)).object().value("states").toArray().isEmpty();

  bool parked = !s.scene.started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  removeMapsButton->setVisible(mapsFolderPath.exists());

  std::string osmDownloadProgress = params.get("OSMDownloadProgress");
  if (!osmDownloadProgress.empty()) {
  downloadMapsButton->setText(tr("取消"));
    downloadStatus->setText("Calculating...");

    downloadStatus->setVisible(true);

    lastMapsDownload->setVisible(false);
    removeMapsButton->setVisible(false);
    resetMapdButton->setVisible(false);

    updateDownloadLabels(osmDownloadProgress);
  } else {
    downloadMapsButton->setEnabled(!cancellingDownload && hasMapsSelected && fs.frogpilot_scene.online && parked);
    downloadMapsButton->setValue(fs.frogpilot_scene.online ? (parked ? "" : "Not parked") : tr("離線中..."));
  }
}


void FrogPilotMapsPanel::updateState(const UIState &s, const FrogPilotUIState &fs) {
  if (!isVisible() || s.sm->frame % (UI_FREQ / 2) != 0) {
    return;
  }

  bool parked = !s.scene.started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  std::string osmDownloadProgress = params.get("OSMDownloadProgress");
  if (!osmDownloadProgress.empty() && !cancellingDownload) {
    updateDownloadLabels(osmDownloadProgress);
  } else {
    downloadMapsButton->setEnabled(!cancellingDownload && hasMapsSelected && fs.frogpilot_scene.online && parked);
    downloadMapsButton->setValue(fs.frogpilot_scene.online ? (parked ? "" : "Not parked") : tr("離線中..."));
  }

  parent->keepScreenOn = !osmDownloadProgress.empty();
}

void FrogPilotMapsPanel::cancelDownload() {
  cancellingDownload = true;

  downloadMapsButton->setEnabled(false);

  downloadETA->setText("Cancelling...");
  downloadMapsButton->setText(tr("已取消"));
  downloadStatus->setText("Cancelling...");
  downloadTimeElapsed->setText("Cancelling...");

  params.remove("OSMDownloadProgress");
  params_memory.remove("OSMDownloadLocations");

  std::system("pkill mapd");

  QTimer::singleShot(2500, [this]() {
    cancellingDownload = false;

    downloadMapsButton->setEnabled(true);

    downloadMapsButton->setText(tr("下載"));

    downloadETA->setVisible(false);
    downloadStatus->setVisible(false);
    downloadTimeElapsed->setVisible(false);

    lastMapsDownload->setVisible(true);
    removeMapsButton->setVisible(mapsFolderPath.exists());
    resetMapdButton->setVisible(true);

    update();
  });
}

void FrogPilotMapsPanel::startDownload() {
  downloadETA->setText("Calculating...");
  downloadMapsButton->setText(tr("取消"));
  downloadStatus->setText("Calculating...");
  downloadTimeElapsed->setText("Calculating...");

  downloadETA->setVisible(true);
  downloadStatus->setVisible(true);
  downloadTimeElapsed->setVisible(true);

  lastMapsDownload->setVisible(false);
  removeMapsButton->setVisible(false);
  resetMapdButton->setVisible(false);

  elapsedTime.start();
  startTime = QDateTime::currentDateTime();

  params_memory.put("OSMDownloadLocations", params.get("MapsSelected"));
}

void FrogPilotMapsPanel::updateDownloadLabels(std::string &osmDownloadProgress) {
  static std::regex fileStatusRegex(R"("total_files":(\d+),.*"downloaded_files":(\d+))");

  std::smatch match;
  if (std::regex_search(osmDownloadProgress, match, fileStatusRegex)) {
    int totalFiles = std::stoi(match[1].str());
    int downloadedFiles = std::stoi(match[2].str());

    if (downloadedFiles == totalFiles) {
      downloadMapsButton->setText(tr("下載"));
      lastMapsDownload->setText(formatCurrentDate());

      downloadETA->setVisible(false);
      downloadStatus->setVisible(false);
      downloadTimeElapsed->setVisible(false);

      lastMapsDownload->setVisible(true);
      removeMapsButton->setVisible(true);
      resetMapdButton->setVisible(true);

      params.put("LastMapsUpdate", formatCurrentDate().toStdString());
      params.remove("OSMDownloadProgress");

      update();

      return;
    }

    static int previousDownloadedFiles = 0;
    if (downloadedFiles != previousDownloadedFiles) {
      std::thread([this]() {
        mapsSize->setText(calculateDirectorySize(mapsFolderPath));
      }).detach();
    }

    downloadETA->setText(QString("%1").arg(formatETA(elapsedTime.elapsed(), downloadedFiles, previousDownloadedFiles, totalFiles, startTime)));
    downloadStatus->setText(QString("%1 / %2 (%3%)").arg(downloadedFiles).arg(totalFiles).arg((downloadedFiles * 100) / totalFiles));
    downloadTimeElapsed->setText(formatElapsedTime(elapsedTime.elapsed()));

    previousDownloadedFiles = downloadedFiles;
  }
}
