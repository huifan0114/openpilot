# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 專案概述

這是 **FrogPilot** 專案 - openpilot 的社群驅動改良版本。FrogPilot 是一個自動駕駛輔助系統,支援 300+ 種車輛,包含許多實驗性功能和改進。

- **基礎**: 基於 openpilot (comma.ai 的開源駕駛輔助系統)
- **語言**: Python (主要), C/C++ (性能關鍵部分), QML (UI)
- **架構**: 分散式進程架構,使用訊息傳遞 (cereal/msgq)
- **主要分支**: FrogPilot (穩定), FrogPilot-Staging (測試), FrogPilot-Development (開發)

## 建構與開發環境

### 建構系統

專案使用 **SCons** 作為主要建構工具:

```bash
# 完整建構
scons -j$(nproc)

# 最小建構 (不包含工具和測試)
scons --minimal

# 特定選項
scons --asan          # 使用 Address Sanitizer
scons --ubsan         # 使用 UndefinedBehavior Sanitizer
scons --compile_db    # 生成 compile_commands.json
```

### 測試

使用 **pytest** 進行測試:

```bash
# 執行所有測試
pytest

# 跳過慢速測試
pytest -m "not slow"

# 執行特定模組測試
pytest selfdrive/car
pytest selfdrive/controls

# C++ 測試會透過 pytest-cpp 自動處理
```

### Python 環境

- Python 版本: **3.11**
- 套件管理: **Poetry** (pyproject.toml)
- 依賴安裝: `poetry install`
- 虛擬環境: 建議使用 venv 或 poetry shell

### 程式碼風格與檢查

```bash
# Linting (使用 ruff)
ruff check .

# 型別檢查 (使用 mypy)
mypy .

# Pre-commit hooks
pre-commit run --all-files
```

## 專案結構

### 核心目錄

- **`selfdrive/`**: 主要駕駛邏輯
  - `car/`: 車輛介面層 (各車廠的 CAN 通訊實作)
  - `controls/`: 控制邏輯 (橫向/縱向控制)
  - `modeld/`: 駕駛模型推理
  - `ui/`: Qt-based 使用者介面

- **`frogpilot/`**: FrogPilot 特定功能
  - `controls/`: FrogPilot 控制增強 (條件實驗模式、速度限制控制器等)
  - `assets/`: 模型和主題管理器
  - `system/`: 系統級功能 (統計、地圖更新等)
  - `ui/`: FrogPilot UI 自訂

- **`system/`**: 系統服務
  - `manager/`: 進程管理器 (啟動和監控所有服務)
  - `camerad/`: 相機守護進程
  - `loggerd/`: 日誌記錄服務
  - `hardware/`: 硬體抽象層

- **`common/`**: 共用工具和函式庫
  - 參數管理、訊息傳遞工具、數學函式等

- **子模組** (使用 Git submodules 或符號連結):
  - `cereal/`: 訊息定義 (使用 Cap'n Proto)
  - `opendbc/`: CAN 資料庫
  - `panda/`: CAN 介面韌體
  - `msgq_repo/`: 訊息佇列實作
  - `rednose_repo/`: 卡爾曼濾波器
  - `tinygrad_repo/`: 深度學習框架

### 重要檔案

- `SConstruct`: SCons 建構配置
- `pyproject.toml`: Python 依賴和工具配置
- `launch_openpilot.sh` → `launch_chffrplus.sh`: 主要啟動腳本
- `conftest.py`: pytest 配置和 fixtures

## 架構概念

### 訊息傳遞架構

系統使用 **cereal + msgq** 進行進程間通訊:

- 訊息定義在 `cereal/*.capnp`
- 使用 PubSub 模式
- 範例:
  ```python
  from cereal import messaging

  # Publisher
  pm = messaging.PubMaster(['topic'])
  pm.send('topic', message)

  # Subscriber
  sm = messaging.SubMaster(['topic'])
  sm.update()
  value = sm['topic']
  ```

### 進程管理

`system/manager/manager.py` 管理所有服務:
- 進程配置在 `system/manager/process_config.py`
- 自動重啟失效的進程
- 根據硬體和模式啟動不同的服務

### 車輛介面

`selfdrive/car/` 下每個車廠有獨立資料夾:
- `interface.py`: 主要介面類別
- `carstate.py`: 解析 CAN 訊息獲取車輛狀態
- `carcontroller.py`: 生成 CAN 控制指令
- `values.py`: 車輛參數定義
- `fingerprints.py`: 車輛識別指紋

### FrogPilot 特色功能

- **Always On Lateral (AOL)**: 持續車道保持
- **Conditional Experimental Mode (CEM)**: 智慧切換駕駛模式
- **Speed Limit Controller (SLC)**: 自動速限調整
- **自訂主題**: 視覺和聲音自訂
- **模型選擇**: 多種駕駛模型支援

## Windows 開發注意事項

由於這是在 Windows 環境開發:

1. **編碼問題**: BAT 和 PS1 檔案可能有中文編碼衝突
2. **Shell 腳本**: .sh 檔案需要在 WSL 或 Git Bash 中執行
3. **Python 執行**: 使用 venv 進行測試
4. **不要直接執行**: 請先在 venv 中測試,確認無誤後再進行編譯和製作安裝程式

## 常見開發任務

### 新增車輛支援

1. 在 `selfdrive/car/[manufacturer]/` 建立相關檔案
2. 更新 `fingerprints.py` 和 `values.py`
3. 實作 `interface.py`, `carstate.py`, `carcontroller.py`
4. 新增測試

### 修改控制邏輯

- 橫向控制: `selfdrive/controls/controlsd.py`
- 縱向控制: `selfdrive/controls/plannerd.py`
- FrogPilot 增強: `frogpilot/controls/frogpilot_planner.py`

### 更新 UI

- Qt/QML 檔案在 `selfdrive/ui/qt/`
- FrogPilot UI 在 `frogpilot/ui/`
- 使用 Qt 5.15.2

### 新增訊息類型

1. 編輯 `cereal/*.capnp` 定義
2. 重新建構以生成 Python bindings
3. 在程式碼中使用新訊息

## 除錯與日誌

- 日誌位置: `/data/media/0/realdata/` (裝置上)
- 使用 `cloudlog` 進行日誌記錄
- FrogPilot 錯誤日誌: 定義在 `frogpilot/common/frogpilot_variables.py`

## 開發知識庫使用指南

**重要**: 本專案有完整的結構化開發文件,請**務必優先使用**!

### 文件位置

所有開發文件位於 `docs/` 目錄:
- **主入口**: `docs/README-DEV.md` - 從這裡開始查找
- **主題文件**: `docs/01-*.md` 到 `docs/12-*.md` - 按功能分類
- **變更紀錄**: `docs/CHANGELOG.md`

### 工作流程

**收到任何開發問題時**:

1. **先查詢知識庫**
   - 讀取 `docs/README-DEV.md` 的快速查找表
   - 定位到對應的主題檔案 (例如: 定速問題 → `06-cruise-control.md`)
   - 完整讀取該檔案 (所有檔案都在 200-2000 行,可完整讀取)

2. **找到答案後直接使用**
   - 不需要重新搜尋程式碼
   - 文件中已包含檔案位置、行數、實作細節

3. **若知識庫沒有內容**
   - 深入研究程式碼
   - **立即更新對應的文件**
   - 記錄到 `docs/CHANGELOG.md`

### 快速查找範例

| 需求 | 查閱文件 |
|------|---------|
| 修改定速功能 | `docs/06-cruise-control.md` |
| 理解 ForceStops | `docs/06-cruise-control.md` - ForceStops 章節 |
| 速限控制 | `docs/08-speed-limit-nav.md` |
| 新增參數 | `docs/02-params-messaging.md` |
| Process 錯誤 | `docs/10-debug-tools.md` |
| 記錄 rlog | `docs/12-api-reference.md` |

### 更新文件規範

研究完成後必須:
- ✅ 更新對應主題的 MD 檔案
- ✅ 新增檔案位置索引 (檔案:行數)
- ✅ 提供實作範例
- ✅ 記錄到 CHANGELOG.md

### 文件優勢

- **避免重複研究**: 不用每次都從頭搜尋
- **完整可讀**: 每個檔案都可以一次讀完
- **結構清晰**: 按主題分類,快速定位
- **包含最新研究**: 所有重要發現都已記錄

**記住**: 知識庫是累積的,每次研究都讓它更完整!

## 重要提醒

1. **安全優先**: 這是自動駕駛系統,任何變更都可能影響安全
2. **充分測試**: 在實車測試前先進行模擬和單元測試
3. **遵循規範**: 使用 pre-commit hooks 確保程式碼品質
4. **不要破壞相容性**: FrogPilot 需要與不同硬體和車輛保持相容
5. **保持文件更新**: 重大變更應更新相關文件

## 參考資源

- openpilot 官方文件: https://docs.comma.ai
- FrogPilot Wiki: https://frogpilot.wiki.gg/
- FrogPilot Discord: https://discord.frogpilot.download
- GitHub Issues: https://github.com/FrogAi/FrogPilot/issues
