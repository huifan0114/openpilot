#pragma once

#include <set>

#include "selfdrive/frogpilot/ui/qt/offroad/frogpilot_settings.h"

class FrogPilotHFOPPanel : public FrogPilotListWidget {
  Q_OBJECT

public:
  explicit FrogPilotHFOPPanel(FrogPilotSettingsWindow *parent);

signals:
  void openParentToggle();

private:
  bool started ;

  void hideToggles();
  void showToggles(const std::set<QString> &keys);
  void updateState(const UIState &s);

  std::map<QString, AbstractControl*> toggles;

  std::set<QString> FuelpriceKeys = {"Fuelcosts"};
  std::set<QString> TrafficModeKeys = {"TrafficModespeed"};
  std::set<QString> VagSpeedKeys = {"VagSpeedFactor"};
  std::set<QString> AutoACCKeys = {"AutoACCspeed", "AutoACCCarAway", "AutoACCGreenLight"};
  std::set<QString> RoadKeys = {"AutoRoadtype","RoadtypeProfile"};
  std::set<QString> NavspeedKeys = {"NavReminder", "speedoverreminder", "speedreminderreset"};
  std::set<QString> DooropenKeys= {"DriverdoorOpen", "CodriverdoorOpen","LpassengerdoorOpen","RpassengerdoorOpen","LuggagedoorOpen"};

  FrogPilotSettingsWindow *parent;

  Params params;
  Params params_memory{"/dev/shm/params"};


};
