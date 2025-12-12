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
}
