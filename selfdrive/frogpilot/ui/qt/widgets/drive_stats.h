#pragma once

#include "selfdrive/ui/ui.h"

class DriveStats : public QFrame {
  Q_OBJECT

public:
  explicit DriveStats(QWidget *parent = 0);

private:
  inline QString getDistanceUnit() const { return metric ? tr("公里") : tr("英哩"); }

  struct StatsLabels {
    QLabel *routes;
    QLabel *distance;
    QLabel *distance_unit;
    QLabel *hours;
////////////////////////
    QLabel *Fuelconsumptionsweek;
    QLabel *Fuelcostsweek;
////////////////////////
  };

  void addStatsLayouts(const QString &title, StatsLabels &labels, bool FrogPilot = false);
  void showEvent(QShowEvent *event) override;
  void updateStats();
  void updateStatsForLabel(const QJsonObject &obj, StatsLabels &labels);
  void updateFrogPilotStats(const QJsonObject &obj, StatsLabels &labels);

  Params params;
  Params paramsTracking{"/persist/tracking"};

  bool metric;

  QJsonDocument stats;
////////////////////////
  bool fuelpriceProfile;
////////////////////////

  StatsLabels all, week, frogPilot;

private slots:
  void parseResponse(const QString &response, bool success);
};
