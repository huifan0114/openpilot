#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include <QDebug>
#include <QScrollBar>

#include "common/watchdog.h"
#include "common/util.h"
#include "selfdrive/ui/qt/network/networking.h"
#include "selfdrive/ui/qt/offroad/settings.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/widgets/prime.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/widgets/ssh_keys.h"

#include "selfdrive/frogpilot/navigation/ui/navigation_settings.h"
#include "selfdrive/frogpilot/ui/qt/offroad/control_settings.h"
#include "selfdrive/frogpilot/ui/qt/offroad/vehicle_settings.h"
#include "selfdrive/frogpilot/ui/qt/offroad/visual_settings.h"
/////////////////////////////////////////////////////
#include "selfdrive/frogpilot/ui/qt/offroad/hfop_settings.h"
/////////////////////////////////////////////////////

TogglesPanel::TogglesPanel(SettingsWindow *parent) : ListWidget(parent) {
  // param, title, desc, icon
  std::vector<std::tuple<QString, QString, QString, QString>> toggle_defs{
    {
      "OpenpilotEnabledToggle",
      tr("啟用 openpilot"),
      tr("使用 openpilot 的主動式巡航和車道保持功能，開啟後您需要持續集中注意力，設定變更在重新啟動車輛後生效。"),
      "../assets/offroad/icon_openpilot.png",
    },
    {
      "ExperimentalLongitudinalEnabled",
      tr("openpilot 縱向控制 (Alpha 版)"),
      QString("<b>%1</b><br><br>%2")
      .arg(tr("警告：此車輛的 Openpilot 縱向控制功能目前處於 Alpha 版本，使用此功能將會停用自動緊急制動（AEB）功能。"))
      .arg(tr("在這輛車上，Openpilot 預設使用車輛內建的主動巡航控制（ACC），而非 Openpilot 的縱向控制。啟用此項功能可切換至 Openpilot 的縱向控制。當啟用 Openpilot 縱向控制 Alpha 版本時，建議同時啟用實驗性模式（Experimental mode）。")),
      "../assets/offroad/icon_speed_limit.png",
    },
    {
      "ExperimentalMode",
      tr("實驗模式"),
      "",
      "../assets/img_experimental_white.svg",
    },
    {
      "DisengageOnAccelerator",
      tr("油門取消控車"),
      tr("啟用後，踩踏油門將會取消 openpilot 控制。"),
      "../assets/offroad/icon_disengage_on_accelerator.svg",
    },
    {
      "IsLdwEnabled",
      tr("啟用車道偏離警告"),
      tr("車速在時速 50 公里 (31 英里) 以上且未打方向燈的情況下，如果偵測到車輛駛出目前車道線時，發出車道偏離警告。"),
      "../assets/offroad/icon_warning.png",
    },
    {
      "RecordFront",
      tr("記錄並上傳駕駛監控影像"),
      tr("上傳駕駛監控的錄像來協助我們提升駕駛監控的準確率。"),
      "../assets/offroad/icon_monitoring.png",
    },
    {
      "IsMetric",
      tr("使用公制單位"),
      tr("啟用後，速度單位顯示將從 mp/h 改為 km/h。"),
      "../assets/offroad/icon_metric.png",
    },
#ifdef ENABLE_MAPS
    {
      "NavSettingTime24h",
      tr("預計到達時間單位改用 24 小時制"),
      tr("使用 24 小時制。(預設值為 12 小時制)"),
      "../assets/offroad/icon_metric.png",
    },
    {
      "NavSettingLeftSide",
      tr("將地圖顯示在畫面的左側"),
      tr("進入分割畫面後，地圖將會顯示在畫面的左側。"),
      "../assets/offroad/icon_road.png",
    },
#endif
  };


  std::vector<QString> longi_button_texts{tr("積極"), tr("標準"), tr("舒適")};
  long_personality_setting = new ButtonParamControl("LongitudinalPersonality", tr("駕駛模式"),
                                          tr("推薦使用標準模式。在積極模式中，openpilot 會更靠近前車並在加速和剎車方面更積極。在舒適模式中，openpilot 會與前車保持較遠的距離。"),
                                          "../assets/offroad/icon_speed_limit.png",
                                          longi_button_texts);

  // set up uiState update for personality setting
  QObject::connect(uiState(), &UIState::uiUpdate, this, &TogglesPanel::updateState);

  for (auto &[param, title, desc, icon] : toggle_defs) {
    auto toggle = new ParamControl(param, title, desc, icon, this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;

    // insert longitudinal personality after NDOG toggle
    if (param == "DisengageOnAccelerator") {
      addItem(long_personality_setting);
    }
  }

  // Toggles with confirmation dialogs
  toggles["ExperimentalMode"]->setActiveIcon("../assets/img_experimental.svg");
  toggles["ExperimentalMode"]->setConfirmation(true, true);
  toggles["ExperimentalLongitudinalEnabled"]->setConfirmation(true, false);

  connect(toggles["ExperimentalLongitudinalEnabled"], &ToggleControl::toggleFlipped, [=]() {
    updateToggles();
  });

  // FrogPilot signals
  connect(toggles["IsMetric"], &ToggleControl::toggleFlipped, [=]() {
    updateMetric();
  });
}

void TogglesPanel::updateState(const UIState &s) {
  const SubMaster &sm = *(s.sm);

  if (sm.updated("controlsState")) {
    auto personality = sm["controlsState"].getControlsState().getPersonality();
    if (personality != s.scene.personality && s.scene.started && isVisible()) {
      long_personality_setting->setCheckedButton(static_cast<int>(personality));
    }
    uiState()->scene.personality = personality;
  }
}

void TogglesPanel::expandToggleDescription(const QString &param) {
  toggles[param.toStdString()]->showDescription();
}

void TogglesPanel::showEvent(QShowEvent *event) {
  updateToggles();
}

void TogglesPanel::updateToggles() {
  auto disengage_on_accelerator_toggle = toggles["DisengageOnAccelerator"];
  disengage_on_accelerator_toggle->setVisible(!params.getBool("AlwaysOnLateral"));
  auto driver_camera_toggle = toggles["RecordFront"];
  driver_camera_toggle->setVisible(!(params.getBool("DeviceManagement") && params.getBool("NoLogging") && params.getBool("NoUploads")));
  auto nav_settings_left_toggle = toggles["NavSettingLeftSide"];
  nav_settings_left_toggle->setVisible(!params.getBool("FullMap"));

  auto experimental_mode_toggle = toggles["ExperimentalMode"];
  auto op_long_toggle = toggles["ExperimentalLongitudinalEnabled"];
  const QString e2e_description = QString("%1<br>"
                                          "<h4>%2</h4><br>"
                                          "%3<br>"
                                          "<h4>%4</h4><br>"
                                          "%5<br>")
                                  .arg(tr("openpilot 預設以 <b>輕鬆模式</b>駕駛. 實驗模式啟用了尚未準備好進入輕鬆模式的 <b>alpha 級功能</b> 。實驗功能如下：:"))
                                  .arg(tr("點對點的縱向控制"))
                                  .arg(tr("讓駕駛模型來控制油門及煞車。openpilot將會模擬人類的駕駛行為，包含在看見紅燈及停止標示時停車。"
                                          "由於車速將由駕駛模型決定，因此您設定的時速將成為速度上限。本功能仍在早期實驗階段，請預期模型有犯錯的可能性。"))
                                  .arg(tr("新的駕駛視覺介面"))
                                  .arg(tr("行駛畫面將在低速時切換至道路朝向的廣角鏡頭，以更好地顯示一些轉彎。實驗模式圖標也將顯示在右上角。當設定了導航目的地並且行駛模型正在將其作為輸入時，地圖上的行駛路徑將變為綠色。"));

  auto cp_bytes = params.get("CarParamsPersistent");
  if (!cp_bytes.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(cp_bytes.data(), cp_bytes.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();

    if (!CP.getExperimentalLongitudinalAvailable()) {
      params.remove("ExperimentalLongitudinalEnabled");
    }
    op_long_toggle->setVisible(CP.getExperimentalLongitudinalAvailable());
    if (hasLongitudinalControl(CP)) {
      // normal description and toggle
      bool conditional_experimental = params.getBool("ConditionalExperimental");
      if (conditional_experimental) {
        params.putBool("ExperimentalMode", true);
        params.putBool("ExperimentalModeConfirmed", true);
        experimental_mode_toggle->refresh();
      }
      experimental_mode_toggle->setEnabled(!conditional_experimental);
      experimental_mode_toggle->setDescription(e2e_description);
      long_personality_setting->setEnabled(true);
    } else {
      // no long for now
      experimental_mode_toggle->setEnabled(false);
      long_personality_setting->setEnabled(false);
      params.remove("ExperimentalMode");

      const QString unavailable = tr("因車輛使用內建ACC系統，無法在本車輛上啟動實驗模式。");

      QString long_desc = unavailable + " " + \
                          tr("openpilot 縱向控制可能會在未來的更新中提供。");
      if (CP.getExperimentalLongitudinalAvailable()) {
        long_desc = tr("啟用 openpilot 縱向控制 (alpha) 切換以允許實驗模式。");
      }
      experimental_mode_toggle->setDescription("<b>" + long_desc + "</b><br><br>" + e2e_description);
    }

    experimental_mode_toggle->refresh();
  } else {
    experimental_mode_toggle->setDescription(e2e_description);
    op_long_toggle->setVisible(false);
  }
}

DevicePanel::DevicePanel(SettingsWindow *parent) : ListWidget(parent) {
  // power buttons
  QHBoxLayout *power_layout = new QHBoxLayout();
  power_layout->setSpacing(30);

  QPushButton *reboot_btn = new QPushButton(tr("重啟"));
  reboot_btn->setObjectName("reboot_btn");
  power_layout->addWidget(reboot_btn);
  QObject::connect(reboot_btn, &QPushButton::clicked, this, &DevicePanel::reboot);

  QPushButton *poweroff_btn = new QPushButton(tr("關機"));
  poweroff_btn->setObjectName("poweroff_btn");
  power_layout->addWidget(poweroff_btn);
  QObject::connect(poweroff_btn, &QPushButton::clicked, this, &DevicePanel::poweroff);

  if (!Hardware::PC()) {
    connect(uiState(), &UIState::offroadTransition, poweroff_btn, &QPushButton::setVisible);
  }

  setStyleSheet(R"(
    #reboot_btn { height: 120px; border-radius: 15px; background-color: #add8e6; }
    #reboot_btn:pressed { background-color: #0083b5; }
    #poweroff_btn { height: 120px; border-radius: 15px; background-color: #ee8080; }
    #poweroff_btn:pressed { background-color: #FF2424; }
  )");
  addItem(power_layout);

//////////////////////////////////////////////////////////////////////////////////////////////
  fastinstallBtn = new ButtonControl(tr("快速更新"), tr("更新"), "立刻進行更新並重啟機器.");
  connect(fastinstallBtn, &ButtonControl::clicked, [=]() {
    // params.putBool("Faststart", false);
    // paramsMemory.putBool("FrogPilotTogglesUpdated", true);
    std::system("git pull");
    Hardware::reboot();
  });
  addItem(fastinstallBtn);
//////////////////////////////////////////////////////////////////////////////////////////////

  setSpacing(50);
  addItem(new LabelControl(tr("Dongle ID"), getDongleId().value_or(tr("無法使用"))));
  addItem(new LabelControl(tr("序號"), params.get("HardwareSerial").c_str()));

  pair_device = new ButtonControl(tr("Pair Device"), tr("PAIR"),
                                  tr("Pair your device with comma connect (connect.comma.ai) and claim your comma prime offer."));
  connect(pair_device, &ButtonControl::clicked, [=]() {
    PairingPopup popup(this);
    popup.exec();
  });
  addItem(pair_device);

  // offroad-only buttons

  auto dcamBtn = new ButtonControl(tr("駕駛監控鏡頭"), tr("預覽"),
                                   tr("預覽駕駛員監控鏡頭畫面，以確保其具有良好視野。（僅在熄火時可用）"));
  connect(dcamBtn, &ButtonControl::clicked, [=]() { emit showDriverView(); });
  addItem(dcamBtn);

  resetCalibBtn = new ButtonControl(tr("重設校正"), tr("重設"), "");
  connect(resetCalibBtn, &ButtonControl::showDescriptionEvent, this, &DevicePanel::updateCalibDescription);
  connect(resetCalibBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm(tr("是否確定要重設校正?"), tr("重設"), this)) {
      params.remove("CalibrationParams");
      params.remove("LiveTorqueParameters");
    }
  });
  addItem(resetCalibBtn);

  auto retrainingBtn = new ButtonControl(tr("觀看使用教學"), tr("觀看"), tr("觀看 openpilot 的使用規則、功能和限制"));
  connect(retrainingBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to review the training guide?"), tr("Review"), this)) {
      emit reviewTrainingGuide();
    }
  });
  addItem(retrainingBtn);

  if (Hardware::TICI()) {
    auto regulatoryBtn = new ButtonControl(tr("法規/監管"), tr("觀看"), "");
    connect(regulatoryBtn, &ButtonControl::clicked, [=]() {
      const std::string txt = util::read_file("../assets/offroad/fcc.html");
      ConfirmationDialog::rich(QString::fromStdString(txt), this);
    });
    addItem(regulatoryBtn);
  }

  auto translateBtn = new ButtonControl(tr("更改語言"), tr("更改"), "");
  connect(translateBtn, &ButtonControl::clicked, [=]() {
    QMap<QString, QString> langs = getSupportedLanguages();
    QString selection = MultiOptionDialog::getSelection(tr("選擇語系"), langs.keys(), langs.key(uiState()->language), this);
    if (!selection.isEmpty()) {
      // put language setting, exit Qt UI, and trigger fast restart
      params.put("LanguageSetting", langs[selection].toStdString());
      qApp->exit(18);
      watchdog_kick(0);
    }
  });
  addItem(translateBtn);

  QObject::connect(uiState(), &UIState::primeTypeChanged, [this] (PrimeType type) {
    pair_device->setVisible(type == PrimeType::UNPAIRED);
  });
  QObject::connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    for (auto btn : findChildren<ButtonControl *>()) {
      if (btn != pair_device) {
        btn->setEnabled(offroad);
      }
    }
    for (FrogPilotButtonsControl *btn : findChildren<FrogPilotButtonsControl *>()) {
      if (btn != forceStartedBtn) {
        btn->setEnabled(offroad);
      }
    }
  });

  // Delete driving footage
  ButtonControl *deleteDrivingDataBtn = new ButtonControl(tr("Delete Driving Data"), tr("DELETE"), tr("This button provides a swift and secure way to permanently delete all "
    "stored driving footage and data from your device. Ideal for maintaining privacy or freeing up space.")
  );
  connect(deleteDrivingDataBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to permanently delete all of your driving footage and data?"), tr("Delete"), this)) {
      std::thread([&] {
        deleteDrivingDataBtn->setEnabled(false);
        deleteDrivingDataBtn->setValue(tr("Deleting footage..."));

        std::system("rm -rf /data/media/0/realdata");

        deleteDrivingDataBtn->setValue(tr("Deleted!"));

        std::this_thread::sleep_for(std::chrono::seconds(2));
        deleteDrivingDataBtn->setValue("");
        deleteDrivingDataBtn->setEnabled(true);
      }).detach();
    }
  });
  addItem(deleteDrivingDataBtn);

  // Screen recordings
  std::vector<QString> recordingsOptions{tr("DELETE"), tr("RENAME")};
  FrogPilotButtonsControl *screenRecordingsBtn = new FrogPilotButtonsControl(tr("Screen Recordings"), tr("Delete or rename your screen recordings."), "", recordingsOptions);
  connect(screenRecordingsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir recordingsDir("/data/media/0/videos");
    QStringList recordingsNames = recordingsDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    if (id == 0) {
      QString selection = MultiOptionDialog::getSelection(tr("Select a recording to delete"), recordingsNames, "", this);
      if (!selection.isEmpty()) {
        if (!ConfirmationDialog::confirm(tr("Are you sure you want to delete this recording?"), tr("Delete"), this)) return;
        std::thread([=]() {
          screenRecordingsBtn->setEnabled(false);
          screenRecordingsBtn->setValue(tr("Deleting..."));

          QFile fileToDelete(recordingsDir.absoluteFilePath(selection));
          if (fileToDelete.remove()) {
            screenRecordingsBtn->setValue(tr("Deleted!"));
          } else {
            screenRecordingsBtn->setValue(tr("Failed..."));
          }

          std::this_thread::sleep_for(std::chrono::seconds(2));
          screenRecordingsBtn->setValue("");
          screenRecordingsBtn->setEnabled(true);
        }).detach();
      }

    } else if (id == 1) {
      QString selection = MultiOptionDialog::getSelection(tr("Select a recording to rename"), recordingsNames, "", this);
      if (!selection.isEmpty()) {
        QString newName = InputDialog::getText(tr("Enter a new name"), this, tr("Rename Recording"));
        if (!newName.isEmpty()) {
          std::thread([=]() {
            screenRecordingsBtn->setEnabled(false);
            screenRecordingsBtn->setValue(tr("Renaming..."));

            QString oldPath = recordingsDir.absoluteFilePath(selection);
            QString newPath = recordingsDir.absoluteFilePath(newName);

            if (QFile::rename(oldPath, newPath)) {
              screenRecordingsBtn->setValue(tr("Renamed!"));
            } else {
              screenRecordingsBtn->setValue(tr("Failed..."));
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
            screenRecordingsBtn->setValue("");
            screenRecordingsBtn->setEnabled(true);
          }).detach();
        }
      }
    }
  });
  addItem(screenRecordingsBtn);

  // Backup FrogPilot
  std::vector<QString> frogpilotBackupOptions{tr("備份"), tr("刪除"), tr("還原")};
  FrogPilotButtonsControl *frogpilotBackupBtn = new FrogPilotButtonsControl(tr("FrogPilot 備份"), tr("備份、刪除或還原您的 FrogPilot 備份."), "", frogpilotBackupOptions);
  connect(frogpilotBackupBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir backupDir("/data/backups");

    if (id == 0) {
      QString nameSelection = InputDialog::getText(tr("為您的備份命名"), this, "", false, 1);
      if (!nameSelection.isEmpty()) {
        std::thread([=]() {
          frogpilotBackupBtn->setEnabled(false);
          frogpilotBackupBtn->setValue(tr("備份中..."));

          std::string fullBackupPath = backupDir.absolutePath().toStdString() + "/" + nameSelection.toStdString();
          std::string command = "mkdir -p " + fullBackupPath + " && rsync -av /data/openpilot/ " + fullBackupPath + "/";

          int result = std::system(command.c_str());
          frogpilotBackupBtn->setValue(result == 0 ? tr("成功!") : tr("失敗..."));

          std::this_thread::sleep_for(std::chrono::seconds(2));
          frogpilotBackupBtn->setValue("");
          frogpilotBackupBtn->setEnabled(true);
        }).detach();
      }

    } else if (id == 1) {
      QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

      QString selection = MultiOptionDialog::getSelection(tr("選擇要刪除的備份"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (!ConfirmationDialog::confirm(tr("您確定要刪除此備份嗎?"), tr("刪除"), this)) return;
        std::thread([=]() {
          frogpilotBackupBtn->setEnabled(false);
          frogpilotBackupBtn->setValue(tr("刪除中..."));

          QDir dirToDelete(backupDir.absoluteFilePath(selection));

          frogpilotBackupBtn->setValue(dirToDelete.removeRecursively() ? tr("已刪除!") : tr("刪除失敗..."));

          std::this_thread::sleep_for(std::chrono::seconds(2));
          frogpilotBackupBtn->setValue("");
          frogpilotBackupBtn->setEnabled(true);
        }).detach();
      }

    } else if (id == 2) {
      QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

      QString selection = MultiOptionDialog::getSelection(tr("選擇一個還原點"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (!ConfirmationDialog::confirm(tr("您確定要還原此版本的 FrogPilot?"), tr("還原"), this)) return;
        std::thread([=]() {
          frogpilotBackupBtn->setEnabled(false);
          frogpilotBackupBtn->setValue(tr("還原中..."));

          std::string sourcePath = backupDir.absolutePath().toStdString() + "/" + selection.toStdString();
          std::string targetPath = "/data/safe_staging/finalized";
          std::string consistentFilePath = targetPath + "/.overlay_consistent";
          std::string command = "rsync -av --delete -l --exclude='.overlay_consistent' " + sourcePath + "/ " + targetPath + "/";
          int result = std::system(command.c_str());

          if (result == 0) {
            std::ofstream consistentFile(consistentFilePath);
            if (consistentFile) {
              frogpilotBackupBtn->setValue(tr("Restored!"));
              std::this_thread::sleep_for(std::chrono::seconds(2));
              frogpilotBackupBtn->setValue(tr("Rebooting..."));
              std::this_thread::sleep_for(std::chrono::seconds(2));
              consistentFile.close();
              Hardware::reboot();
            } else {
              frogpilotBackupBtn->setValue(tr("失敗..."));
            }
          } else {
            frogpilotBackupBtn->setValue(tr("失敗..."));
          }
          std::this_thread::sleep_for(std::chrono::seconds(2));
          frogpilotBackupBtn->setValue("");
          frogpilotBackupBtn->setEnabled(true);
        }).detach();
      }
    }
  });
  addItem(frogpilotBackupBtn);

  // Backup toggles
  std::vector<QString> toggleBackupOptions{tr("備份"), tr("刪除"), tr("還原")};
  FrogPilotButtonsControl *toggleBackupBtn = new FrogPilotButtonsControl(tr("參數備份"), tr("備份、刪除或還原您的參數備份."), "", toggleBackupOptions);
  connect(toggleBackupBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    QDir backupDir("/data/toggle_backups");

    if (id == 0) {
      QString nameSelection = InputDialog::getText(tr("為您的備份命名"), this, "", false, 1);
      if (!nameSelection.isEmpty()) {
        std::thread([=]() {
          toggleBackupBtn->setEnabled(false);
          toggleBackupBtn->setValue(tr("備份中..."));

          std::string fullBackupPath = backupDir.absolutePath().toStdString() + "/" + nameSelection.toStdString() + "/";
          std::string command = "mkdir -p " + fullBackupPath + " && rsync -av /data/params/d/ " + fullBackupPath;

          int result = std::system(command.c_str());
          toggleBackupBtn->setValue(result == 0 ? tr("成功!") : tr("失敗..."));

          std::this_thread::sleep_for(std::chrono::seconds(2));
          toggleBackupBtn->setValue("");
          toggleBackupBtn->setEnabled(true);
        }).detach();
      }

    } else if (id == 1) {
      QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

      QString selection = MultiOptionDialog::getSelection(tr("選擇要刪除的備份"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (!ConfirmationDialog::confirm(tr("您確定要刪除此備份嗎?"), tr("刪除"), this)) return;
        std::thread([=]() {
          toggleBackupBtn->setEnabled(false);
          toggleBackupBtn->setValue(tr("刪除中..."));

          QDir dirToDelete(backupDir.absoluteFilePath(selection));

          toggleBackupBtn->setValue(dirToDelete.removeRecursively() ? tr("已刪除!") : tr("失敗..."));

          std::this_thread::sleep_for(std::chrono::seconds(2));
          toggleBackupBtn->setValue("");
          toggleBackupBtn->setEnabled(true);
        }).detach();
      }

    } else if (id == 2) {
      QStringList backupNames = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

      QString selection = MultiOptionDialog::getSelection(tr("選擇一個還原點"), backupNames, "", this);
      if (!selection.isEmpty()) {
        if (!ConfirmationDialog::confirm(tr("您確定要還原此切換備份嗎?"), tr("還原"), this)) return;
        std::thread([=]() {
          toggleBackupBtn->setEnabled(false);
          toggleBackupBtn->setValue(tr("還原中..."));

          std::string sourcePath = backupDir.absolutePath().toStdString() + "/" + selection.toStdString() + "/";
          std::string targetPath = "/data/params/d/";
          std::string command = "rsync -av --delete -l " + sourcePath + " " + targetPath;
          int result = std::system(command.c_str());

          if (result == 0) {
            toggleBackupBtn->setValue(tr("成功!"));
            updateFrogPilotToggles();
          } else {
            toggleBackupBtn->setValue(tr("失敗..."));
          }

          std::this_thread::sleep_for(std::chrono::seconds(2));
          toggleBackupBtn->setValue("");
          toggleBackupBtn->setEnabled(true);
        }).detach();
      }
    }
  });
  addItem(toggleBackupBtn);

  // Panda flashing
  ButtonControl *flashPandaBtn = new ButtonControl(tr("Flash Panda"), tr("FLASH"), tr("Use this button to troubleshoot and update the Panda device's firmware."));
  connect(flashPandaBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to flash the Panda?"), tr("Flash"), this)) {
      std::thread([=]() {
        flashPandaBtn->setEnabled(false);
        flashPandaBtn->setValue(tr("Flashing..."));

        QProcess recoverProcess;
        recoverProcess.setWorkingDirectory("/data/openpilot/panda/board");
        recoverProcess.start("/bin/sh", QStringList{"-c", "./recover.py"});
        if (!recoverProcess.waitForFinished()) {
          flashPandaBtn->setValue(tr("Recovery Failed..."));
          flashPandaBtn->setEnabled(true);
          return;
        }

        QProcess flashProcess;
        flashProcess.setWorkingDirectory("/data/openpilot/panda/board");
        flashProcess.start("/bin/sh", QStringList{"-c", "./flash.py"});
        if (!flashProcess.waitForFinished()) {
          flashPandaBtn->setValue(tr("Flash Failed..."));
          flashPandaBtn->setEnabled(true);
          return;
        }

        flashPandaBtn->setValue(tr("Flashed!"));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        flashPandaBtn->setValue(tr("Rebooting..."));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        Hardware::reboot();
      }).detach();
    }
  });
  addItem(flashPandaBtn);

  // Reset toggles to default
  ButtonControl *resetTogglesBtn = new ButtonControl(tr("將切換重設為預設值"), tr("重置"), tr("將您的切換設定重設為預設設定."));
  connect(resetTogglesBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("您確定要完全重設所有切換設定嗎?"), tr("重置"), this)) {
      std::thread([&] {
        resetTogglesBtn->setEnabled(false);
        resetTogglesBtn->setValue(tr("重置設定..."));

        std::system("rm -rf /persist/params");
        params.putBool("DoToggleReset", true);

        resetTogglesBtn->setValue(tr("重置!"));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        resetTogglesBtn->setValue(tr("重新啟動..."));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        Hardware::reboot();
      }).detach();
    }
  });
  addItem(resetTogglesBtn);

  // Force offroad/onroad
  std::vector<QString> forceStartedOptions{tr("OFFROAD"), tr("ONROAD"), tr("OFF")};
  forceStartedBtn = new FrogPilotButtonsControl(tr("Force Started State"), tr("Force openpilot either offroad or onroad."), "", forceStartedOptions, true);
  connect(forceStartedBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    if (id == 0) {
      paramsMemory.putBool("ForceOffroad", true);
      paramsMemory.putBool("ForceOnroad", false);
    } else if (id == 1) {
      paramsMemory.putBool("ForceOffroad", false);
      paramsMemory.putBool("ForceOnroad", true);
    } else if (id == 2) {
      paramsMemory.putBool("ForceOffroad", false);
      paramsMemory.putBool("ForceOnroad", false);
    }
    forceStartedBtn->updateButtonStyles(id);
  });
  forceStartedBtn->updateButtonStyles(2);
  addItem(forceStartedBtn);

}

void DevicePanel::updateCalibDescription() {
  QString desc =
      tr("openpilot 需要將設備固定在左右偏差 4° 以內，朝上偏差 5° 以内或朝下偏差 8° 以内。 "
         "鏡頭在後台會持續自動校準，很少有需要重置的情况。");
  std::string calib_bytes = params.get("CalibrationParams");
  if (!calib_bytes.empty()) {
    try {
      AlignedBuffer aligned_buf;
      capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
      auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
      if (calib.getCalStatus() != cereal::LiveCalibrationData::Status::UNCALIBRATED) {
        double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
        double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
        desc += tr("你的設備目前朝 %1° %2 以及朝 %3° %4.")
                    .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? tr("下") : tr("上"),
                         QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? tr("左") : tr("右"));
      }
    } catch (kj::Exception) {
      qInfo() << "invalid CalibrationParams";
    }
  }
  qobject_cast<ButtonControl *>(sender())->setDescription(desc);
}

void DevicePanel::reboot() {
  if (!uiState()->engaged()) {
    if (ConfirmationDialog::confirm(tr("是否確定要重新啟動?"), tr("重新啟動"), this)) {
      // Check engaged again in case it changed while the dialog was open
      if (!uiState()->engaged()) {
        params.putBool("DoReboot", true);
      }
    }
  } else {
    ConfirmationDialog::alert(tr("請先取消控車才能重新啟動"), this);
  }
}

void DevicePanel::poweroff() {
  if (!uiState()->engaged()) {
    if (ConfirmationDialog::confirm(tr("是否確定要關機?"), tr("關機"), this)) {
      // Check engaged again in case it changed while the dialog was open
      if (!uiState()->engaged()) {
        params.putBool("DoShutdown", true);
      }
    }
  } else {
    ConfirmationDialog::alert(tr("請先取消控車才能關機"), this);
  }
}

void DevicePanel::showEvent(QShowEvent *event) {
  pair_device->setVisible(uiState()->primeType() == PrimeType::UNPAIRED);
  ListWidget::showEvent(event);

  resetCalibBtn->setVisible(!params.getBool("ModelManagement"));
}

void SettingsWindow::hideEvent(QHideEvent *event) {
  closeParentToggle();

  parentToggleOpen = false;
  subParentToggleOpen = false;
  subSubParentToggleOpen = false;

  previousScrollPosition = 0;
}

void SettingsWindow::showEvent(QShowEvent *event) {
  setCurrentPanel(0);
}

void SettingsWindow::setCurrentPanel(int index, const QString &param) {
  panel_widget->setCurrentIndex(index);
  nav_btns->buttons()[index]->setChecked(true);
  if (!param.isEmpty()) {
    emit expandToggleDescription(param);
  }
}

SettingsWindow::SettingsWindow(QWidget *parent) : QFrame(parent) {

  // setup two main layouts
  sidebar_widget = new QWidget;
  QVBoxLayout *sidebar_layout = new QVBoxLayout(sidebar_widget);
  panel_widget = new QStackedWidget();

  // close button
  QPushButton *close_btn = new QPushButton(tr("← 返回"));
  close_btn->setStyleSheet(R"(
    QPushButton {
      font-size: 50px;
      border-radius: 25px;
      background-color: #292929;
      font-weight: 500;
    }
    QPushButton:pressed {
      background-color: #ADADAD;
    }
  )");
  close_btn->setFixedSize(300, 125);
  sidebar_layout->addSpacing(10);
  sidebar_layout->addWidget(close_btn, 0, Qt::AlignRight);
  QObject::connect(close_btn, &QPushButton::clicked, [this]() {
    if (subSubParentToggleOpen) {
      closeSubSubParentToggle();
      subSubParentToggleOpen = false;
    } else if (subParentToggleOpen) {
      closeSubParentToggle();
      subParentToggleOpen = false;
    } else if (parentToggleOpen) {
      closeParentToggle();
      parentToggleOpen = false;
    } else {
      closeSettings();
    }
  });

  // setup panels
  DevicePanel *device = new DevicePanel(this);
  QObject::connect(device, &DevicePanel::reviewTrainingGuide, this, &SettingsWindow::reviewTrainingGuide);
  QObject::connect(device, &DevicePanel::showDriverView, this, &SettingsWindow::showDriverView);

  TogglesPanel *toggles = new TogglesPanel(this);
  QObject::connect(this, &SettingsWindow::expandToggleDescription, toggles, &TogglesPanel::expandToggleDescription);
  QObject::connect(toggles, &TogglesPanel::updateMetric, this, &SettingsWindow::updateMetric);

  FrogPilotControlsPanel *frogpilotControls = new FrogPilotControlsPanel(this);
  QObject::connect(frogpilotControls, &FrogPilotControlsPanel::openParentToggle, this, [this]() {parentToggleOpen=true;});
  QObject::connect(frogpilotControls, &FrogPilotControlsPanel::openSubParentToggle, this, [this]() {subParentToggleOpen=true;});
  QObject::connect(frogpilotControls, &FrogPilotControlsPanel::openSubSubParentToggle, this, [this]() {subSubParentToggleOpen=true;});

  FrogPilotVisualsPanel *frogpilotVisuals = new FrogPilotVisualsPanel(this);
  QObject::connect(frogpilotVisuals, &FrogPilotVisualsPanel::openParentToggle, this, [this]() {parentToggleOpen=true;});

/////////////////////////////////////////////////////
  HFOPControlsPanel *hfoppilotControls = new HFOPControlsPanel(this);
  QObject::connect(hfoppilotControls, &HFOPControlsPanel::openParentToggle, this, [this]() {parentToggleOpen = true;});
/////////////////////////////////////////////////////

  QList<QPair<QString, QWidget *>> panels = {
    {tr("設備資訊"), device},
    {tr("網路設定"), new Networking(this)},
    {tr("官方設定"), toggles},
    {tr("軟體資訊"), new SoftwarePanel(this)},
    {tr("控制設定"), frogpilotControls},
    {tr("導航設定"), new FrogPilotNavigationPanel(this)},
    {tr("車輛設定"), new FrogPilotVehiclesPanel(this)},
    {tr("介面設定"), frogpilotVisuals},
/////////////////////////////////////////////////////
    {tr("H F O P"), hfoppilotControls},
/////////////////////////////////////////////////////
  };

  nav_btns = new QButtonGroup(this);
  for (auto &[name, panel] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setChecked(nav_btns->buttons().size() == 0);
    btn->setStyleSheet(R"(
      QPushButton {
        color: grey;
        border: none;
        background: none;
        font-size: 55px;
        font-weight: 500;
      }
      QPushButton:checked {
        color: white;
      }
      QPushButton:pressed {
        color: #68beca;
      }
    )");
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    nav_btns->addButton(btn);
    sidebar_layout->addWidget(btn, 0, Qt::AlignHCenter);

    const int lr_margin = name != tr("網路") ? 50 : 0;  // Network panel handles its own margins
    panel->setContentsMargins(lr_margin, 25, lr_margin, 25);

    ScrollView *panel_frame = new ScrollView(panel, this);
    panel_widget->addWidget(panel_frame);

    if (name == tr("Controls") || name == tr("Visuals")) {
      QScrollBar *scrollbar = panel_frame->verticalScrollBar();

      QObject::connect(scrollbar, &QScrollBar::valueChanged, this, [this](int value) {
        if (!parentToggleOpen) {
          previousScrollPosition = value;
        }
      });

      QObject::connect(scrollbar, &QScrollBar::rangeChanged, this, [this, panel_frame]() {
        if (!parentToggleOpen) {
          panel_frame->restorePosition(previousScrollPosition);
        }
      });
    }

    QObject::connect(btn, &QPushButton::clicked, [=, w = panel_frame]() {
      closeParentToggle();
      previousScrollPosition = 0;
      btn->setChecked(true);
      panel_widget->setCurrentWidget(w);
    });
  }
  sidebar_layout->setContentsMargins(50, 50, 100, 50);

  // main settings layout, sidebar + main panel
  QHBoxLayout *main_layout = new QHBoxLayout(this);

  sidebar_widget->setFixedWidth(500);
  main_layout->addWidget(sidebar_widget);
  main_layout->addWidget(panel_widget);

  setStyleSheet(R"(
    * {
      color: white;
      font-size: 50px;
    }
    SettingsWindow {
      background-color: black;
    }
    QStackedWidget, ScrollView {
      background-color: #292929;
      border-radius: 30px;
    }
  )");
}
