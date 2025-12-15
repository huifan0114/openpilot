#include "frogpilot/ui/qt/offroad/theme_settings.h"

bool isUserCreatedTheme(const QString &themeName) {
  return themeName.endsWith("-user_created");
}

void updateAssetParam(const QString &assetParam, Params &params, const QString &value, bool add) {
  QStringList assets = QString::fromStdString(params.get(assetParam.toStdString())).split(",", QString::SkipEmptyParts);
  if (add) {
    if (!assets.contains(value)) {
      assets.append(value);
    }
  } else {
    assets.removeAll(value);
  }
  assets.sort();

  params.put(assetParam.toStdString(), assets.join(",").toStdString());
}

void deleteThemeAsset(QDir &directory, const QString &subFolder, const QString &assetParam, const QString &themeToDelete, Params &params) {
  bool useFiles = subFolder.isEmpty();

  QString baseName = themeToDelete.toLower();
  baseName.replace("(", "-").replace(")", "").replace(" ", "-");
  baseName.remove(QRegularExpression("[^a-z0-9\\-]"));
  while (baseName.endsWith("-")) {
    baseName.chop(1);
  }

  QString baseUnderscore = baseName;
  baseUnderscore.replace("-", "_");

  QStringList candidateNames = {
    baseName,
    baseName + "-user-created",
    baseUnderscore,
    baseUnderscore + "-user_created"
  };

  if (useFiles) {
    QStringList files = directory.entryList(QDir::Files);
    for (QString &file : files) {
      QString normalizedFile = QFileInfo(file).baseName().toLower();
      normalizedFile.replace("_", "-");
      normalizedFile.remove(QRegularExpression("[^a-z0-9\\-~]"));

      if (candidateNames.contains(normalizedFile)) {
        QFile::remove(directory.filePath(file));
        break;
      }
    }
  } else {
    for (QString &candidate : candidateNames) {
      QString fullSubPath = QDir(candidate).filePath(subFolder);
      QDir targetDir(directory.filePath(fullSubPath));

      if (targetDir.exists()) {
        targetDir.removeRecursively();
        break;
      }
    }
  }

  updateAssetParam(assetParam, params, themeToDelete, true);
}

void downloadThemeAsset(const QString &input, const std::string &paramKey, const QString &assetParam, Params &params, Params &params_memory) {
  QString output = input;
  int tilde = output.indexOf("~");
  if (tilde >= 0) {
    output = output.left(tilde).toLower() + "~" + output.mid(tilde + 1);
  } else {
    output = output.toLower();
  }
  output.remove("(").remove(")");
  output.replace(" ", input.contains("(") ? "-" : "_");

  params_memory.put(paramKey, output.toStdString());
}

QStringList getHolidayThemes() {
  return QStringList()
         << "New Year's"
         << "Valentine's Day"
         << "St. Patrick's Day"
         << "World Frog Day"
         << "April Fools"
         << "Easter"
         << "May the Fourth"
         << "Cinco de Mayo"
         << "Stitch Day"
         << "Fourth of July"
         << "Halloween"
         << "Thanksgiving"
         << "Christmas";
}

QStringList getThemeList(const bool &randomThemes, const QDir &themePacksDirectory, const QString &subFolder, const QString &assetParam, Params &params) {
  bool useFiles = subFolder.isEmpty();

  QString currentAsset = randomThemes ? "" : QString::fromStdString(params.get(assetParam.toStdString()));

  QStringList themeList;
  for (const QFileInfo &entry : themePacksDirectory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
    if (entry.baseName() == currentAsset) {
      continue;
    }

    if (useFiles && entry.isDir()) {
      continue;
    }

    if (!useFiles) {
      QString targetPath = QDir(entry.filePath()).filePath(subFolder);
      if (!QFileInfo(targetPath).exists()) {
        continue;
      }
    }

    QString baseName = entry.baseName();
    bool userCreated = isUserCreatedTheme(baseName);
    if (userCreated) {
      baseName = baseName.replace("-user_created", "");
    }

    int tildeIdx = baseName.indexOf("~");
    QString creator;
    if (tildeIdx >= 0) {
      creator = baseName.mid(tildeIdx + 1);
      baseName = baseName.left(tildeIdx);
    }

    QStringList parts = baseName.split(baseName.contains("-") ? "-" : "_", QString::SkipEmptyParts);
    for (QString &part : parts) {
      part[0] = part[0].toUpper();
    }

    QString displayName;
    if (userCreated) {
      displayName = parts.join(" ");
    } else {
      displayName = (parts.size() <= 1 || useFiles) ? parts.join(" ") : QString("%1 (%2)").arg(parts[0], parts.mid(1).join(" "));
    }

    if (userCreated) {
      displayName += " 🌟";
    }
    if (!creator.isEmpty()) {
      displayName += " - by: " + creator;
    }

    themeList.append(displayName);
  }

  return themeList;
}

QString getThemeName(const std::string &paramKey, Params &params) {
  QString value = QString::fromStdString(params.get(paramKey));

  QString baseName = value;

  int tildeIdx = baseName.indexOf("~");
  QString creator;
  if (tildeIdx >= 0) {
    creator = baseName.mid(tildeIdx + 1);
    baseName = baseName.left(tildeIdx);
  }

  QStringList parts = baseName.split(baseName.contains("-") ? "-" : "_", QString::SkipEmptyParts);
  for (QString &part : parts) {
    part[0] = part[0].toUpper();
  }

  QString displayName;
  if (baseName.contains("-") && parts.size() > 1) {
    displayName = QString("%1 (%2)").arg(parts[0], parts.mid(1).join(" "));
  } else {
    displayName = parts.join(" ");
  }

  if (isUserCreatedTheme(value)) {
    displayName = displayName.split(" (")[0] + " 🌟";
  }
  if (!creator.isEmpty()) {
    displayName += " - by: " + creator;
  }

  return displayName;
}

QString storeThemeName(const QString &input, const std::string &paramKey, Params &params) {
  QString output = input.toLower().remove("(").remove(")").remove("'").remove(".");
  output.replace(" ", input.contains("(") ? "-" : "_");
  output.replace("_🌟", "-user_created");
  output = output.trimmed();

  params.put(paramKey, output.toStdString());

  return getThemeName(paramKey, params);
}

FrogPilotThemesPanel::FrogPilotThemesPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  QStackedLayout *themesLayout = new QStackedLayout();
  addItem(themesLayout);

  FrogPilotListWidget *themesList = new FrogPilotListWidget(this);

  ScrollView *themesPanel = new ScrollView(themesList, this);

  themesLayout->addWidget(themesPanel);

  FrogPilotListWidget *customThemesList = new FrogPilotListWidget(this);

  ScrollView *customThemesPanel = new ScrollView(customThemesList, this);

  themesLayout->addWidget(customThemesPanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> themeToggles {
    {"PersonalizeOpenpilot", tr("自訂主題"), tr("<b>openpilot 的整體外觀與風格。</b> 使用 \"The Pond\" 中的「主題製作器」來建立並分享您的主題！"), "../../frogpilot/assets/toggle_icons/icon_frog.png"},
    {"CustomColors", tr("配色方案"), tr("<b>整個 openpilot 使用的配色方案。</b> 使用 \"The Pond\" 中的「主題製作器」建立並分享您的主題！"), ""},
    {"CustomDistanceIcons", tr("距離按鈕"), tr("<b>顯示於駕駛畫面的距離按鈕圖示。</b> 使用 \"The Pond\" 中的「主題製作器」建立並分享您的主題！"), ""},
    {"CustomIcons", tr("圖示套件"), tr("<b>整個 openpilot 使用的圖示風格。</b> 使用 \"The Pond\" 中的「主題製作器」建立並分享您的主題！"), ""},
    {"CustomSounds", tr("音效套件"), tr("<b>openpilot 使用的音效套件。</b> 使用 \"The Pond\" 中的「主題製作器」建立並分享您的主題！"), ""},
    {"WheelIcon", tr("方向盤圖示"), tr("<b>顯示於駕駛畫面右上方的方向盤圖示</b>。 使用 \"The Pond\" 中的「主題製作器」建立並分享您的主題！"), ""},
    {"CustomSignals", tr("方向燈"), tr("<b>主題化的方向燈動畫。</b> 使用 \"The Pond\" 中的「主題製作器」建立並分享您的主題！"), ""},
    {"DownloadStatusLabel", tr("下載狀態"), "", ""},

    {"HolidayThemes", tr("節日主題"), tr("<b>基於美國節日的主題。</b> 小型節日持續一天；重大節日（聖誕節、復活節、萬聖節）則為期一週。"), "../../frogpilot/assets/toggle_icons/icon_calendar.png"},
    {"RainbowPath", tr("彩虹路徑"), tr("<b>將行駛路徑著色成類似 Mario Kart 的 \"彩虹之路\"。</b>"), "../../frogpilot/assets/toggle_icons/icon_rainbow.png"},
    {"RandomEvents", tr("隨機事件"), tr("<b>依照行車情況觸發的偶發畫面效果。</b> 這些僅為視覺效果，並不影響 openpilot 的行駛！"), "../../frogpilot/assets/toggle_icons/icon_random.png"},
    {"RandomThemes", tr("隨機主題"), tr("<b>在每次行駛之間從已下載的主題中隨機選擇一個主題</b>，可在不變更設定的情況下增加多樣性。"), "../../frogpilot/assets/toggle_icons/icon_random_themes.png"},
    {"StartupAlert", tr("啟動提示"), tr("<b>自訂每次行駛開始時顯示的 \"啟動提示\" 訊息</b>。"), "../../frogpilot/assets/toggle_icons/icon_message.png"}
  };

  for (const auto &[param, title, desc, icon] : themeToggles) {
    AbstractControl *themeToggle;

    if (param == "PersonalizeOpenpilot") {
      FrogPilotManageControl *personalizeOpenpilotToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(personalizeOpenpilotToggle, &FrogPilotManageControl::manageButtonClicked, [customThemesPanel, themesLayout]() {
        themesLayout->setCurrentWidget(customThemesPanel);
      });
      themeToggle = personalizeOpenpilotToggle;
    } else if (param == "CustomColors") {
      manageCustomColorsButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("下載"), tr("選擇")});
      QObject::connect(manageCustomColorsButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList colorSchemes = getThemeList(randomThemes, QDir(themePacksDirectory.path()), "colors", "CustomColors", params);

        if (id == 0) {
          QString colorSchemeToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的配色方案"), colorSchemes, "", this);
          if (!colorSchemeToDelete.isEmpty() && ConfirmationDialog::confirm(tr("刪除「%1」配色方案？").arg(colorSchemeToDelete), tr("刪除"), this)) {
            colorsDownloaded = false;

            deleteThemeAsset(themePacksDirectory, "colors", "DownloadableColors", colorSchemeToDelete, params);
          }
        } else if (id == 1) {
          if (colorDownloading) {
            cancellingDownload = true;

            params_memory.putBool("CancelThemeDownload", true);

            QTimer::singleShot(2500, [this]() {
              cancellingDownload = false;
              colorDownloading = false;
              themeDownloading = false;

              params_memory.putBool("CancelThemeDownload", false);
            });
          } else {
            QStringList downloadableColorSchemes = QString::fromStdString(params.get("DownloadableColors")).split(",");
            colorSchemeToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的配色方案"), downloadableColorSchemes, "", this);
            if (!colorSchemeToDownload.isEmpty()) {
              colorDownloading = true;
              themeDownloading = true;

              params_memory.put("ThemeDownloadProgress", "Downloading...");

              downloadThemeAsset(colorSchemeToDownload, "ColorToDownload", "DownloadableColors", params, params_memory);

              downloadStatusLabel->setText("Downloading...");
            }
          }
        } else if (id == 2) {
          colorSchemes.append("Stock");
          colorSchemes.append(getHolidayThemes());
          colorSchemes.sort();

          QString colorSchemeToSelect = MultiOptionDialog::getSelection(tr("選擇配色方案"), colorSchemes, getThemeName("CustomColors", params), this);
          if (!colorSchemeToSelect.isEmpty()) {
            manageCustomColorsButton->setValue(storeThemeName(colorSchemeToSelect, "CustomColors", params));
          }
        }
      });
      manageCustomColorsButton->setValue(getThemeName(param.toStdString(), params));
      themeToggle = manageCustomColorsButton;
    } else if (param == "CustomDistanceIcons") {
      manageDistanceIconsButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("下載"), tr("選擇")});
      QObject::connect(manageDistanceIconsButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList distanceIconPacks = getThemeList(randomThemes, QDir(themePacksDirectory.path()), "distance_icons", "CustomDistanceIcons", params);

        if (id == 0) {
          QString distanceIconPackToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的距離圖示套件"), distanceIconPacks, "", this);
          if (!distanceIconPackToDelete.isEmpty() && ConfirmationDialog::confirm(tr("刪除「%1」距離圖示套件？").arg(distanceIconPackToDelete), tr("刪除"), this)) {
            distanceIconsDownloaded = false;

            deleteThemeAsset(themePacksDirectory, "distance_icons", "DownloadableDistanceIcons", distanceIconPackToDelete, params);
          }
        } else if (id == 1) {
          if (distanceIconDownloading) {
            cancellingDownload = true;

            params_memory.putBool("CancelThemeDownload", true);

            QTimer::singleShot(2500, [this]() {
              cancellingDownload = false;
              distanceIconDownloading = false;
              themeDownloading = false;

              params_memory.putBool("CancelThemeDownload", false);
            });
          } else {
            QStringList downloadableDistanceIconPacks = QString::fromStdString(params.get("DownloadableDistanceIcons")).split(",");
            distanceIconPackToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的距離圖示套件"), downloadableDistanceIconPacks, "", this);
            if (!distanceIconPackToDownload.isEmpty()) {
              distanceIconDownloading = true;
              themeDownloading = true;

              params_memory.put("ThemeDownloadProgress", "Downloading...");

              downloadThemeAsset(distanceIconPackToDownload, "DistanceIconToDownload", "DownloadableDistanceIcons", params, params_memory);

              downloadStatusLabel->setText("Downloading...");
            }
          }
        } else if (id == 2) {
          distanceIconPacks.append("Stock");
          distanceIconPacks.append(getHolidayThemes());
          distanceIconPacks.sort();

          QString distanceIconPackToSelect = MultiOptionDialog::getSelection(tr("選擇距離圖示套件"), distanceIconPacks, getThemeName("CustomDistanceIcons", params), this);
          if (!distanceIconPackToSelect.isEmpty()) {
            manageDistanceIconsButton->setValue(storeThemeName(distanceIconPackToSelect, "CustomDistanceIcons", params));
          }
        }
      });
      manageDistanceIconsButton->setValue(getThemeName(param.toStdString(), params));
      themeToggle = manageDistanceIconsButton;
    } else if (param == "CustomIcons") {
      manageCustomIconsButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("下載"), tr("選擇")});
      QObject::connect(manageCustomIconsButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList iconPacks = getThemeList(randomThemes, QDir(themePacksDirectory.path()), "icons", "CustomIcons", params);

        if (id == 0) {
          QString iconPackToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的圖示套件"), iconPacks, "", this);
          if (!iconPackToDelete.isEmpty() && ConfirmationDialog::confirm(tr("刪除「%1」圖示套件？").arg(iconPackToDelete), tr("刪除"), this)) {
            iconsDownloaded = false;

            deleteThemeAsset(themePacksDirectory, "icons", "DownloadableIcons", iconPackToDelete, params);
          }
        } else if (id == 1) {
          if (iconDownloading) {
            cancellingDownload = true;

            params_memory.putBool("CancelThemeDownload", true);

            QTimer::singleShot(2500, [this]() {
              cancellingDownload = false;
              iconDownloading = false;
              themeDownloading = false;

              params_memory.putBool("CancelThemeDownload", false);
            });
          } else {
            QStringList downloadableIconPacks = QString::fromStdString(params.get("DownloadableIcons")).split(",");
            iconPackToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的圖示套件"), downloadableIconPacks, "", this);
            if (!iconPackToDownload.isEmpty()) {
              iconDownloading = true;
              themeDownloading = true;

              params_memory.put("ThemeDownloadProgress", "Downloading...");

              downloadThemeAsset(iconPackToDownload, "IconToDownload", "DownloadableIcons", params, params_memory);

              downloadStatusLabel->setText("Downloading...");
            }
          }
        } else if (id == 2) {
          iconPacks.append("Stock");
          iconPacks.append(getHolidayThemes());
          iconPacks.sort();

          QString iconPackToSelect = MultiOptionDialog::getSelection(tr("選擇圖示套件"), iconPacks, getThemeName("CustomIcons", params), this);
          if (!iconPackToSelect.isEmpty()) {
            manageCustomIconsButton->setValue(storeThemeName(iconPackToSelect, "CustomIcons", params));
          }
        }
      });
      manageCustomIconsButton->setValue(getThemeName(param.toStdString(), params));
      themeToggle = manageCustomIconsButton;
    } else if (param == "CustomSignals") {
      manageCustomSignalsButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("下載"), tr("選擇")});
      QObject::connect(manageCustomSignalsButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList signalAnimations = getThemeList(randomThemes, QDir(themePacksDirectory.path()), "signals", "CustomSignals", params);

        if (id == 0) {
          QString signalAnimationToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的方向燈動畫"), signalAnimations, "", this);
          if (!signalAnimationToDelete.isEmpty() && ConfirmationDialog::confirm(tr("刪除「%1」方向燈動畫？").arg(signalAnimationToDelete), tr("刪除"), this)) {
            signalsDownloaded = false;

            deleteThemeAsset(themePacksDirectory, "signals", "DownloadableSignals", signalAnimationToDelete, params);
          }
        } else if (id == 1) {
          if (signalDownloading) {
            cancellingDownload = true;

            params_memory.putBool("CancelThemeDownload", true);

            QTimer::singleShot(2500, [this]() {
              cancellingDownload = false;
              signalDownloading = false;
              themeDownloading = false;

              params_memory.putBool("CancelThemeDownload", false);
            });
          } else {
            QStringList downloadableSignalAnimations = QString::fromStdString(params.get("DownloadableSignals")).split(",");
            signalAnimationToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的方向燈動畫"), downloadableSignalAnimations, "", this);
            if (!signalAnimationToDownload.isEmpty()) {
              signalDownloading = true;
              themeDownloading = true;

              params_memory.put("ThemeDownloadProgress", "Downloading...");

              downloadThemeAsset(signalAnimationToDownload, "SignalToDownload", "DownloadableSignals", params, params_memory);

              downloadStatusLabel->setText("Downloading...");
            }
          }
        } else if (id == 2) {
          signalAnimations.append("None");
          signalAnimations.append(getHolidayThemes());
          signalAnimations.sort();

          QString signalAnimationToSelect = MultiOptionDialog::getSelection(tr("選擇方向燈動畫"), signalAnimations, getThemeName("CustomSignals", params), this);
          if (!signalAnimationToSelect.isEmpty()) {
            manageCustomSignalsButton->setValue(storeThemeName(signalAnimationToSelect, "CustomSignals", params));
          }
        }
      });
      manageCustomSignalsButton->setValue(getThemeName(param.toStdString(), params));
      themeToggle = manageCustomSignalsButton;
    } else if (param == "CustomSounds") {
      manageCustomSoundsButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("下載"), tr("選擇")});
      QObject::connect(manageCustomSoundsButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList soundPacks = getThemeList(randomThemes, QDir(themePacksDirectory.path()), "sounds", "CustomSounds", params);

        if (id == 0) {
          QString soundPackToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的音效套件"), soundPacks, "", this);
          if (!soundPackToDelete.isEmpty() && ConfirmationDialog::confirm(tr("刪除「%1」音效套件？").arg(soundPackToDelete), tr("刪除"), this)) {
            soundsDownloaded = false;

            deleteThemeAsset(themePacksDirectory, "sounds", "DownloadableSounds", soundPackToDelete, params);
          }
        } else if (id == 1) {
          if (soundDownloading) {
            cancellingDownload = true;

            params_memory.putBool("CancelThemeDownload", true);

            QTimer::singleShot(2500, [this]() {
              cancellingDownload = false;
              soundDownloading = false;
              themeDownloading = false;

              params_memory.putBool("CancelThemeDownload", false);
            });
          } else {
            QStringList downloadableSoundPacks = QString::fromStdString(params.get("DownloadableSounds")).split(",");
            soundPackToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的音效套件"), downloadableSoundPacks, "", this);
            if (!soundPackToDownload.isEmpty()) {
              soundDownloading = true;
              themeDownloading = true;

              params_memory.put("ThemeDownloadProgress", "Downloading...");

              downloadThemeAsset(soundPackToDownload, "SoundToDownload", "DownloadableSounds", params, params_memory);

              downloadStatusLabel->setText("Downloading...");
            }
          }
        } else if (id == 2) {
          soundPacks.append("Stock");
          soundPacks.append(getHolidayThemes());
          soundPacks.sort();

          QString soundPackToSelect = MultiOptionDialog::getSelection(tr("選擇音效套件"), soundPacks, getThemeName("CustomSounds", params), this);
          if (!soundPackToSelect.isEmpty()) {
            manageCustomSoundsButton->setValue(storeThemeName(soundPackToSelect, "CustomSounds", params));
          }
        }
      });
      manageCustomSoundsButton->setValue(getThemeName(param.toStdString(), params));
      themeToggle = manageCustomSoundsButton;
    } else if (param == "WheelIcon") {
      manageWheelIconsButton = new FrogPilotButtonsControl(title, desc, icon, {tr("刪除"), tr("下載"), tr("選擇")});
      QObject::connect(manageWheelIconsButton, &FrogPilotButtonsControl::buttonClicked, [this](int id) {
        QStringList wheelIcons = getThemeList(randomThemes, QDir(wheelsDirectory.path()), "", "WheelIcon", params);

        if (id == 0) {
          QString wheelIconToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的方向盤圖示"), wheelIcons, "", this);
          if (!wheelIconToDelete.isEmpty() && ConfirmationDialog::confirm(tr("刪除「%1」方向盤圖示？").arg(wheelIconToDelete), tr("刪除"), this)) {
            wheelsDownloaded = false;

            deleteThemeAsset(wheelsDirectory, "", "DownloadableWheels", wheelIconToDelete, params);
          }
        } else if (id == 1) {
          if (wheelDownloading) {
            cancellingDownload = true;

            params_memory.putBool("CancelThemeDownload", true);

            QTimer::singleShot(2500, [this]() {
              cancellingDownload = false;
              wheelDownloading = false;
              themeDownloading = false;

              params_memory.putBool("CancelThemeDownload", false);
            });
          } else {
            QStringList downloadableWheels = QString::fromStdString(params.get("DownloadableWheels")).split(",");
            wheelToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的方向盤圖示"), downloadableWheels, "", this);
            if (!wheelToDownload.isEmpty()) {
              wheelDownloading = true;
              themeDownloading = true;

              params_memory.put("ThemeDownloadProgress", "Downloading...");

              downloadThemeAsset(wheelToDownload, "WheelToDownload", "DownloadableWheels", params, params_memory);

              downloadStatusLabel->setText("Downloading...");
            }
          }
        } else if (id == 2) {
          wheelIcons.append("None");
          wheelIcons.append("Stock");
          wheelIcons.append(getHolidayThemes());
          wheelIcons.sort();

          QString steeringWheelToSelect = MultiOptionDialog::getSelection(tr("選擇方向盤圖示"), wheelIcons, getThemeName("WheelIcon", params), this);
          if (!steeringWheelToSelect.isEmpty()) {
            manageWheelIconsButton->setValue(storeThemeName(steeringWheelToSelect, "WheelIcon", params));
          }
        }
      });
      manageWheelIconsButton->setValue(getThemeName(param.toStdString(), params));
      themeToggle = manageWheelIconsButton;
    } else if (param == "DownloadStatusLabel") {
      downloadStatusLabel = new LabelControl(title, "Idle");
      themeToggle = downloadStatusLabel;
    } else if (param == "StartupAlert") {
      FrogPilotButtonsControl *startupAlertButton = new FrogPilotButtonsControl(title, desc, icon, {tr("原廠"), tr("FROGPILOT"), tr("自訂"), tr("清除")}, true);

      QString currentTop = QString::fromStdString(params.get("StartupMessageTop"));
      QString currentBottom = QString::fromStdString(params.get("StartupMessageBottom"));

      QString stockTop = "Be ready to take over at any time";
      QString stockBottom = "Always keep hands on wheel and eyes on road";

      QString frogpilotTop = "Hop in and buckle up!";
      QString frogpilotBottom = "Human-tested, frog-approved 🐸";

      if (currentTop == stockTop && currentBottom == stockBottom) {
        startupAlertButton->setCheckedButton(0);
      } else if (currentTop == frogpilotTop && currentBottom == frogpilotBottom) {
        startupAlertButton->setCheckedButton(1);
      } else if (!currentTop.isEmpty() || !currentBottom.isEmpty()) {
        startupAlertButton->setCheckedButton(2);
      }

      QObject::connect(startupAlertButton, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        int maxLengthTop = 35;
        int maxLengthBottom = 45;

        if (id == 0) {
          params.put("StartupMessageTop", stockTop.toStdString());
          params.put("StartupMessageBottom", stockBottom.toStdString());
        } else if (id == 1) {
          params.put("StartupMessageTop", frogpilotTop.toStdString());
          params.put("StartupMessageBottom", frogpilotBottom.toStdString());
        } else if (id == 2) {
          QString newTop = InputDialog::getText(tr("輸入上半段文字"), this, tr("字元數：0/%1").arg(maxLengthTop), false, -1, currentTop, maxLengthTop).trimmed();
          if (!newTop.isEmpty()) {
            params.put("StartupMessageTop", newTop.toStdString());
            QString newBottom = InputDialog::getText(tr("輸入下半段文字"), this, tr("字元數：0/%1").arg(maxLengthBottom), false, -1, currentBottom, maxLengthBottom).trimmed();
            if (!newBottom.isEmpty()) {
              params.put("StartupMessageBottom", newBottom.toStdString());
            }
          }
        } else if (id == 3) {
          if (FrogPilotConfirmationDialog::yesorno(tr("確定要完全重設啟動訊息嗎？"), this)) {
            params.remove("StartupMessageTop");
            params.remove("StartupMessageBottom");

            startupAlertButton->clearCheckedButtons(true);
          }
        }
      });
      themeToggle = startupAlertButton;

    } else {
      themeToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = themeToggle;

    if (customThemeKeys.contains(param)) {
      customThemesList->addItem(themeToggle);
    } else {
      themesList->addItem(themeToggle);

      if (param == "PersonalizeOpenpilot") {
        parentKeys.insert(param);
      }
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(themeToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, [this]() {
        emit openSubPanel();
        openDescriptions(forceOpenDescriptions, toggles);
      });
    }

    QObject::connect(themeToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(themeToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  openDescriptions(forceOpenDescriptions, toggles);

  QObject::connect(static_cast<ToggleControl *>(toggles["PersonalizeOpenpilot"]), &ToggleControl::toggleFlipped, this, &FrogPilotThemesPanel::updateToggles);
  QObject::connect(static_cast<ToggleControl*>(toggles["RandomThemes"]), &ToggleControl::toggleFlipped, [this](bool state) {
    if (state) {
      ConfirmationDialog::alert(tr("「隨機主題」僅適用於已下載的主題，請先下載您想要使用的主題！"), this);

      manageCustomColorsButton->setValue("");
      manageCustomColorsButton->setVisibleButton(2, false);

      manageCustomIconsButton->setValue("");
      manageCustomIconsButton->setVisibleButton(2, false);

      manageCustomSignalsButton->setValue("");
      manageCustomSignalsButton->setVisibleButton(2, false);

      manageCustomSoundsButton->setValue("");
      manageCustomSoundsButton->setVisibleButton(2, false);

      manageDistanceIconsButton->setValue("");
      manageDistanceIconsButton->setVisibleButton(2, false);

      manageWheelIconsButton->setValue("");
      manageWheelIconsButton->setVisibleButton(2, false);
    } else {
      manageCustomColorsButton->setValue(getThemeName("CustomColors", params));
      manageCustomColorsButton->setVisibleButton(2, true);

      manageCustomIconsButton->setValue(getThemeName("CustomIcons", params));
      manageCustomIconsButton->setVisibleButton(2, true);

      manageCustomSignalsButton->setValue(getThemeName("CustomSignals", params));
      manageCustomSignalsButton->setVisibleButton(2, true);

      manageCustomSoundsButton->setValue(getThemeName("CustomSounds", params));
      manageCustomSoundsButton->setVisibleButton(2, true);

      manageDistanceIconsButton->setValue(getThemeName("CustomDistanceIcons", params));
      manageDistanceIconsButton->setVisibleButton(2, true);

      manageWheelIconsButton->setValue(getThemeName("WheelIcon", params));
      manageWheelIconsButton->setVisibleButton(2, true);
    }

    randomThemes = state;
  });

  QObject::connect(parent, &FrogPilotSettingsWindow::closeSubPanel, [themesLayout, themesPanel, this] {
    openDescriptions(forceOpenDescriptions, toggles);
    themesLayout->setCurrentWidget(themesPanel);
  });
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotThemesPanel::updateState);
}

void FrogPilotThemesPanel::showEvent(QShowEvent *event) {
  colorsDownloaded = params.get("DownloadableColors").empty();
  distanceIconsDownloaded = params.get("DownloadableDistanceIcons").empty();
  iconsDownloaded = params.get("DownloadableIcons").empty();
  signalsDownloaded = params.get("DownloadableSignals").empty();
  soundsDownloaded = params.get("DownloadableSounds").empty();
  wheelsDownloaded = params.get("DownloadableWheels").empty();

  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  if (params.getBool("RandomThemes")) {
    manageCustomColorsButton->setValue("");
    manageCustomColorsButton->setVisibleButton(2, false);

    manageCustomIconsButton->setValue("");
    manageCustomIconsButton->setVisibleButton(2, false);

    manageCustomSignalsButton->setValue("");
    manageCustomSignalsButton->setVisibleButton(2, false);

    manageCustomSoundsButton->setValue("");
    manageCustomSoundsButton->setVisibleButton(2, false);

    manageDistanceIconsButton->setValue("");
    manageDistanceIconsButton->setVisibleButton(2, false);

    manageWheelIconsButton->setValue("");
    manageWheelIconsButton->setVisibleButton(2, false);

    randomThemes = true;
  }

  updateToggles();
}

void FrogPilotThemesPanel::updateState(const UIState &s, const FrogPilotUIState &fs) {
  if (!isVisible() || finalizingDownload) {
    return;
  }

  if (themeDownloading) {
    QString progress = QString::fromStdString(params_memory.get("ThemeDownloadProgress"));
    bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|failed|offline", QRegularExpression::CaseInsensitiveOption));

    if (progress != "Downloading...") {
      downloadStatusLabel->setText(progress);
    }

    if (progress == "Downloaded!" || downloadFailed) {
      finalizingDownload = true;

      QTimer::singleShot(2500, [this]() {
        colorDownloading = false;
        distanceIconDownloading = false;
        finalizingDownload = false;
        iconDownloading = false;
        signalDownloading = false;
        soundDownloading = false;
        themeDownloading = false;
        wheelDownloading = false;

        colorsDownloaded = params.get("DownloadableColors").empty();
        distanceIconsDownloaded = params.get("DownloadableDistanceIcons").empty();
        iconsDownloaded = params.get("DownloadableIcons").empty();
        signalsDownloaded = params.get("DownloadableSignals").empty();
        soundsDownloaded = params.get("DownloadableSounds").empty();
        wheelsDownloaded = params.get("DownloadableWheels").empty();

        params_memory.remove("CancelThemeDownload");
        params_memory.remove("ThemeDownloadProgress");

        downloadStatusLabel->setText("Idle");
      });
    }
  }

  bool parked = !s.scene.started || fs.frogpilot_scene.parked || fs.frogpilot_toggles.value("frogs_go_moo").toBool();

  manageCustomColorsButton->setText(1, colorDownloading ? tr("CANCEL") : tr("DOWNLOAD"));
  manageCustomColorsButton->setEnabledButtons(0, !themeDownloading);
  manageCustomColorsButton->setEnabledButtons(1, fs.frogpilot_scene.online && (!themeDownloading || colorDownloading) && !cancellingDownload && !finalizingDownload && !colorsDownloaded && parked);
  manageCustomColorsButton->setEnabledButtons(2, !themeDownloading);

  manageCustomIconsButton->setText(1, iconDownloading ? tr("CANCEL") : tr("DOWNLOAD"));
  manageCustomIconsButton->setEnabledButtons(0, !themeDownloading);
  manageCustomIconsButton->setEnabledButtons(1, fs.frogpilot_scene.online && (!themeDownloading || iconDownloading) && !cancellingDownload && !finalizingDownload && !iconsDownloaded && parked);
  manageCustomIconsButton->setEnabledButtons(2, !themeDownloading);

  manageCustomSignalsButton->setText(1, signalDownloading ? tr("CANCEL") : tr("DOWNLOAD"));
  manageCustomSignalsButton->setEnabledButtons(0, !themeDownloading);
  manageCustomSignalsButton->setEnabledButtons(1, fs.frogpilot_scene.online && (!themeDownloading || signalDownloading) && !cancellingDownload && !finalizingDownload && !signalsDownloaded && parked);
  manageCustomSignalsButton->setEnabledButtons(2, !themeDownloading);

  manageCustomSoundsButton->setText(1, soundDownloading ? tr("CANCEL") : tr("DOWNLOAD"));
  manageCustomSoundsButton->setEnabledButtons(0, !themeDownloading);
  manageCustomSoundsButton->setEnabledButtons(1, fs.frogpilot_scene.online && (!themeDownloading || soundDownloading) && !cancellingDownload && !finalizingDownload && !soundsDownloaded && parked);
  manageCustomSoundsButton->setEnabledButtons(2, !themeDownloading);

  manageDistanceIconsButton->setText(1, distanceIconDownloading ? tr("CANCEL") : tr("DOWNLOAD"));
  manageDistanceIconsButton->setEnabledButtons(0, !themeDownloading);
  manageDistanceIconsButton->setEnabledButtons(1, fs.frogpilot_scene.online && (!themeDownloading || distanceIconDownloading) && !cancellingDownload && !finalizingDownload && !distanceIconsDownloaded && parked);
  manageDistanceIconsButton->setEnabledButtons(2, !themeDownloading);

  manageWheelIconsButton->setText(1, wheelDownloading ? tr("CANCEL") : tr("DOWNLOAD"));
  manageWheelIconsButton->setEnabledButtons(0, !themeDownloading);
  manageWheelIconsButton->setEnabledButtons(1, fs.frogpilot_scene.online && (!themeDownloading || wheelDownloading) && !cancellingDownload && !finalizingDownload && !wheelsDownloaded && parked);
  manageWheelIconsButton->setEnabledButtons(2, !themeDownloading);

  parent->keepScreenOn = themeDownloading;
}

void FrogPilotThemesPanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      toggle->setVisible(false);
    }
  }

  for (auto &[key, toggle] : toggles) {
    if (parentKeys.contains(key)) {
      continue;
    }

    bool setVisible = parent->tuningLevel >= frogpilotToggleLevels[key].toDouble();

    if (key == "CustomDistanceIcons") {
      setVisible &= params.getBool("QOLVisuals") && params.getBool("OnroadDistanceButton");
    }

    else if (key == "RandomThemes") {
      setVisible &= params.getBool("PersonalizeOpenpilot");
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (customThemeKeys.contains(key)) {
        toggles["PersonalizeOpenpilot"]->setVisible(true);
      }
    }
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
