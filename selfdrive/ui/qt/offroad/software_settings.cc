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
  connect(automaticUpdatesToggle, &ToggleControl::toggleFlipped, this, updateFrogPilotToggles);
  addItem(automaticUpdatesToggle);

  // download update btn
  downloadBtn = new ButtonControl(tr("下載"), tr("檢查"));
  connect(downloadBtn, &ButtonControl::clicked, [=]() {
    device()->resetInteractiveTimeout(300);

    downloadBtn->setEnabled(false);
    if (downloadBtn->text() == tr("檢查")) {
      checkForUpdates();
    } else {
      std::system("pkill -SIGHUP -f system.updated.updated");
    }
    paramsMemory.putBool("ManualUpdateInitiated", true);
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
    if (getDongleId().value_or("") != "FrogsGoMoo") {
      branches.removeAll("FrogPilot-Development");
      branches.removeAll("FrogPilot-New");
      branches.removeAll("FrogPilot-Test");
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
    }
  });
  addItem(targetBranchBtn);

  // uninstall button
  auto uninstallBtn = new ButtonControl(tr("解除安裝 %1").arg(getBrand()), tr("解除安裝"));
  connect(uninstallBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm(tr("是否確定要解除安裝?"), tr("解除安裝"), this)) {
      if (FrogPilotConfirmationDialog::yesorno(tr("Do you want to permanently delete any additional FrogPilot assets? This is 100% unrecoverable and includes backups, downloaded models, themes, and long-term storage toggle settings for easy reinstalls."), this)) {
        std::system("rm -rf /data/backups");
        std::system("rm -rf /data/crashes");
        std::system("rm -rf /data/media/0/videos");
        std::system("rm -rf /data/themes");
        std::system("rm -rf /data/toggle_backups");
        std::system("rm -rf /persist/params");
        std::system("rm -rf /persist/tracking");
      }
      params.putBool("DoUninstall", true);
    }
  });
  addItem(uninstallBtn);

  // error log button
  auto errorLogBtn = new ButtonControl(tr("錯誤資訊"), tr("查看"), "查看錯誤訊息.");
  connect(errorLogBtn, &ButtonControl::clicked, [=]() {
    const std::string txt = util::read_file("/data/crashes/error.txt");
    ConfirmationDialog::rich(QString::fromStdString(txt), this);
  });
  addItem(errorLogBtn);

//////////////////////////////////////////////////////////////////////////////////////////////
  delLogBtn = new ButtonControl(tr("刪除訊息"), tr("刪除"), "刪除訊息.");
  connect(delLogBtn, &ButtonControl::clicked, [=]() {
    std::system("rm -r /data/crashes && mkdir -p /data/crashes/");
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
}

void SoftwarePanel::updateLabels() {
  UIState *s = uiState();
  UIScene &scene = s->scene;

  // add these back in case the files got removed
  fs_watch->addParam("LastUpdateTime");
  fs_watch->addParam("UpdateFailedCount");
  fs_watch->addParam("UpdaterState");
  fs_watch->addParam("UpdateAvailable");

  if (!isVisible()) {
    return;
  }

  // updater only runs offroad or when parked
  bool parked = scene.parked;

  onroadLbl->setVisible(is_onroad && !parked);
  downloadBtn->setVisible(!is_onroad || parked);

  // download update
  QString updater_state = QString::fromStdString(params.get("UpdaterState"));
  bool failed = std::atoi(params.get("UpdateFailedCount").c_str()) > 0;
  if (updater_state != "idle") {
    downloadBtn->setEnabled(false);
    downloadBtn->setValue(updater_state);
  } else {
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

  bool install_ready = (!is_onroad || parked) && params.getBool("UpdateAvailable");
  if (!installBtn->isVisible() && install_ready) {
    device()->resetInteractiveTimeout(30);
  }

  installBtn->setVisible(install_ready);
  installBtn->setValue(QString::fromStdString(params.get("UpdaterNewDescription")));
  installBtn->setDescription(QString::fromStdString(params.get("UpdaterNewReleaseNotes")));

  update();
}
