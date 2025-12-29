# 使用 GitHub Actions 編譯預編譯版本完整指南

## 📋 前提條件

您已完成:
- ✅ Fork 專案到: `https://github.com/huifan0114/openpilot.git`
- ✅ 本地程式碼已更新(包含預編譯功能)
- ✅ 目前分支: `7`

## 🚀 完整操作步驟

### **步驟 1: 推送程式碼到 GitHub**

```powershell
# 在您的 Windows 電腦上執行
cd d:\FORK\huifan0114

# 檢查修改的檔案
git status

# 加入所有新建和修改的檔案
git add system/manager/precompile_python.py
git add system/manager/build.py
git add precompile_all.sh
git add .github/workflows/compile_frogpilot.yaml
git add PRECOMPILE_README.md

# 提交變更
git commit -m "Add precompilation support to speed up boot time

- Add precompile_python.py for Python bytecode compilation
- Update build.py to auto-compile Python modules
- Add precompile_all.sh for manual compilation
- Update GitHub Actions to preserve optimized bytecode
- Add documentation for precompilation usage"

# 推送到您的 GitHub
git push origin 7
```

### **步驟 2: 設定 GitHub Actions (如果需要)**

#### 2.1 檢查 Actions 是否啟用

1. 前往您的 GitHub 儲存庫
2. 點擊 **"Actions"** 標籤
3. 如果看到提示需要啟用,點擊 **"I understand my workflows, go ahead and enable them"**

#### 2.2 設定 Self-hosted Runner (如果有實體設備)

這個工作流使用 `self-hosted` runner (c3 或 c3x),這通常是 Comma 設備。

**如果您沒有 self-hosted runner:**

需要修改工作流使用 GitHub 提供的 runner。讓我幫您建立一個修改版本。

### **步驟 3: 執行 GitHub Actions 工作流**

#### 方法 A: 使用 GitHub 網頁介面 (推薦)

1. **前往 Actions 頁面**
   ```
   https://github.com/huifan0114/openpilot/actions
   ```

2. **選擇工作流**
   - 在左側選單點擊 **"Compile FrogPilot"**

3. **手動觸發工作流**
   - 點擊右上角的 **"Run workflow"** 下拉選單
   - 選擇您的分支 (例如: `7`)
   - 設定參數:
     - **not_vetted**: 如果是測試版本,勾選
     - **publish_custom_branch**: 輸入 `7-precompiled` (發布到自訂分支)
     - **runner**: 選擇 `c3` 或 `c3x`
   - 點擊綠色的 **"Run workflow"** 按鈕

4. **監控執行進度**
   - 工作流會出現在列表中
   - 點擊進入查看詳細執行日誌
   - 等待完成 (通常需要 10-30 分鐘)

#### 方法 B: 使用 GitHub CLI (進階)

```powershell
# 安裝 GitHub CLI (如果還沒安裝)
# winget install GitHub.cli

# 登入 GitHub
gh auth login

# 觸發工作流
gh workflow run "Compile FrogPilot" `
  --ref 7 `
  -f publish_custom_branch="7-precompiled" `
  -f runner="c3"

# 查看執行狀態
gh run list --workflow="Compile FrogPilot"

# 查看執行日誌
gh run watch
```

### **步驟 4: 等待編譯完成**

工作流會執行以下操作:

1. ✅ 取得程式碼
2. ✅ 編譯 C/C++ 程式碼 (SCons)
3. ✅ 預編譯 Python 模組 (自動執行)
4. ✅ 清理開發檔案
5. ✅ 建立 `prebuilt` 標記
6. ✅ 推送到指定分支

完成後,編譯好的版本會推送到:
```
https://github.com/huifan0114/openpilot/tree/7-precompiled
```

### **步驟 5: 下載編譯好的版本到設備**

#### 方法 A: 在 Comma 設備上直接拉取 (推薦)

```bash
# SSH 連線到您的 Comma 設備
ssh comma@192.168.x.x

# 切換到 openpilot 目錄
cd /data/openpilot

# 備份當前版本 (可選)
cp -r /data/openpilot /data/openpilot.backup

# 更新到編譯版本
git remote add huifan https://github.com/huifan0114/openpilot.git
git fetch huifan 7-precompiled
git checkout huifan/7-precompiled

# 或者直接切換
git checkout 7-precompiled
git pull huifan 7-precompiled

# 重新開機
sudo reboot
```

#### 方法 B: 使用 Comma 設定介面

1. 在 Comma 設備上進入 **Settings**
2. 選擇 **Software**
3. 點擊 **Uninstall openpilot** (如果需要全新安裝)
4. 選擇 **Custom Software**
5. 輸入 URL:
   ```
   https://github.com/huifan0114/openpilot
   ```
6. 輸入分支名稱: `7-precompiled`
7. 安裝完成後重新開機

#### 方法 C: 下載後手動傳輸

```powershell
# 在 Windows 上下載編譯版本
git clone --depth 1 --branch 7-precompiled https://github.com/huifan0114/openpilot.git openpilot-precompiled

# 使用 SCP 傳輸到設備
scp -r openpilot-precompiled/* comma@192.168.x.x:/data/openpilot/
```

### **步驟 6: 驗證預編譯版本**

重新開機後,在設備上檢查:

```bash
# 檢查 prebuilt 標記是否存在
ls -la /data/openpilot/prebuilt

# 檢查 Python 字節碼
find /data/openpilot -name "*.opt-2.pyc" | head -20

# 查看開機日誌
tail -f /data/media/0/log/launch_log_*.txt

# 測量開機時間
# 從開機到看到 UI 的時間應該在 15-30 秒內
```

## 📊 預期結果

編譯成功後,您應該看到:

```
7-precompiled/
├── prebuilt                          # ✅ 預編譯標記
├── system/
│   └── manager/
│       └── __pycache__/
│           └── *.opt-2.pyc           # ✅ Python 字節碼
├── selfdrive/
│   └── __pycache__/
│       └── *.opt-2.pyc
└── [其他目錄的字節碼檔案]
```

## ⚠️ 注意事項

### 如果沒有 Self-hosted Runner

GitHub Actions 預設需要 `self-hosted` runner (Comma 設備)。如果您沒有:

**選項 1: 使用 GitHub Runner (需修改工作流)**

需要修改 `.github/workflows/compile_frogpilot.yaml`,將:
```yaml
runs-on: [self-hosted, "${{ inputs.runner }}"]
```
改為:
```yaml
runs-on: ubuntu-latest
```

但這樣可能無法完全模擬 Comma 設備的編譯環境。

**選項 2: 在本地 Linux 環境編譯**

如果您有 Ubuntu/Debian 系統或 WSL:

```bash
# 安裝依賴
sudo apt-get update
sudo apt-get install -y build-essential python3 python3-pip git scons

# 執行預編譯
cd /path/to/openpilot
chmod +x precompile_all.sh
./precompile_all.sh

# 手動推送到 GitHub
git add -f .
git commit -m "Precompiled build"
git push origin 7-precompiled
```

## 🔧 故障排除

### Actions 執行失敗

1. **檢查 Runner 狀態**
   - Settings → Actions → Runners
   - 確保 runner 在線

2. **查看錯誤日誌**
   - 點擊失敗的工作流
   - 展開失敗的步驟
   - 根據錯誤訊息修正

3. **權限問題**
   - Settings → Actions → General
   - 確保 "Allow all actions and reusable workflows" 已啟用
   - 確保 "Read and write permissions" 已啟用

### 設備下載失敗

1. **檢查網路連線**
   ```bash
   ping github.com
   ```

2. **使用 SSH 協定**
   ```bash
   git remote set-url huifan git@github.com:huifan0114/openpilot.git
   ```

3. **使用淺層 clone**
   ```bash
   git clone --depth 1 --branch 7-precompiled https://github.com/huifan0114/openpilot.git
   ```

## 📚 相關文件

- [PRECOMPILE_README.md](../PRECOMPILE_README.md) - 預編譯技術說明
- [GitHub Actions 文件](https://docs.github.com/en/actions)
- [Git 使用指南](https://git-scm.com/doc)

## 💡 提示

1. **開發與生產分離**
   - 開發分支: `7` (原始碼)
   - 編譯分支: `7-precompiled` (預編譯版本)

2. **自動化流程**
   - 可以在每次推送時自動觸發編譯
   - 修改工作流的 `on:` 部分加入 `push:` 觸發器

3. **版本管理**
   - 每次編譯建議標記版本號
   - 使用 Git tags: `git tag v1.0.0-precompiled`

4. **定期更新**
   - 當修改程式碼後,重新執行工作流
   - 保持編譯版本與開發版本同步
