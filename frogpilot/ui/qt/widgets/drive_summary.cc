#include "selfdrive/ui/qt/widgets/scrollview.h"

#include "frogpilot/ui/qt/widgets/drive_summary.h"

FrogPilotDriveSummary::FrogPilotDriveSummary(QWidget *parent, bool randomEvents) : QFrame(parent), displayRandomEvents(randomEvents) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(15);

  titleLabel = new QLabel(randomEvents ? tr("隨機事件摘要") : tr("駕駛摘要"), this);
  titleLabel->setAlignment(Qt::AlignCenter);
  titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  titleLabel->setStyleSheet(R"(
    QLabel {
      background-color: #444444;
      border-radius: 12px;
      color: #FFFFFF;
      font-size: 50px;
      font-weight: normal;
      padding: 12px 28px;
    }
  )");
  titleLabel->setMaximumHeight(titleLabel->sizeHint().height());

  mainLayout->addWidget(titleLabel);
  mainLayout->addSpacing(10);

  QWidget *containerWidget = new QWidget(this);
  QVBoxLayout *listLayout = new QVBoxLayout(containerWidget);
  listLayout->setAlignment(Qt::AlignTop);
  listLayout->setSpacing(20);

  if (displayRandomEvents) {
    randomEventsMap.insert("accel30", tr("UwUs"));
    randomEventsMap.insert("accel35", tr("Loch Ness Encounters"));
    randomEventsMap.insert("accel40", tr("Visits to 1955"));
    randomEventsMap.insert("dejaVuCurve", tr("Deja Vu Moments"));
    randomEventsMap.insert("firefoxSteerSaturated", tr("Internet Explorer Weeeeeeees"));
    randomEventsMap.insert("hal9000", tr("HAL 9000 Denials"));
    randomEventsMap.insert("openpilotCrashedRandomEvent", tr("openpilot Crashes"));
    randomEventsMap.insert("thisIsFineSteerSaturated", tr("This Is Fine Moments"));
    randomEventsMap.insert("toBeContinued", tr("To Be Continued Moments"));
    randomEventsMap.insert("vCruise69", tr("Noices"));
    randomEventsMap.insert("yourFrogTriedToKillMe", tr("Attempted Frog Murders"));
    randomEventsMap.insert("youveGotMail", tr("Total Mail Received"));
  } else {
    listLayout->addWidget(createStatBox(tr("駕駛過程啟用 openpilot"), &engagementValue, this));
    listLayout->addWidget(createStatBox(tr("駕駛距離"), &frogPilotMetersValue, this));
    listLayout->addWidget(createStatBox(tr("駕駛時間"), &trackedTimeValue, this));
    listLayout->addWidget(createStatBox(tr("駕駛過程處於“實驗模式”"), &experimentalModeTimeValue, this));

/////////////////////////////////////////////////////
    // 油耗和油資左右並排顯示
    QWidget *fuelContainer = new QWidget(this);
    QHBoxLayout *fuelLayout = new QHBoxLayout(fuelContainer);
    fuelLayout->setSpacing(20);
    fuelLayout->setContentsMargins(0, 0, 0, 0);
    fuelLayout->addWidget(createStatBox(tr("油耗"), &fuelConsumptionValue, this));
    fuelLayout->addWidget(createStatBox(tr("油資"), &avgFuelEconomyValue, this));
    listLayout->addWidget(fuelContainer);
/////////////////////////////////////////////////////
  }

  if (displayRandomEvents) {
    eventsListLayout = listLayout;
    mainLayout->addWidget(new ScrollView(containerWidget, this), 1);
  } else {
    mainLayout->addWidget(containerWidget, 1);
  }

  setLayout(mainLayout);

  setStyleSheet(R"(
    QFrame {
      background-color: #333333;
    }
  )");

  QObject::connect(device(), &Device::interactiveTimeout, [this]() {
    emit panelClosed();
  });
  QObject::connect(uiState(), &UIState::offroadTransition, [this](bool offroad) {
    if (!offroad) {
      previousStats = QJsonDocument::fromJson(QString::fromStdString(params.get("FrogPilotStats")).toUtf8()).object();
    }
  });
}

void FrogPilotDriveSummary::mousePressEvent(QMouseEvent *e) {
  emit panelClosed();
}

void FrogPilotDriveSummary::showEvent(QShowEvent *event) {
  bool isMetric = params.getBool("IsMetric");

  QJsonObject currentStats = QJsonDocument::fromJson(QString::fromStdString(params.get("FrogPilotStats")).toUtf8()).object();

  if (displayRandomEvents) {
    QJsonObject currentRandomEvents = currentStats.value("RandomEvents").toObject();
    QJsonObject previousRandomEvents = previousStats.value("RandomEvents").toObject();

    QList<QPair<QString, int>> eventsList;
    for (QMap<QString, QString>::const_iterator it = randomEventsMap.constBegin(); it != randomEventsMap.constEnd(); ++it) {
      int currentValue = currentRandomEvents.value(it.key()).toInt();
      int previousValue = previousRandomEvents.value(it.key()).toInt();
      int diffValue = currentValue - previousValue;

      if (diffValue > 0) {
        eventsList.append(qMakePair(it.value(), diffValue));
      }
    }

    std::sort(eventsList.begin(), eventsList.end(), [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
      if (a.second != b.second) {
        return a.second > b.second;
      } else {
        return a.first.localeAwareCompare(b.first) < 0;
      }
    });

    QLayoutItem *child;
    while ((child = eventsListLayout->takeAt(0)) != nullptr) {
      if (QWidget *widget = child->widget()) {
        widget->deleteLater();
      }
      delete child;
    }
    randomEventLabels.clear();

    if (eventsList.isEmpty()) {
      eventsListLayout->setAlignment(Qt::AlignCenter);

      QLabel *noEventsLabel = new QLabel(tr("沒有隨機事件發生!"), this);
      noEventsLabel->setAlignment(Qt::AlignCenter);
      noEventsLabel->setStyleSheet(R"(
        QLabel {
          font-size: 50px;
          font-weight: normal;
          color: #FFFFFF;
        }
      )");
      eventsListLayout->addWidget(noEventsLabel);
    } else {
      eventsListLayout->setAlignment(Qt::AlignTop);

      for (QList<QPair<QString, int>>::const_iterator it = eventsList.constBegin(); it != eventsList.constEnd(); ++it) {
        QLabel *valueLabel = nullptr;
        eventsListLayout->addWidget(createStatBox(it->first, &valueLabel, this));
        randomEventLabels.insert(it->first, valueLabel);
        valueLabel->setText(QLocale().toString(it->second));
      }
    }
  } else {
    std::function<double(const QString &)> diffDouble = [currentStats, this](const QString &key) -> double {
      return currentStats.value(key).toDouble() - previousStats.value(key).toDouble();
    };

    std::function<QString(double)> formatDistance = [&](double meters) {
      double value;
      QString unit;
      if (isMetric) {
        value = meters / 1000.0;
        unit = (qRound(value) == 1) ? tr(" 公里") : tr(" 公里");
      } else {
        value = meters * METER_TO_MILE;
        unit = (qRound(value) == 1) ? tr(" 英里") : tr(" 英里");
      }
      return QLocale().toString(qRound(value)) + unit;
    };

    std::function<QString(int)> formatTime = [&](int seconds) {
      static int secondsInDay = 60 * 60 * 24;
      static int secondsInHour = 60 * 60;

      int days = seconds / secondsInDay;
      int hours = (seconds % secondsInDay) / secondsInHour;
      int minutes = (seconds % secondsInHour) / 60;

      QString result;
      if (days > 0) result += QLocale().toString(days) + (days == 1 ? tr(" 天 ") : tr(" 天 "));
      if (hours > 0 || days > 0) result += QLocale().toString(hours) + (hours == 1 ? tr(" 小時 ") : tr(" 小時 "));
      result += QLocale().toString(minutes) + (minutes == 1 ? tr(" 分鐘") : tr(" 分鐘"));
      return result.trimmed();
    };

    int engagedTime = diffDouble("AOLTime") + diffDouble("LongitudinalTime");
    int experimentalTime = diffDouble("ExperimentalModeTime");
    int trackedTime = diffDouble("TrackedTime");

    engagementValue->setText(QLocale().toString((trackedTime > 0) ? (engagedTime * 100 / trackedTime) : 0) + "%");
    experimentalModeTimeValue->setText(QLocale().toString((trackedTime > 0) ? (experimentalTime * 100 / trackedTime) : 0) + "%");
    frogPilotMetersValue->setText(formatDistance(diffDouble("FrogPilotMeters")));
    trackedTimeValue->setText(formatTime(trackedTime));

/////////////////////////////////////////////////////
    // 油耗油資計算（比照 drive_stats.cc 的方式）
    int fuelConsumptionNow = params.getInt("Fuelconsumptionnow");
    int fuelConsumptionPre = params.getInt("Fuelconsumptionpre");
    int fuelCostsNow = params.getInt("Fuelcostsnow");
    int fuelCostsPre = params.getInt("Fuelcostspre");

    int fuelConsumptionDiff = fuelConsumptionNow - fuelConsumptionPre;
    int fuelCostsDiff = fuelCostsNow - fuelCostsPre;

    if (fuelConsumptionDiff > 0) {
      fuelConsumptionValue->setText(QString::number(fuelConsumptionDiff / 100.0, 'f', 1));
    } else {
      fuelConsumptionValue->setText("-");
    }

    if (fuelCostsDiff > 0) {
      avgFuelEconomyValue->setText(QString::number(fuelCostsDiff / 100.0, 'f', 1));
    } else {
      avgFuelEconomyValue->setText("-");
    }
/////////////////////////////////////////////////////
  }
}

void FrogPilotDriveSummary::hideEvent(QHideEvent *event) {
  emit panelClosed();
}

QWidget *FrogPilotDriveSummary::createStatBox(const QString &title, QLabel **valueLabel, QWidget *parent) {
  QWidget *box = new QWidget(parent);

/////////////////////////////////////////////////////
  QHBoxLayout *layout = new QHBoxLayout(box);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(20);

  QLabel *statTitleLabel = new QLabel(title, box);
  statTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  statTitleLabel->setStyleSheet(R"(
    QLabel {
      color: #AAAAAA;
      font-size: 40px;
      font-weight: normal;
    }
  )");

  QLabel *value = new QLabel("-", box);
  value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  value->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  value->setStyleSheet(R"(
    QLabel {
      color: #FFFFFF;
      font-size: 75px;
      font-weight: normal;
    }
  )");
/////////////////////////////////////////////////////

  *valueLabel = value;

  layout->addWidget(statTitleLabel);
  layout->addWidget(value);

  return box;
}
