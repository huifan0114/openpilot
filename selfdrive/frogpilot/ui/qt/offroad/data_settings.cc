#include <filesystem>

#include "selfdrive/frogpilot/ui/qt/offroad/data_settings.h"

FrogPilotDataPanel::FrogPilotDataPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  ButtonControl *deleteDrivingDataBtn = new ButtonControl(tr("刪除駕駛錄影和數據"), tr("刪除"), tr("此按鈕提供了一種快速、安全的方式來永久刪除裝置中所有儲存的駕駛錄影和資料。非常適合維護隱私或釋放空間."));
  QObject::connect(deleteDrivingDataBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("您確定要永久刪除所有駕駛錄影和資料嗎?"), tr("刪除"), this)) {
      std::thread([=] {
        deleteDrivingDataBtn->setEnabled(false);
        deleteDrivingDataBtn->setValue(tr("正在刪除..."));

        std::system("rm -rf /data/media/0/realdata");

        deleteDrivingDataBtn->setValue(tr("已刪除!"));

        util::sleep_for(2000);
        deleteDrivingDataBtn->setValue("");
        deleteDrivingDataBtn->setEnabled(true);
      }).detach();
    }
  });
  addItem(deleteDrivingDataBtn);

  FrogPilotButtonsControl *screenRecordingsBtn = new FrogPilotButtonsControl(tr("螢幕錄製"), tr("管理您的螢幕錄製."), {tr("刪除"), tr("重新命名")});
  QObject::connect(screenRecordingsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir recordingsDir("/data/media/0/videos");
    QStringList recordingsNames = recordingsDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    if (id == 0) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇要刪除的錄影"), recordingsNames, "", this);
      if (!selection.isEmpty()) {
        if (ConfirmationDialog::confirm(tr("您確定要刪除此錄影嗎?"), tr("刪除"), this)) {
          std::thread([=]() {
            screenRecordingsBtn->setEnabled(false);
            screenRecordingsBtn->setValue(tr("正在刪除..."));

            QFile fileToDelete(recordingsDir.absoluteFilePath(selection));
            if (fileToDelete.remove()) {
              screenRecordingsBtn->setValue(tr("已刪除!"));
            } else {
              screenRecordingsBtn->setValue(tr("失敗的..."));
            }

            util::sleep_for(2000);
            screenRecordingsBtn->setValue("");
            screenRecordingsBtn->setEnabled(true);
          }).detach();
        }
      }

    } else if (id == 1) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇要重新命名的錄音"), recordingsNames, "", this);
      if (!selection.isEmpty()) {
        QString newName = InputDialog::getText(tr("輸入新名稱"), this, tr("重新命名錄音"));
        if (!newName.isEmpty()) {
          std::thread([=]() {
            screenRecordingsBtn->setEnabled(false);
            screenRecordingsBtn->setValue(tr("重新命名..."));

            QString oldPath = recordingsDir.absoluteFilePath(selection);
            QString newPath = recordingsDir.absoluteFilePath(newName);
            if (QFile::rename(oldPath, newPath)) {
              screenRecordingsBtn->setValue(tr("更名!"));
            } else {
              screenRecordingsBtn->setValue(tr("失敗的..."));
            }

            util::sleep_for(2000);
            screenRecordingsBtn->setValue("");
            screenRecordingsBtn->setEnabled(true);
          }).detach();
        }
      }
    }
  });
  addItem(screenRecordingsBtn);

  FrogPilotButtonsControl *frogpilotBackupBtn = new FrogPilotButtonsControl(tr("FrogPilot 備份"), tr("管理您的 FrogPilot 備份."), {tr("備份"), tr("刪除"), tr("恢復")});
  QObject::connect(frogpilotBackupBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir backupDir("/data/backups");
    QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name).filter(QRegularExpression("^(?!.*_in_progress$).*$"));

    if (id == 0) {
      QString nameSelection = InputDialog::getText(tr("為您的備份命名"), this, "", false, 1);
      if (!nameSelection.isEmpty()) {
        bool compressed = FrogPilotConfirmationDialog::yesorno(tr("您想壓縮此備份嗎？最終檔案大小將縮小 2.25 倍，但可能需要 10 分鐘以上."), this);
        std::thread([=]() {
          device()->resetInteractiveTimeout(300);

          frogpilotBackupBtn->setEnabled(false);
          frogpilotBackupBtn->setValue(tr("備份中..."));

          std::string fullBackupPath = backupDir.absolutePath().toStdString() + "/" + nameSelection.toStdString();
          std::string inProgressBackupPath = fullBackupPath + "_in_progress";
          int result = std::system(("mkdir -p " + inProgressBackupPath + " && rsync -av /data/openpilot/ " + inProgressBackupPath + "/").c_str());

          if (result == 0 && compressed) {
            frogpilotBackupBtn->setValue(tr("壓縮中..."));
            result = std::system(("tar -czf " + fullBackupPath + "_in_progress.tar.gz -C " + inProgressBackupPath + " . && rm -rf " + inProgressBackupPath).c_str());
            if (result == 0) {
              result = std::system(("mv " + fullBackupPath + "_in_progress.tar.gz " + fullBackupPath + ".tar.gz").c_str());
            }
          } else if (result == 0) {
            result = std::system(("mv " + inProgressBackupPath + " " + fullBackupPath).c_str());
          }

          if (result == 0) {
            frogpilotBackupBtn->setValue(tr("成功!"));
          } else {
            frogpilotBackupBtn->setValue(tr("失敗..."));
            std::system(("rm -rf " + inProgressBackupPath).c_str());
          }

          util::sleep_for(2000);
          frogpilotBackupBtn->setValue("");
          frogpilotBackupBtn->setEnabled(true);

          device()->resetInteractiveTimeout(30);
        }).detach();
      }

    } else if (id == 1) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇要刪除的備份"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (ConfirmationDialog::confirm(tr("您確定要刪除此備份嗎?"), tr("刪除"), this)) {
          std::thread([=]() {
            frogpilotBackupBtn->setEnabled(false);
            frogpilotBackupBtn->setValue(tr("正在刪除..."));

            QDir dirToDelete(backupDir.absoluteFilePath(selection));
            if (selection.endsWith(".tar.gz")) {
              if (QFile::remove(dirToDelete.absolutePath())) {
                frogpilotBackupBtn->setValue(tr("已刪除!"));
              } else {
                frogpilotBackupBtn->setValue(tr("失敗..."));
              }
            } else {
              if (dirToDelete.removeRecursively()) {
                frogpilotBackupBtn->setValue(tr("已刪除!"));
              } else {
                frogpilotBackupBtn->setValue(tr("失敗..."));
              }
            }

            util::sleep_for(2000);
            frogpilotBackupBtn->setValue("");
            frogpilotBackupBtn->setEnabled(true);
          }).detach();
        }
      }

    } else if (id == 2) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇一個還原點"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (ConfirmationDialog::confirm(tr("您確定要還原此版本的 FrogPilot?"), tr("恢復"), this)) {
          std::thread([=]() {
            device()->resetInteractiveTimeout(300);

            frogpilotBackupBtn->setEnabled(false);
            frogpilotBackupBtn->setValue(tr("正在恢復..."));

            std::string sourcePath = backupDir.absolutePath().toStdString() + "/" + selection.toStdString();
            std::string targetPath = "/data/safe_staging/finalized";
            std::string extractDirectory = "/data/restore_temp";

            if (selection.endsWith(".tar.gz")) {
              frogpilotBackupBtn->setValue(tr("提取..."));

              if (std::system(("mkdir -p " + extractDirectory).c_str()) != 0 || std::system(("tar --strip-components=1 -xzf " + sourcePath + " -C " + extractDirectory).c_str()) != 0) {
                frogpilotBackupBtn->setValue(tr("失敗..."));
                util::sleep_for(2000);
                frogpilotBackupBtn->setValue("");
                frogpilotBackupBtn->setEnabled(true);
                return;
              }

              sourcePath = extractDirectory;
              frogpilotBackupBtn->setValue(tr("正在恢復..."));
            }

            if (std::system(("rsync -av --delete -l --exclude='.overlay_consistent' " + sourcePath + "/ " + targetPath + "/").c_str()) == 0) {
              std::ofstream consistentFile(targetPath + "/.overlay_consistent");
              if (consistentFile) {
                frogpilotBackupBtn->setValue(tr("已恢復!"));
                params.putBool("AutomaticUpdates", false);
                util::sleep_for(2000);

                frogpilotBackupBtn->setValue(tr("重新啟動中..."));
                consistentFile.close();
                std::filesystem::remove_all(extractDirectory);

                util::sleep_for(2000);
                Hardware::reboot();
              } else {
                frogpilotBackupBtn->setValue(tr("失敗..."));
                util::sleep_for(2000);
                frogpilotBackupBtn->setValue("");
                frogpilotBackupBtn->setEnabled(true);

                device()->resetInteractiveTimeout(30);
              }
            } else {
              frogpilotBackupBtn->setValue(tr("失敗..."));
              util::sleep_for(2000);
              frogpilotBackupBtn->setValue("");
              frogpilotBackupBtn->setEnabled(true);

              device()->resetInteractiveTimeout(30);
            }
          }).detach();
        }
      }
    }
  });
  addItem(frogpilotBackupBtn);

  FrogPilotButtonsControl *toggleBackupBtn = new FrogPilotButtonsControl(tr("切換備份"), tr("管理您的切換備份."), {tr("備份"), tr("刪除"), tr("恢復")});
  QObject::connect(toggleBackupBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir backupDir("/data/toggle_backups");
    QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (id == 0) {
      QString nameSelection = InputDialog::getText(tr("為您的備份命名"), this, "", false, 1);
      if (!nameSelection.isEmpty()) {
        std::thread([=]() {
          toggleBackupBtn->setEnabled(false);
          toggleBackupBtn->setValue(tr("備份中..."));

          std::string command = "mkdir -p " + backupDir.absolutePath().toStdString() + "/" + nameSelection.toStdString() + " && rsync -av /data/params/d/ " + backupDir.absolutePath().toStdString() + "/" + nameSelection.toStdString();
          int result = std::system(command.c_str());

          if (result == 0) {
            toggleBackupBtn->setValue(tr("成功!"));
          } else {
            toggleBackupBtn->setValue(tr("失敗..."));
          }

          util::sleep_for(2000);
          toggleBackupBtn->setValue("");
          toggleBackupBtn->setEnabled(true);
        }).detach();
      }

    } else if (id == 1) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇要刪除的備份"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (ConfirmationDialog::confirm(tr("您確定要刪除此備份嗎?"), tr("刪除"), this)) {
          std::thread([=]() {
            toggleBackupBtn->setEnabled(false);
            toggleBackupBtn->setValue(tr("正在刪除..."));

            QDir dirToDelete(backupDir.absoluteFilePath(selection));
            if (dirToDelete.removeRecursively()) {
              toggleBackupBtn->setValue(tr("已刪除!"));
            } else {
              toggleBackupBtn->setValue(tr("失敗..."));
            }

            util::sleep_for(2000);
            toggleBackupBtn->setValue("");
            toggleBackupBtn->setEnabled(true);
          }).detach();
        }
      }

    } else if (id == 2) {
      QString selection = MultiOptionDialog::getSelection(tr("選擇一個還原點"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (ConfirmationDialog::confirm(tr("您確定要還原此切換備份嗎?"), tr("恢復"), this)) {
          std::thread([=]() {
            toggleBackupBtn->setEnabled(false);
            toggleBackupBtn->setValue(tr("正在恢復..."));

            std::string targetPath = "/data/params/d/";
            std::string tempBackupPath = "/data/params/d_backup/";

            int backupResult = std::system(("rsync -av --delete -l " + targetPath + " " + tempBackupPath).c_str());

            if (backupResult == 0) {
              toggleBackupBtn->setValue(tr("正在恢復..."));

              std::string restoreCommand = "rsync -av --delete -l " + backupDir.absolutePath().toStdString() + "/" + selection.toStdString() + "/ " + targetPath;
              int restoreResult = std::system(restoreCommand.c_str());

              if (restoreResult == 0) {
                toggleBackupBtn->setValue(tr("成功!"));
              } else {
                toggleBackupBtn->setValue(tr("失敗..."));

                std::system(("rsync -av --delete -l " + tempBackupPath + " " + targetPath).c_str());
              }

              std::system(("rm -rf " + tempBackupPath).c_str());
            } else {
              toggleBackupBtn->setValue(tr("失敗..."));
            }

            util::sleep_for(2000);
            toggleBackupBtn->setValue("");
            toggleBackupBtn->setEnabled(true);
          }).detach();
        }
      }
    }
  });
  addItem(toggleBackupBtn);
}
