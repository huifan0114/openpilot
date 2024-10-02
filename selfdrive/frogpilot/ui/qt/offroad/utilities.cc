#include <filesystem>

#include "selfdrive/frogpilot/ui/qt/offroad/utilities.h"

UtilitiesPanel::UtilitiesPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  ButtonControl *flashPandaBtn = new ButtonControl(tr("刷新 Panda"), tr("刷新"), tr("如果遇到問題，請使用此按鈕刷新 Panda 裝置的韌體."));
  QObject::connect(flashPandaBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("您確定要刷新 Panda嗎 ?"), tr("刷新"), this)) {
      std::thread([=]() {
        device()->resetInteractiveTimeout(300);

        flashPandaBtn->setEnabled(false);
        flashPandaBtn->setValue(tr("刷新中..."));

        system("python3 /data/openpilot/panda/board/flash.py");
        system("python3 /data/openpilot/panda/board/recover.py");
        system("python3 /data/openpilot/panda/tests/reflash_internal_panda.py");

        flashPandaBtn->setValue(tr("已刷新!"));
        util::sleep_for(2000);
        flashPandaBtn->setValue(tr("重新啟動..."));
        util::sleep_for(2000);
        Hardware::reboot();
      }).detach();
    }
  });
  addItem(flashPandaBtn);

  forceStartedBtn = new FrogPilotButtonsControl(tr("強制啟動狀態"), tr("強制 openpilot 不管停止或行進中."), {tr("停止"), tr("行進中"), tr("關閉")}, true);
  QObject::connect(forceStartedBtn, &FrogPilotButtonsControl::buttonClicked, [=](int id) {
    if (id == 0) {
      paramsMemory.putBool("ForceOffroad", true);
      paramsMemory.putBool("ForceOnroad", false);
    } else if (id == 1) {
      paramsMemory.putBool("ForceOffroad", false);
      paramsMemory.putBool("ForceOnroad", true);
    } else if (id == 2) {
      paramsMemory.putBool("ForceOffroad", false);
      paramsMemory.putBool("ForceOnroad", false);
    }
    forceStartedBtn->setCheckedButton(id);
  });
  forceStartedBtn->setCheckedButton(2);
  addItem(forceStartedBtn);

  ButtonControl *resetTogglesBtn = new ButtonControl(tr("將切換重設為預設值"), tr("重置"), tr("將您的切換設定重設為預設設定."));
  QObject::connect(resetTogglesBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("您確定要完全重設所有切換設定嗎?"), tr("重置"), this)) {
      std::thread([=] {
        resetTogglesBtn->setEnabled(false);
        resetTogglesBtn->setValue(tr("重置中..."));

        std::system("rm -rf /persist/params");
        params.putBool("DoToggleReset", true);

        resetTogglesBtn->setValue(tr("已重置!"));
        util::sleep_for(2000);
        resetTogglesBtn->setValue(tr("重新啟動..."));
        util::sleep_for(2000);
        Hardware::reboot();
      }).detach();
    }
  });
  addItem(resetTogglesBtn);
}
