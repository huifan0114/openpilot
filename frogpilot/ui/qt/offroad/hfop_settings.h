#pragma once

#include <set>

#include "frogpilot/ui/qt/offroad/frogpilot_settings.h"

class FrogPilotHFOPPanel : public FrogPilotListWidget {
  Q_OBJECT

public:
  explicit FrogPilotHFOPPanel(FrogPilotSettingsWindow *parent);

signals:
  void openSubPanel();
  void openSubSubPanel();

protected:
  void showEvent(QShowEvent *event) override;

private:
  // void updateMetric(bool metric, bool bootRun);
  void updateToggles();

  bool developerUIOpen;
  bool forceOpenDescriptions;

  std::map<QString, AbstractControl*> toggles;

  QSet<QString> AutoACCKeys = {"AutoACCspeed", "AutoACCCarAway", "AutoACCGreenLight"};
  QSet<QString> RoadKeys = {"AutoRoadtype", "RoadtypeProfile"};
  QSet<QString> TrafficModeKeys = {"TrafficModespeed"};
  QSet<QString> VagSpeedKeys = {"VagSpeedFactor"};
  QSet<QString> NavspeedKeys = {"speedoverreminder", "speedreminderreset"};
  QSet<QString> DooropenKeys = {"DriverdoorOpen", "CodriverdoorOpen", "LpassengerdoorOpen", "RpassengerdoorOpen", "LuggagedoorOpen"};
  QSet<QString> FuelpriceKeys = {"Fuelcosts"};

  QSet<QString> parentKeys;

  FrogPilotButtonToggleControl *borderMetricsButton;

  FrogPilotSettingsWindow *parent;

  Params params;

  QJsonObject frogpilotToggleLevels;
};
