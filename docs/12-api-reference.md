# API 快速參考

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

## API 快速參考

### Params 常用操作

```python
from openpilot.common.params import Params

params = Params()

# 讀寫字串
params.put("Key", "value")
value = params.get("Key", encoding='utf8')

# 讀寫布林
params.put_bool("Flag", True)
flag = params.get_bool("Flag")

# 讀寫整數/浮點數
params.put_int("Count", 42)
params.put_float("Value", 3.14)

# 非阻塞寫入（推薦）
params.put_nonblocking("Key", "value")

# 移除
params.remove("Key")
```

### 訊息發布訂閱

```python
import cereal.messaging as messaging

# 發布
pm = messaging.PubMaster(['topic'])
msg = messaging.new_message('topic')
pm.send('topic', msg)

# 訂閱
sm = messaging.SubMaster(['topic'])
sm.update(1000)
data = sm['topic']
```

### 日誌記錄

```python
from openpilot.common.swaglog import cloudlog

cloudlog.info("Message")
cloudlog.warning("Warning")
cloudlog.error("Error")
cloudlog.exception("Exception")
```

### 時間取得

```python
import time
from openpilot.common.realtime import Ratekeeper

# Python 單調時間
t = time.monotonic()

# Python 系統時間
t = time.time()
t_ns = time.time_ns()

# 速率保持
rk = Ratekeeper(100)  # 100 Hz
while True:
    # ... 邏輯 ...
    rk.keep_time()
```

```cpp
// C++ 時間
#include "common/timing.h"

uint64_t mono_time = nanos_since_boot();
uint64_t wall_time = nanos_since_epoch();
double seconds = seconds_since_boot();
```

---

## 如何記錄變數到 rlog

### 概述

openpilot 使用 **cereal** 訊息系統來記錄所有資料到 rlog 檔案。要記錄自訂變數,需要經過以下步驟:

1. 在 `.capnp` 檔案中定義訊息結構
2. 在 Python 程式中建立並發送訊息
3. 重新編譯以生成 Python bindings

---

### 步驟 1: 定義訊息結構 (`.capnp` 檔案)

訊息結構定義在 `cereal/` 目錄下的 `.capnp` 檔案中:

- **`log.capnp`**: openpilot 核心訊息 (不建議修改)
- **`car.capnp`**: 車輛相關訊息 (不建議修改)
- **`custom.capnp`**: **FrogPilot 自訂訊息** (建議在此添加)

#### 範例: 在 `custom.capnp` 中添加新欄位

假設你想在 `FrogPilotPlan` 訊息中添加一個新的變數 `myCustomValue`:

```capnp
struct FrogPilotPlan @0xa1680744031fdb2d {
  accelerationJerk @0 :Float32;
  accelerationJerkStock @1 :Float32;
  # ... 其他現有欄位 ...

  # 添加你的新欄位
  myCustomValue @35 :Float32;
  myDebugInfo @36 :Text;
}
```

**重要規則**:
- **編號必須唯一**: `@N` 的編號不能與現有欄位重複
- **編號必須遞增**: 新欄位的編號應該大於現有最大編號
- **不要修改現有欄位**: 修改現有欄位會破壞向後相容性
- **資料型別**: 常用型別包括 `Float32`, `Int32`, `Bool`, `Text`, `List(Type)`

#### 建立全新的訊息結構

如果要建立全新的訊息類型:

```capnp
struct MyCustomMessage @0x<unique_id> {
  timestamp @0 :UInt64;
  speed @1 :Float32;
  acceleration @2 :Float32;
  debugText @3 :Text;

  # 巢狀結構
  extraData @4 :ExtraData;

  struct ExtraData {
    value1 @0 :Float32;
    value2 @0 :Int32;
  }
}
```

**如何生成唯一 ID**:
```bash
# 使用 capnp 工具生成唯一 ID
capnp id
# 輸出範例: @0x9a1b2c3d4e5f6789
```

---

### 步驟 2: 在 Python 中發送訊息

#### 2.1 初始化 Publisher

首先需要建立 `PubMaster` 並指定要發送的訊息頻道:

```python
import cereal.messaging as messaging

# 在 __init__ 中初始化
self.pm = messaging.PubMaster(['frogpilotPlan', 'myCustomMessage'])
```

#### 2.2 建立並填充訊息

在需要記錄資料的地方建立訊息並填充數值:

```python
# 建立新訊息
frogpilot_plan_send = messaging.new_message('frogpilotPlan')
frogpilot_plan_send.valid = True  # 設定訊息有效性
frogpilotPlan = frogpilot_plan_send.frogpilotPlan

# 填充現有欄位
frogpilotPlan.accelerationJerk = 1.5
frogpilotPlan.maxAcceleration = 2.0

# 填充你的自訂欄位
frogpilotPlan.myCustomValue = some_variable
frogpilotPlan.myDebugInfo = f"Debug: {some_info}"

# 發送訊息
self.pm.send('frogpilotPlan', frogpilot_plan_send)
```

#### 2.3 完整範例

以 `frogpilot_planner.py` 為例:

```python
# frogpilot/controls/frogpilot_planner.py

class FrogPilotPlanner:
  def __init__(self):
    # ... 初始化代碼 ...
    self.my_custom_value = 0.0  # 你要記錄的變數

  def update(self, v_ego, sm, frogpilot_toggles):
    # ... 更新邏輯 ...

    # 計算你的自訂數值
    self.my_custom_value = v_ego * 2.5  # 範例計算

  def publish(self, theme_updated, toggles_updated, sm, pm, frogpilot_toggles):
    frogpilot_plan_send = messaging.new_message("frogpilotPlan")
    frogpilot_plan_send.valid = sm.all_checks(service_list=["carState", "controlsState"])
    frogpilotPlan = frogpilot_plan_send.frogpilotPlan

    # 填充現有欄位
    frogpilotPlan.accelerationJerk = self.frogpilot_following.acceleration_jerk
    frogpilotPlan.tFollow = self.frogpilot_following.t_follow

    # 填充你的自訂欄位
    frogpilotPlan.myCustomValue = self.my_custom_value
    frogpilotPlan.myDebugInfo = f"Speed: {v_ego:.2f}"

    # 發送訊息
    pm.send("frogpilotPlan", frogpilot_plan_send)
```

---

### 步驟 3: 在 `log.capnp` 中註冊訊息

如果你建立了全新的訊息類型,需要在 `log.capnp` 的 `Event` union 中註冊:

```capnp
# cereal/log.capnp

struct Event {
  logMonoTime @0 :UInt64;
  valid @1 :Bool = true;

  union {
    # ... 現有訊息類型 ...
    frogpilotPlan @133 :Custom.FrogPilotPlan;

    # 添加你的新訊息
    myCustomMessage @200 :Custom.MyCustomMessage;
  }
}
```

---

### 步驟 4: 重新編譯

修改 `.capnp` 檔案後,必須重新編譯以生成 Python bindings:

```bash
cd /data/openpilot  # 或你的專案目錄

# 重新編譯 cereal
scons cereal/

# 或完整編譯
scons -j$(nproc)
```

**Windows 開發環境**:
- 在 Windows 上開發時,修改 `.capnp` 檔案後需要在 C3 設備上重新編譯
- 或使用 WSL/Docker 進行編譯

---

### 步驟 5: 驗證記錄

#### 5.1 即時查看訊息

使用 `cereal` 工具查看訊息是否正常發送:

```bash
# 查看特定訊息
cereal peek frogpilotPlan

# 查看所有訊息
cereal peek --all
```

#### 5.2 檢查 rlog 檔案

記錄完成後,可以從 rlog 檔案中讀取數值:

```python
from tools.lib.logreader import LogReader

# 讀取 rlog 檔案
lr = LogReader("/data/media/0/realdata/segment_path/rlog")

# 遍歷訊息
for msg in lr:
    if msg.which() == 'frogpilotPlan':
        print(f"myCustomValue: {msg.frogpilotPlan.myCustomValue}")
        print(f"myDebugInfo: {msg.frogpilotPlan.myDebugInfo}")
```

---

### 常見訊息類型和位置

| 訊息名稱 | 定義位置 | 發送位置 | 頻率 | 用途 |
|---------|---------|---------|-----|-----|
| `carState` | `car.capnp` | 車輛介面 | 100Hz | 車輛狀態 (速度、轉向角等) |
| `carControl` | `car.capnp` | `controlsd.py` | 100Hz | 控制指令 (油門、煞車、轉向) |
| `controlsState` | `log.capnp` | `controlsd.py` | 100Hz | 控制系統狀態 |
| `longitudinalPlan` | `log.capnp` | `longitudinal_planner.py` | 20Hz | 縱向規劃 (MPC 輸出) |
| `frogpilotPlan` | `custom.capnp` | `frogpilot_planner.py` | 20Hz | FrogPilot 規劃資料 |
| `frogpilotControlsState` | `custom.capnp` | `controlsd.py` | 100Hz | FrogPilot 控制狀態 |
| `frogpilotCarState` | `custom.capnp` | 車輛介面 | 100Hz | FrogPilot 車輛狀態 |

---

### 最佳實踐

#### 1. 選擇適當的訊息類型

- **需要高頻記錄 (100Hz)**: 使用 `controlsState` 或 `carControl`
- **中頻記錄 (20Hz)**: 使用 `frogpilotPlan` 或 `longitudinalPlan`
- **低頻記錄 (1Hz)**: 建立新的訊息類型

#### 2. 資料型別選擇

```capnp
# 浮點數
Float32  # 單精度,節省空間,精度足夠大多數情況
Float64  # 雙精度,只在需要高精度時使用

# 整數
Int32    # 有符號 32位元
UInt32   # 無符號 32位元
Int64    # 64位元整數,用於時間戳等

# 布林值
Bool     # true/false

# 字串
Text     # 字串,用於除錯訊息等

# 列表
List(Float32)  # 浮點數陣列
List(Text)     # 字串陣列
```

#### 3. 命名慣例

- **駝峰式命名**: `myCustomValue` (不是 `my_custom_value`)
- **有意義的名稱**: `actualAcceleration` 優於 `accel1`
- **避免縮寫**: `maximumSpeed` 優於 `maxSpd`

#### 4. 效能考量

```python
# ✅ 好的做法: 每個週期發送一次完整訊息
def publish(self, pm):
    msg = messaging.new_message('myMessage')
    msg.myMessage.value1 = self.value1
    msg.myMessage.value2 = self.value2
    pm.send('myMessage', msg)

# ❌ 壞的做法: 不要在高頻迴圈中建立過大的訊息
def update(self, pm):  # 這個函數可能每 10ms 呼叫一次
    huge_list = [i for i in range(10000)]  # 避免
    msg.myMessage.hugeData = huge_list
```

#### 5. 除錯技巧

```python
# 在程式中添加除錯輸出
from openpilot.common.swaglog import cloudlog

cloudlog.info(f"Publishing myCustomValue: {self.my_custom_value}")

# 使用 valid 標誌標示訊息有效性
frogpilot_plan_send.valid = self.data_is_valid

# 在關鍵時刻記錄額外資訊
if error_condition:
    frogpilotPlan.myDebugInfo = f"ERROR: {error_message}"
```

---

### cloudlog 記錄位置

#### 什麼是 cloudlog?

`cloudlog` 是 openpilot 的日誌系統，用於記錄除錯訊息、錯誤、警告等文字日誌。與 cereal 訊息系統不同，cloudlog 主要用於除錯和診斷。

#### cloudlog 記錄到哪裡?

cloudlog 訊息會同時記錄到 **三個地方**：

##### 1. 終端輸出 (Console)

根據環境變數 `LOGPRINT` 決定輸出等級：

```bash
# 設定輸出等級
export LOGPRINT=debug    # 輸出所有等級 (DEBUG, INFO, WARNING, ERROR)
export LOGPRINT=info     # 輸出 INFO 以上
export LOGPRINT=warning  # 輸出 WARNING 以上 (預設)

# 執行程式後可在終端看到日誌
python selfdrive/controls/controlsd.py
```

##### 2. swaglog 檔案 (`/data/log/`)

**位置**:
- **裝置上**: `/data/log/swaglog.*`
- **PC 上**: `~/.comma/log/swaglog.*`

**特性**:
- 每 60 秒或 256KB 輪替一個新檔案
- 保留最近 2500 個檔案
- 檔案命名: `swaglog.0000000001`, `swaglog.0000000002`, ...
- 格式: 文字檔，可直接用 `cat` 或 `tail` 查看

**查看方式**:
```bash
# 查看最新日誌
tail -f /data/log/swaglog.*

# 查看特定時間的日誌
cat /data/log/swaglog.0000001234

# 搜尋特定關鍵字
grep "Publishing" /data/log/swaglog.*

# 查看錯誤訊息
grep "ERROR" /data/log/swaglog.*
```

##### 3. rlog 的 logMessage 訊息

`logmessaged` 服務會將所有日誌發送到 cereal 訊息系統：

**訊息類型**:
- **`logMessage`**: 所有 INFO 等級以上的日誌
- **`errorLogMessage`**: ERROR 等級的日誌

**流程圖**:
```
程式中呼叫 cloudlog.info()
         ↓
透過 ZMQ IPC 發送到 logmessaged
    (/tmp/logmessage)
         ↓
    logmessaged 服務
         ↓
   ┌─────┴─────┐
   ↓           ↓
寫入檔案    發送 cereal 訊息
/data/log/   (logMessage)
            ↓
        記錄到 rlog
```

**從 rlog 讀取日誌**:
```python
from tools.lib.logreader import LogReader

lr = LogReader("/data/media/0/realdata/segment_path/rlog")

for msg in lr:
    if msg.which() == 'logMessage':
        print(msg.logMessage)
    elif msg.which() == 'errorLogMessage':
        print(f"ERROR: {msg.errorLogMessage}")
```

---

#### cloudlog 使用方式

##### 基本用法

```python
from openpilot.common.swaglog import cloudlog

# DEBUG: 詳細除錯資訊
cloudlog.debug("Variable x = %s", x)

# INFO: 一般資訊
cloudlog.info("System initialized successfully")

# WARNING: 警告 (但不影響運行)
cloudlog.warning("Speed exceeds recommended limit")

# ERROR: 錯誤 (可能影響功能)
cloudlog.error("Failed to read sensor data")

# CRITICAL: 嚴重錯誤 (系統無法運行)
cloudlog.critical("System crash imminent")

# EXCEPTION: 記錄例外 (包含 stack trace)
try:
    risky_operation()
except Exception as e:
    cloudlog.exception("Operation failed")
```

##### 格式化輸出

```python
# 使用 f-string (Python 3.6+)
cloudlog.info(f"Speed: {v_ego:.2f} m/s, Accel: {a_ego:.3f} m/s²")

# 使用 % 格式化
cloudlog.info("Speed: %.2f m/s, Accel: %.3f m/s²", v_ego, a_ego)

# 使用 .format()
cloudlog.info("Speed: {} m/s, Accel: {} m/s²".format(v_ego, a_ego))
```

##### 條件式日誌

```python
# 只在除錯模式記錄
if debug_mode:
    cloudlog.debug("Detailed state: %s", state_dict)

# 只在錯誤發生時記錄
if error_detected:
    cloudlog.error("Error code: %d, details: %s", error_code, error_msg)

# 定期記錄 (避免日誌過多)
if self.frame % 100 == 0:  # 每 100 個週期記錄一次
    cloudlog.info("Still running, speed: %.2f", v_ego)
```

##### timestamp 功能

openpilot 特有的 `timestamp` 方法，用於效能分析：

```python
# 記錄時間戳 (用於效能分析)
cloudlog.timestamp("Function A started")
some_heavy_function()
cloudlog.timestamp("Function A completed")

# 這些 timestamp 會出現在日誌中，可用來計算執行時間
```

---

#### cloudlog vs cereal 訊息

| 特性 | cloudlog | cereal 訊息 |
|------|---------|-----------|
| **用途** | 除錯、錯誤記錄、系統訊息 | 結構化資料記錄 |
| **資料型別** | 文字字串 | 結構化數值 (Float, Int, Bool 等) |
| **效能影響** | 較低 (只寫入檔案和 IPC) | 極低 (二進位格式) |
| **適合場景** | 診斷問題、追蹤執行流程 | 記錄感測器資料、控制參數 |
| **查看方式** | `tail`, `grep`, `cat` | `cereal peek`, LogReader |
| **記錄頻率** | 低頻 (事件驅動) | 高頻 (週期性) |
| **檔案大小** | 較小 | 較大 |

**使用建議**:
- **使用 cloudlog**: 記錄狀態變化、錯誤、警告、初始化訊息
- **使用 cereal**: 記錄連續變化的數值、感測器資料、控制輸出

**範例**:
```python
# ✅ 適合用 cloudlog
cloudlog.info("System started")
cloudlog.warning("GPS signal weak")
cloudlog.error("Failed to initialize camera")

# ✅ 適合用 cereal
frogpilotPlan.myCustomValue = current_speed  # 連續變化的數值
frogpilotPlan.accelerationJerk = jerk_value  # 控制參數

# ❌ 不建議
cloudlog.info(f"Speed: {v_ego}")  # 每 10ms 記錄一次會造成日誌爆炸
```

---

#### 實際應用範例

##### 範例 1: 追蹤函數執行

```python
def my_complex_function(self, param1, param2):
    cloudlog.info("Starting complex function with params: %s, %s", param1, param2)

    try:
        result = self.do_calculation(param1)
        cloudlog.debug("Calculation result: %s", result)

        if result > threshold:
            cloudlog.warning("Result exceeds threshold: %.2f > %.2f", result, threshold)

        return result

    except Exception as e:
        cloudlog.exception("Complex function failed")
        return None
```

##### 範例 2: 狀態轉換記錄

```python
def update_state(self, new_state):
    if self.state != new_state:
        cloudlog.info("State transition: %s -> %s", self.state, new_state)
        self.state = new_state
```

##### 範例 3: 效能監控

```python
def update(self, sm):
    cloudlog.timestamp("Update started")

    self.process_sensor_data(sm)
    cloudlog.timestamp("Sensor processing done")

    self.run_mpc()
    cloudlog.timestamp("MPC completed")

    self.publish_results()
    cloudlog.timestamp("Results published")
```

##### 範例 4: 條件式除錯

```python
class MyController:
    def __init__(self):
        self.debug_counter = 0

    def update(self, v_ego):
        # 只在特定條件下記錄詳細資訊
        if abs(v_ego - self.target_speed) > 5.0:
            cloudlog.warning("Large speed error: current=%.2f, target=%.2f",
                           v_ego, self.target_speed)

        # 定期記錄健康狀態
        self.debug_counter += 1
        if self.debug_counter % 1000 == 0:
            cloudlog.info("Controller health check: errors=%d", self.error_count)
```

---

#### 查看日誌的實用指令

```bash
# 即時查看最新日誌
tail -f /data/log/swaglog.* | grep --line-buffered "ERROR\|WARNING"

# 查看特定進程的日誌
grep "controlsd" /data/log/swaglog.* | tail -20

# 統計錯誤數量
grep -c "ERROR" /data/log/swaglog.*

# 查看特定時間範圍的日誌 (根據檔案編號)
cat /data/log/swaglog.{0000001200..0000001250}

# 搜尋特定關鍵字並顯示前後文
grep -B 2 -A 2 "Failed to initialize" /data/log/swaglog.*

# 只看 ERROR 等級並去重
grep "ERROR" /data/log/swaglog.* | sort | uniq

# 結合 Python 分析日誌
python << EOF
import re
errors = {}
for line in open('/data/log/swaglog.0000001234'):
    if 'ERROR' in line:
        match = re.search(r'ERROR: (.*)', line)
        if match:
            err_msg = match.group(1)
            errors[err_msg] = errors.get(err_msg, 0) + 1

for msg, count in sorted(errors.items(), key=lambda x: x[1], reverse=True):
    print(f"{count:4d}x {msg}")
EOF
```

---

### 總結

- **cloudlog 記錄位置**:
  1. 終端輸出 (根據 `LOGPRINT` 環境變數)
  2. `/data/log/swaglog.*` 檔案
  3. rlog 的 `logMessage` 和 `errorLogMessage` 訊息

- **適用場景**:
  - 記錄系統事件、狀態變化
  - 除錯和診斷問題
  - 記錄錯誤和警告

- **與 cereal 的區別**:
  - cloudlog: 文字日誌，低頻，事件驅動
  - cereal: 結構化資料，高頻，週期性

- **最佳實踐**:
  - 選擇適當的日誌等級
  - 避免在高頻迴圈中使用 cloudlog
  - 使用條件式記錄避免日誌爆炸
  - 結合 cloudlog 和 cereal 達到最佳除錯效果

---

### 完整範例: 添加新的除錯變數

假設你想記錄縱向控制的內部狀態用於除錯:

#### 1. 修改 `custom.capnp`

```capnp
struct FrogPilotPlan @0xa1680744031fdb2d {
  # ... 現有欄位 ...
  vCruise @34 :Float32;

  # 新增除錯欄位
  pidError @35 :Float32;
  pidOutput @36 :Float32;
  pidIntegrator @37 :Float32;
  controlState @38 :Text;
}
```

#### 2. 修改 `frogpilot_planner.py`

```python
class FrogPilotPlanner:
  def __init__(self):
    self.pid_error = 0.0
    self.pid_output = 0.0
    self.pid_integrator = 0.0
    self.control_state = "unknown"

  def update(self, sm):
    # 從 longitudinalPlan 取得 PID 資訊
    if 'longitudinalPlan' in sm:
        self.pid_error = sm['longitudinalPlan'].aTarget - sm['carState'].aEgo
        # ... 其他計算 ...

  def publish(self, pm, sm):
    frogpilot_plan_send = messaging.new_message("frogpilotPlan")
    frogpilotPlan = frogpilot_plan_send.frogpilotPlan

    # 填充除錯資訊
    frogpilotPlan.pidError = self.pid_error
    frogpilotPlan.pidOutput = self.pid_output
    frogpilotPlan.pidIntegrator = self.pid_integrator
    frogpilotPlan.controlState = self.control_state

    pm.send("frogpilotPlan", frogpilot_plan_send)
```

#### 3. 重新編譯

```bash
cd /data/openpilot
scons cereal/
```

#### 4. 測試並查看

```bash
# 即時查看
cereal peek frogpilotPlan | grep -E "pidError|pidOutput"

# 事後分析
python << EOF
from tools.lib.logreader import LogReader
lr = LogReader("/data/media/0/realdata/.../rlog")
for msg in lr:
    if msg.which() == 'frogpilotPlan':
        print(f"PID Error: {msg.frogpilotPlan.pidError:.3f}, "
              f"Output: {msg.frogpilotPlan.pidOutput:.3f}")
EOF
```

---

### 常見問題

#### Q1: 修改 `.capnp` 後沒有效果?
**A**: 確認已重新編譯 `scons cereal/`，並重新啟動 openpilot

#### Q2: 出現 "AttributeError: 'FrogPilotPlan' has no attribute 'myField'"?
**A**: 檢查:
1. `.capnp` 檔案中是否正確定義欄位
2. 是否重新編譯
3. 欄位名稱是否拼寫正確 (區分大小寫)

#### Q3: rlog 檔案太大?
**A**:
- 避免記錄大型陣列或字串
- 降低訊息發送頻率
- 只在需要時記錄除錯資訊

#### Q4: 如何只在特定條件下記錄?
**A**:
```python
def publish(self, pm):
    # 只在除錯模式或出錯時記錄詳細資訊
    if self.debug_mode or self.error_detected:
        frogpilotPlan.myDebugInfo = detailed_info
    else:
        frogpilotPlan.myDebugInfo = ""
```

---

## 開發流程與最佳實踐

### 建構系統

```bash
# 完整建構
scons -j$(nproc)

# 最小建構（不含工具和測試）
scons --minimal

# 建構特定目標
scons selfdrive/controls/controlsd

# 清理
scons -c
```

### 測試

```bash
# 執行所有測試
pytest

# 跳過慢速測試
pytest -m "not slow"

# 執行特定測試
pytest selfdrive/car
pytest system/manager/test_manager.py
```

### 程式碼檢查

```bash
# Linting
ruff check .

# 型別檢查
mypy .

# Pre-commit
pre-commit run --all-files
```

### Git 工作流程

```bash
# 當前分支: hfop
# 主分支: FrogPilot

# 建立功能分支
git checkout -b feature/my-feature

# 提交變更
git add .
git commit -m "描述"

# 合併到主分支
git checkout FrogPilot
git merge feature/my-feature
```

### 開發環境設定（Windows）

1. **使用 venv 進行測試**
   ```bash
   python -m venv venv
   venv\Scripts\activate
   pip install -r requirements.txt
   ```

2. **測試單一檔案**
   ```bash
   python system/timed.py
   ```

3. **注意編碼問題**
   - BAT 和 PS1 檔案可能有中文編碼衝突
   - Python 檔案使用 UTF-8

4. **不要直接執行**
   - 在 venv 中測試
   - 確認無誤後再編譯和製作安裝程式

### 新增功能最佳實踐

1. **規劃階段**
   - 研究現有架構
   - 確認修改點
   - 設計資料流程

2. **實作階段**
   - 最小化變更範圍
   - 保持向後相容
   - 使用非阻塞 I/O

3. **測試階段**
   - 單元測試
   - 整合測試
   - 實機測試

4. **文件階段**
   - 更新此文件
   - 添加程式碼註解
   - 記錄已知問題

---

## 實作案例

### 案例 1：時間持久化實作（已實作）

**需求**: 解決無 GPS 時日誌檔案時間錯誤的問題

**問題**：無網路/GPS 時，系統時間預設為 2023/11/23，導致日誌檔案修改時間錯誤

**解決方案**：
- 開機時從 PARAMS 讀取上次已知的正確時間
- GPS 同步時保存時間
- 關機時保存當前時間

**重要設計決策**：
- ⚠️ **時間格式使用 Unix timestamp（float）**，不使用 isoformat
- 原因：timed.py 內部統一使用 `fromtimestamp()` 處理數字型時間戳
- 格式：`time.time()` 返回的 float 值（如 `1729584000.123456`）
- 儲存：轉為字串 `str(timestamp)` 儲存到 PARAMS
- 讀取：用 `float(timestamp_str)` 解析後用 `fromtimestamp()` 轉為 datetime

**實作步驟**：

1. **定義參數** (`common/params.cc:159`):
```cpp
{"LastKnownGoodTime", PERSISTENT},
```

2. **開機時恢復時間** (`system/timed.py:92-111`) - **已改進**:
```python
# 嘗試從 PARAMS 恢復時間
# 無論 system_time_valid() 如何判斷,都檢查是否需要恢復
last_good_timestamp_str = params.get("LastKnownGoodTime", encoding='utf8')
if last_good_timestamp_str is not None:
  try:
    last_good_timestamp = float(last_good_timestamp_str)
    current_timestamp = time.time()

    # 如果系統時間明顯無效,或保存的時間比當前時間新很多
    if not system_time_valid() or (last_good_timestamp > current_timestamp + 86400):
      last_good_time = datetime.datetime.fromtimestamp(last_good_timestamp)
      cloudlog.info(f"Restoring system time from LastKnownGoodTime: {last_good_time}")
      cloudlog.info(f"Current time was: {datetime.datetime.now()}")
      set_time(last_good_time)
    else:
      cloudlog.debug(f"System time seems valid, not restoring from LastKnownGoodTime")
  except (ValueError, TypeError) as e:
    cloudlog.error(f"Failed to parse LastKnownGoodTime: {e}")
else:
  cloudlog.warning("No LastKnownGoodTime available for time restoration")
```

**改進重點**:
- 雙重檢查機制: `not system_time_valid()` **或** `last_good_timestamp > current_timestamp + 86400`
- 防止 `system_time_valid()` 誤判 (systemd 檔案時間也錯誤時)
- 檢查 LastKnownGoodTime 是否比當前時間新超過 1 天,如果是則系統時間肯定錯誤

3. **GPS 同步時保存** (`system/timed.py:123-125`):
```python
# GPS 同步後立即保存時間（這是唯一的寫入時機）
params.put_nonblocking("LastKnownGoodTime", str(llk.unixTimestampMillis / 1000.))
cloudlog.debug(f"Saved time from GPS: {gps_time.isoformat()}")
```
注意：直接使用 GPS 原始 timestamp `llk.unixTimestampMillis / 1000.`，避免格式轉換

4. **關機時保存** (`system/timed.py:51-64`):
```python
def save_time_to_param(params: Params):
    """保存當前時間到 PARAMS（關機時調用）"""
    if system_time_valid():
        current_timestamp = time.time()
        params.put("LastKnownGoodTime", str(current_timestamp))
        cloudlog.info(f"Saved LastKnownGoodTime: {datetime.datetime.fromtimestamp(current_timestamp)}")

def signal_handler(signum, frame):
    """處理關機信號，保存時間後退出"""
    cloudlog.info(f"Received signal {signum}, saving time before exit")
    params = Params()
    save_time_to_param(params)
    sys.exit(0)

# 註冊信號處理器（在 main() 函數中）
signal.signal(signal.SIGTERM, signal_handler)
signal.signal(signal.SIGINT, signal_handler)
```

5. **必要的 imports** (`system/timed.py:1-16`):
```python
import signal  # 新增
import sys     # 新增
import time    # 已存在
```

**優勢**：
- ✅ 最小化寫入次數（GPS 同步 + 關機時）
- ✅ 延長 Flash 壽命
- ✅ 時間準確性高（直接使用數字型 timestamp）
- ✅ 完全向後相容
- ✅ 格式統一（與 timed.py 內部一致）

**修改檔案**：
- `common/params.cc` - 新增參數定義
- `system/timed.py` - 實作時間持久化邏輯（3 個位置）

**除錯提示**：
- ❌ 不要使用 `.isoformat()` 儲存時間
- ✅ 使用 `time.time()` 取得 timestamp
- ✅ 使用 `str()` 轉換後儲存
- ✅ 使用 `float()` 解析並用 `fromtimestamp()` 轉換

---

## rlog 時間戳研究 (2025-10-26)

### 問題發現

在實際測試中發現,即使實作了永久時間保存機制,**rlog 檔案的 ctime 仍然可能是錯誤的**。

**範例**:
```bash
stat /data/media/0/realdata/00000009--f5d34548e1--40/rlog

Modify: 2023-11-22 05:52:05  # 錯誤的時間
Change: 2023-11-22 05:52:05  # 錯誤的時間
```

但 `LastKnownGoodTime` 參數值是正確的:
```python
LastKnownGoodTime = 1761445422.4985726  # 2025-10-25
```

### 根本原因分析

#### 1. 檔案系統時間 vs rlog 內部時間

rlog 中有**多種時間欄位**,來源和意義都不同:

| 時間欄位 | 位置 | 來源 | 是否可靠 |
|---------|------|------|----------|
| **檔案系統 ctime** | 檔案屬性 | loggerd 建立檔案時的系統時間 | ❌ 可能錯誤 |
| **InitData.wallTimeNanos** | rlog 開頭 | logger_build_init_data() 時的系統時間 | ❌ 可能錯誤 |
| **Clocks.wallTimeNanos** | 每秒廣播 | timed.py 的 `time.time_ns()` | ✅ GPS 同步後正確 |
| **liveLocationKalman.unixTimestampMillis** | GPS 訊息 | GPS 衛星時間計算 | ✅ 永遠正確 |
| **EncodeData.unixTimestampNanos** | 視訊幀 | encoder 執行時的 `timespec_get()` | ⚠️ 取決於執行時系統時間 |
| **logMonoTime** | 每個 Event | `time.monotonic()` 單調時鐘 | ✅ 相對時間可靠 |

#### 2. liveLocationKalman.unixTimestampMillis 的計算方式

這是**最可靠的時間來源**,因為它不依賴系統時間:

```python
# system/qcomgpsd/qcomgpsd.py:350-353
dt_timestamp = (datetime.datetime(1980, 1, 6, 0, 0, 0, 0, datetime.UTC) +
                datetime.timedelta(weeks=report['w_GpsWeekNumber']) +
                datetime.timedelta(seconds=(1e-3*report['q_GpsFixTimeMs'] - 18)))
gps.unixTimestampMillis = dt_timestamp.timestamp()*1e3
```

**時間流程**:
```
1. GPS 硬體接收衛星訊號
   ↓ (GPS Week Number + GPS Time of Week)

2. qcomgpsd.py 計算 UNIX 時間戳
   GPS_EPOCH (1980-01-06) + GPS_Week + GPS_Time - 18秒(閏秒)
   ↓

3. locationd.cc 接收並保存
   this->unix_timestamp_millis = log.getUnixTimestampMillis()
   ↓

4. 發布到 liveLocationKalman
   fix.setUnixTimestampMillis(this->unix_timestamp_millis)
   ↓

5. timed.py 讀取並同步系統時間
   gps_time = datetime.fromtimestamp(llk.unixTimestampMillis / 1000.)
   set_time(gps_time)
```

**關鍵**: GPS 時間戳是從衛星訊號**直接計算**的,完全獨立於系統時間!

#### 3. 為什麼檔案 ctime 會錯誤?

**競爭條件 (Race Condition)**:

```
T0: [manager 啟動所有進程 - 並行]
  ├─ timed.py 啟動
  └─ loggerd 啟動

T1: [timed.py]
    讀取 LastKnownGoodTime
    檢查系統時間 (2023-11-22)
    準備執行 set_time()...

T2: [loggerd]
    立即建立 segment 0
    → rlog ctime = 2023-11-22 ✗ (系統時間還沒恢復!)

T3: [timed.py]
    set_time(2025-10-25) ✓
    但 rlog 已經建立了...
```

**程式碼位置**:
```cpp
// system/loggerd/loggerd.cc:232-234
LoggerdState s;
// init logger
logger_rotate(&s);  // ← 立即建立第一個 segment!
```

**結論**: loggerd 在 timed.py 恢復系統時間**之前**就建立了檔案。

### 解決方案評估

#### 方案 A: 讓 loggerd 等待時間有效 ❌
```cpp
// 等待系統時間有效後再建立檔案
while (!system_time_valid()) {
  util::sleep_for(100);
}
logger_rotate(&s);
```
**問題**: 會造成整個系統 lag,不可行。

#### 方案 B: 在 manager_init() 恢復時間 ⚠️
```python
def manager_init():
  # 在啟動進程前恢復時間
  restore_time_from_params()
  # 然後啟動所有進程
```
**問題**: 仍有微小的競爭條件風險。

#### 方案 C: 接受初始 ctime 錯誤,用 GPS 時間推算 ✅ (採用)

**理由**:
1. `liveLocationKalman.unixTimestampMillis` 永遠是正確的 (來自 GPS 衛星)
2. 檔案 ctime 只是元數據,不影響實際記錄內容
3. 前幾個 segment (GPS 未 fix) ctime 錯誤可接受
4. GPS fix 後的 segment,可用 `liveLocationKalman.unixTimestampMillis` 推算正確時間

### 實際應用建議

#### 讀取 rlog 真實時間的方法

**方法 1: 使用 liveLocationKalman.unixTimestampMillis (最準確)**
```python
from tools.lib.logreader import LogReader
from datetime import datetime

lr = LogReader("rlog")
for msg in lr:
    if msg.which() == 'liveLocationKalman' and msg.liveLocationKalman.gpsOK:
        unix_ts_millis = msg.liveLocationKalman.unixTimestampMillis
        real_time = datetime.fromtimestamp(unix_ts_millis / 1000.)
        print(f"GPS 時間: {real_time}")
        break  # 第一個 GPS OK 的訊息
```

**方法 2: 使用 Clocks.wallTimeNanos (GPS 同步後)**
```python
for msg in lr:
    if msg.which() == 'clocks' and msg.valid:
        wall_time = msg.clocks.wallTimeNanos / 1e9
        real_time = datetime.fromtimestamp(wall_time)
        print(f"系統時間: {real_time}")
```

**方法 3: 使用檔案 ctime (可能錯誤)**
```python
import os
ctime = os.path.getctime("rlog")
file_time = datetime.fromtimestamp(ctime)
print(f"檔案時間: {file_time}")  # 可能是 2023 年!
```

#### 檢查工具腳本

```python
#!/usr/bin/env python3
"""檢查 rlog 的時間欄位"""
from tools.lib.logreader import LogReader
from datetime import datetime
import os
import sys

def check_rlog_times(rlog_path):
    print(f"檢查: {rlog_path}\n")

    # 檔案系統時間
    ctime = os.path.getctime(rlog_path)
    print(f"=== 檔案系統 ctime ===")
    print(f"{datetime.fromtimestamp(ctime)}\n")

    lr = LogReader(rlog_path)
    init_time = None
    first_gps_time = None
    first_clock_time = None

    for msg in lr:
        # InitData
        if msg.which() == 'initData' and not init_time:
            init_time = msg.initData.wallTimeNanos / 1e9
            print(f"=== InitData.wallTimeNanos ===")
            print(f"{datetime.fromtimestamp(init_time)}\n")

        # liveLocationKalman
        if msg.which() == 'liveLocationKalman' and not first_gps_time:
            if msg.liveLocationKalman.gpsOK:
                first_gps_time = msg.liveLocationKalman.unixTimestampMillis / 1000.
                print(f"=== liveLocationKalman.unixTimestampMillis (GPS) ===")
                print(f"{datetime.fromtimestamp(first_gps_time)}")
                print(f"gpsOK: {msg.liveLocationKalman.gpsOK}\n")

        # Clocks
        if msg.which() == 'clocks' and not first_clock_time:
            if msg.valid:
                first_clock_time = msg.clocks.wallTimeNanos / 1e9
                print(f"=== Clocks.wallTimeNanos ===")
                print(f"{datetime.fromtimestamp(first_clock_time)}")
                print(f"valid: {msg.valid}\n")

        if init_time and first_gps_time and first_clock_time:
            break

    print("=== 建議 ===")
    if first_gps_time:
        print(f"✓ 使用 liveLocationKalman.unixTimestampMillis: {datetime.fromtimestamp(first_gps_time)}")
    elif first_clock_time:
        print(f"⚠ 使用 Clocks.wallTimeNanos: {datetime.fromtimestamp(first_clock_time)}")
    else:
        print(f"✗ GPS 未 fix,檔案 ctime 可能不準確")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("用法: python check_rlog_time.py <rlog_path>")
        sys.exit(1)
    check_rlog_times(sys.argv[1])
```

### 總結

**現況**:
- ✅ 永久時間保存機制有效 (timed.py 改進後)
- ✅ GPS 同步後系統時間正確
- ✅ `liveLocationKalman.unixTimestampMillis` 永遠正確
- ⚠️ 前幾個 segment 的檔案 ctime 可能錯誤 (GPS 未 fix 時)
- ✅ 可用 GPS 時間推算正確記錄時間

**最佳實踐**:
1. 匯入/分析 rlog 時,**優先使用 `liveLocationKalman.unixTimestampMillis`**
2. 不要依賴檔案系統的 ctime
3. GPS 未 fix 的 segment,時間可能不準確 (但這通常只有開機後前幾個 segment)

**相關程式碼**:
- `system/qcomgpsd/qcomgpsd.py:350-353` - GPS 時間計算
- `selfdrive/locationd/locationd.cc:376` - 接收 GPS 時間
- `system/timed.py:92-111` - 時間恢復邏輯 (已改進)
- `system/loggerd/logger.cc:16-21` - InitData.wallTimeNanos 設定
- `system/loggerd/loggerd.cc:232-234` - loggerd 立即建立檔案

### 案例 2：發布自訂訊息

**需求**: 發布自訂的狀態訊息

**步驟**:

1. **定義訊息** (`cereal/log.capnp`):
```capnp
struct MyCustomMessage {
  value @0 :Float32;
  status @1 :Text;
}

struct Event {
  # ... 現有欄位 ...
  myCustom @999 :MyCustomMessage;
}
```

2. **發布訊息** (Python):
```python
import cereal.messaging as messaging

pm = messaging.PubMaster(['myCustom'])

msg = messaging.new_message('myCustom')
msg.myCustom.value = 42.0
msg.myCustom.status = "OK"
pm.send('myCustom', msg)
```

3. **訂閱訊息**:
```python
sm = messaging.SubMaster(['myCustom'])

while True:
    sm.update(1000)
    if sm.updated['myCustom']:
        print(f"Value: {sm['myCustom'].value}")
```

### 案例 3：添加新的系統服務

**需求**: 添加一個監控服務

**步驟**:

1. **建立服務檔案** (`system/my_monitor.py`):
```python
#!/usr/bin/env python3
import time
from openpilot.common.params import Params
from openpilot.common.swaglog import cloudlog

def main():
    params = Params()

    while True:
        # 監控邏輯
        cloudlog.info("Monitoring...")
        time.sleep(1)

if __name__ == "__main__":
    main()
```

2. **註冊進程** (`system/manager/process_config.py`):
```python
procs = [
    # ... 現有進程 ...
    PythonProcess("my_monitor", "system.my_monitor", always_run),
]
```

3. **測試**:
```bash
python system/my_monitor.py
```

---

