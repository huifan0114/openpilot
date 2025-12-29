# 🚀 快速開始 - 3 分鐘完成預編譯

## 方案選擇

### 如果您有 Self-hosted Runner (Comma 設備)
👉 使用原本的 `compile_frogpilot.yaml`

### 如果您沒有 Self-hosted Runner (大多數用戶)
👉 使用新的 `compile_standard.yaml` (推薦)

---

## 📝 步驟 1: 推送程式碼 (2 分鐘)

在 PowerShell 中執行:

```powershell
cd d:\FORK\huifan0114

# 加入所有變更
git add .

# 提交
git commit -m "Add precompilation support for faster boot"

# 推送到 GitHub
git push origin 7
```

---

## 🎯 步驟 2: 執行 GitHub Actions (1 分鐘設定 + 10-20 分鐘編譯)

### 方法 A: 網頁介面 (最簡單)

1. **打開瀏覽器,前往:**
   ```
   https://github.com/huifan0114/openpilot/actions
   ```

2. **選擇工作流:**
   - 點擊左側的 **"Compile FrogPilot (Standard Runner)"**

3. **執行工作流:**
   - 點擊右上角 **"Run workflow"**
   - 選擇分支: `7`
   - 輸入發布分支名稱: `7-precompiled`
   - 點擊綠色的 **"Run workflow"** 按鈕

4. **等待完成 (10-20 分鐘)**
   - 會看到一個黃色圓圈 🟡 (執行中)
   - 完成後變成綠色勾勾 ✅
   - 失敗會顯示紅色叉叉 ❌

### 方法 B: 使用命令列

```powershell
# 安裝 GitHub CLI (如果還沒有)
winget install GitHub.cli

# 登入
gh auth login

# 執行工作流
gh workflow run "compile_standard.yaml" `
  --ref 7 `
  -f publish_branch="7-precompiled"

# 查看狀態
gh run watch
```

---

## 📥 步驟 3: 安裝到設備

### 方法 A: 在 Comma 設備上操作 (推薦)

```bash
# SSH 連線到設備
ssh comma@192.168.x.x

# 切換到 openpilot 目錄
cd /data/openpilot

# 加入您的遠端儲存庫
git remote add myrepo https://github.com/huifan0114/openpilot.git

# 拉取並切換到預編譯版本
git fetch myrepo 7-precompiled
git checkout 7-precompiled

# 重新開機
sudo reboot
```

### 方法 B: 使用 Comma 介面

1. 在設備上開啟 **Settings** → **Software**
2. 點擊 **Custom Software**
3. 輸入:
   - URL: `https://github.com/huifan0114/openpilot`
   - Branch: `7-precompiled`
4. 等待安裝完成
5. 重新開機

---

## ✅ 驗證安裝

重新開機後,在設備上檢查:

```bash
# 確認 prebuilt 標記存在
ls -la /data/openpilot/prebuilt

# 查看編譯資訊
cat /data/openpilot/PRECOMPILED_INFO.txt

# 查看 Python 字節碼數量
find /data/openpilot -name "*.opt-2.pyc" | wc -l
# 應該顯示幾百到幾千個檔案

# 測量開機時間
# 從重新開機到看到介面,應該在 15-30 秒內
```

---

## 🎉 完成!

您的系統現在已經:
- ✅ 使用預編譯版本
- ✅ 開機速度提升 5-10 倍
- ✅ 從 2-5 分鐘縮短到 15-30 秒

---

## ❓ 常見問題

### Q: GitHub Actions 執行失敗怎麼辦?

**A:** 點擊失敗的工作流查看錯誤訊息:

1. **依賴安裝失敗** - 通常會自動重試
2. **編譯失敗** - 檢查程式碼是否有語法錯誤
3. **權限不足** - 前往 Settings → Actions → General 啟用寫入權限

### Q: 設備上看不到新分支?

**A:** 確保網路連線正常:

```bash
# 測試連線
ping github.com

# 強制更新
git remote update
git fetch myrepo --all
git branch -r | grep myrepo
```

### Q: 開機還是很慢?

**A:** 檢查幾個項目:

```bash
# 1. prebuilt 檔案是否存在
test -f /data/openpilot/prebuilt && echo "✅ prebuilt 存在" || echo "❌ prebuilt 不存在"

# 2. 字節碼是否存在
[ $(find /data/openpilot -name "*.opt-2.pyc" | wc -l) -gt 100 ] && echo "✅ 字節碼充足" || echo "❌ 字節碼不足"

# 3. 查看啟動日誌
tail -100 /data/media/0/log/launch_log_$(ls -t /data/media/0/log/launch_log_* | head -1 | xargs basename)
```

### Q: 如何更新預編譯版本?

**A:** 當您修改程式碼後:

1. 推送變更到 GitHub
2. 重新執行 GitHub Actions
3. 在設備上拉取最新版本:
   ```bash
   cd /data/openpilot
   git pull myrepo 7-precompiled
   sudo reboot
   ```

### Q: 如何恢復到原始版本?

**A:** 切換回原始分支:

```bash
cd /data/openpilot
git checkout 7
git pull origin 7
sudo reboot
```

---

## 📚 更多資訊

- [完整編譯指南](COMPILE_GUIDE.md) - 詳細的技術說明
- [預編譯技術文件](../PRECOMPILE_README.md) - 原理和優化細節
- [GitHub Actions 文件](https://docs.github.com/en/actions) - 官方文件

---

## 💡 提示

1. **自動化**: 可以設定每次推送時自動編譯
2. **版本管理**: 使用不同分支管理不同版本
3. **定期更新**: 同步上游的 FrogPilot 更新
4. **備份**: 保留原始版本以便隨時切換

需要幫助? 檢查日誌或開 Issue!
