#include <sys/xattr.h>

#include "frogpilot/ui/qt/offroad/data_settings.h"

FrogPilotDataPanel::FrogPilotDataPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  bool forceOpenDescriptions = false;
  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *dataLayout = new QStackedLayout();
  addItem(dataLayout);

  FrogPilotListWidget *dataMainList = new FrogPilotListWidget(this);
  ScrollView *dataMainPanel = new ScrollView(dataMainList, this);
  dataLayout->addWidget(dataMainPanel);

  FrogPilotListWidget *statsLabelsList = new FrogPilotListWidget(this);
  ScrollView *statsLabelsPanel = new ScrollView(statsLabelsList, this);
  dataLayout->addWidget(statsLabelsPanel);

  ButtonControl *deleteDrivingDataButton = new ButtonControl(tr("刪除行車數據"), tr("刪除"), tr("<b>刪除所有儲存的行車影片和資料</b>，以釋放空間並清除個人資訊。"));
  QObject::connect(deleteDrivingDataButton, &ButtonControl::clicked, [=]() {
    QDir hdDataDir("/data/media/0/realdata_HD/");
    QDir konikDataDir("/data/media/0/realdata_konik/");
    QDir realDataDir("/data/media/0/realdata/");

    if (ConfirmationDialog::confirm(tr("刪除所有行車數據和錄像?"), tr("刪除"), this)) {
      std::thread([=]() mutable {
        parent->keepScreenOn = true;

        deleteDrivingDataButton->setEnabled(false);
        deleteDrivingDataButton->setValue(tr("刪除中..."));

        QList<QDir> footageDirs = {hdDataDir, konikDataDir, realDataDir};
        for (const QDir &footageDir : footageDirs) {
          if (!footageDir.exists()) {
            continue;
          }

          QFileInfoList entries = footageDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
          for (const QFileInfo &entry : entries) {
            char value[10] = {0};
            if (!(getxattr(entry.absoluteFilePath().toUtf8().constData(), "user.preserve", value, sizeof(value)) > 0 && strcmp(value, "1") == 0)) {
              QDir(entry.absoluteFilePath()).removeRecursively();
            }
          }
        }

        deleteDrivingDataButton->setValue(tr("已刪除!"));

        util::sleep_for(2500);

        deleteDrivingDataButton->setEnabled(true);
        deleteDrivingDataButton->setValue("");

        parent->keepScreenOn = false;
      }).detach();
    }
  });
  if (forceOpenDescriptions) {
    deleteDrivingDataButton->showDescription();
  }
  dataMainList->addItem(deleteDrivingDataButton);

  ButtonControl *deleteErrorLogsButton = new ButtonControl(tr("刪除錯誤日誌"), tr("刪除"), tr("<b>刪除收集的錯誤日誌</b>，以釋放空間並清除舊的崩潰紀錄。"));
  QObject::connect(deleteErrorLogsButton, &ButtonControl::clicked, [=]() {
    QDir errorLogsDir("/data/error_logs");

    if (ConfirmationDialog::confirm(tr("是否刪除所有錯誤日誌？"), tr("刪除"), this)) {
      std::thread([=]() mutable {
        parent->keepScreenOn = true;

        deleteErrorLogsButton->setEnabled(false);
        deleteErrorLogsButton->setValue(tr("刪除中..."));

        errorLogsDir.removeRecursively();
        errorLogsDir.mkpath(".");

        deleteErrorLogsButton->setValue(tr("已刪除!"));

        util::sleep_for(2500);

        deleteErrorLogsButton->setEnabled(true);
        deleteErrorLogsButton->setValue("");

        parent->keepScreenOn = false;
      }).detach();
    }
  });
  if (forceOpenDescriptions) {
    deleteErrorLogsButton->showDescription();
  }
  dataMainList->addItem(deleteErrorLogsButton);

  FrogPilotButtonsControl *screenRecordingsButton = new FrogPilotButtonsControl(tr("螢幕錄影"), tr("<b>刪除或重新命名螢幕錄影。</b>"), "", {tr("刪除"), tr("全部刪除"), tr("重新命名")});
  QObject::connect(screenRecordingsButton, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir recordingsDir("/data/media/screen_recordings");
    QStringList recordingsNames = recordingsDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    QStringList mp4Recordings;
    for (const QString &name : recordingsNames) {
      if (name.endsWith(".mp4", Qt::CaseInsensitive)) {
        mp4Recordings << name;
      }
    }

    if (id == 0) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇要刪除的螢幕錄影"), mp4Recordings, "", this);
      if (!selection.isEmpty()) {
        if (ConfirmationDialog::confirm(tr("是否刪除此螢幕錄影？"), tr("刪除"), this)) {
          std::thread([=]() {
            parent->keepScreenOn = true;

            screenRecordingsButton->setEnabled(false);
            screenRecordingsButton->setValue(tr("刪除中..."));

            screenRecordingsButton->setVisibleButton(1, false);
            screenRecordingsButton->setVisibleButton(2, false);

            QFile::remove(recordingsDir.absoluteFilePath(selection));

            screenRecordingsButton->setValue(tr("已刪除!"));

            util::sleep_for(2500);

            screenRecordingsButton->setEnabled(true);
            screenRecordingsButton->setValue("");

            screenRecordingsButton->setVisibleButton(1, true);
            screenRecordingsButton->setVisibleButton(2, true);

            parent->keepScreenOn = false;
          }).detach();
        }
      }

    } else if (id == 1) {
      if (ConfirmationDialog::confirm(tr("是否刪除所有螢幕錄影？"), tr("全部刪除"), this)) {
        std::thread([=]() mutable {
          parent->keepScreenOn = true;

          screenRecordingsButton->setEnabled(false);
          screenRecordingsButton->setValue(tr("刪除中..."));

          screenRecordingsButton->setVisibleButton(0, false);
          screenRecordingsButton->setVisibleButton(2, false);

          recordingsDir.removeRecursively();
          recordingsDir.mkpath(".");

          screenRecordingsButton->setValue(tr("已刪除!"));

          util::sleep_for(2500);

          screenRecordingsButton->setEnabled(true);
          screenRecordingsButton->setValue("");

          screenRecordingsButton->setVisibleButton(0, true);
          screenRecordingsButton->setVisibleButton(2, true);

          parent->keepScreenOn = false;
        }).detach();
      }

    } else if (id == 2) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇要重新命名的螢幕錄影"), mp4Recordings, "", this);
      if (!selection.isEmpty()) {
        QString newBase = InputDialog::getText(tr("輸入新名稱"), this, tr("重新命名螢幕錄影")).trimmed().replace(" ", "_");
        if (!newBase.isEmpty()) {
          QString newName = newBase + ".mp4";
          if (recordingsNames.contains(newName)) {
            ConfirmationDialog::alert(tr("名稱已被使用。請選擇其他名稱。"), this);
            return;
          }
          std::thread([=]() {
            parent->keepScreenOn = true;

            screenRecordingsButton->setEnabled(false);
            screenRecordingsButton->setValue(tr("重新命名中..."));

            screenRecordingsButton->setVisibleButton(0, false);
            screenRecordingsButton->setVisibleButton(1, false);

            QString newPath = recordingsDir.absoluteFilePath(newName);
            QString oldPath = recordingsDir.absoluteFilePath(selection);
            QFile::rename(oldPath, newPath);

            screenRecordingsButton->setValue(tr("重新命名完成!"));

            util::sleep_for(2500);

            screenRecordingsButton->setEnabled(true);
            screenRecordingsButton->setValue("");

            screenRecordingsButton->setVisibleButton(0, true);
            screenRecordingsButton->setVisibleButton(1, true);

            parent->keepScreenOn = false;
          }).detach();
        }
      }
    }
  });
  if (forceOpenDescriptions) {
    screenRecordingsButton->showDescription();
  }
  dataMainList->addItem(screenRecordingsButton);

  FrogPilotButtonsControl *frogpilotBackupButton = new FrogPilotButtonsControl(tr("FrogPilot 備份"), tr("<b>建立、刪除或還原 FrogPilot 備份。</b>"), "", {tr("備份"), tr("刪除"), tr("全部刪除"), tr("還原")});
  QObject::connect(frogpilotBackupButton, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir backupDir("/data/backups");
    QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name).filter(QRegularExpression("^(?!.*_in_progress(?:\\..*)?$).*$"));

    QRegularExpression autoRegex("^(.*)_(\\d{4}-\\d{2}-\\d{2})_auto(?:\\..*)?$");

    QMap<QString, QString> backupFriendlyMap;
    for (const QString &name : backupNames) {
      QString friendly = name;

      QRegularExpressionMatch match = autoRegex.match(name);
      if (match.hasMatch()) {
        friendly = match.captured(1) + ": " + match.captured(2);
      }

      backupFriendlyMap.insert(friendly, name);
    }

    if (id == 0) {
      QString nameSelection = InputDialog::getText(tr("為此備份輸入名稱"), this, "", false, 1).trimmed().replace(" ", "_");
      if (!nameSelection.isEmpty()) {
        if (backupNames.contains(nameSelection)) {
          ConfirmationDialog::alert(tr("名稱已被使用。請選擇其他名稱。"), this);
          return;
        }
        bool compressed = FrogPilotConfirmationDialog::yesorno(tr("是否壓縮此備份？壓縮可節省空間並在背景執行，但會花較長時間。"), this);
        std::thread([=]() {
          parent->keepScreenOn = true;

          frogpilotBackupButton->setEnabled(false);
          frogpilotBackupButton->setValue(tr("備份中..."));

          frogpilotBackupButton->setVisibleButton(1, false);
          frogpilotBackupButton->setVisibleButton(2, false);
          frogpilotBackupButton->setVisibleButton(3, false);

          QString fullBackupPath = backupDir.filePath(nameSelection);
          QString inProgressBackupPath = fullBackupPath + "_in_progress";

          QDir().mkpath(inProgressBackupPath);
          std::system(qPrintable("rsync -av /data/openpilot/ " + inProgressBackupPath + "/"));

          if (compressed) {
            frogpilotBackupButton->setValue(tr("壓縮中..."));

            std::system(qPrintable("tar -cf - -C " + inProgressBackupPath + " . | zstd -2 -T0 -o " + fullBackupPath + "_in_progress.tar.zst"));

            QDir(inProgressBackupPath).removeRecursively();

            QString oldTar = fullBackupPath + "_in_progress.tar.zst";
            QString newTar = fullBackupPath + ".tar.zst";
            QFile::rename(oldTar, newTar);
          } else {
            QDir().rename(inProgressBackupPath, fullBackupPath);
          }

            frogpilotBackupButton->setValue(tr("備份已建立！"));

          util::sleep_for(2500);

          frogpilotBackupButton->setEnabled(true);
          frogpilotBackupButton->setValue("");

          frogpilotBackupButton->setVisibleButton(1, true);
          frogpilotBackupButton->setVisibleButton(2, true);
          frogpilotBackupButton->setVisibleButton(3, true);

          parent->keepScreenOn = false;
        }).detach();
      }

    } else if (id == 1) {
      QString selectionFriendly = MultiOptionDialog::getSelection(tr("選擇要刪除的 FrogPilot 備份"), backupFriendlyMap.keys(), "", this);
      if (!selectionFriendly.isEmpty()) {
        QString selection = backupFriendlyMap.value(selectionFriendly);
        if (ConfirmationDialog::confirm(tr("刪除此備份？"), tr("刪除"), this)) {
          std::thread([=]() {
            parent->keepScreenOn = true;

            frogpilotBackupButton->setEnabled(false);
            frogpilotBackupButton->setValue(tr("刪除中..."));

            frogpilotBackupButton->setVisibleButton(0, false);
            frogpilotBackupButton->setVisibleButton(2, false);
            frogpilotBackupButton->setVisibleButton(3, false);

            if (selection.endsWith(".tar.gz") || selection.endsWith(".tar.zst")) {
              QFile::remove(backupDir.filePath(selection));
            } else {
              QDir(backupDir.filePath(selection)).removeRecursively();
            }

            frogpilotBackupButton->setValue(tr("已刪除！"));

            util::sleep_for(2500);

            frogpilotBackupButton->setEnabled(true);
            frogpilotBackupButton->setValue("");

            frogpilotBackupButton->setVisibleButton(0, true);
            frogpilotBackupButton->setVisibleButton(2, true);
            frogpilotBackupButton->setVisibleButton(3, true);

            parent->keepScreenOn = false;
          }).detach();
        }
      }

    } else if (id == 2) {
      if (ConfirmationDialog::confirm(tr("刪除所有備份？"), tr("全部刪除"), this)) {
        std::thread([=]() mutable {
          parent->keepScreenOn = true;

          frogpilotBackupButton->setEnabled(false);
          frogpilotBackupButton->setValue(tr("刪除中..."));

          frogpilotBackupButton->setVisibleButton(0, false);
          frogpilotBackupButton->setVisibleButton(1, false);
          frogpilotBackupButton->setVisibleButton(3, false);

          backupDir.removeRecursively();
          backupDir.mkpath(".");

          frogpilotBackupButton->setValue(tr("已刪除！"));

          util::sleep_for(2500);

          frogpilotBackupButton->setEnabled(true);
          frogpilotBackupButton->setValue("");

          frogpilotBackupButton->setVisibleButton(0, true);
          frogpilotBackupButton->setVisibleButton(1, true);
          frogpilotBackupButton->setVisibleButton(3, true);

          parent->keepScreenOn = false;
        }).detach();
      }

    } else if (id == 3) {
      QString selectionFriendly = MultiOptionDialog::getSelection(tr("選擇要還原的備份"), backupFriendlyMap.keys(), "", this);
      if (!selectionFriendly.isEmpty()) {
        QString selection = backupFriendlyMap.value(selectionFriendly);
        if (ConfirmationDialog::confirm(tr("還原此備份？"), tr("還原"), this)) {
          std::thread([=]() {
            parent->keepScreenOn = true;

            frogpilotBackupButton->setEnabled(false);
            frogpilotBackupButton->setValue(tr("還原中..."));

            frogpilotBackupButton->setVisibleButton(0, false);
            frogpilotBackupButton->setVisibleButton(1, false);
            frogpilotBackupButton->setVisibleButton(2, false);

            QString extractDirectory = "/data/restore_temp";
            QString sourcePath = backupDir.filePath(selection);
            QString targetPath = "/data/safe_staging/finalized";

            QDir().mkpath(extractDirectory);

            if (selection.endsWith(".tar.gz")) {
              frogpilotBackupButton->setValue(tr("解壓縮中..."));

              std::system(qPrintable("tar --strip-components=1 -xzf " + sourcePath + " -C " + extractDirectory));
            } else if (selection.endsWith(".tar.zst")) {
              frogpilotBackupButton->setValue(tr("解壓縮中..."));

              std::system(qPrintable("zstd -d " + sourcePath + " -o " + extractDirectory + "/backup.tar"));
              std::system(qPrintable("tar --strip-components=1 -xf " + extractDirectory + "/backup.tar -C " + extractDirectory));

              QFile::remove(extractDirectory + "/backup.tar");
            } else {
              std::system(qPrintable("rsync -av " + sourcePath + "/ " + extractDirectory + "/"));
            }

            QDir().mkpath(targetPath);

            std::system(qPrintable("rsync -av --delete -l " + extractDirectory + "/ " + targetPath + "/"));

            QFile overlayFile(targetPath + "/.overlay_consistent");
            overlayFile.open(QIODevice::WriteOnly);
            overlayFile.close();

            if (QFileInfo::exists(extractDirectory)) {
              QDir(extractDirectory).removeRecursively();
            }

            QFile("/cache/on_backup").open(QIODevice::WriteOnly);

              frogpilotBackupButton->setValue(tr("已還原！"));

            util::sleep_for(2500);

            frogpilotBackupButton->setValue(tr("重新啟動中..."));

            util::sleep_for(2500);

            Hardware::reboot();
          }).detach();
        }
      }
    }
  });
  if (forceOpenDescriptions) {
    frogpilotBackupButton->showDescription();
  }
  dataMainList->addItem(frogpilotBackupButton);

  FrogPilotButtonsControl *toggleBackupButton = new FrogPilotButtonsControl(tr("切換備份"), tr("<b>建立、刪除或還原切換備份。</b>"), "", {tr("備份"), tr("刪除"), tr("全部刪除"), tr("還原")});
  QObject::connect(toggleBackupButton, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir backupDir("/data/toggle_backups");
    QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name).filter(QRegularExpression("^(?!.*_in_progress$).*$"));

    QRegularExpression autoRegex("^(\\d{4}-\\d{2}-\\d{2})_(\\d{2}-\\d{2}[APMapm]{2})_auto(?:\\..*)?$");

    QMap<QString, QString> backupFriendlyMap;
    for (const QString &name : backupNames) {
      QRegularExpressionMatch match = autoRegex.match(name);

      QString friendly = name;
      if (match.hasMatch()) {
        QString datePart = match.captured(1);
        QString timePart = match.captured(2);

        timePart.replace("-", ":");
        if (timePart.endsWith("pm", Qt::CaseInsensitive)) {
          timePart = timePart.left(timePart.size() - 2) + " PM";
        } else if (timePart.endsWith("am", Qt::CaseInsensitive)) {
          timePart = timePart.left(timePart.size() - 2) + " AM";
        }

        friendly = datePart + " - " + timePart;
      }
      backupFriendlyMap.insert(friendly, name);
    }

    if (id == 0) {
      QString nameSelection = InputDialog::getText(tr("為此備份輸入名稱"), this, "", false, 1).trimmed().replace(" ", "_");
      if (!nameSelection.isEmpty()) {
        if (backupNames.contains(nameSelection)) {
          ConfirmationDialog::alert(tr("名稱已被使用。請選擇其他名稱。"), this);
          return;
        }
        std::thread([=]() {
          parent->keepScreenOn = true;

          toggleBackupButton->setEnabled(false);
          toggleBackupButton->setValue(tr("備份中..."));

          toggleBackupButton->setVisibleButton(1, false);
          toggleBackupButton->setVisibleButton(2, false);
          toggleBackupButton->setVisibleButton(3, false);

          QString fullBackupPath = backupDir.filePath(nameSelection);
          QString inProgressBackupPath = fullBackupPath + "_in_progress";

          QDir().mkpath(inProgressBackupPath);

          std::system(qPrintable("rsync -av /data/params/d/ " + inProgressBackupPath + "/"));

          QDir().rename(inProgressBackupPath, fullBackupPath);

          toggleBackupButton->setValue(tr("備份已建立！"));

          util::sleep_for(2500);

          toggleBackupButton->setEnabled(true);
          toggleBackupButton->setValue("");

          toggleBackupButton->setVisibleButton(1, true);
          toggleBackupButton->setVisibleButton(2, true);
          toggleBackupButton->setVisibleButton(3, true);

          parent->keepScreenOn = false;
        }).detach();
      }

    } else if (id == 1) {
      QString selectionFriendly = MultiOptionDialog::getSelection(tr("選擇要刪除的備份"), backupFriendlyMap.keys(), "", this);
      if (!selectionFriendly.isEmpty()) {
        QString selection = backupFriendlyMap.value(selectionFriendly);
        if (ConfirmationDialog::confirm(tr("刪除此備份？"), tr("刪除"), this)) {
          std::thread([=]() {
            parent->keepScreenOn = true;

            toggleBackupButton->setEnabled(false);
            toggleBackupButton->setValue(tr("刪除中..."));

            toggleBackupButton->setVisibleButton(0, false);
            toggleBackupButton->setVisibleButton(2, false);
            toggleBackupButton->setVisibleButton(3, false);

            QDir dirToDelete(backupDir.filePath(selection));
            dirToDelete.removeRecursively();

            toggleBackupButton->setValue(tr("已刪除！"));

            util::sleep_for(2500);

            toggleBackupButton->setEnabled(true);
            toggleBackupButton->setValue("");

            toggleBackupButton->setVisibleButton(0, true);
            toggleBackupButton->setVisibleButton(2, true);
            toggleBackupButton->setVisibleButton(3, true);

            parent->keepScreenOn = false;
          }).detach();
        }
      }

    } else if (id == 2) {
      if (ConfirmationDialog::confirm(tr("刪除所有備份？"), tr("全部刪除"), this)) {
        std::thread([=]() mutable {
          parent->keepScreenOn = true;

          toggleBackupButton->setEnabled(false);
          toggleBackupButton->setValue(tr("刪除中..."));

          toggleBackupButton->setVisibleButton(0, false);
          toggleBackupButton->setVisibleButton(1, false);
          toggleBackupButton->setVisibleButton(3, false);

          backupDir.removeRecursively();
          backupDir.mkpath(".");

          toggleBackupButton->setValue(tr("已刪除！"));

          util::sleep_for(2500);

          toggleBackupButton->setEnabled(true);
          toggleBackupButton->setValue("");

          toggleBackupButton->setVisibleButton(0, true);
          toggleBackupButton->setVisibleButton(1, true);
          toggleBackupButton->setVisibleButton(3, true);

          parent->keepScreenOn = false;
        }).detach();
      }

    } else if (id == 3) {
      QString selectionFriendly = MultiOptionDialog::getSelection(tr("選擇要還原的備份"), backupFriendlyMap.keys(), "", this);
      if (!selectionFriendly.isEmpty()) {
        QString selection = backupFriendlyMap.value(selectionFriendly);
        if (ConfirmationDialog::confirm(tr("還原此備份？"), tr("還原"), this)) {
          std::thread([=]() {
            parent->keepScreenOn = true;

            toggleBackupButton->setEnabled(false);
            toggleBackupButton->setValue(tr("還原中..."));

            toggleBackupButton->setVisibleButton(0, false);
            toggleBackupButton->setVisibleButton(1, false);
            toggleBackupButton->setVisibleButton(2, false);

            QString sourcePath = backupDir.filePath(selection);
            QString targetPath = "/data/params/d";

            QDir().mkpath(targetPath);

            std::system(qPrintable("rsync -av -l " + sourcePath + "/ " + targetPath + "/"));

            updateFrogPilotToggles();

            toggleBackupButton->setValue(tr("已還原！"));

            util::sleep_for(2500);

            toggleBackupButton->setEnabled(true);
            toggleBackupButton->setValue("");

            toggleBackupButton->setVisibleButton(0, true);
            toggleBackupButton->setVisibleButton(1, true);
            toggleBackupButton->setVisibleButton(2, true);

            parent->keepScreenOn = false;
          }).detach();
        }
      }
    }
  });
  if (forceOpenDescriptions) {
    toggleBackupButton->showDescription();
  }
  dataMainList->addItem(toggleBackupButton);

  FrogPilotButtonsControl *viewStatsButton = new FrogPilotButtonsControl(tr("FrogPilot Stats"), tr("<b>View your collected FrogPilot stats.</b>"), "", {tr("RESET"), tr("VIEW")});
  QObject::connect(viewStatsButton, &FrogPilotButtonsControl::buttonClicked, [dataLayout, statsLabelsPanel, this](int id) {
    if (id == 0) {
      if (ConfirmationDialog::confirm(tr("Are you sure you want to reset all of your FrogPilot stats?"), tr("Reset"), this)) {
        params.remove("FrogPilotStats");
        params_cache.remove("FrogPilotStats");
      }
    } else if (id == 1) {
      emit openSubPanel();
      dataLayout->setCurrentWidget(statsLabelsPanel);
    }
  });
  if (forceOpenDescriptions) {
    viewStatsButton->showDescription();
  }
  dataMainList->addItem(viewStatsButton);

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [dataLayout, dataMainPanel] {
    dataLayout->setCurrentWidget(dataMainPanel);
  });
  QObject::connect(parent, &FrogPilotSettingsWindow::updateMetric, [this](bool metric){isMetric = metric;});
  QObject::connect(uiState(), &UIState::offroadTransition, [statsLabelsList, this](bool offroad) {
    if (offroad) {
      updateStatsLabels(statsLabelsList);
    }
  });

  updateStatsLabels(statsLabelsList);
}

void FrogPilotDataPanel::updateStatsLabels(FrogPilotListWidget *labelsList) {
  labelsList->clear();

  QJsonObject stats = QJsonDocument::fromJson(QByteArray::fromStdString(params.get("FrogPilotStats"))).object();

  static QMap<QString, QPair<QString, QString>> keyMap = {
    {"AEBEvents", {tr("Total Emergency Brake Alerts"), "count"}},
    {"AOLTime", {tr("Time Using \"Always On Lateral\""), "timePercent"}},
    {"CruiseSpeedTimes", {tr("Favorite Set Speed"), "speed"}},
    {"CurrentMonthsMeters", {tr("Distance Driven This Month"), "distance"}},
    {"DayTime", {tr("Time Driving (Daytime)"), "timePercent"}},
    {"Disengages", {tr("Total Disengagements"), "count"}},
    {"Engages", {tr("Total Engagements"), "count"}},
    {"ExperimentalModeTime", {tr("Time Using \"Experimental Mode\""), "timePercent"}},
    {"FrogChirps", {tr("Total Frog Chirps"), "count"}},
    {"FrogHops", {tr("Total Frog Hops"), "count"}},
    {"FrogPilotDrives", {tr("Total Drives"), "count"}},
    {"FrogPilotMeters", {tr("Total Distance Driven"), "distance"}},
    {"FrogPilotSeconds", {tr("Total Driving Time"), "time"}},
    {"FrogSqueaks", {tr("Total Frog Squeaks"), "count"}},
    {"GoatScreams", {tr("Total Goat Screams"), "count"}},
    {"HighestAcceleration", {tr("Highest Acceleration Rate"), "accel"}},
    {"LateralTime", {tr("Time Using Lateral Control"), "timePercent"}},
    {"LongestDistanceWithoutOverride", {tr("Longest Distance Without an Override"), "distance"}},
    {"LongitudinalTime", {tr("Time Using Longitudinal Control"), "timePercent"}},
    {"ModelTimes", {tr("Driving Models:"), "parent"}},
    {"Month", {tr("Month"), "other"}},
    {"NightTime", {tr("Time Driving (Nighttime)"), "timePercent"}},
    {"Overrides", {tr("Total Overrides"), "count"}},
    {"OverrideTime", {tr("Time Overriding openpilot"), "timePercent"}},
    {"PersonalityTimes", {tr("Driving Personalities:"), "parent"}},
    {"RandomEvents", {tr("Random Events:"), "parent"}},
    {"StandstillTime", {tr("Time Stopped"), "timePercent"}},
    {"StopLightTime", {tr("Time Spent at Stoplights"), "timePercent"}},
    {"TrackedTime", {tr("Total Time Tracked"), "time"}},
    {"WeatherTimes", {tr("Time Driven (Weather):"), "parent"}}
  };

  static QMap<QString, QString> randomEventsMap = {
    {"accel30", tr("UwUs")},
    {"accel35", tr("Loch Ness Encounters")},
    {"accel40", tr("Visits to 1955")},
    {"dejaVuCurve", tr("Deja Vu Moments")},
    {"firefoxSteerSaturated", tr("Internet Explorer Weeeeeeees")},
    {"hal9000", tr("HAL 9000 Denials")},
    {"openpilotCrashedRandomEvent", tr("openpilot Crashes")},
    {"thisIsFineSteerSaturated", tr("This Is Fine Moments")},
    {"toBeContinued", tr("To Be Continued Moments")},
    {"vCruise69", tr("Noices")},
    {"yourFrogTriedToKillMe", tr("Attempted Frog Murders")},
    {"youveGotMail", tr("Total Mail Received")}
  };

  static QSet<QString> ignoredKeys = {
    "Month"
  };

  QStringList keys = keyMap.keys();
  std::sort(keys.begin(), keys.end(), [&](const QString &a, const QString &b) {
    return keyMap.value(a).first.toLower() < keyMap.value(b).first.toLower();
  });

  std::function<QString(double)> format_number = [&](double number) {
    return QLocale().toString(number);
  };

  std::function<QString(double)> format_distance = [&](double meters) {
    double value;
    QString unit;
    if (isMetric) {
      value = meters / 1000.0;
      unit = (value == 1.0) ? tr(" kilometer") : tr(" kilometers");
    } else {
      value = meters * METER_TO_MILE;
      unit = (value == 1.0) ? tr(" mile") : tr(" miles");
    }
    return format_number(qRound(value)) + unit;
  };

  std::function<QString(int)> format_time = [&](int seconds) {
    static int secondsInDay = 60 * 60 * 24;
    static int secondsInHour = 60 * 60;

    int days = seconds / secondsInDay;
    int hours = (seconds % secondsInDay) / secondsInHour;
    int minutes = (seconds % secondsInHour) / 60;

    QString result;
    if (days > 0) {
      result += format_number(days) + (days == 1 ? tr(" day ") : tr(" days "));
    }
    if (hours > 0 || days > 0) {
      result += format_number(hours) + (hours == 1 ? tr(" hour ") : tr(" hours "));
    }
    result += format_number(minutes) + (minutes == 1 ? tr(" minute") : tr(" minutes"));
    return result.trimmed();
  };

  double trackedTime = stats.contains("TrackedTime") ? stats.value("TrackedTime").toDouble() : 0.0;

  for (const QString &key : keys) {
    if (ignoredKeys.contains(key)) {
      continue;
    }

    QJsonValue value = stats.contains(key) ? stats.value(key) : QJsonValue(0);
    QString labelText = keyMap.value(key).first;
    QString type = keyMap.value(key).second;

    if (key == "AEBEvents") {
      QJsonObject totalEvents = stats.value("TotalEvents").toObject();

      QString trimmedLabel = labelText;
      QString prefix = tr("Total ");
      if (trimmedLabel.startsWith(prefix)) {
        trimmedLabel = trimmedLabel.mid(prefix.length());
      }
      QString displayValue = format_number(totalEvents.value("stockAeb").toInt(0) + totalEvents.value("fcw").toInt(0)) + " " + trimmedLabel;

      labelsList->addItem(new LabelControl(labelText, displayValue, "", this));

    } else if (key == "CruiseSpeedTimes" && value.isObject()) {
      QJsonObject speeds = value.toObject();

      double maxTime = -1.0;
      QString bestSpeed;
      for (const QString &speedKey : speeds.keys()) {
        double time = speeds.value(speedKey).toDouble();
        if (time > maxTime) {
          bestSpeed = speedKey;
          maxTime = time;
        }
      }

      QString displaySpeed;
      if (isMetric) {
        displaySpeed = QString::number(qRound(bestSpeed.toDouble() * MS_TO_KPH)) + " " + tr("km/h");
      } else {
        displaySpeed = QString::number(qRound(bestSpeed.toDouble() * MS_TO_MPH)) + " " + tr("mph");
      }

      labelsList->addItem(new LabelControl(labelText, displaySpeed + " (" + format_time(maxTime) + ")", "", this));
    } else if (type == "parent" && value.isObject()) {
      labelsList->addItem(new LabelControl(labelText, "", "", this));

      QJsonObject subObject = value.toObject();
      QStringList subKeys;

      if (key == "RandomEvents") {
        subKeys = randomEventsMap.keys();
      } else {
        subKeys = subObject.keys();
      }

      std::sort(subKeys.begin(), subKeys.end(), [&](const QString &a, const QString &b) {
        QString displayA, displayB;
        if (key == "RandomEvents") {
          displayA = randomEventsMap.value(a, a);
          displayB = randomEventsMap.value(b, b);
        } else {
          displayA = a;
          displayB = b;
        }
        return displayA.toLower() < displayB.toLower();
      });

      for (const QString &subkey : subKeys) {
        if (subkey == "Unknown") {
          continue;
        }

        QString displaySubKey;
        if (key == "ModelTimes") {
          displaySubKey = processModelName(subkey);
        } else if (key == "RandomEvents") {
          displaySubKey = randomEventsMap.value(subkey, subkey);
        } else if (key == "WeatherTimes") {
          displaySubKey = subkey.left(1).toUpper() + subkey.mid(1);
        } else {
          displaySubKey = subkey;
        }

        QString subvalue;
        if (key.endsWith("Times")) {
          subvalue = format_time(subObject.value(subkey).toDouble());
        } else {
          subvalue = format_number(subObject.value(subkey).toInt(0));
        }

        labelsList->addItem(new LabelControl("     " + displaySubKey, subvalue, "", this));
      }
    } else {
      QString displayValue;
      if (type == "accel") {
        displayValue = QString::number(value.toDouble(), 'f', 2) + " " + tr("m/s²");
      } else if (type == "count") {
        QString trimmedLabel = labelText;
        QString prefix = tr("Total ");
        if (trimmedLabel.startsWith(prefix)) {
          trimmedLabel = trimmedLabel.mid(prefix.length());
        }
        displayValue = format_number(value.toInt()) + " " + trimmedLabel;
      } else if (type == "distance") {
        displayValue = format_distance(value.toDouble());
      } else if (type == "time" || type == "timePercent") {
        displayValue = format_time(value.toDouble());
      } else {
        QString stringValue = value.toVariant().toString();
        displayValue = stringValue.isEmpty() ? "0" : stringValue;
      }

      labelsList->addItem(new LabelControl(labelText, displayValue, "", this));

      if (type == "timePercent") {
        int percent = 0;
        if (trackedTime > 0.0) {
          percent = (value.toDouble() * 100.0) / trackedTime;
        }

        labelsList->addItem(new LabelControl(tr("% of ") + labelText, format_number(percent) + "%", "", this));
      }
    }
  }
}
