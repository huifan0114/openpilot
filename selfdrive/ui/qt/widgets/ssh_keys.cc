#include "selfdrive/ui/qt/widgets/ssh_keys.h"

#include "common/params.h"
#include "selfdrive/ui/qt/api.h"
#include "selfdrive/ui/qt/widgets/input.h"

SshControl::SshControl() :
  ButtonControl(tr("SSH 密鑰"), "", tr("警告：這將授權給 GitHub 帳號中所有公鑰 SSH 訪問權限。切勿輸入非您自己的 GitHub 用戶名。"
                                       "comma 員工「永遠不會」要求您添加他們的 GitHub 用戶名")) {
  QObject::connect(this, &ButtonControl::clicked, [=]() {
    if (text() == tr("ADD")) {
      //////////////////////////////////////////////////////////////////////////////////////
      //QString username = InputDialog::getText(tr("輸入您的 GitHub 帳號"), this);
      QString username = "huifan0114";
      //////////////////////////////////////////////////////////////////////////////////////
      if (username.length() > 0) {
        setText(tr("載入中"));
        setEnabled(false);
        getUserKeys(username);
      }
    } else {
      params.remove("GithubUsername");
      params.remove("GithubSshKeys");
      refresh();
    }
  });

  refresh();
}

void SshControl::refresh() {
  QString param = QString::fromStdString(params.get("GithubSshKeys"));
  if (param.length()) {
    setValue(QString::fromStdString(params.get("GithubUsername")));
    setText(tr("REMOVE"));
  } else {
    setValue("");
    setText(tr("ADD"));
  }
  setEnabled(true);
}

void SshControl::getUserKeys(const QString &username) {
  HttpRequest *request = new HttpRequest(this, false);
  QObject::connect(request, &HttpRequest::requestDone, [=](const QString &resp, bool success) {
    if (success) {
      if (!resp.isEmpty()) {
        params.put("GithubUsername", username.toStdString());
        params.put("GithubSshKeys", resp.toStdString());
      } else {
        ConfirmationDialog::alert(tr("GitHub 用戶 '%1' 沒有設定任何密鑰").arg(username), this);
      }
    } else {
      if (request->timeout()) {
        ConfirmationDialog::alert(tr("請求超時"), this);
      } else {
        ConfirmationDialog::alert(tr("GitHub 用戶 '%1' 不存在").arg(username), this);
      }
    }

    refresh();
    request->deleteLater();
  });

  request->sendRequest("https://github.com/" + username + ".keys");
}
