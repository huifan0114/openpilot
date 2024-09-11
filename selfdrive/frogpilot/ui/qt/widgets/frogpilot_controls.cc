#include "selfdrive/ui/ui.h"

Params paramsMemory{"/dev/shm/params"};

std::atomic<int> callCounter(0);
void updateFrogPilotToggles() {
  int currentCall = ++callCounter;

  std::thread([currentCall]() {
    paramsMemory.putBool("FrogPilotTogglesUpdated", true);
    util::sleep_for(1000);

    if (currentCall == callCounter.load()) {
      paramsMemory.putBool("FrogPilotTogglesUpdated", false);
    }
  }).detach();
}

QColor loadThemeColors(const QString &colorKey) {
  QFile file("../frogpilot/assets/active_theme/colors/colors.json");
  if (!file.open(QIODevice::ReadOnly)) return QColor();

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject()) return QColor();

  QJsonObject colorObj = doc.object().value(colorKey).toObject();
  return QColor(
    colorObj["red"].toInt(),
    colorObj["green"].toInt(),
    colorObj["blue"].toInt(),
    colorObj["alpha"].toInt()
  );
}

bool FrogPilotConfirmationDialog::toggle(const QString &prompt_text, const QString &confirm_text, QWidget *parent) {
  ConfirmationDialog d(prompt_text, confirm_text, tr("Reboot Later"), false, parent);
  return d.exec();
}

bool FrogPilotConfirmationDialog::toggleAlert(const QString &prompt_text, const QString &button_text, QWidget *parent) {
  ConfirmationDialog d(prompt_text, button_text, "", false, parent);
  return d.exec();
}

bool FrogPilotConfirmationDialog::yesorno(const QString &prompt_text, QWidget *parent) {
  ConfirmationDialog d(prompt_text, tr("Yes"), tr("No"), false, parent);
  return d.exec();
}
