#include "frogpilot/ui/qt/offroad/wheel_settings.h"

FrogPilotWheelPanel::FrogPilotWheelPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QJsonObject shownDescriptions = QJsonDocument::fromJson(QString::fromStdString(params.get("ShownToggleDescriptions")).toUtf8()).object();
  QString className = this->metaObject()->className();

  if (!shownDescriptions.value(className).toBool(false)) {
    forceOpenDescriptions = true;
    shownDescriptions.insert(className, true);
    params.put("ShownToggleDescriptions", QJsonDocument(shownDescriptions).toJson(QJsonDocument::Compact).toStdString());
  }

  const std::vector<std::tuple<QString, QString, QString, QString>> wheelToggles {
    {"DistanceButtonControl", tr("距離按鈕"), tr("<b>按下 \"距離\" 按鈕時執行的操作。</b>"), "../../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"LongDistanceButtonControl", tr("距離按鈕（長按）"), tr("<b>按住 \"距離\" 按鈕超過 0.5 秒事执行的操作。</b>"), "../../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"VeryLongDistanceButtonControl", tr("距離按鈕（非常長按）"), tr("<b>按住 \"距離\" 按鈕超過 2.5 秒事执行的操作。</b>"), "../../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"LKASButtonControl", tr("LKAS 按鈕"), tr("<b>按下 \"LKAS\" 按鈕時執行的操作。</b>"), "../../frogpilot/assets/toggle_icons/icon_mute.png"}
  };

  for (const auto &[param, title, desc, icon] : wheelToggles) {
    QMap<int, QString> functionsMap {
      {0, tr("無動作")},
      {3, tr("暫停轉向")}
    };

    QMap<int, QString> longitudinalFunctionsMap {
      {1, tr("變更 \"個人駕駛檔\"" )},
      {2, tr("強制 openpilot COAST")},
      {4, tr("暫停加速/制動")},
      {5, tr("切換 \"實驗模式\" 開/關")},
      {6, tr("切換 \"塞車模式\" 開/關")}
    };

    ButtonControl *wheelToggle = new ButtonControl(title, tr("選擇"), desc);
    QObject::connect(wheelToggle, &ButtonControl::clicked, [functionsMap, longitudinalFunctionsMap, key = param, parent, wheelToggle, this]() mutable {
      if (parent->hasOpenpilotLongitudinal) {
        QMap<int, QString>::const_iterator it;
        for (it = longitudinalFunctionsMap.constBegin(); it != longitudinalFunctionsMap.constEnd(); ++it) {
          functionsMap[it.key()] = it.value();
        }
      }

      QString selection = MultiOptionDialog::getSelection(tr("選擇要分類到此按鈕的功能"), functionsMap.values(), functionsMap[params.getInt(key.toStdString())], this);
      if (!selection.isEmpty()) {
        params.putInt(key.toStdString(), functionsMap.key(selection));

        wheelToggle->setValue(selection);
      }
    });
    QMap<int, QString> mergedFunctionsMap = functionsMap;
    QMap<int, QString>::const_iterator it;
    for (it = longitudinalFunctionsMap.constBegin(); it != longitudinalFunctionsMap.constEnd(); ++it) {
      mergedFunctionsMap[it.key()] = it.value();
    }
    wheelToggle->setValue(mergedFunctionsMap[params.getInt(param.toStdString())]);

    toggles[param] = wheelToggle;

    addItem(wheelToggle);

    QObject::connect(wheelToggle, &AbstractControl::hideDescriptionEvent, [this]() {
      update();
    });
    QObject::connect(wheelToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  openDescriptions(forceOpenDescriptions, toggles);
}

void FrogPilotWheelPanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;

  updateToggles();
}

void FrogPilotWheelPanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    bool setVisible = parent->tuningLevel >= frogpilotToggleLevels[key].toDouble();

    if (key == "LKASButtonControl") {
      setVisible &= !parent->isSubaru;
      setVisible &= !(params.getBool("AlwaysOnLateral") && params.getBool("AlwaysOnLateralLKAS"));
    }

    toggle->setVisible(setVisible);
  }

  openDescriptions(forceOpenDescriptions, toggles);

  update();
}
