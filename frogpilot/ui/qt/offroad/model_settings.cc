#include "frogpilot/ui/qt/offroad/model_settings.h"

bool hasAllTinygradFiles(const QDir &modelDir, const QString &modelKey) {
  QStringList tinygradSuffixes = {
    "_driving_policy_metadata.pkl",
    "_driving_policy_tinygrad.pkl",
    "_driving_vision_metadata.pkl",
    "_driving_vision_tinygrad.pkl"
  };

  for (const QString &suffix : tinygradSuffixes) {
    if (!modelDir.exists(modelKey + suffix)) {
      return false;
    }
  }
  return true;
}

QString normalizeModelKey(QString key) {
  key = key.toLower();
  if (key.endsWith("_default")) {
    key.chop(QString("_default").size());
  }
  return key;
}

FrogPilotModelPanel::FrogPilotModelPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *modelLayout = new QStackedLayout();
  addItem(modelLayout);

  FrogPilotListWidget *modelList = new FrogPilotListWidget(this);

  ScrollView *modelPanel = new ScrollView(modelList, this);

  modelLayout->addWidget(modelPanel);

  FrogPilotListWidget *modelLabelsList = new FrogPilotListWidget(this);

  ScrollView *modelLabelsPanel = new ScrollView(modelLabelsList, this);

  modelLayout->addWidget(modelLabelsPanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> modelToggles {
    {"AutomaticallyDownloadModels", tr("自動下載新模型"), tr("<b>當有新駕駛模型可用時自動下載</b>。"), ""},
    {"DeleteModel", tr("刪除駕駛模型"), tr("<b>刪除已下載的駕駛模型</b>以釋放儲存空間。"), ""},
    {"DownloadModel", tr("下載駕駛模型"), tr("<b>手動下載駕駛模型</b>至裝置。"), ""},
    {"ModelRandomizer", tr("模型隨機選擇"), tr("<b>每次行駛隨機選擇駕駛模型</b>，並在行程結束時使用回饋提示以找出最適合您的模型！"), ""},
    {"ManageBlacklistedModels", tr("管理模型黑名單"), tr("<b>將駕駛模型加入或移出「模型隨機選擇」黑名單。</b>"), ""},
    {"ManageScores", tr("管理模型評分"), tr("<b>查看或重設「模型隨機選擇」使用的已保存模型評分</b>。"), ""},
    {"SelectModel", tr("選擇駕駛模型"), tr("<b>選擇 openpilot 使用的駕駛模型。</b>"), ""},
    {"UpdateTinygrad", tr("更新模型管理器"), tr("<b>更新「模型管理器」</b>以支援最新模型。"), ""}
  };

  for (const auto &[param, title, desc, icon] : modelToggles) {
    AbstractControl *modelToggle;

    if (param == "DeleteModel") {
      deleteModelButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("全部刪除")});
      QObject::connect(deleteModelButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList deletableModels;
        for (const QString &file : modelDir.entryList(QDir::Files)) {
          QString base = QFileInfo(file).baseName();
          for (const QString &modelKey : modelFileToNameMapProcessed.keys()) {
            if (base.startsWith(modelKey)) {
              QString modelName = modelFileToNameMapProcessed.value(modelKey);
              if (!deletableModels.contains(modelName)) {
                deletableModels.append(modelName);
              }
            }
          }
        }
        deletableModels.removeAll(processModelName(currentModel));
        deletableModels.removeAll(modelFileToNameMapProcessed.value(normalizeModelKey(QString::fromStdString(params_default.get("Model")))));
        noModelsDownloaded = deletableModels.isEmpty();

          if (id == 0) {
          QString modelToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的駕駛模型"), deletableModels, "", this);
          if (!modelToDelete.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除\"%1\"模型嗎？").arg(modelToDelete), tr("刪除"), this)) {
            QString modelFile = modelFileToNameMapProcessed.key(modelToDelete);
            for (const QString &file : modelDir.entryList(QDir::Files)) {
              QString base = QFileInfo(file).baseName();
              if (base.startsWith(modelFile)) {
                QFile::remove(modelDir.filePath(file));
              }
            }

            allModelsDownloaded = false;
          }
        } else if (id == 1) {
          if (ConfirmationDialog::confirm(tr("您確定要刪除所有已下載的駕駛模型嗎？"), tr("刪除"), this)) {
            for (const QString &file : modelDir.entryList(QDir::Files)) {
              QString base = QFileInfo(file).baseName();
              for (const QString &modelKey : modelFileToNameMapProcessed.keys()) {
                QString modelName = modelFileToNameMapProcessed.value(modelKey);
                if (deletableModels.contains(modelName) && base.startsWith(modelKey)) {
                  QFile::remove(modelDir.filePath(file));
                  break;
                }
              }
            }

            allModelsDownloaded = false;
            noModelsDownloaded = true;
          }
        }
      });
      modelToggle = deleteModelButton;
    } else if (param == "DownloadModel") {
      downloadModelButton = new FrogPilotButtonsControl(title, desc, icon, {tr("下載"), tr("全部下載")});
      QObject::connect(downloadModelButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
          if (tinygradUpdate) {
          if (FrogPilotConfirmationDialog::yesorno(tr("Tinygrad 已過期，必須先更新才能下載新模型。現在要更新嗎？"), this)) {
            if (FrogPilotConfirmationDialog::yesorno(tr("更新 Tinygrad 會刪除所有現有的 Tinygrad 型模型，需重新下載。是否繼續？"), this)) {
              params_memory.putBool("UpdateTinygrad", true);
              params_memory.put("ModelDownloadProgress", "Downloading...");

              updateTinygradButton->setText(0, tr("取消"));
              updateTinygradButton->setValue(tr("正在更新..."));

              updatingTinygrad = true;
            }
          }
        } else if (id == 0) {
          if (modelDownloading) {
            params_memory.putBool("CancelModelDownload", true);

            cancellingDownload = true;
          } else {
            QStringList downloadableModels = availableModelNames;
            for (const QString &modelKey : modelFileToNameMap.keys()) {
              QString modelName = modelFileToNameMap.value(modelKey);
              if (modelDir.exists(modelKey + ".thneed") || hasAllTinygradFiles(modelDir, modelKey)) {
                downloadableModels.removeAll(modelName);
              }
            }
            allModelsDownloaded = downloadableModels.isEmpty();

            QString modelToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的駕駛模型"), downloadableModels, "", this);
            if (!modelToDownload.isEmpty()) {
              params_memory.put("ModelToDownload", modelFileToNameMap.key(modelToDownload).toStdString());
              params_memory.put("ModelDownloadProgress", "Downloading...");

              downloadModelButton->setText(0, tr("取消"));

              downloadModelButton->setValue("Downloading...");

              downloadModelButton->setVisibleButton(1, false);

              modelDownloading = true;
            }
          }
        } else if (id == 1) {
          if (allModelsDownloading) {
            params_memory.putBool("CancelModelDownload", true);

            cancellingDownload = true;
          } else {
            params_memory.putBool("DownloadAllModels", true);
            params_memory.put("ModelDownloadProgress", "Downloading...");

            downloadModelButton->setText(1, tr("取消"));

            downloadModelButton->setValue("Downloading...");

            downloadModelButton->setVisibleButton(0, false);

            allModelsDownloading = true;
          }
        }
      });
      modelToggle = downloadModelButton;
    } else if (param == "ManageBlacklistedModels") {
      FrogPilotButtonsControl *blacklistButton = new FrogPilotButtonsControl(title, desc, icon, {tr("加入"), tr("移除"), tr("全部移除")});
      QObject::connect(blacklistButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList blacklistedModels = QString::fromStdString(params.get("BlacklistedModels")).split(",");
        blacklistedModels.removeAll("");

        if (id == 0) {
          QStringList blacklistableModels;
          for (const QString &model : modelFileToNameMapProcessed.keys()) {
            if (!blacklistedModels.contains(model)) {
              blacklistableModels.append(modelFileToNameMapProcessed.value(model));
            }
          }

          if (blacklistableModels.size() <= 1) {
            ConfirmationDialog::alert(tr("沒有更多可加入黑名單的駕駛模型。唯一可用的模型是\"%1\"！").arg(blacklistableModels.first()), this);
          } else {
            QString modelToBlacklist = MultiOptionDialog::getSelection(tr("選擇要加入黑名單的駕駛模型"), blacklistableModels, "", this);
            if (!modelToBlacklist.isEmpty()) {
              if (ConfirmationDialog::confirm(tr("您確定要將\"%1\"模型加入黑名單嗎？").arg(modelToBlacklist), tr("加入"), this)) {
                blacklistedModels.append(modelFileToNameMapProcessed.key(modelToBlacklist));

                params.put("BlacklistedModels", blacklistedModels.join(",").toStdString());
              }
            }
          }
        } else if (id == 1) {
          QStringList whitelistableModels;
          for (const QString &model : blacklistedModels) {
            QString modelName = modelFileToNameMapProcessed.value(model);
            whitelistableModels.append(modelName);
          }
          whitelistableModels.sort();

          QString modelToWhitelist = MultiOptionDialog::getSelection(tr("選擇要從黑名單移除的駕駛模型"), whitelistableModels, "", this);
          if (!modelToWhitelist.isEmpty()) {
            if (ConfirmationDialog::confirm(tr("您確定要將\"%1\"模型從黑名單移除嗎？").arg(modelToWhitelist), tr("移除"), this)) {
              blacklistedModels.removeAll(modelFileToNameMapProcessed.key(modelToWhitelist));

              params.put("BlacklistedModels", blacklistedModels.join(",").toStdString());
            }
          }
        } else if (id == 2) {
          if (FrogPilotConfirmationDialog::yesorno(tr("您確定要移除所有已加入黑名單的駕駛模型嗎？"), this)) {
            params.remove("BlacklistedModels");
            params_cache.remove("BlacklistedModels");
          }
        }
      });
      modelToggle = blacklistButton;
    } else if (param == "ManageScores") {
      FrogPilotButtonsControl *manageScoresButton = new FrogPilotButtonsControl(title, desc, icon, {tr("重設"), tr("檢視")});
      QObject::connect(manageScoresButton, &FrogPilotButtonsControl::buttonClicked, [modelLayout, modelLabelsList, modelLabelsPanel, this](int id) {
        if (id == 0) {
          if (FrogPilotConfirmationDialog::yesorno(tr("重設所有模型的行駛紀錄與評分？這會清除您的行駛歷史與收集的回饋！"), this)) {
            params.remove("ModelDrivesAndScores");
            params_cache.remove("ModelDrivesAndScores");
          }
        } else if (id == 1) {
          openSubPanel();

          updateModelLabels(modelLabelsList);

          modelLayout->setCurrentWidget(modelLabelsPanel);
        }
      });
      modelToggle = manageScoresButton;
    } else if (param == "SelectModel") {
      selectModelButton = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(selectModelButton, &ButtonControl::clicked, [this]() {
        QStringList selectableModels;
        for (const QString &modelKey : modelFileToNameMap.keys()) {
          QString modelName = modelFileToNameMap.value(modelKey);
          if (modelName.contains("(Default)")) {
            continue;
          }

          if (modelDir.exists(modelKey + ".thneed") || hasAllTinygradFiles(modelDir, modelKey)) {
            selectableModels.append(modelName);
          }
        }
        selectableModels.sort();
        selectableModels.prepend(modelFileToNameMap.value(normalizeModelKey(QString::fromStdString(params_default.get("Model")))));

        QString modelToSelect = MultiOptionDialog::getSelection(tr("選擇模型 — 🗺️ = 導航 | 📡 = 雷達 | 👀 = VOACC"), selectableModels, currentModel, this);
        if (!modelToSelect.isEmpty()) {
          currentModel = modelToSelect;

          params.put("Model", modelFileToNameMap.key(modelToSelect).toStdString());

          updateFrogPilotToggles();

          if (started) {
            if (FrogPilotConfirmationDialog::toggleReboot(this)) {
              Hardware::reboot();
            }
          }
          selectModelButton->setValue(modelToSelect);

          QStringList deletableModels;
          for (const QString &file : modelDir.entryList(QDir::Files)) {
            QString base = QFileInfo(file).baseName();
            for (const QString &modelKey : modelFileToNameMapProcessed.keys()) {
              if (base.startsWith(modelKey)) {
                QString modelName = modelFileToNameMapProcessed.value(modelKey);
                if (!deletableModels.contains(modelName)) {
                  deletableModels.append(modelName);
                }
              }
            }
          }
          deletableModels.removeAll(processModelName(currentModel));
          deletableModels.removeAll(modelFileToNameMapProcessed.value(normalizeModelKey(QString::fromStdString(params_default.get("Model")))));
          noModelsDownloaded = deletableModels.isEmpty();
        }
      });
      modelToggle = selectModelButton;

    } else if (param == "UpdateTinygrad") {
      updateTinygradButton = new FrogPilotButtonsControl(title, desc, icon, {tr("更新")});
      QObject::connect(updateTinygradButton, &FrogPilotButtonsControl::buttonClicked, [this]() {
        if (updatingTinygrad) {
          params_memory.putBool("CancelModelDownload", true);

          updateTinygradButton->setEnabled(false);
          updateTinygradButton->setValue(tr("取消中..."));

          cancellingDownload = true;
        } else {
          if (FrogPilotConfirmationDialog::yesorno(tr("更新 Tinygrad 會刪除現有基於 Tinygrad 的駕駛模型，需重新下載。是否繼續？"), this)) {
            params_memory.putBool("UpdateTinygrad", true);
            params_memory.put("ModelDownloadProgress", "Downloading...");

            updateTinygradButton->setText(0, tr("取消"));
            updateTinygradButton->setValue(tr("正在更新..."));

            updatingTinygrad = true;
          }
        }
      });
      modelToggle = updateTinygradButton;

    } else {
      modelToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = modelToggle;

    modelList->addItem(modelToggle);

    QObject::connect(modelToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(modelToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(static_cast<ToggleControl*>(toggles["ModelRandomizer"]), &ToggleControl::toggleFlipped, [this](bool state) {
    updateToggles();

    if (state && !allModelsDownloaded) {
      if (FrogPilotConfirmationDialog::yesorno(tr("「模型隨機選擇」僅在已下載模型時有效。現在下載所有模型嗎？"), this)) {
        params_memory.putBool("DownloadAllModels", true);
        params_memory.put("ModelDownloadProgress", "Downloading...");

        downloadModelButton->setValue("Downloading...");

        allModelsDownloading = true;
      }
    }
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [modelLayout, modelPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    modelLayout->setCurrentWidget(modelPanel);
  });
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotModelPanel::updateState);
}

void FrogPilotModelPanel::showEvent(QShowEvent *event) {
  FrogPilotUIState &fs = *frogpilotUIState();
  UIState &s = *uiState();

  allModelsDownloading = params_memory.getBool("DownloadAllModels");
  modelDownloading = !params_memory.get("ModelDownloadProgress").empty();
  tinygradUpdate = params.getBool("TinygradUpdateAvailable");
  updatingTinygrad = params_memory.getBool("UpdateTinygrad");

  modelDownloading &= !updatingTinygrad;

  QStringList availableModels = QString::fromStdString(params.get("AvailableModels")).split(",");
  availableModels.sort();
  availableModelNames = QString::fromStdString(params.get("AvailableModelNames")).split(",");
  availableModelNames.sort();

  modelFileToNameMap.clear();
  modelFileToNameMapProcessed.clear();
  for (int i = 0; i < qMin(availableModels.size(), availableModelNames.size()); ++i) {
    modelFileToNameMap.insert(availableModels[i], availableModelNames[i]);
    modelFileToNameMapProcessed.insert(availableModels[i], processModelName(availableModelNames[i]));
  }

  QStringList downloadableModels = availableModelNames;
  for (const QString &modelKey : modelFileToNameMap.keys()) {
    QString modelName = modelFileToNameMap.value(modelKey);
    if (modelDir.exists(modelKey + ".thneed") || hasAllTinygradFiles(modelDir, modelKey)) {
      downloadableModels.removeAll(modelName);
    }
  }
  allModelsDownloaded = downloadableModels.isEmpty();

  QStringList deletableModels;
  for (const QString &file : modelDir.entryList(QDir::Files)) {
    QString base = QFileInfo(file).baseName();
    for (const QString &modelKey : modelFileToNameMapProcessed.keys()) {
      if (base.startsWith(modelKey)) {
        QString modelName = modelFileToNameMapProcessed.value(modelKey);
        if (!deletableModels.contains(modelName)) {
          deletableModels.append(modelName);
        }
      }
    }
  }
  deletableModels.removeAll(processModelName(currentModel));
  deletableModels.removeAll(modelFileToNameMapProcessed.value(normalizeModelKey(QString::fromStdString(params_default.get("Model")))));
  noModelsDownloaded = deletableModels.isEmpty();

  QString modelKey = normalizeModelKey(QString::fromStdString(params.get("Model")));
  if (!modelDir.exists(modelKey + ".thneed") && !hasAllTinygradFiles(modelDir, modelKey)) {
    modelKey = normalizeModelKey(QString::fromStdString(params_default.get("Model")));
  }
  currentModel = modelFileToNameMap.value(modelKey);
  selectModelButton->setValue(currentModel);

  bool parked = !s.scene.started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  deleteModelButton->setEnabled(!(allModelsDownloading || modelDownloading || noModelsDownloaded));

  downloadModelButton->setEnabledButtons(0, !allModelsDownloaded && !allModelsDownloading && !cancellingDownload && !updatingTinygrad && fs.frogpilot_scene.online && parked);
  downloadModelButton->setEnabledButtons(1, !allModelsDownloaded && !modelDownloading && !cancellingDownload && !updatingTinygrad && fs.frogpilot_scene.online && parked);

  downloadModelButton->setValue(fs.frogpilot_scene.online ? (parked ? "" : "Not parked") : tr("離線..."));

  updateTinygradButton->setEnabled(!modelDownloading && !cancellingDownload && fs.frogpilot_scene.online && parked && tinygradUpdate);
  updateTinygradButton->setValue(tinygradUpdate ? tr("有可用更新！") : tr("已是最新！"));

  started = s.scene.started;

  updateToggles();
}

void FrogPilotModelPanel::updateState(const UIState &s, const FrogPilotUIState &fs) {
  if (!isVisible() || finalizingDownload) {
    return;
  }

  bool parked = !started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  if (allModelsDownloading || modelDownloading) {
    QString progress = QString::fromStdString(params_memory.get("ModelDownloadProgress"));
    bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|failed|missing|offline", QRegularExpression::CaseInsensitiveOption));

    if (progress != "Downloading...") {
      downloadModelButton->setValue(progress);
    }

    if (progress == "All models downloaded!" || progress == "Downloaded!" && !allModelsDownloading || downloadFailed) {
      finalizingDownload = true;

      QTimer::singleShot(2500, [progress, this]() {
        allModelsDownloading = false;
        cancellingDownload = false;
        finalizingDownload = false;
        modelDownloading = false;
        noModelsDownloaded = false;

        QStringList downloadableModels = availableModelNames;
        for (const QString &modelKey : modelFileToNameMap.keys()) {
          QString modelName = modelFileToNameMap.value(modelKey);
          if (modelDir.exists(modelKey + ".thneed") || hasAllTinygradFiles(modelDir, modelKey)) {
            downloadableModels.removeAll(modelName);
          }
        }
        allModelsDownloaded = downloadableModels.isEmpty();

        params_memory.remove("ModelDownloadProgress");

        downloadModelButton->setEnabled(true);
        downloadModelButton->setValue("");
      });
    }
  } else {
    downloadModelButton->setValue(fs.frogpilot_scene.online ? (parked ? "" : "Not parked") : tr("離線..."));
  }

  if (updatingTinygrad) {
    QString progress = QString::fromStdString(params_memory.get("ModelDownloadProgress"));
    bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|failed|missing|offline", QRegularExpression::CaseInsensitiveOption));

    if (progress != "Downloading...") {
      updateTinygradButton->setValue(progress);
    }

    if (progress == "Updated!" && updatingTinygrad || downloadFailed) {
      finalizingDownload = true;

      QTimer::singleShot(2500, [progress, this]() {
        modelDownloading = !params_memory.get("ModelDownloadProgress").empty();

        if (modelDownloading) {
          downloadModelButton->setText(1, tr("取消"));

          downloadModelButton->setValue("Downloading...");

          downloadModelButton->setVisibleButton(0, false);
        } else {
          cancellingDownload = false;
        }

        tinygradUpdate = params.getBool("TinygradUpdateAvailable");

        finalizingDownload = false;
        updatingTinygrad = false;

        updateTinygradButton->setEnabled(tinygradUpdate);
        updateTinygradButton->setText(0, tr("更新"));
        updateTinygradButton->setValue(tinygradUpdate ? tr("有可用更新！") : tr("已是最新！"));
      });
    }
  }

  deleteModelButton->setEnabled(!(allModelsDownloading || modelDownloading || noModelsDownloaded));

  downloadModelButton->setText(0, modelDownloading ? tr("取消") : tr("下載"));
  downloadModelButton->setText(1, allModelsDownloading ? tr("取消") : tr("全部下載"));

  downloadModelButton->setEnabledButtons(0, !allModelsDownloaded && !allModelsDownloading && !cancellingDownload && !finalizingDownload && !updatingTinygrad && fs.frogpilot_scene.online && parked);
  downloadModelButton->setEnabledButtons(1, !allModelsDownloaded && !modelDownloading && !cancellingDownload && !finalizingDownload && !updatingTinygrad && fs.frogpilot_scene.online && parked);

  downloadModelButton->setVisibleButton(0, !allModelsDownloading);
  downloadModelButton->setVisibleButton(1, !modelDownloading);

  updateTinygradButton->setEnabled(!modelDownloading && !cancellingDownload && !cancellingDownload && !finalizingDownload && fs.frogpilot_scene.online && parked && tinygradUpdate);

  started = s.scene.started;

  parent->keepScreenOn = allModelsDownloading || modelDownloading || updatingTinygrad;
}

void FrogPilotModelPanel::updateModelLabels(FrogPilotListWidget *labelsList) {
  labelsList->clear();

  QJsonObject modelDrivesAndScores = QJsonDocument::fromJson(QString::fromStdString(params.get("ModelDrivesAndScores")).toUtf8()).object();

  for (const QString &modelName : availableModelNames) {
    QJsonObject modelData = modelDrivesAndScores.value(processModelName(modelName)).toObject();

    int drives = modelData.value("Drives").toInt(0);
    int score = modelData.value("Score").toInt(0);

    QString drivesDisplay = drives == 1 ? QString("%1 Drive").arg(drives) : drives > 0 ? QString("%1 Drives").arg(drives) : "N/A";
    QString scoreDisplay = drives > 0 ? QString("Score: %1%").arg(score) : "N/A";

    QString labelTitle = processModelName(modelName);
    QString labelText = QString("%1 (%2)").arg(scoreDisplay, drivesDisplay);

    LabelControl *labelControl = new LabelControl(labelTitle, labelText, "", this);
    labelsList->addItem(labelControl);
  }
}

void FrogPilotModelPanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    bool setVisible = parent->tuningLevel >= parent->frogpilotToggleLevels[key].toDouble();

    if (key == "ManageBlacklistedModels" || key == "ManageScores") {
      setVisible &= params.getBool("ModelRandomizer");
    }

    else if (key == "SelectModel") {
      setVisible &= !params.getBool("ModelRandomizer");
    }

    toggle->setVisible(setVisible);
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
