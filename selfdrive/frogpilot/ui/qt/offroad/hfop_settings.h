#pragma once

#include <set>

#include "selfdrive/frogpilot/ui/qt/offroad/frogpilot_settings.h"
// #include "selfdrive/ui/qt/offroad/settings.h"

class FrogPilotHFOPPanel : public FrogPilotListWidget {
  Q_OBJECT

public:
  explicit FrogPilotHFOPPanel(FrogPilotSettingsWindow *parent);

signals:
  void openParentToggle();

private:
  FrogPilotSettingsWindow *parent;
  void hideToggles();
  void showEvent(QShowEvent *event, const UIState &s);
  void updateState(const UIState &s);

  std::set<QString> FuelpriceKeys = {"Fuelcosts"};
  std::set<QString> VagSpeedKeys = {"VagSpeedFactor"};
  std::set<QString> AutoACCKeys = {"AutoACCspeed", "AutoACCCarAway", "AutoACCGreenLight", "TrafficModespeed"};
  std::set<QString> RoadKeys = {"RoadtypeProfile"};
  std::set<QString> NavspeedKeys = {"NavReminder", "speedoverreminder", "speedreminderreset"};
  std::set<QString> DooropenKeys= {"DriverdoorOpen", "CodriverdoorOpen","LpassengerdoorOpen","RpassengerdoorOpen","LuggagedoorOpen"};

  std::map<std::string, AbstractControl*> toggles;

  Params params;
  Params paramsMemory{"/dev/shm/params"};
  bool started = false;
  bool isRelease;
};
