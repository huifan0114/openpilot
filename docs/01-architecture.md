# 專案架構概述

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

## 專案架構概述

### 目錄結構

```
open251021/
├── selfdrive/          # 主要駕駛邏輯
│   ├── car/           # 車輛介面（各車廠 CAN 通訊）
│   ├── controls/      # 控制邏輯（橫向/縱向）
│   ├── modeld/        # 駕駛模型推理
│   └── ui/            # Qt-based UI
├── frogpilot/         # FrogPilot 特定功能
│   ├── controls/      # 控制增強
│   ├── assets/        # 模型和主題
│   ├── system/        # 系統級功能
│   ├── ui/            # UI 自訂
│   └── common/        # 共用工具
├── system/            # 系統服務
│   ├── manager/       # 進程管理器
│   ├── camerad/       # 相機守護進程
│   ├── loggerd/       # 日誌記錄
│   ├── hardware/      # 硬體抽象層
│   ├── timed.py       # 時間同步服務
│   └── athena/        # 雲端通訊
├── common/            # 共用函式庫
│   ├── params.*       # 參數系統
│   ├── timing.h       # 時間工具
│   ├── swaglog.*      # 日誌系統
│   └── realtime.py    # 即時控制工具
├── cereal/            # 訊息定義（Cap'n Proto）
├── opendbc/           # CAN 資料庫
├── panda/             # CAN 介面韌體
└── tools/             # 開發工具
```

### 技術棧

- **語言**: Python 3.11, C/C++, QML
- **建構**: SCons
- **測試**: pytest
- **訊息**: Cap'n Proto + msgq
- **UI**: Qt 5.15.2

---

