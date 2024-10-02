#pragma once

#include <set>

#include "selfdrive/ui/qt/offroad/settings.h"
#include "selfdrive/ui/ui.h"

class HFOPControlsPanel : public FrogPilotListWidget {
  Q_OBJECT

public:
  explicit HFOPControlsPanel(SettingsWindow *parent);

signals:
  void openParentToggle();

private:
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
