#include "selfdrive/ui/qt/request_repeater.h"

#include "frogpilot/ui/qt/widgets/drive_stats.h"

static QLabel *newLabel(const QString &text, const QString &type) {
  QLabel *label = new QLabel(text);
  label->setProperty("type", type);
  return label;
}

DriveStats::DriveStats(QWidget *parent) : QFrame(parent) {
  isMetric = params.getBool("IsMetric");
  konik = useKonikServer();
////////////////////////////////////////////////////////
  fuelpriceProfile = params.getBool("Fuelprice");
////////////////////////////////////////////////////////
  QVBoxLayout *main_layout = new QVBoxLayout(this);
////////////////////////////////////////////////////////
  main_layout->setContentsMargins(20, 20, 20, 20);
////////////////////////////////////////////////////////

  addStatsLayouts(tr(konik ? "全部旅程 (KONIK)" : "全部旅程"), all);
  addStatsLayouts(tr(konik ? "過去一週 (KONIK)" : "過去一週"), week);
  addStatsLayouts(tr("FROGPILOT"), frogPilot, true);

  std::optional<QString> dongleId = getDongleId();
  if (dongleId.has_value()) {
    QString url = CommaApi::BASE_URL + "/v1.1/devices/" + dongleId.value() + "/stats";
    RequestRepeater *repeater = new RequestRepeater(this, url, "ApiCache_DriveStats", 30);
    QObject::connect(repeater, &RequestRepeater::requestDone, this, &DriveStats::parseResponse);
  }

  setStyleSheet(R"(
    DriveStats {
      background-color: #333333;
      border-radius: 10px;
    }

    QLabel[type="frogpilot_title"] { font-size: 50px; font-weight: 500; color: #178643; }
    QLabel[type="number"] { font-size: 65px; font-weight: 400; }
    QLabel[type="title"] { font-size: 50px; font-weight: 500; }
    QLabel[type="unit"] { font-size: 50px; font-weight: 300; color: #A0A0A0; }
  )");
}

void DriveStats::showEvent(QShowEvent *event) {
  isMetric = params.getBool("IsMetric");

  updateStats();
}

void DriveStats::addStatsLayouts(const QString &title, StatsLabels &labels, bool FrogPilot) {
  QGridLayout *grid_layout = new QGridLayout;
  grid_layout->setVerticalSpacing(10);
  grid_layout->setContentsMargins(0, 10, 0, 10);

  int row = 0;
  grid_layout->addWidget(newLabel(title, FrogPilot ? "frogpilot_title" : "title"), row++, 0, 1, 3);
  grid_layout->addItem(new QSpacerItem(0, 10), row++, 0, 1, 1);

  grid_layout->addWidget(labels.routes = newLabel("0", "number"), row, 0, Qt::AlignLeft);
  grid_layout->addWidget(labels.distance = newLabel("0", "number"), row, 1, Qt::AlignLeft);
  grid_layout->addWidget(labels.hours = newLabel("0", "number"), row, 2, Qt::AlignLeft);
////////////////////////////////////////////////////////
    if (fuelpriceProfile){
      if (FrogPilot) {
        grid_layout->addWidget(labels.Fuelconsumptionsweek = newLabel("0", "number"), row, 3, Qt::AlignLeft);
        grid_layout->addWidget(labels.Fuelcostsweek = newLabel("0", "number"), row, 4, Qt::AlignLeft);
      }
    }
////////////////////////////////////////////////////////

  grid_layout->addWidget(newLabel(tr("行程"), "unit"), row + 1, 0, Qt::AlignLeft);
  grid_layout->addWidget(labels.distance_unit = newLabel(isMetric ? tr("公里") : tr("英里"), "unit"), row + 1, 1, Qt::AlignLeft);
  grid_layout->addWidget(newLabel(tr("小時"), "unit"), row + 1, 2, Qt::AlignLeft);
////////////////////////////////////////////////////////
    if (fuelpriceProfile){
      if (FrogPilot) {
        grid_layout->addWidget(newLabel(tr("油耗"), "unit"), row + 1, 3, Qt::AlignLeft);
        grid_layout->addWidget(newLabel(tr("油資"), "unit"), row + 1, 4, Qt::AlignLeft);
      }
    }
////////////////////////////////////////////////////////

  QVBoxLayout *main_layout = static_cast<QVBoxLayout *>(layout());
  main_layout->addLayout(grid_layout);
  main_layout->addStretch(1);
}

void DriveStats::parseResponse(const QString &response, bool success) {
  if (!success) {
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(response.trimmed().toUtf8());
  if (doc.isNull()) {
    qDebug() << "JSON Parse failed on getting past drives statistics";
    return;
  }
  stats = doc;
  updateStats();
}

void DriveStats::updateStatsForLabel(const QJsonObject &obj, StatsLabels &labels) {
  labels.distance->setText(QString::number(int(obj["distance"].toDouble() * (isMetric ? MILE_TO_KM : 1))));
  labels.distance_unit->setText(isMetric ? tr("公里") : tr("英里"));
  labels.hours->setText(QString::number((int)(obj["minutes"].toDouble() / 60)));
  labels.routes->setText(QString::number((int)obj["routes"].toDouble()));
}

void DriveStats::updateFrogPilotStatsForLabel(StatsLabels &labels) {
  QJsonObject frogpilot_stats = QJsonDocument::fromJson(QByteArray::fromStdString(params.get("FrogPilotStats"))).object();

  labels.distance->setText(QString::number(int(frogpilot_stats.value("FrogPilotMeters").toDouble() * (isMetric ? 0.001 : METER_TO_MILE))));
  labels.distance_unit->setText(isMetric ? tr("公里") : tr("英里"));
  labels.hours->setText(QString::number(int(frogpilot_stats.value("FrogPilotSeconds").toDouble() / (60 * 60))));
  labels.routes->setText(QString::number(frogpilot_stats.value("FrogPilotDrives").toInt()));

  // 更新油耗油資數據
////////////////////////////////////////////////////////
    if (fuelpriceProfile) {
      labels.Fuelconsumptionsweek->setText(QString::number(params.getFloat("Fuelconsumptionweek") / 100, 'f', 1));
      labels.Fuelcostsweek->setText(QString::number(params.getFloat("Fuelcostsweek") / 100, 'f', 1));
    }
////////////////////////////////////////////////////////
}

void DriveStats::updateStats() {
  QJsonObject json = stats.object();

  // 油耗油資累積處理
////////////////////////////////////////////////////////
  if (fuelpriceProfile) {
    int Fuelconsumptionnow = params.getInt("Fuelconsumptionnow");
    int Fuelconsumptionweek = params.getInt("Fuelconsumptionweek");
    if (Fuelconsumptionnow !=0 ){
      Fuelconsumptionweek = Fuelconsumptionweek + Fuelconsumptionnow;
      params.putInt("Fuelconsumptionweek", Fuelconsumptionweek );
      params.putInt("Fuelconsumptionnow", 0);
      params.putInt("Fuelconsumptionpre", 0);
    }

    int Fuelcostsnow = params.getInt("Fuelcostsnow");
    int Fuelcostsweek = params.getInt("Fuelcostsweek");
    if (Fuelcostsnow !=0 ){
      Fuelcostsweek = Fuelcostsweek + Fuelcostsnow;
      params.putInt("Fuelcostsweek", Fuelcostsweek);
      params.putInt("Fuelcostsnow", 0);
      params.putInt("Fuelcostspre", 0);
    }
  }
////////////////////////////////////////////////////////

  updateStatsForLabel(json["all"].toObject(), all);
  updateStatsForLabel(json["week"].toObject(), week);
  updateFrogPilotStatsForLabel(frogPilot);

  params.putIntNonBlocking(konik ? "KonikMinutes" : "openpilotMinutes", json["all"].toObject()["minutes"].toDouble());
}
