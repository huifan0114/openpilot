#include "selfdrive/ui/qt/offroad/settings.h"

#include <cassert>
#include <cmath>
#include <string>

#include <QDebug>
#include <QLabel>

#include "common/params.h"
#include "common/util.h"
#include "selfdrive/ui/ui.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "system/hardware/hw.h"


void SoftwarePanel::checkForUpdates() {
  std::system("pkill -SIGUSR1 -f system.updated.updated");
}

SoftwarePanel::SoftwarePanel(QWidget* parent) : ListWidget(parent) {
  onroadLbl = new QLabel(tr("系統更新只會在熄火時下載."));
  onroadLbl->setStyleSheet("font-size: 50px; font-weight: 400; text-align: left; padding-top: 30px; padding-bottom: 30px;");
  addItem(onroadLbl);

  // current version
  versionLbl = new LabelControl(tr("目前版本"), "");
  addItem(versionLbl);

//////////////////////////////////////////////////////////////////////////////////////////////
  fastinstallBtn = new ButtonControl(tr("快速更新"), tr("更新"), "立刻進行更新並重啟機器.");
  connect(fastinstallBtn, &ButtonControl::clicked, [=]() {
    // params.putBool("Faststart", false);
    // params.putBool("FrogPilotTogglesUpdated", true);
    std::system("git pull");
    Hardware::reboot();
  });
  addItem(fastinstallBtn);
//////////////////////////////////////////////////////////////////////////////////////////////

  // automatic updates toggle
  ParamControl *automaticUpdatesToggle = new ParamControl("AutomaticUpdates", tr("自動更新"),
                                                       tr("待機熄火狀態若有連上網路會自動更新."), "");
  automaticUpdatesToggle->setVisible(params.getBool("IsReleaseBranch"));
  addItem(automaticUpdatesToggle);

  // download update btn
  downloadBtn = new ButtonControl(tr("下載"), tr("檢查"));
  connect(downloadBtn, &ButtonControl::clicked, [=]() {
    downloadBtn->setEnabled(false);
    if (downloadBtn->text() == tr("檢查")) {
      checkForUpdates();
    } else {
      std::system("pkill -SIGHUP -f system.updated.updated");
    }
    frogpilotUIState()->params_memory.putBool("ManualUpdateInitiated", true);
  });
  addItem(downloadBtn);

  // install update btn
  installBtn = new ButtonControl(tr("安裝更新"), tr("安裝"));
  connect(installBtn, &ButtonControl::clicked, [=]() {
    installBtn->setEnabled(false);
    params.putBool("DoReboot", true);
  });
  addItem(installBtn);

  // branch selecting
  targetBranchBtn = new ButtonControl(tr("目標分支"), tr("選擇"));
  connect(targetBranchBtn, &ButtonControl::clicked, [=]() {
    auto current = params.get("GitBranch");
    QStringList branches = QString::fromStdString(params.get("UpdaterAvailableBranches")).split(",");
    if (!frogpilotUIState()->frogpilot_scene.frogpilot_toggles.value("frogs_go_moo").toBool()) {
      for (int i = branches.size() - 1; i >= 0; --i) {
        if (branches[i].startsWith("FrogPilot-Development", Qt::CaseInsensitive)) {
          branches.removeAt(i);
        }
      }
      branches.removeAll("FrogPilot-Vetting");
      branches.removeAll("MAKE-PRS-HERE");
    }
    for (QString b : {current.c_str(), "devel-staging", "devel", "nightly", "master-ci", "master"}) {
      auto i = branches.indexOf(b);
      if (i >= 0) {
        branches.removeAt(i);
        branches.insert(0, b);
      }
    }

    QString cur = QString::fromStdString(params.get("UpdaterTargetBranch"));
    QString selection = MultiOptionDialog::getSelection(tr("選擇分支"), branches, cur, this);
    if (!selection.isEmpty()) {
      params.put("UpdaterTargetBranch", selection.toStdString());
      targetBranchBtn->setValue(QString::fromStdString(params.get("UpdaterTargetBranch")));
      checkForUpdates();

      if (selection.toStdString() != current) {
        if (FrogPilotConfirmationDialog::yesorno(tr("This branch must be downloaded before switching. Would you like to download it now?"), this)) {
          std::system("pkill -SIGHUP -f system.updated.updated");

          frogpilotUIState()->params_memory.putBool("ManualUpdateInitiated", true);
        }
      }
    }
  });
  addItem(targetBranchBtn);

  // uninstall button
  auto uninstallBtn = new ButtonControl(tr("解除安裝 %1").arg(getBrand()), tr("解除安裝"));
  connect(uninstallBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm(tr("是否確定要解除安裝?"), tr("解除安裝"), this)) {
      if (FrogPilotConfirmationDialog::yesorno(tr("您想刪除深層存儲FrogPilot資產嗎？這包括您的切換設置以快速重新安裝."), this)) {
        if (FrogPilotConfirmationDialog::yesorno(tr("你確定嗎？這是100％無法恢復的，如果您重新安裝FrogPilot，您將失去所有以前的設置!"), this)) {
          std::system("rm -rf /cache/params/d");
          std::system("rm -rf /persist/params");
          std::system("rm -rf /cache/params");
          std::system("rm -rf /persist/tracking");
          std::system("rm -rf /cache/tracking");
          std::system("rm -rf /data/backups");
          std::system("rm -rf /data/crashes");
          std::system("rm -rf /data/media/screen_recordings");
          std::system("rm -rf /data/themes");
          std::system("rm -rf /data/toggle_backups");
          std::system("rm -rf /data/models");
          std::system("rm -rf /data/media/0/osm/mapd");
          std::system("rm -rf /data/media/0/osm/offline");
          std::system("rm -rf /data/media/0/realdata");
          std::system("rm -rf /data/media/screen_recordings");
        }
      }
      params.putBool("DoUninstall", true);
    }
  });
  addItem(uninstallBtn);

  // error log button
  auto errorLogBtn = new ButtonControl(tr("錯誤資訊"), tr("查看"), "查看錯誤訊息.");
  connect(errorLogBtn, &ButtonControl::clicked, [=]() {
    std::string txt = util::read_file("/data/error_logs/error.txt");
    ConfirmationDialog::rich(QString::fromStdString(txt), this);
  });
  addItem(errorLogBtn);

//////////////////////////////////////////////////////////////////////////////////////////////
  delLogBtn = new ButtonControl(tr("刪除訊息"), tr("刪除"), "刪除訊息.");
  connect(delLogBtn, &ButtonControl::clicked, [=]() {
    std::system("rm -r /data/crashes && mkdir -p /data/crashes/");
    std::system("rm -r /data/error_logs && mkdir -p /data/error_logs/");
  });
  addItem(delLogBtn);
//////////////////////////////////////////////////////////////////////////////////////////////

  fs_watch = new ParamWatcher(this);
  QObject::connect(fs_watch, &ParamWatcher::paramChanged, [=](const QString &param_name, const QString &param_value) {
    updateLabels();
  });

  connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    is_onroad = !offroad;
    updateLabels();
  });

  updateLabels();
}

void SoftwarePanel::showEvent(QShowEvent *event) {
  // nice for testing on PC
  installBtn->setEnabled(true);

  updateLabels();

  // FrogPilot variables
  FrogPilotUIState &fs = *frogpilotUIState();
  FrogPilotUIScene &frogpilot_scene = fs.frogpilot_scene;

  if (frogpilot_scene.online && params.get("UpdaterState") == "idle") {
    checkForUpdates();
  }
}

void SoftwarePanel::updateLabels() {
  FrogPilotUIState &fs = *frogpilotUIState();
  FrogPilotUIScene &frogpilot_scene = fs.frogpilot_scene;

  // add these back in case the files got removed
  fs_watch->addParam("LastUpdateTime");
  fs_watch->addParam("UpdateFailedCount");
  fs_watch->addParam("UpdaterState");
  fs_watch->addParam("UpdateAvailable");

  if (!isVisible()) {
    frogpilot_scene.downloading_update = false;
    return;
  }

  // updater only runs offroad or when parked
  bool parked = frogpilot_scene.parked || frogpilot_scene.frogpilot_toggles.value("frogs_go_moo").toBool();

  onroadLbl->setVisible(is_onroad && !parked);
  downloadBtn->setVisible(!is_onroad || parked);

  // download update
  QString updater_state = QString::fromStdString(params.get("UpdaterState"));
  bool failed = std::atoi(params.get("UpdateFailedCount").c_str()) > 0;
  if (updater_state != "idle") {
    downloadBtn->setEnabled(false);
    QString stateText = updater_state;
    if (updater_state == "downloading...") {
      stateText = tr("downloading…");
    } else if (updater_state == "checking...") {
      stateText = tr("checking…");
    } else if (updater_state == "waiting for vehicle to go offroad...") {
      stateText = tr("waiting for vehicle to go offroad...");
    } else if (updater_state == "finalizing update...") {
      stateText = tr("finalizing update...");
    }

    downloadBtn->setValue(stateText);
    frogpilot_scene.downloading_update = true;
  } else {
    frogpilot_scene.downloading_update = false;
    if (failed) {
      downloadBtn->setText(tr("檢查"));
      downloadBtn->setValue(tr("檢查更新失敗"));
    } else if (params.getBool("UpdaterFetchAvailable")) {
      downloadBtn->setText(tr("下載"));
      downloadBtn->setValue(tr("有新版本"));
    } else {
      QString lastUpdate = tr("從未更新");
      auto tm = params.get("LastUpdateTime");
      if (!tm.empty()) {
        lastUpdate = timeAgo(QDateTime::fromString(QString::fromStdString(tm + "Z"), Qt::ISODate));
      }
      downloadBtn->setText(tr("檢查"));
      downloadBtn->setValue(tr("已經是最新版本，上次檢查時間為 %1").arg(lastUpdate));
    }
    downloadBtn->setEnabled(true);
  }
  targetBranchBtn->setValue(QString::fromStdString(params.get("UpdaterTargetBranch")));

  // current + new versions
  versionLbl->setText(QString::fromStdString(params.get("UpdaterCurrentDescription")));
  versionLbl->setDescription(QString::fromStdString(params.get("UpdaterCurrentReleaseNotes")));

  installBtn->setVisible((!is_onroad || parked) && params.getBool("UpdateAvailable"));
  installBtn->setValue(QString::fromStdString(params.get("UpdaterNewDescription")));
  installBtn->setDescription(QString::fromStdString(params.get("UpdaterNewReleaseNotes")));

  update();
}
