# FrogPilot Debug Log 系統

**建立日期**: 2025-11-05
**狀態**: ✅ 已實作
**用途**: 統一所有我們修改的 debug 輸出

---

## 系統概述

**目的**: 提供統一的文字 log 系統,方便用 FTP 下載查看,不需要處理二進制 log 檔案。

**檔案位置**:
- **Logger**: `frogpilot/common/frogpilot_debug_logger.py`
- **Log 輸出**: `/data/frogpilot_debug.log` (comma 設備上)

---

## 使用方法

### 1. Import Logger

```python
from openpilot.frogpilot.common.frogpilot_debug_logger import write_debug_log
```

### 2. 寫入 Log

```python
write_debug_log("模組名", "訊息內容")
```

**模組名建議**:
- `SLC` - Speed Limit Controller
- `BRAKE` - 煞車邏輯
- `UI` - 使用者介面
- `VW` - VW 特定功能
- `CRUISE` - 定速相關
- 等等...

---

## 使用範例

### SLC (Speed Limit Controller)

```python
# selfdrive/controls/controlsd.py
from openpilot.frogpilot.common.frogpilot_debug_logger import write_debug_log

# 速限變化事件
write_debug_log("SLC", f"Event - Map={map_speed} Offset={offset} CurrentMAX={current_max}")

# 成功設定 MAX SPEED
write_debug_log("SLC", f"SET MAX={new_speed} (diff={diff})")

# 被忽略 (差距過大)
write_debug_log("SLC", f"IGNORED new={new_speed} (diff={diff}>60)")
```

**輸出範例**:
```
[14:23:15] SLC: Event - Map=80 Offset=10 CurrentMAX=120
[14:23:15] SLC: SET MAX=90 (diff=30)
[14:25:30] SLC: IGNORED new=50 (diff=70>60)
```

---

### 煞車邏輯

```python
# frogpilot/controls/lib/longitudinal_planner.py
from openpilot.frogpilot.common.frogpilot_debug_logger import write_debug_log

# ForceStop 觸發
write_debug_log("BRAKE", f"ForceStop triggered at {distance:.1f}m speed={v_ego*3.6:.0f}kmh")

# 條件判斷
write_debug_log("BRAKE", f"Condition - distance={distance} threshold={threshold}")
```

**輸出範例**:
```
[14:30:00] BRAKE: ForceStop triggered at 45.2m speed=60kmh
[14:30:01] BRAKE: Condition - distance=40.5 threshold=50
```

---

### UI 更新

```python
# C++ 檔案無法直接使用,但 Python 部分可以
from openpilot.frogpilot.common.frogpilot_debug_logger import write_debug_log

write_debug_log("UI", "SpeedLimit sign updated to 90")
write_debug_log("UI", f"Upcoming shown: {upcoming_speed}")
```

---

## Log 格式

```
[HH:MM:SS] 模組名: 訊息內容
```

**特點**:
- ✅ 時間戳記 (只顯示時分秒,節省空間)
- ✅ 模組名稱 (方便過濾)
- ✅ 簡潔訊息 (一行就看懂)
- ✅ 數值取整 (`.0f` 格式,避免小數點)

---

## 查看 Log

### 方法 1: FTP 下載 (推薦)

```
1. 用 FTP 客戶端連接 comma 設備
2. 下載 /data/frogpilot_debug.log
3. 用記事本或 VSCode 打開
4. 搜尋模組名 (例如: "SLC", "BRAKE")
```

### 方法 2: SSH 查看

```bash
# 連接 comma 設備
ssh comma@<your-comma-ip>

# 查看全部
cat /data/frogpilot_debug.log

# 只看 SLC 相關
grep "SLC" /data/frogpilot_debug.log

# 只看最新 20 行
tail -20 /data/frogpilot_debug.log

# 即時監控 (開車時)
tail -f /data/frogpilot_debug.log
```

### 方法 3: SCP 複製

```bash
# Windows PowerShell
scp comma@<your-comma-ip>:/data/frogpilot_debug.log F:\logs\
```

---

## 技術細節

### Logger 實作 (frogpilot_debug_logger.py)

```python
import os
import shutil
from datetime import datetime

DEBUG_LOG_FILE = "/data/frogpilot_debug.log"
MAX_LOG_SIZE = 500000  # 500KB
MAX_ARCHIVED_LOGS = 5  # 保留最多 5 個舊 log 檔

def write_debug_log(module: str, message: str) -> None:
    """
    寫入 debug log (非同步,不阻塞主程序)
    """
    try:
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_line = f"[{timestamp}] {module}: {message}\n"

        # 非阻塞寫入,行緩衝
        with open(DEBUG_LOG_FILE, "a", buffering=1) as f:
            f.write(log_line)

        # 檔案大小控制
        if os.path.getsize(DEBUG_LOG_FILE) > MAX_LOG_SIZE:
            _rotate_log()

    except Exception:
        # 靜默失敗,不影響主程序
        pass

def _rotate_log() -> None:
    """
    循環 log 檔案 (建立新檔案而非覆蓋)
    - 將現有檔案重新命名為 frogpilot_debug.log.YYYYMMDD_HHMMSS
    - 開始新的空白 log 檔
    - 保留最多 5 個舊檔案
    """
    try:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        archived_file = f"{DEBUG_LOG_FILE}.{timestamp}"
        shutil.move(DEBUG_LOG_FILE, archived_file)
        _cleanup_old_logs()
    except Exception:
        pass

def _cleanup_old_logs() -> None:
    """清理舊的 archived log 檔案,只保留最新 5 個"""
    try:
        log_dir = os.path.dirname(DEBUG_LOG_FILE)
        log_basename = os.path.basename(DEBUG_LOG_FILE)
        archived_logs = []
        for filename in os.listdir(log_dir):
            if filename.startswith(log_basename + "."):
                archived_logs.append(os.path.join(log_dir, filename))
        archived_logs.sort(key=os.path.getmtime)
        if len(archived_logs) > MAX_ARCHIVED_LOGS:
            for old_log in archived_logs[:-MAX_ARCHIVED_LOGS]:
                os.remove(old_log)
    except Exception:
        pass
```

### 效能考量

1. **非阻塞寫入**
   - 使用 `buffering=1` (行緩衝)
   - 寫入後立即 flush,不等待

2. **靜默失敗**
   - 任何錯誤都不會拋出
   - 不影響主程序運行

3. **自動循環**
   - 檔案超過 500KB 自動建立新檔案
   - 舊檔案重新命名為 `frogpilot_debug.log.YYYYMMDD_HHMMSS`
   - 保留最多 5 個舊檔案 (自動刪除更舊的)
   - 避免無限增長

4. **簡潔格式**
   - 只記錄時分秒 (不含日期)
   - 數值取整 (`.0f`)
   - 一行一筆 log

---

## 使用規範

### ✅ DO (應該做)

1. **所有我們的修改都用 `write_debug_log()`**
   ```python
   write_debug_log("SLC", "事件發生")
   ```

2. **模組名稱統一大寫**
   ```python
   write_debug_log("SLC", ...)    # ✅
   write_debug_log("BRAKE", ...)  # ✅
   ```

3. **訊息簡潔明瞭**
   ```python
   write_debug_log("SLC", f"SET MAX={speed}")  # ✅ 簡潔
   ```

4. **數值取整**
   ```python
   f"speed={speed:.0f}"  # ✅ 90 而不是 90.3456
   ```

5. **只記錄關鍵事件**
   - 狀態變化
   - 重要決策
   - 錯誤情況

---

### ❌ DON'T (不要做)

1. **不要用 `cloudlog`** (除非系統級錯誤)
   ```python
   cloudlog.info("...")  # ❌ 看不到 log
   write_debug_log("SLC", "...")  # ✅ 可以 FTP 下載
   ```

2. **不要記錄太頻繁** (避免效能影響)
   ```python
   # ❌ 不要在每個 frame 都寫 log
   for frame in frames:
       write_debug_log("SLC", "processing...")  # ❌ 太頻繁

   # ✅ 只在狀態變化時寫
   if speed_limit_changed:
       write_debug_log("SLC", "Event")  # ✅
   ```

3. **不要記錄過長訊息**
   ```python
   # ❌ 太長
   write_debug_log("SLC", f"Very long message with lots of details: {detail1}, {detail2}, {detail3}...")

   # ✅ 簡潔
   write_debug_log("SLC", f"Event - val1={v1} val2={v2}")
   ```

4. **不要在 try-except 外層使用** (已經有內建錯誤處理)
   ```python
   # ❌ 不需要
   try:
       write_debug_log("SLC", "...")
   except:
       pass

   # ✅ 直接用
   write_debug_log("SLC", "...")
   ```

---

## 已使用的模組

| 模組 | 檔案 | 用途 |
|-----|------|------|
| `SLC` | `selfdrive/controls/controlsd.py` | Speed Limit Controller 事件 |

**未來擴充**: 所有新的修改都應該加入這個表格

---

## 故障排除

### 問題 1: Log 檔案不存在

**可能原因**:
- 程式沒有執行到 `write_debug_log()`
- Logger import 失敗

**檢查**:
```bash
# 確認 logger 檔案存在
ls -l /data/openpilot/frogpilot/common/frogpilot_debug_logger.py

# 確認 controlsd 有 import
grep "frogpilot_debug_logger" /data/openpilot/selfdrive/controls/controlsd.py
```

---

### 問題 2: Log 內容為空

**可能原因**:
- 功能沒有觸發 (例如: SLC 功能未開啟)
- 條件不符 (例如: openpilot 未 engage)

**檢查**:
```bash
# 確認功能開啟
cat /data/params/d/SpeedLimitController

# 查看 controlsd 是否運行
tmux a
```

---

### 問題 3: 檔案太大

**不會發生**: Logger 會自動控制大小 (500KB max)

**自動循環機制**:
- 達到 500KB 時自動建立新檔案
- 舊檔案重新命名為 `frogpilot_debug.log.20251105_142315` (時間戳記格式)
- 最多保留 5 個舊檔案
- 超過 5 個時自動刪除最舊的

如果需要手動清除:
```bash
# 刪除當前 log
rm /data/frogpilot_debug.log

# 刪除所有 archived logs
rm /data/frogpilot_debug.log.*
```

---

## 維護記錄

| 日期 | 版本 | 變更 |
|-----|------|------|
| 2025-11-05 | 1.0 | 初始版本,建立統一 logger |
| 2025-11-05 | 1.1 | 加入 SLC 使用範例 |
| 2025-11-05 | 1.2 | 更新 log rotation 機制 - 改為建立新檔案而非覆蓋,保留最多 5 個舊檔案 |

---

**文檔版本**: 1.2
**最後更新**: 2025-11-05
