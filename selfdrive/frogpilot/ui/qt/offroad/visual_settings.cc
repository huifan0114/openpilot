#include "selfdrive/frogpilot/ui/qt/offroad/visual_settings.h"

FrogPilotVisualsPanel::FrogPilotVisualsPanel(SettingsWindow *parent) : FrogPilotListWidget(parent) {
  std::string branch = params.get("GitBranch");
  isRelease = branch == "FrogPilot";

  const std::vector<std::tuple<QString, QString, QString, QString>> visualToggles {
    {"AlertVolumeControl", tr("警報音量控制"), tr("控制 openpilot 中每個聲音的音量級別."), "../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"DisengageVolume", tr("  失效音量"), tr("相關提醒:\n\n巡航停用\n手煞車\n煞車\n速度太低"), ""},
    {"EngageVolume", tr("  啟用音量"), tr("相關提醒:\n\nNNFF 扭矩控制器已加載\nopenpilot 啟用"), ""},
    {"PromptVolume", tr("  提示音量"), tr("相關提醒:\n\n盲點偵測到汽車\n綠燈提醒\n速度太低\n轉向低於“X”時不可用\n控制，轉彎超出轉向極限"), ""},
    {"PromptDistractedVolume", tr("  注意力分散的音量"), tr("相關提醒:\n\n請注意，司機分心\n觸摸方向盤，駕駛反應遲鈍"), ""},
    {"RefuseVolume", tr("  拒絕音量"), tr("相關提醒:\n\nopenpilot 不可用"), ""},
    {"WarningSoftVolume", tr("  警告音量"), tr("相關提醒:\n\n煞車！，碰撞危險\n立即控制"), ""},
    {"WarningImmediateVolume", tr("  警告即時音量"), tr("相關提醒:\n\n立即脫離，駕駛分心\n立即脫離，駕駛員沒有反應"), ""},

    {"BonusContent", tr("額外選項"), tr("FrogPilot 額外功能讓 openpilot 變得更有趣!"), "../frogpilot/assets/toggle_icons/frog.png"},
    {"GoatScream", tr("山羊尖叫"), tr("啟用著名的“山羊尖叫”，它給世界各地的 FrogPilot 用戶帶來了歡樂和憤怒!"), ""},
    {"HolidayThemes", tr("節日主題"), tr("openpilot 主題會根據當前/即將到來的假期而變化。小假期持續一天，而大假期（復活節、聖誕節、萬聖節等）持續一周."), ""},
    {"PersonalizeOpenpilot", tr("個性化 openpilot"), tr("依照您的個人喜好自訂 openpilot!"), ""},
    {"CustomColors", tr("顏色主題"), tr("以主題顏色取代標準 openpilot 配色方案.\n\n想提交自己的配色方案嗎？將其發佈到 FrogPilot Discord 的「功能請求」頻道中!"), ""},
    {"CustomIcons", tr("圖示包"), tr("用一組主題圖標取代標準 openpilot 圖標.\n\n想要提交自己的圖標包嗎？將其發佈到 FrogPilot Discord 的「功能請求」頻道中!"), ""},
    {"CustomSounds", tr("聲音包"), tr("用一組主題聲音取代標準的 openpilot 聲音.\n\n想提交自己的聲音包嗎？將其發佈到 FrogPilot Discord 的「功能請求」頻道中!"), ""},
    {"CustomSignals", tr("方向燈"), tr("為您的方向燈添加主題動畫.\n\n想要提交您自己的方向燈動畫嗎？將其發佈到 FrogPilot Discord 的「功能請求」頻道中!"), ""},
    {"WheelIcon", tr("方向盤"), tr("將預設方向盤圖示替換為自訂圖標."), ""},
    {"DownloadStatusLabel", tr("下載狀態"), "", ""},
    {"StartupAlert", tr("啟動警報"), tr("自訂上路時顯示的「啟動」警報."), ""},
    {"RandomEvents", tr("隨機事件"), tr("享受在某些駕駛條件下可能發生的隨機事件的一些不可預測性。這純粹是裝飾性的，對駕駛控制沒有影響!"), ""},

    {"CustomAlerts", tr("自訂警報"), tr("針對各種邏輯或情況變化啟用自訂警報."), "../frogpilot/assets/toggle_icons/icon_green_light.png"},
    {"GreenLightAlert", tr("  綠燈提醒"), tr("當交通燈由紅變綠時收到警報."), ""},
    {"LeadDepartingAlert", tr("  前車遠離警告"), tr("當您處於靜止狀態時前方車輛開始出發時收到警報."), ""},
    {"LoudBlindspotAlert", tr("  大聲盲點警報"), tr("當嘗試變換車道時在盲點偵測到車輛時，啟用更響亮的警報."), ""},

    {"CustomUI", tr("自定義道路畫面"), tr("定義自己喜歡的道路介面."), "../assets/offroad/icon_road.png"},
    {"Compass", tr("  羅盤"), tr("將指南針加入道路使用者介面."), ""},
    {"CustomPaths", tr("  路徑"), tr("顯示您在行駛路徑上的預期加速度、偵測到的相鄰車道或在盲點中偵測到車輛時的加速度."), ""},
    {"PedalsOnUI", tr("  踏板"), tr("在方向盤圖示下方的道路使用者介面上顯示煞車踏板和油門踏板."), ""},
    {"RoadNameUI", tr("  道路名稱"), tr("在螢幕底部顯示目前道路的名稱。來源自 OpenStreetMap."), ""},
    {"RotatingWheel", tr("  旋轉方向盤"), tr("在道路使用者介面中沿著實體方向盤旋轉方向盤."), ""},
    {"ShowStoppingPoint", tr("  停止點"), tr("顯示 openpilot 因紅燈或停車標誌而停止的位子."), ""},

    {"DeveloperUI", tr("開發者介面"), tr("獲取 openpilot 在幕後所做的各種詳細信息."), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"BorderMetrics", tr("  邊界指標"), tr("在道路 UI 邊框中顯示指標."), ""},
    {"FPSCounter", tr("  FPS 計數器"), tr("顯示道路 UI 的「每秒幀數」(FPS)，以監控系統效能."), ""},
    {"LateralMetrics", tr("  橫向指標"), tr("顯示與 openpilot 橫向性能相關的各種指標."), ""},
    {"LongitudinalMetrics", tr("  縱向指標"), tr("顯示與 openpilot 縱向效能相關的各種指標."), ""},
    {"NumericalTemp", tr("  數位溫度計"), tr("將「GOOD」、「OK」和「HIGH」溫度狀態替換為基於記憶體、CPU 和 GPU 之間最高溫度的數位溫度計."), ""},
    {"SidebarMetrics", tr("  側邊欄"), tr("在側邊欄上顯示 CPU、GPU、RAM、IP 和已使用/剩餘儲存的各種自訂指標."), ""},
    {"UseSI", tr("  使用國際單位制"), tr("以 SI 格式顯示相關指標."), ""},

    {"ModelUI", tr("路徑外觀"), tr("個性化模型的可視化在螢幕上的顯示方式."), "../assets/offroad/icon_calibration.png"},
    {"DynamicPathWidth", tr("  動態路徑寬度"), tr("根據 openpilot 目前的接合狀態動態調整路徑寬度."), ""},
    {"HideLeadMarker", tr("  隱藏引導標記"), tr("從道路 UI 中隱藏領先標記."), ""},
    {"LaneLinesWidth", tr("  車道寬"), tr("調整顯示器上車道線的視覺粗細.\n\n預設值為 MUTCD 平均車道線寬度 4 英寸."), ""},
    {"PathEdgeWidth", tr("  路徑邊寬"), tr("自定義顯示當前駕駛狀態的路徑邊緣寬度。預設為總路徑的 20%.\n\n藍色 = 導航\n淺藍色=“全時置中”\n綠色 = 預設\n橙色=“實驗模式”\n紅色=“塞車模式”\n黃色=“條件實驗模式”被覆蓋"), ""},
    {"PathWidth", tr("  路徑寬"), tr("自定義路徑寬度。\n\n預設為 skoda kodiaq 的寬度."), ""},
    {"RoadEdgesWidth", tr("  道路邊寬"), tr("自定義道路邊緣寬度。\n\n預設值為 MUTCD 平均車道線寬度 4 英寸."), ""},
    {"UnlimitedLength", tr("  無限道路"), tr("將路徑、車道線和道路邊緣的顯示器擴展到系統可以偵測的範圍內，提供更廣闊的前方道路視野."), ""},

    {"QOLVisuals", tr("優化體驗"), tr("各種控制細項的調整可改善您的openpilot體驗."), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"BigMap", tr("  大地圖"), tr("增加道路使用者介面中地圖的大小."), ""},
    {"CameraView", tr("  相機視圖"), tr("為道路使用者介面選擇您喜歡的攝影機視圖。這純粹是視覺上的變化，不會影響 openpilot 的駕駛方式."), ""},
    {"DriverCamera", tr("  倒車駕駛員攝影機"), tr("倒車時顯示駕駛攝影機畫面."), ""},
    {"HideSpeed", tr("  隱藏速度"), tr("隱藏道路使用者介面中的速度指示器。附加切換允許透過點擊速度本身來隱藏/顯示."), ""},
    {"MapStyle", tr("  地圖樣式"), tr("選擇用於導航的地圖樣式."), ""},
    {"StoppedTimer", tr("  停止計時器"), tr("在道路使用者介面中顯示計時器，指示您已停車的時間."), ""},
    {"WheelSpeed", tr("  使用輪速"), tr("在道路使用者介面中使用車輪速度而不是人工速度."), ""},

    {"ScreenManagement", tr("螢幕管理"), tr("管理螢幕亮度、超時設定並隱藏道路 UI 元素."), "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"HideUIElements", tr("  隱藏使用者介面元素"), tr("在道路畫面上隱藏選定的 UI 元素."), ""},
    {"ScreenBrightness", tr("  螢幕亮度"), tr("自訂停止時的螢幕亮度."), ""},
    {"ScreenBrightnessOnroad", tr("  螢幕亮度 (行進間)"), tr("自訂行進時的螢幕亮度."), ""},
    {"ScreenRecorder", tr("  螢幕錄影機"), tr("啟用在路上錄製螢幕的功能."), ""},
    {"ScreenTimeout", tr("  螢幕待機"), tr("自訂螢幕關閉所需的時間."), ""},
    {"ScreenTimeoutOnroad", tr("  螢幕待機 (行進間)"), tr("自訂行進間螢幕關閉所需的時間."), ""},
    {"StandbyMode", tr("  待機模式"), tr("在路上螢幕超時後關閉螢幕，但在參與狀態變更或觸發重要警報時將其喚醒."), ""},
    {"GooffScreen", tr("  啟動關閉螢幕"), tr("車輛起動後自動關閉螢幕，熄火後恢復螢幕."), ""},
  };

  for (const auto &[param, title, desc, icon] : visualToggles) {
    AbstractControl *visualToggle;

    if (param == "AlertVolumeControl") {
      FrogPilotParamManageControl *alertVolumeControlToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(alertVolumeControlToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(alertVolumeControlKeys.find(key.c_str()) != alertVolumeControlKeys.end());
        }
      });
      visualToggle = alertVolumeControlToggle;
    } else if (alertVolumeControlKeys.find(param) != alertVolumeControlKeys.end()) {
      if (param == "WarningImmediateVolume") {
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 25, 100, std::map<int, QString>(), this, false, "%");
      } else {
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, "%");
      }

    } else if (param == "BonusContent") {
      FrogPilotParamManageControl *BonusContentToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(BonusContentToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(bonusContentKeys.find(key.c_str()) != bonusContentKeys.end());
        }
      });
      visualToggle = BonusContentToggle;
    } else if (param == "PersonalizeOpenpilot") {
      FrogPilotParamManageControl *personalizeOpenpilotToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(personalizeOpenpilotToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openSubParentToggle();

        personalizeOpenpilotOpen = true;
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(personalizeOpenpilotKeys.find(key.c_str()) != personalizeOpenpilotKeys.end());
        }
      });
      visualToggle = personalizeOpenpilotToggle;
    } else if (param == "CustomColors") {
      manageCustomColorsBtn = new FrogPilotButtonsControl(title, {tr("刪除"), tr("下載"), tr("選擇")}, desc);

      std::function<QString(QString)> formatColorName = [](QString name) -> QString {
        QChar separator = name.contains('_') ? '_' : '-';
        QStringList parts = name.replace(separator, ' ').split(' ');

        for (int i = 0; i < parts.size(); ++i) {
          parts[i][0] = parts[i][0].toUpper();
        }

        if (separator == '-' && parts.size() > 1) {
          return parts.first() + " (" + parts.last() + ")";
        }

        return parts.join(' ');
      };

      std::function<QString(QString)> formatColorNameForStorage = [](QString name) -> QString {
        name = name.toLower();
        name = name.replace(" (", "-");
        name = name.replace(' ', '_');
        name.remove('(').remove(')');
        return name;
      };

      QObject::connect(manageCustomColorsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QDir themesDir{"/data/themes/theme_packs"};
        QFileInfoList dirList = themesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

        QString currentColor = QString::fromStdString(params.get("CustomColors")).replace('_', ' ').replace('-', " (").toLower();
        currentColor[0] = currentColor[0].toUpper();
        for (int i = 1; i < currentColor.length(); i++) {
          if (currentColor[i - 1] == ' ' || currentColor[i - 1] == '(') {
            currentColor[i] = currentColor[i].toUpper();
          }
        }
        if (currentColor.contains(" (")) {
          currentColor.append(')');
        }

        QStringList availableColors;
        for (const QFileInfo &dirInfo : dirList) {
          QString colorSchemeDir = dirInfo.absoluteFilePath();

          if (QDir(colorSchemeDir + "/colors").exists()) {
            availableColors << formatColorName(dirInfo.fileName());
          }
        }
        availableColors.append("Stock");
        std::sort(availableColors.begin(), availableColors.end());

        if (id == 0) {
          QStringList colorSchemesList = availableColors;
          colorSchemesList.removeAll("Stock");
          colorSchemesList.removeAll(currentColor);

          QString colorSchemeToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的配色方案"), colorSchemesList, "", this);
          if (!colorSchemeToDelete.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 配色方案?").arg(colorSchemeToDelete), tr("刪除"), this)) {
            themeDeleting = true;
            colorsDownloaded = false;

            QString selectedColor = formatColorNameForStorage(colorSchemeToDelete);
            for (const QFileInfo &dirInfo : dirList) {
              if (dirInfo.fileName() == selectedColor) {
                QDir colorDir(dirInfo.absoluteFilePath() + "/colors");
                if (colorDir.exists()) {
                  colorDir.removeRecursively();
                }
              }
            }

            QStringList downloadableColors = QString::fromStdString(params.get("DownloadableColors")).split(",");
            downloadableColors << colorSchemeToDelete;
            downloadableColors.removeDuplicates();
            downloadableColors.removeAll("");
            std::sort(downloadableColors.begin(), downloadableColors.end());

            params.put("DownloadableColors", downloadableColors.join(",").toStdString());
            themeDeleting = false;
          }
        } else if (id == 1) {
          if (colorDownloading) {
            paramsMemory.putBool("CancelThemeDownload", true);
            cancellingDownload = true;

            QTimer::singleShot(2000, [=]() {
              paramsMemory.putBool("CancelThemeDownload", false);
              cancellingDownload = false;
              colorDownloading = false;
              themeDownloading = false;
            });
          } else {
            QStringList downloadableColors = QString::fromStdString(params.get("DownloadableColors")).split(",");
            QString colorSchemeToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的配色方案"), downloadableColors, "", this);

            if (!colorSchemeToDownload.isEmpty()) {
              QString convertedColorScheme = formatColorNameForStorage(colorSchemeToDownload);
              paramsMemory.put("ColorToDownload", convertedColorScheme.toStdString());
              downloadStatusLabel->setText("Downloading...");
              paramsMemory.put("ThemeDownloadProgress", "Downloading...");
              colorDownloading = true;
              themeDownloading = true;

              downloadableColors.removeAll(colorSchemeToDownload);
              params.put("DownloadableColors", downloadableColors.join(",").toStdString());
            }
          }
        } else if (id == 2) {
          QString colorSchemeToSelect = MultiOptionDialog::getSelection(tr("選擇配色方案"), availableColors, currentColor, this);
          if (!colorSchemeToSelect.isEmpty()) {
            params.put("CustomColors", formatColorNameForStorage(colorSchemeToSelect).toStdString());
            manageCustomColorsBtn->setValue(colorSchemeToSelect);
            paramsMemory.putBool("UpdateTheme", true);
          }
        }
      });

      QString currentColor = QString::fromStdString(params.get("CustomColors")).replace('_', ' ').replace('-', " (").toLower();
      currentColor[0] = currentColor[0].toUpper();
      for (int i = 1; i < currentColor.length(); i++) {
        if (currentColor[i - 1] == ' ' || currentColor[i - 1] == '(') {
          currentColor[i] = currentColor[i].toUpper();
        }
      }
      if (currentColor.contains(" (")) {
        currentColor.append(')');
      }
      manageCustomColorsBtn->setValue(currentColor);
      visualToggle = reinterpret_cast<AbstractControl*>(manageCustomColorsBtn);
    } else if (param == "CustomIcons") {
      manageCustomIconsBtn = new FrogPilotButtonsControl(title, {tr("刪除"), tr("下載"), tr("選擇")}, desc);

      std::function<QString(QString)> formatIconName = [](QString name) -> QString {
        QChar separator = name.contains('_') ? '_' : '-';
        QStringList parts = name.replace(separator, ' ').split(' ');

        for (int i = 0; i < parts.size(); ++i) {
          parts[i][0] = parts[i][0].toUpper();
        }

        if (separator == '-' && parts.size() > 1) {
          return parts.first() + " (" + parts.last() + ")";
        }

        return parts.join(' ');
      };

      std::function<QString(QString)> formatIconNameForStorage = [](QString name) -> QString {
        name = name.toLower();
        name = name.replace(" (", "-");
        name = name.replace(' ', '_');
        name.remove('(').remove(')');
        return name;
      };

      QObject::connect(manageCustomIconsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QDir themesDir{"/data/themes/theme_packs"};
        QFileInfoList dirList = themesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

        QString currentIcon = QString::fromStdString(params.get("CustomIcons")).replace('_', ' ').replace('-', " (").toLower();
        currentIcon[0] = currentIcon[0].toUpper();
        for (int i = 1; i < currentIcon.length(); i++) {
          if (currentIcon[i - 1] == ' ' || currentIcon[i - 1] == '(') {
            currentIcon[i] = currentIcon[i].toUpper();
          }
        }
        if (currentIcon.contains(" (")) {
          currentIcon.append(')');
        }

        QStringList availableIcons;
        for (const QFileInfo &dirInfo : dirList) {
          QString iconDir = dirInfo.absoluteFilePath();
          if (QDir(iconDir + "/icons").exists()) {
            availableIcons << formatIconName(dirInfo.fileName());
          }
        }
        availableIcons.append("Stock");
        std::sort(availableIcons.begin(), availableIcons.end());

        if (id == 0) {
          QStringList customIconsList = availableIcons;
          customIconsList.removeAll("Stock");
          customIconsList.removeAll(currentIcon);

          QString iconPackToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的圖示包"), customIconsList, "", this);
          if (!iconPackToDelete.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 圖示包?").arg(iconPackToDelete), tr("刪除"), this)) {
            themeDeleting = true;
            iconsDownloaded = false;

            QString selectedIcon = formatIconNameForStorage(iconPackToDelete);
            for (const QFileInfo &dirInfo : dirList) {
              if (dirInfo.fileName() == selectedIcon) {
                QDir iconDir(dirInfo.absoluteFilePath() + "/icons");
                if (iconDir.exists()) {
                  iconDir.removeRecursively();
                }
              }
            }

            QStringList downloadableIcons = QString::fromStdString(params.get("DownloadableIcons")).split(",");
            downloadableIcons << iconPackToDelete;
            downloadableIcons.removeDuplicates();
            downloadableIcons.removeAll("");
            std::sort(downloadableIcons.begin(), downloadableIcons.end());

            params.put("DownloadableIcons", downloadableIcons.join(",").toStdString());
            themeDeleting = false;
          }
        } else if (id == 1) {
          if (iconDownloading) {
            paramsMemory.putBool("CancelThemeDownload", true);
            cancellingDownload = true;

            QTimer::singleShot(2000, [=]() {
              paramsMemory.putBool("CancelThemeDownload", false);
              cancellingDownload = false;
              iconDownloading = false;
              themeDownloading = false;
            });
          } else {
            QStringList downloadableIcons = QString::fromStdString(params.get("DownloadableIcons")).split(",");
            QString iconPackToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的圖示包"), downloadableIcons, "", this);

            if (!iconPackToDownload.isEmpty()) {
              QString convertedIconPack = formatIconNameForStorage(iconPackToDownload);
              paramsMemory.put("IconToDownload", convertedIconPack.toStdString());
              downloadStatusLabel->setText("Downloading...");
              paramsMemory.put("ThemeDownloadProgress", "Downloading...");
              iconDownloading = true;
              themeDownloading = true;

              downloadableIcons.removeAll(iconPackToDownload);
              params.put("DownloadableIcons", downloadableIcons.join(",").toStdString());
            }
          }
        } else if (id == 2) {
          QString iconPackToSelect = MultiOptionDialog::getSelection(tr("選擇一個圖標包"), availableIcons, currentIcon, this);
          if (!iconPackToSelect.isEmpty()) {
            params.put("CustomIcons", formatIconNameForStorage(iconPackToSelect).toStdString());
            manageCustomIconsBtn->setValue(iconPackToSelect);
            paramsMemory.putBool("UpdateTheme", true);
          }
        }
      });

      QString currentIcon = QString::fromStdString(params.get("CustomIcons")).replace('_', ' ').replace('-', " (").toLower();
      currentIcon[0] = currentIcon[0].toUpper();
      for (int i = 1; i < currentIcon.length(); i++) {
        if (currentIcon[i - 1] == ' ' || currentIcon[i - 1] == '(') {
          currentIcon[i] = currentIcon[i].toUpper();
        }
      }
      if (currentIcon.contains(" (")) {
        currentIcon.append(')');
      }
      manageCustomIconsBtn->setValue(currentIcon);
      visualToggle = reinterpret_cast<AbstractControl*>(manageCustomIconsBtn);
    } else if (param == "CustomSignals") {
      manageCustomSignalsBtn = new FrogPilotButtonsControl(title, {tr("刪除"), tr("下載"), tr("選擇")}, desc);

      std::function<QString(QString)> formatSignalName = [](QString name) -> QString {
        QChar separator = name.contains('_') ? '_' : '-';
        QStringList parts = name.replace(separator, ' ').split(' ');

        for (int i = 0; i < parts.size(); ++i) {
          parts[i][0] = parts[i][0].toUpper();
        }

        if (separator == '-' && parts.size() > 1) {
          return parts.first() + " (" + parts.last() + ")";
        }

        return parts.join(' ');
      };

      std::function<QString(QString)> formatSignalNameForStorage = [](QString name) -> QString {
        name = name.toLower();
        name = name.replace(" (", "-");
        name = name.replace(' ', '_');
        name.remove('(').remove(')');
        return name;
      };

      QObject::connect(manageCustomSignalsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QDir themesDir{"/data/themes/theme_packs"};
        QFileInfoList dirList = themesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

        QString currentSignal = QString::fromStdString(params.get("CustomSignals")).replace('_', ' ').replace('-', " (").toLower();
        currentSignal[0] = currentSignal[0].toUpper();
        for (int i = 1; i < currentSignal.length(); i++) {
          if (currentSignal[i - 1] == ' ' || currentSignal[i - 1] == '(') {
            currentSignal[i] = currentSignal[i].toUpper();
          }
        }
        if (currentSignal.contains(" (")) {
          currentSignal.append(')');
        }

        QStringList availableSignals;
        for (const QFileInfo &dirInfo : dirList) {
          QString signalDir = dirInfo.absoluteFilePath();
          if (QDir(signalDir + "/signals").exists()) {
            availableSignals << formatSignalName(dirInfo.fileName());
          }
        }
        availableSignals.append("Stock");
        std::sort(availableSignals.begin(), availableSignals.end());

        if (id == 0) {
          QStringList customSignalsList = availableSignals;
          customSignalsList.removeAll("Stock");
          customSignalsList.removeAll(currentSignal);

          QString signalPackToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的訊號包"), customSignalsList, "", this);
          if (!signalPackToDelete.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 訊號包?").arg(signalPackToDelete), tr("刪除"), this)) {
            themeDeleting = true;
            signalsDownloaded = false;

            QString selectedSignal = formatSignalNameForStorage(signalPackToDelete);
            for (const QFileInfo &dirInfo : dirList) {
              if (dirInfo.fileName() == selectedSignal) {
                QDir signalDir(dirInfo.absoluteFilePath() + "/signals");
                if (signalDir.exists()) {
                  signalDir.removeRecursively();
                }
              }
            }

            QStringList downloadableSignals = QString::fromStdString(params.get("DownloadableSignals")).split(",");
            downloadableSignals << signalPackToDelete;
            downloadableSignals.removeDuplicates();
            downloadableSignals.removeAll("");
            std::sort(downloadableSignals.begin(), downloadableSignals.end());

            params.put("DownloadableSignals", downloadableSignals.join(",").toStdString());
            themeDeleting = false;
          }
        } else if (id == 1) {
          if (signalDownloading) {
            paramsMemory.putBool("CancelThemeDownload", true);
            cancellingDownload = true;

            QTimer::singleShot(2000, [=]() {
              paramsMemory.putBool("CancelThemeDownload", false);
              cancellingDownload = false;
              signalDownloading = false;
              themeDownloading = false;
            });
          } else {
            QStringList downloadableSignals = QString::fromStdString(params.get("DownloadableSignals")).split(",");
            QString signalPackToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的信號包"), downloadableSignals, "", this);

            if (!signalPackToDownload.isEmpty()) {
              QString convertedSignalPack = formatSignalNameForStorage(signalPackToDownload);
              paramsMemory.put("SignalToDownload", convertedSignalPack.toStdString());
              downloadStatusLabel->setText("Downloading...");
              paramsMemory.put("ThemeDownloadProgress", "Downloading...");
              signalDownloading = true;
              themeDownloading = true;

              downloadableSignals.removeAll(signalPackToDownload);
              params.put("DownloadableSignals", downloadableSignals.join(",").toStdString());
            }
          }
        } else if (id == 2) {
          QString signalPackToSelect = MultiOptionDialog::getSelection(tr("選擇訊號包"), availableSignals, currentSignal, this);
          if (!signalPackToSelect.isEmpty()) {
            params.put("CustomSignals", formatSignalNameForStorage(signalPackToSelect).toStdString());
            manageCustomSignalsBtn->setValue(signalPackToSelect);
            paramsMemory.putBool("UpdateTheme", true);
          }
        }
      });

      QString currentSignal = QString::fromStdString(params.get("CustomSignals")).replace('_', ' ').replace('-', " (").toLower();
      currentSignal[0] = currentSignal[0].toUpper();
      for (int i = 1; i < currentSignal.length(); i++) {
        if (currentSignal[i - 1] == ' ' || currentSignal[i - 1] == '(') {
          currentSignal[i] = currentSignal[i].toUpper();
        }
      }
      if (currentSignal.contains(" (")) {
        currentSignal.append(')');
      }
      manageCustomSignalsBtn->setValue(currentSignal);
      visualToggle = reinterpret_cast<AbstractControl*>(manageCustomSignalsBtn);
    } else if (param == "CustomSounds") {
      manageCustomSoundsBtn = new FrogPilotButtonsControl(title, {tr("刪除"), tr("下載"), tr("選擇")}, desc);

      std::function<QString(QString)> formatSoundName = [](QString name) -> QString {
        QChar separator = name.contains('_') ? '_' : '-';
        QStringList parts = name.replace(separator, ' ').split(' ');

        for (int i = 0; i < parts.size(); ++i) {
          parts[i][0] = parts[i][0].toUpper();
        }

        if (separator == '-' && parts.size() > 1) {
          return parts.first() + " (" + parts.last() + ")";
        }

        return parts.join(' ');
      };

      std::function<QString(QString)> formatSoundNameForStorage = [](QString name) -> QString {
        name = name.toLower();
        name = name.replace(" (", "-");
        name = name.replace(' ', '_');
        name.remove('(').remove(')');
        return name;
      };

      QObject::connect(manageCustomSoundsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QDir themesDir{"/data/themes/theme_packs"};
        QFileInfoList dirList = themesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

        QString currentSound = QString::fromStdString(params.get("CustomSounds")).replace('_', ' ').replace('-', " (").toLower();
        currentSound[0] = currentSound[0].toUpper();
        for (int i = 1; i < currentSound.length(); i++) {
          if (currentSound[i - 1] == ' ' || currentSound[i - 1] == '(') {
            currentSound[i] = currentSound[i].toUpper();
          }
        }
        if (currentSound.contains(" (")) {
          currentSound.append(')');
        }

        QStringList availableSounds;
        for (const QFileInfo &dirInfo : dirList) {
          QString soundDir = dirInfo.absoluteFilePath();
          if (QDir(soundDir + "/sounds").exists()) {
            availableSounds << formatSoundName(dirInfo.fileName());
          }
        }
        availableSounds.append("Stock");
        std::sort(availableSounds.begin(), availableSounds.end());

        if (id == 0) {
          QStringList customSoundsList = availableSounds;
          customSoundsList.removeAll("Stock");
          customSoundsList.removeAll(currentSound);

          QString soundSchemeToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的聲音包"), customSoundsList, "", this);
          if (!soundSchemeToDelete.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 聲音方案?").arg(soundSchemeToDelete), tr("刪除"), this)) {
            themeDeleting = true;
            soundsDownloaded = false;

            QString selectedSound = formatSoundNameForStorage(soundSchemeToDelete);
            for (const QFileInfo &dirInfo : dirList) {
              if (dirInfo.fileName() == selectedSound) {
                QDir soundDir(dirInfo.absoluteFilePath() + "/sounds");
                if (soundDir.exists()) {
                  soundDir.removeRecursively();
                }
              }
            }

            QStringList downloadableSounds = QString::fromStdString(params.get("DownloadableSounds")).split(",");
            downloadableSounds << soundSchemeToDelete;
            downloadableSounds.removeDuplicates();
            downloadableSounds.removeAll("");
            std::sort(downloadableSounds.begin(), downloadableSounds.end());

            params.put("DownloadableSounds", downloadableSounds.join(",").toStdString());
            themeDeleting = false;
          }
        } else if (id == 1) {
          if (soundDownloading) {
            paramsMemory.putBool("CancelThemeDownload", true);
            cancellingDownload = true;

            QTimer::singleShot(2000, [=]() {
              paramsMemory.putBool("CancelThemeDownload", false);
              cancellingDownload = false;
              soundDownloading = false;
              themeDownloading = false;
            });
          } else {
            QStringList downloadableSounds = QString::fromStdString(params.get("DownloadableSounds")).split(",");
            QString soundSchemeToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的聲音包"), downloadableSounds, "", this);

            if (!soundSchemeToDownload.isEmpty()) {
              QString convertedSoundScheme = formatSoundNameForStorage(soundSchemeToDownload);
              paramsMemory.put("SoundToDownload", convertedSoundScheme.toStdString());
              downloadStatusLabel->setText("Downloading...");
              paramsMemory.put("ThemeDownloadProgress", "Downloading...");
              soundDownloading = true;
              themeDownloading = true;

              downloadableSounds.removeAll(soundSchemeToDownload);
              params.put("DownloadableSounds", downloadableSounds.join(",").toStdString());
            }
          }
        } else if (id == 2) {
          QString soundSchemeToSelect = MultiOptionDialog::getSelection(tr("選擇聲音方案"), availableSounds, currentSound, this);
          if (!soundSchemeToSelect.isEmpty()) {
            params.put("CustomSounds", formatSoundNameForStorage(soundSchemeToSelect).toStdString());
            manageCustomSoundsBtn->setValue(soundSchemeToSelect);
            paramsMemory.putBool("UpdateTheme", true);
          }
        }
      });

      QString currentSound = QString::fromStdString(params.get("CustomSounds")).replace('_', ' ').replace('-', " (").toLower();
      currentSound[0] = currentSound[0].toUpper();
      for (int i = 1; i < currentSound.length(); i++) {
        if (currentSound[i - 1] == ' ' || currentSound[i - 1] == '(') {
          currentSound[i] = currentSound[i].toUpper();
        }
      }
      if (currentSound.contains(" (")) {
        currentSound.append(')');
      }
      manageCustomSoundsBtn->setValue(currentSound);
      visualToggle = reinterpret_cast<AbstractControl*>(manageCustomSoundsBtn);
    } else if (param == "WheelIcon") {
      manageWheelIconsBtn = new FrogPilotButtonsControl(title, {tr("刪除"), tr("下載"), tr("選擇")}, desc);

      std::function<QString(QString)> formatWheelName = [](QString name) -> QString {
        QChar separator = name.contains('_') ? '_' : '-';
        QStringList parts = name.replace(separator, ' ').split(' ');

        for (int i = 0; i < parts.size(); ++i) {
          parts[i][0] = parts[i][0].toUpper();
        }

        if (separator == '-' && parts.size() > 1) {
          return parts.first() + " (" + parts.last() + ")";
        }

        return parts.join(' ');
      };

      std::function<QString(QString)> formatWheelNameForStorage = [](QString name) -> QString {
        name = name.toLower();
        name = name.replace(" (", "-");
        name = name.replace(' ', '_');
        name.remove('(').remove(')');
        return name;
      };

      QObject::connect(manageWheelIconsBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        QDir wheelDir{"/data/themes/steering_wheels"};
        QFileInfoList fileList = wheelDir.entryInfoList(QDir::Files);

        QString currentWheel = QString::fromStdString(params.get("WheelIcon")).replace('_', ' ').replace('-', " (").toLower();
        currentWheel[0] = currentWheel[0].toUpper();
        for (int i = 1; i < currentWheel.length(); i++) {
          if (currentWheel[i - 1] == ' ' || currentWheel[i - 1] == '(') {
            currentWheel[i] = currentWheel[i].toUpper();
          }
        }
        if (currentWheel.contains(" (")) {
          currentWheel.append(')');
        }

        QStringList availableWheels;
        for (const QFileInfo &fileInfo : fileList) {
          QString baseName = fileInfo.completeBaseName();
          QString formattedName = formatWheelName(baseName);
          if (formattedName != "Img Chffr Wheel") {
            availableWheels << formattedName;
          }
        }
        availableWheels.append("Stock");
        availableWheels.append("None");
        std::sort(availableWheels.begin(), availableWheels.end());

        if (id == 0) {
          QStringList steeringWheelList = availableWheels;
          steeringWheelList.removeAll("None");
          steeringWheelList.removeAll("Stock");
          steeringWheelList.removeAll(currentWheel);

          QString imageToDelete = MultiOptionDialog::getSelection(tr("選擇要刪除的方向盤"), steeringWheelList, "", this);
          if (!imageToDelete.isEmpty() && ConfirmationDialog::confirm(tr("您確定要刪除 '%1' 方向盤影像?").arg(imageToDelete), tr("刪除"), this)) {
            themeDeleting = true;
            wheelsDownloaded = false;

            QString selectedImage = formatWheelNameForStorage(imageToDelete);
            for (const QFileInfo &fileInfo : fileList) {
              if (fileInfo.completeBaseName() == selectedImage) {
                QFile::remove(fileInfo.filePath());
              }
            }

            QStringList downloadableWheels = QString::fromStdString(params.get("DownloadableWheels")).split(",");
            downloadableWheels << imageToDelete;
            downloadableWheels.removeDuplicates();
            downloadableWheels.removeAll("");
            std::sort(downloadableWheels.begin(), downloadableWheels.end());

            params.put("DownloadableWheels", downloadableWheels.join(",").toStdString());
            themeDeleting = false;
          }
        } else if (id == 1) {
          if (wheelDownloading) {
            paramsMemory.putBool("CancelThemeDownload", true);
            cancellingDownload = true;

            QTimer::singleShot(2000, [=]() {
              paramsMemory.putBool("CancelThemeDownload", false);
              cancellingDownload = false;
              wheelDownloading = false;
              themeDownloading = false;
            });
          } else {
            QStringList downloadableWheels = QString::fromStdString(params.get("DownloadableWheels")).split(",");
            QString wheelToDownload = MultiOptionDialog::getSelection(tr("選擇要下載的方向盤"), downloadableWheels, "", this);

            if (!wheelToDownload.isEmpty()) {
              QString convertedImage = formatWheelNameForStorage(wheelToDownload);
              paramsMemory.put("WheelToDownload", convertedImage.toStdString());
              downloadStatusLabel->setText("Downloading...");
              paramsMemory.put("ThemeDownloadProgress", "Downloading...");
              themeDownloading = true;
              wheelDownloading = true;

              downloadableWheels.removeAll(wheelToDownload);
              params.put("DownloadableWheels", downloadableWheels.join(",").toStdString());
            }
          }
        } else if (id == 2) {
          QString imageToSelect = MultiOptionDialog::getSelection(tr("選擇方向盤"), availableWheels, currentWheel, this);
          if (!imageToSelect.isEmpty()) {
            params.put("WheelIcon", formatWheelNameForStorage(imageToSelect).toStdString());
            manageWheelIconsBtn->setValue(imageToSelect);
            paramsMemory.putBool("UpdateTheme", true);
          }
        }
      });

      QString currentWheel = QString::fromStdString(params.get("WheelIcon")).replace('_', ' ').replace('-', " (").toLower();
      currentWheel[0] = currentWheel[0].toUpper();
      for (int i = 1; i < currentWheel.length(); i++) {
        if (currentWheel[i - 1] == ' ' || currentWheel[i - 1] == '(') {
          currentWheel[i] = currentWheel[i].toUpper();
        }
      }
      if (currentWheel.contains(" (")) {
        currentWheel.append(')');
      }
      manageWheelIconsBtn->setValue(currentWheel);
      visualToggle = reinterpret_cast<AbstractControl*>(manageWheelIconsBtn);
    } else if (param == "DownloadStatusLabel") {
      downloadStatusLabel = new LabelControl(title, "Idle");
      visualToggle = reinterpret_cast<AbstractControl*>(downloadStatusLabel);
    } else if (param == "StartupAlert") {
      FrogPilotButtonsControl *startupAlertButton = new FrogPilotButtonsControl(title, {tr("STOCK"), tr("FROGPILOT"), tr("CUSTOM"), tr("CLEAR")}, desc);
      QObject::connect(startupAlertButton, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
        int maxLengthTop = 35;
        int maxLengthBottom = 45;

        QString stockTop = "Be ready to take over at any time";
        QString stockBottom = "Always keep hands on wheel and eyes on road";

        QString frogpilotTop = "Hippity hoppity this is my property";
        QString frogpilotBottom = "so I do what I want 🐸";

        QString currentTop = QString::fromStdString(params.get("StartupMessageTop"));
        QString currentBottom = QString::fromStdString(params.get("StartupMessageBottom"));

        if (id == 0) {
          params.put("StartupMessageTop", stockTop.toStdString());
          params.put("StartupMessageBottom", stockBottom.toStdString());
        } else if (id == 1) {
          params.put("StartupMessageTop", frogpilotTop.toStdString());
          params.put("StartupMessageBottom", frogpilotBottom.toStdString());
        } else if (id == 2) {
          QString newTop = InputDialog::getText(tr("輸入上半部的文字"), this, tr("人物: 0/%1").arg(maxLengthTop), false, -1, currentTop, maxLengthTop).trimmed();
          if (newTop.length() > 0) {
            params.putNonBlocking("StartupMessageTop", newTop.toStdString());
            QString newBottom = InputDialog::getText(tr("輸入下半部的文字"), this, tr("人物: 0/%1").arg(maxLengthBottom), false, -1, currentBottom, maxLengthBottom).trimmed();
            if (newBottom.length() > 0) {
              params.putNonBlocking("StartupMessageBottom", newBottom.toStdString());
            }
          }
        } else if (id == 3) {
          params.remove("StartupMessageTop");
          params.remove("StartupMessageBottom");
        }
      });
      visualToggle = reinterpret_cast<AbstractControl*>(startupAlertButton);

    } else if (param == "CustomAlerts") {
      FrogPilotParamManageControl *customAlertsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customAlertsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedCustomAlertsKeys = customAlertsKeys;

          if (!hasBSM) {
            modifiedCustomAlertsKeys.erase("LoudBlindspotAlert");
          }

          toggle->setVisible(modifiedCustomAlertsKeys.find(key.c_str()) != modifiedCustomAlertsKeys.end());
        }
      });
      visualToggle = customAlertsToggle;

    } else if (param == "CustomUI") {
      FrogPilotParamManageControl *customUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(customOnroadUIKeys.find(key.c_str()) != customOnroadUIKeys.end());
        }
      });
      visualToggle = customUIToggle;
    } else if (param == "CustomPaths") {
      std::vector<QString> pathToggles{"AccelerationPath", "AdjacentPath", "BlindSpotPath", "AdjacentPathMetrics"};
      std::vector<QString> pathToggleNames{tr("加速"), tr("鄰近的"), tr("盲點"), tr("指標")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, pathToggles, pathToggleNames);
    } else if (param == "PedalsOnUI") {
      std::vector<QString> pedalsToggles{"DynamicPedalsOnUI", "StaticPedalsOnUI"};
      std::vector<QString> pedalsToggleNames{tr("動態的"), tr("靜止的")};
      FrogPilotParamToggleControl *pedalsToggle = new FrogPilotParamToggleControl(param, title, desc, icon, pedalsToggles, pedalsToggleNames);
      QObject::connect(pedalsToggle, &FrogPilotParamToggleControl::buttonTypeClicked, this, [this, pedalsToggle](int index) {
        if (index == 0) {
          params.putBool("StaticPedalsOnUI", false);
        } else if (index == 1) {
          params.putBool("DynamicPedalsOnUI", false);
        }

        pedalsToggle->updateButtonStates();
      });
      visualToggle = pedalsToggle;
    } else if (param == "ShowStoppingPoint") {
      std::vector<QString> stoppingPointToggles{"ShowStoppingPointMetrics"};
      std::vector<QString> stoppingPointToggleNames{tr("顯示距離")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, stoppingPointToggles, stoppingPointToggleNames);

    } else if (param == "DeveloperUI") {
      FrogPilotParamManageControl *developerUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(developerUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedDeveloperUIKeys  = developerUIKeys ;

          toggle->setVisible(modifiedDeveloperUIKeys.find(key.c_str()) != modifiedDeveloperUIKeys.end());
        }
      });
      visualToggle = developerUIToggle;
    } else if (param == "BorderMetrics") {
      std::vector<QString> borderToggles{"BlindSpotMetrics", "ShowSteering", "SignalMetrics"};
      std::vector<QString> borderToggleNames{tr("盲點"), tr("轉向扭矩"), tr("轉彎訊號")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, borderToggles, borderToggleNames);
    } else if (param == "LateralMetrics") {
      std::vector<QString> lateralToggles{"TuningInfo"};
      std::vector<QString> lateralToggleNames{tr("自動 Tune")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, lateralToggles, lateralToggleNames);
    } else if (param == "LongitudinalMetrics") {
      std::vector<QString> longitudinalToggles{"LeadInfo", "JerkInfo"};
      std::vector<QString> longitudinalToggleNames{tr("前車訊息"), tr("Longitudinal Jerk")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, longitudinalToggles, longitudinalToggleNames);
    } else if (param == "NumericalTemp") {
      std::vector<QString> temperatureToggles{"Fahrenheit"};
      std::vector<QString> temperatureToggleNames{tr("華氏")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, temperatureToggles, temperatureToggleNames);
    } else if (param == "SidebarMetrics") {
      std::vector<QString> sidebarMetricsToggles{"ShowCPU", "ShowGPU", "ShowIP", "ShowMemoryUsage", "ShowStorageLeft", "ShowStorageUsed"};
      std::vector<QString> sidebarMetricsToggleNames{tr("CPU"), tr("GPU"), tr("IP"), tr("RAM"), tr("SSD 剩餘量"), tr("SSD 使用量")};
      FrogPilotParamToggleControl *sidebarMetricsToggle = new FrogPilotParamToggleControl(param, title, desc, icon, sidebarMetricsToggles, sidebarMetricsToggleNames, this, 125);
      QObject::connect(sidebarMetricsToggle, &FrogPilotParamToggleControl::buttonTypeClicked, this, [this, sidebarMetricsToggle](int index) {
        if (index == 0) {
          params.putBool("ShowGPU", false);
        } else if (index == 1) {
          params.putBool("ShowCPU", false);
        } else if (index == 3) {
          params.putBool("ShowStorageLeft", false);
          params.putBool("ShowStorageUsed", false);
        } else if (index == 4) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageUsed", false);
        } else if (index == 5) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageLeft", false);
        }

        sidebarMetricsToggle->updateButtonStates();
      });
      visualToggle = sidebarMetricsToggle;

    } else if (param == "ModelUI") {
      FrogPilotParamManageControl *modelUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(modelUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedModelUIKeysKeys = modelUIKeys;

          if (!hasOpenpilotLongitudinal || disableOpenpilotLongitudinal) {
            modifiedModelUIKeysKeys.erase("HideLeadMarker");
          }

          toggle->setVisible(modifiedModelUIKeysKeys.find(key.c_str()) != modifiedModelUIKeysKeys.end());
        }
      });
      visualToggle = modelUIToggle;
    } else if (param == "LaneLinesWidth" || param == "RoadEdgesWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 24, std::map<int, QString>(), this, false, tr(" inches"));
    } else if (param == "PathEdgeWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, tr("%"));
    } else if (param == "PathWidth") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, tr(" feet"), 10);

    } else if (param == "QOLVisuals") {
      FrogPilotParamManageControl *qolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(qolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(qolKeys.find(key.c_str()) != qolKeys.end());
        }
      });
      visualToggle = qolToggle;
    } else if (param == "CameraView") {
      std::vector<QString> cameraOptions{tr("自動"), tr("駕駛"), tr("標準"), tr("廣角")};
      ButtonParamControl *preferredCamera = new ButtonParamControl(param, title, desc, icon, cameraOptions);
      visualToggle = preferredCamera;
    } else if (param == "BigMap") {
      std::vector<QString> mapToggles{"FullMap"};
      std::vector<QString> mapToggleNames{tr("全螢幕")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, mapToggles, mapToggleNames);
    } else if (param == "HideSpeed") {
      std::vector<QString> hideSpeedToggles{"HideSpeedUI"};
      std::vector<QString> hideSpeedToggleNames{tr("透過螢幕控制")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, hideSpeedToggles, hideSpeedToggleNames);
    } else if (param == "MapStyle") {
      QMap<int, QString> styleMap = {
        {0, tr("原始 openpilot")},
        {1, tr("Mapbox 街道")},
        {2, tr("Mapbox 戶外活動")},
        {3, tr("Mapbox 白天")},
        {4, tr("Mapbox 夜晚")},
        {5, tr("Mapbox 衛星")},
        {6, tr("Mapbox 衛星街道")},
        {7, tr("Mapbox 導航(白天)")},
        {8, tr("Mapbox 導航(夜晚)")},
        {9, tr("Mapbox 交通(夜晚)")},
        {10, tr("mike854's (Satellite hybrid)")},
        {11, tr("huifan's 白天")},
        {12, tr("huifan's 導航(夜晚)")},
      };

      QStringList styles = styleMap.values();
      ButtonControl *mapStyleButton = new ButtonControl(title, tr("選擇"), desc);
      QObject::connect(mapStyleButton, &ButtonControl::clicked, [=]() {
        QStringList styles = styleMap.values();
        QString selection = MultiOptionDialog::getSelection(tr("選擇地圖樣式"), styles, "", this);
        if (!selection.isEmpty()) {
          int selectedStyle = styleMap.key(selection);
          params.putIntNonBlocking("MapStyle", selectedStyle);
          mapStyleButton->setValue(selection);
          updateFrogPilotToggles();
        }
      });

      int currentStyle = params.getInt("MapStyle");
      mapStyleButton->setValue(styleMap[currentStyle]);

      visualToggle = mapStyleButton;

    } else if (param == "ScreenManagement") {
      FrogPilotParamManageControl *screenToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(screenToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();

        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(screenKeys.find(key.c_str()) != screenKeys.end());
        }
      });
      visualToggle = screenToggle;
    } else if (param == "HideUIElements") {
      std::vector<QString> uiElementsToggles{"HideAlerts", "HideMapIcon", "HideMaxSpeed"};
      std::vector<QString> uiElementsToggleNames{tr("警告"), tr("地圖"), tr("最高速")};
      visualToggle = new FrogPilotParamToggleControl(param, title, desc, icon, uiElementsToggles, uiElementsToggleNames);
    } else if (param == "ScreenBrightness" || param == "ScreenBrightnessOnroad") {
      std::map<int, QString> brightnessLabels;
      if (param == "ScreenBrightnessOnroad") {
        for (int i = 0; i <= 101; i++) {
          brightnessLabels[i] = (i == 0) ? tr("螢幕關閉") : (i == 101) ? tr("自動") : QString::number(i) + "%";
        }
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 101, brightnessLabels, this, false);
      } else {
        for (int i = 1; i <= 101; i++) {
          brightnessLabels[i] = (i == 101) ? tr("自動") : QString::number(i) + "%";
        }
        visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 101, brightnessLabels, this, false);
      }
    } else if (param == "ScreenTimeout" || param == "ScreenTimeoutOnroad") {
      visualToggle = new FrogPilotParamValueControl(param, title, desc, icon, 5, 60, std::map<int, QString>(), this, false, tr(" 秒"));

    } else {
      visualToggle = new ParamControl(param, title, desc, icon, this);
    }

    addItem(visualToggle);
    toggles[param.toStdString()] = visualToggle;

    QObject::connect(static_cast<ToggleControl*>(visualToggle), &ToggleControl::toggleFlipped, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamToggleControl*>(visualToggle), &FrogPilotParamToggleControl::buttonTypeClicked, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamValueControl*>(visualToggle), &FrogPilotParamValueControl::valueChanged, [this]() {
      bool screen_management = params.getBool("ScreenManagement");
      if (!started) {
        uiState()->scene.screen_brightness = screen_management ? params.getInt("ScreenBrightness") : 101;
      } else {
        uiState()->scene.screen_brightness_onroad = screen_management ? params.getInt("ScreenBrightnessOnroad") : 101;
      }
      updateFrogPilotToggles();
    });

    QObject::connect(visualToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });

    QObject::connect(static_cast<FrogPilotParamManageControl*>(visualToggle), &FrogPilotParamManageControl::manageButtonClicked, [this]() {
      update();
    });
  }

  QObject::connect(parent, &SettingsWindow::closeParentToggle, this, &FrogPilotVisualsPanel::hideToggles);
  QObject::connect(parent, &SettingsWindow::closeSubParentToggle, this, &FrogPilotVisualsPanel::hideSubToggles);
  QObject::connect(parent, &SettingsWindow::updateMetric, this, &FrogPilotVisualsPanel::updateMetric);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &FrogPilotVisualsPanel::updateCarToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotVisualsPanel::updateState);

  updateMetric();
}

void FrogPilotVisualsPanel::showEvent(QShowEvent *event) {
  disableOpenpilotLongitudinal = params.getBool("DisableOpenpilotLongitudinal");

  colorsDownloaded = params.get("DownloadableColors").empty();
  iconsDownloaded = params.get("DownloadableIcons").empty();
  signalsDownloaded = params.get("DownloadableSignals").empty();
  soundsDownloaded = params.get("DownloadableSounds").empty();
  wheelsDownloaded = params.get("DownloadableWheels").empty();
}

void FrogPilotVisualsPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  if (personalizeOpenpilotOpen) {
    if (themeDownloading) {
      QString progress = QString::fromStdString(paramsMemory.get("ThemeDownloadProgress"));
      bool downloadFailed = progress.contains(QRegularExpression("cancelled|exists|Failed|offline", QRegularExpression::CaseInsensitiveOption));

      if (progress != "Downloading...") {
        downloadStatusLabel->setText(progress);
      }

      if (progress == "Downloaded!" || downloadFailed) {
        QTimer::singleShot(2000, [=]() {
          if (!themeDownloading) {
            downloadStatusLabel->setText("Idle");
          }
        });
        paramsMemory.remove("ThemeDownloadProgress");
        colorDownloading = false;
        iconDownloading = false;
        signalDownloading = false;
        soundDownloading = false;
        themeDownloading = false;
        wheelDownloading = false;

        colorsDownloaded = params.get("DownloadableColors").empty();
        iconsDownloaded = params.get("DownloadableIcons").empty();
        signalsDownloaded = params.get("DownloadableSignals").empty();
        soundsDownloaded = params.get("DownloadableSounds").empty();
        wheelsDownloaded = params.get("DownloadableWheels").empty();
      }
    }

    manageCustomColorsBtn->setText(1, colorDownloading ? tr("取消") : tr("下載"));
    manageCustomColorsBtn->setEnabledButtons(0, !themeDeleting && !themeDownloading);
    manageCustomColorsBtn->setEnabledButtons(1, s.scene.online && (!themeDownloading || colorDownloading) && !cancellingDownload && !themeDeleting && !colorsDownloaded);
    manageCustomColorsBtn->setEnabledButtons(2, !themeDeleting && !themeDownloading);

    manageCustomIconsBtn->setText(1, iconDownloading ? tr("取消") : tr("下載"));
    manageCustomIconsBtn->setEnabledButtons(0, !themeDeleting && !themeDownloading);
    manageCustomIconsBtn->setEnabledButtons(1, s.scene.online && (!themeDownloading || iconDownloading) && !cancellingDownload && !themeDeleting && !iconsDownloaded);
    manageCustomIconsBtn->setEnabledButtons(2, !themeDeleting && !themeDownloading);

    manageCustomSignalsBtn->setText(1, signalDownloading ? tr("取消") : tr("下載"));
    manageCustomSignalsBtn->setEnabledButtons(0, !themeDeleting && !themeDownloading);
    manageCustomSignalsBtn->setEnabledButtons(1, s.scene.online && (!themeDownloading || signalDownloading) && !cancellingDownload && !themeDeleting && !signalsDownloaded);
    manageCustomSignalsBtn->setEnabledButtons(2, !themeDeleting && !themeDownloading);

    manageCustomSoundsBtn->setText(1, soundDownloading ? tr("取消") : tr("下載"));
    manageCustomSoundsBtn->setEnabledButtons(0, !themeDeleting && !themeDownloading);
    manageCustomSoundsBtn->setEnabledButtons(1, s.scene.online && (!themeDownloading || soundDownloading) && !cancellingDownload && !themeDeleting && !soundsDownloaded);
    manageCustomSoundsBtn->setEnabledButtons(2, !themeDeleting && !themeDownloading);

    manageWheelIconsBtn->setText(1, wheelDownloading ? tr("取消") : tr("下載"));
    manageWheelIconsBtn->setEnabledButtons(0, !themeDeleting && !themeDownloading);
    manageWheelIconsBtn->setEnabledButtons(1, s.scene.online && (!themeDownloading || wheelDownloading) && !cancellingDownload && !themeDeleting && !wheelsDownloaded);
    manageWheelIconsBtn->setEnabledButtons(2, !themeDeleting && !themeDownloading);
  }

  started = s.scene.started;
}

void FrogPilotVisualsPanel::updateCarToggles() {
  auto carParams = params.get("CarParamsPersistent");
  if (!carParams.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(carParams.data(), carParams.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();
    auto carName = CP.getCarName();

    hasAutoTune = (carName == "hyundai" || carName == "toyota") && CP.getLateralTuning().which() == cereal::CarParams::LateralTuning::TORQUE;
    hasBSM = CP.getEnableBsm();
    hasOpenpilotLongitudinal = hasLongitudinalControl(CP);
  } else {
    hasAutoTune = true;
    hasBSM = true;
    hasOpenpilotLongitudinal = true;
  }

  hideToggles();
}

void FrogPilotVisualsPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? INCH_TO_CM : CM_TO_INCH;
    double speedConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;

    params.putIntNonBlocking("LaneLinesWidth", std::nearbyint(params.getInt("LaneLinesWidth") * distanceConversion));
    params.putIntNonBlocking("RoadEdgesWidth", std::nearbyint(params.getInt("RoadEdgesWidth") * distanceConversion));

    params.putIntNonBlocking("PathWidth", std::nearbyint(params.getInt("PathWidth") * speedConversion));
  }

  FrogPilotParamValueControl *laneLinesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneLinesWidth"]);
  FrogPilotParamValueControl *roadEdgesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["RoadEdgesWidth"]);
  FrogPilotParamValueControl *pathWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["PathWidth"]);

  if (isMetric) {
    laneLinesWidthToggle->setDescription(tr("自訂車道線寬度.\n\n預設值符合維也納平均 10 厘米."));
    roadEdgesWidthToggle->setDescription(tr("自訂道路邊緣寬度.\n\n預設為維也納平均車道線寬度 10 公分的 1/2."));

    laneLinesWidthToggle->updateControl(0, 60, tr(" 公分"));
    roadEdgesWidthToggle->updateControl(0, 60, tr(" 公分"));
    pathWidthToggle->updateControl(0, 30, tr(" 公尺"), 10);
  } else {
    laneLinesWidthToggle->setDescription(tr("Customize the lane line width.\n\nDefault matches the MUTCD average of 4 inches."));
    roadEdgesWidthToggle->setDescription(tr("Customize the road edges width.\n\nDefault is 1/2 of the MUTCD average lane line width of 4 inches."));

    laneLinesWidthToggle->updateControl(0, 24, tr(" inches"));
    roadEdgesWidthToggle->updateControl(0, 24, tr(" inches"));

    pathWidthToggle->updateControl(0, 100, tr(" feet"), 10);
  }

  laneLinesWidthToggle->refresh();
  roadEdgesWidthToggle->refresh();
}

void FrogPilotVisualsPanel::hideToggles() {
  personalizeOpenpilotOpen = false;

  for (auto &[key, toggle] : toggles) {
    bool subToggles = alertVolumeControlKeys.find(key.c_str()) != alertVolumeControlKeys.end() ||
                      bonusContentKeys.find(key.c_str()) != bonusContentKeys.end() ||
                      customAlertsKeys.find(key.c_str()) != customAlertsKeys.end() ||
                      customOnroadUIKeys.find(key.c_str()) != customOnroadUIKeys.end() ||
                      personalizeOpenpilotKeys.find(key.c_str()) != personalizeOpenpilotKeys.end() ||
                      developerUIKeys.find(key.c_str()) != developerUIKeys.end() ||
                      modelUIKeys.find(key.c_str()) != modelUIKeys.end() ||
                      qolKeys.find(key.c_str()) != qolKeys.end() ||
                      screenKeys.find(key.c_str()) != screenKeys.end();
    toggle->setVisible(!subToggles);
  }

  update();
}

void FrogPilotVisualsPanel::hideSubToggles() {
  if (personalizeOpenpilotOpen) {
    for (auto &[key, toggle] : toggles) {
      bool isVisible = bonusContentKeys.find(key.c_str()) != bonusContentKeys.end();
      toggle->setVisible(isVisible);
    }
  }

  update();
}
