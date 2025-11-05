# FrogPilot / openpilot 開發知識庫

> **版本**: v2.8
> **最後更新**: 2025-11-02
> **目的**: 整合所有架構研究成果，作為開發時的快速參考

---

## 🎯 知識庫使用指南

### **重要工作流程**

```
收到新任務
  ↓
📖 先讀取對應的 MD 文件
  ├─ 知識庫已有內容 → 直接使用
  └─ 知識庫沒有內容 → 深入研究 → 更新知識庫
  ↓
執行開發工作
  ↓
📝 將研究成果補充到知識庫
```

### **核心原則**

1. **先查後找**: 任何研究前，先讀取對應文件，避免重複搜尋
2. **即時更新**: 每次深入研究後，立即更新對應章節
3. **結構化整理**: 保持章節清晰，便於快速查閱
4. **實作導向**: 提供可直接使用的範例代碼

---

## 📚 文件目錄

### 核心架構與系統

| 文件 | 內容 | 適用場景 |
|------|------|---------|
| [01-architecture.md](01-architecture.md) | 專案架構、目錄結構、技術棧 | 初次了解專案結構 |
| [02-params-messaging.md](02-params-messaging.md) | 參數系統 (PARAMS)、訊息傳遞 (cereal/msgq) | 需要存取參數或進程通訊 |
| [03-process-management.md](03-process-management.md) | 進程管理、時間系統、日誌記錄 | 新增進程或處理時間 |
| [04-power-management.md](04-power-management.md) | 電源管理、關機流程、電池管理 | 處理電源相關問題 |

### 控制系統

| 文件 | 內容 | 適用場景 |
|------|------|---------|
| [05-control-system.md](05-control-system.md) | 控制系統基礎、狀態機、事件系統 | 理解控制邏輯基礎 |
| [06-cruise-control.md](06-cruise-control.md) | 定速控制深度研究 (VCruiseHelper, FrogPilotVCruise, ForceStops) | 修改定速功能 |

### FrogPilot 功能

| 文件 | 內容 | 適用場景 |
|------|------|---------|
| [07-frogpilot-features.md](07-frogpilot-features.md) | 實驗模式、輕鬆模式、條件式實驗模式 (CEM) | 修改 FrogPilot 駕駛模式 |
| [08-speed-limit-nav.md](08-speed-limit-nav.md) | 速限控制、導航、地圖系統 | 處理速限或導航功能 |
| [09-ui-system.md](09-ui-system.md) | UI 系統、彎道圖標、位置計算 | 修改 UI 顯示 |

### 開發工具

| 文件 | 內容 | 適用場景 |
|------|------|---------|
| [10-debug-tools.md](10-debug-tools.md) | 除錯與診斷、TMux、Process 錯誤 | 遇到問題需要除錯 |
| [11-git-workflow.md](11-git-workflow.md) | Git 工作流程、分支管理、合併上游 | Git 操作 |
| [12-api-reference.md](12-api-reference.md) | API 快速參考、rlog 使用、實作案例 | 查詢 API 用法 |

### 變更紀錄

| 文件 | 內容 |
|------|------|
| [CHANGELOG.md](CHANGELOG.md) | 所有版本的變更紀錄 |

---

## 🔍 快速查找

### 我想要...

- **新增一個參數**: 看 [02-params-messaging.md](02-params-messaging.md) - PARAMS 系統
- **修改定速功能**: 看 [06-cruise-control.md](06-cruise-control.md) - VCruiseHelper 章節
- **理解 ForceStops**: 看 [06-cruise-control.md](06-cruise-control.md) - ForceStops 章節
- **修改速限控制**: 看 [08-speed-limit-nav.md](08-speed-limit-nav.md) - SLC 章節
- **新增進程**: 看 [03-process-management.md](03-process-management.md) - 進程管理章節
- **記錄變數到 rlog**: 看 [12-api-reference.md](12-api-reference.md) - rlog 章節
- **Process 錯誤**: 看 [10-debug-tools.md](10-debug-tools.md) - 除錯章節
- **修改 UI**: 看 [09-ui-system.md](09-ui-system.md) - UI 系統
- **實驗模式**: 看 [07-frogpilot-features.md](07-frogpilot-features.md) - 實驗模式章節

---

## 📝 文件維護指南

### 更新規範

每次更新知識庫時必須：
- ✅ 新增/更新對應文件的章節內容
- ✅ 提供檔案位置索引（如適用）
- ✅ 提供實作範例（如適用）
- ✅ 記錄到 [CHANGELOG.md](CHANGELOG.md)
- ✅ 更新這個 README 的版本號（如有重大變更）

### 檔案大小控制

- 每個 MD 檔案建議保持在 **500-1500 行**
- 超過 2000 行考慮再次分割
- 確保 Claude 可以完整讀取

---

## 🚀 下次研究主題建議

- [ ] 車輛介面（selfdrive/car）架構
- [ ] 模型推理流程（modeld）
- [ ] CAN 通訊機制
- [ ] 日誌上傳和下載流程
- [ ] 相機系統 (camerad)

---

## 📊 文件統計

| 文件 | 行數 (預估) | 狀態 |
|------|------------|------|
| 01-architecture.md | ~300 | ✅ |
| 02-params-messaging.md | ~500 | ✅ |
| 03-process-management.md | ~400 | ✅ |
| 04-power-management.md | ~300 | ✅ |
| 05-control-system.md | ~1200 | ✅ |
| 06-cruise-control.md | ~1500 | ✅ |
| 07-frogpilot-features.md | ~1000 | ✅ |
| 08-speed-limit-nav.md | ~800 | ✅ |
| 09-ui-system.md | ~400 | ✅ |
| 10-debug-tools.md | ~400 | ✅ |
| 11-git-workflow.md | ~300 | ✅ |
| 12-api-reference.md | ~600 | ✅ |

**總計**: ~7700 行 (原 7254 行，分割後更容易維護)

---

**提示**: 使用文字編輯器的全文搜尋功能可以快速在所有文件中查找關鍵字!
