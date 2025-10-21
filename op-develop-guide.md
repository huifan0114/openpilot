# FrogPilot / openpilot 開發指南

> **文件版本**: v1.1
> **最後更新**: 2025-10-22
> **目的**: 整合所有架構研究成果，作為開發時的快速參考文件

---

## 🎯 知識庫使用指南

### **重要工作流程**

```
收到新任務
  ↓
📖 先讀取 op-develop-guide.md
  ├─ 知識庫已有內容 → 直接使用
  └─ 知識庫沒有內容 → 深入研究 → 更新知識庫
  ↓
執行開發工作
  ↓
📝 將研究成果補充到知識庫
```

### **核心原則**

1. **先查後找**: 任何研究前，先讀取此文件，避免重複搜尋
2. **即時更新**: 每次深入研究後，立即更新對應章節
3. **結構化整理**: 保持章節清晰，便於快速查閱
4. **實作導向**: 提供可直接使用的範例代碼

### **更新規範**

每次更新知識庫時必須：
- ✅ 新增/更新對應章節內容
- ✅ 更新目錄索引
- ✅ 添加檔案位置索引（如適用）
- ✅ 提供實作範例（如適用）
- ✅ 記錄到變更紀錄

### **知識庫涵蓋範圍**

本文件整合以下研究成果：
- 核心系統架構（PARAMS、訊息傳遞、進程管理）
- 時間系統（取得、日誌、同步）
- 電源管理（關機流程、電池管理）
- FrogPilot 模式分析（實驗模式、輕鬆模式）
- 除錯與診斷流程
- Git 工作流程與更新合併
- API 快速參考與實作案例

---

## 📚 目錄

1. [知識庫使用指南](#知識庫使用指南)
2. [專案架構概述](#專案架構概述)
3. [核心系統](#核心系統)
   - [參數系統 (PARAMS)](#參數系統-params)
   - [訊息傳遞系統](#訊息傳遞系統)
   - [進程管理](#進程管理)
4. [時間系統](#時間系統)
   - [時間取得機制](#時間取得機制)
   - [日誌記錄時間](#日誌記錄時間)
   - [時間同步服務](#時間同步服務)
5. [電源管理](#電源管理)
   - [關機流程](#關機流程)
   - [電源監控](#電源監控)
   - [Comma 3 電池管理](#comma-3-電池管理)
6. [FrogPilot 模式系統](#frogpilot-模式系統)
   - [OpenPilot 縱向控制](#openpilot-縱向控制)
   - [實驗模式 (Experimental Mode)](#實驗模式-experimental-mode)
   - [輕鬆模式 (Relaxed Mode)](#輕鬆模式-relaxed-mode)
7. [除錯與診斷](#除錯與診斷)
   - [查看 Process 錯誤](#查看-process-錯誤)
   - [卡 LOGO 診斷流程](#卡-logo-診斷流程)
   - [TMux 操作](#tmux-操作)
8. [Git 工作流程](#git-工作流程)
   - [分支管理](#分支管理)
   - [上游更新合併](#上游更新合併)
   - [回復與修復](#回復與修復)
9. [API 快速參考](#api-快速參考)
10. [開發流程與最佳實踐](#開發流程與最佳實踐)
11. [實作案例](#實作案例)

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

## 核心系統

## 參數系統 (PARAMS)

### 概述

PARAMS 是一個跨語言（C++/Python）、原子操作、支援持久化的參數儲存系統。

### 儲存位置

| 環境 | 路徑 |
|------|------|
| 裝置（Linux/AGNOS） | `/data/params/d/` |
| PC 開發環境 | `~/.comma/params/` |

### 檔案結構

**關鍵檔案**：

| 檔案 | 說明 |
|------|------|
| `common/params.h` | C++ 類別定義 |
| `common/params.cc` | C++ 實作 + 參數鍵定義（第 94-580 行）|
| `common/params_pyx.pyx` | Cython 包裝層 |
| `common/params.py` | Python 導入介面 |

### C++ API

```cpp
#include "common/params.h"

class Params {
public:
  // 建構
  explicit Params(const std::string &path = {});

  // 讀取操作
  std::string get(const std::string &key, bool block = false);
  bool getBool(const std::string &key, bool block = false);
  int getInt(const std::string &key, bool block = false);
  float getFloat(const std::string &key, bool block = false);
  std::map<std::string, std::string> readAll();

  // 寫入操作
  int put(const char *key, const char *val, size_t value_size);
  int putBool(const std::string &key, bool val);
  int putInt(const std::string &key, int val);
  int putFloat(const std::string &key, float val);

  // 非阻塞寫入（時間敏感代碼推薦）
  void putNonBlocking(const std::string &key, const std::string &val);
  void putBoolNonBlocking(const std::string &key, bool val);
  void putIntNonBlocking(const std::string &key, int val);
  void putFloatNonBlocking(const std::string &key, float val);

  // 其他
  int remove(const std::string &key);
  void clearAll(ParamKeyType type);
  bool checkKey(const std::string &key);
  std::vector<std::string> allKeys() const;
};
```

### Python API

```python
from openpilot.common.params import Params

# 初始化
params = Params()  # 使用預設路徑
params = Params("/custom/path")  # 自訂路徑

# 字串操作
params.put("MyKey", "my_value")
value = params.get("MyKey")  # 返回 bytes
value_str = params.get("MyKey", encoding='utf8')  # 返回 str

# 布林值
params.put_bool("IsEnabled", True)
is_enabled = params.get_bool("IsEnabled")

# 整數
params.put_int("Counter", 42)
count = params.get_int("Counter")

# 浮點數
params.put_float("Temperature", 98.6)
temp = params.get_float("Temperature")

# 非阻塞寫入（推薦）
params.put_nonblocking("QuickUpdate", "value")

# 阻塞讀取（等到有值）
value = params.get("CarParams", block=True)

# 檢查鍵是否存在
if params.check_key("MyKey"):
    value = params.get("MyKey")

# 移除參數
params.remove("MyKey")

# 取得所有鍵
all_keys = params.all_keys()
```

### 參數生命週期（ParamKeyType）

```cpp
enum ParamKeyType {
  PERSISTENT = 0x02,                      // 永久保存
  CLEAR_ON_MANAGER_START = 0x04,         // 管理器啟動時清除
  CLEAR_ON_ONROAD_TRANSITION = 0x08,     // 進入行駛時清除
  CLEAR_ON_OFFROAD_TRANSITION = 0x10,    // 離開行駛時清除
  DONT_LOG = 0x20,                       // 不記錄到日誌
  DEVELOPMENT_ONLY = 0x40,               // 僅開發版本
  ALL = 0xFFFFFFFF
};
```

### 新增參數步驟

**1. 在 `common/params.cc:94` 新增參數定義**：

```cpp
std::unordered_map<std::string, uint32_t> keys = {
    // ... 現有參數 ...
    {"YourNewParam", PERSISTENT},  // 新增這一行
    // ...
};
```

**2. 在程式碼中使用**：

```python
# Python
from openpilot.common.params import Params
params = Params()
params.put("YourNewParam", "value")
```

```cpp
// C++
#include "common/params.h"
Params params;
params.put("YourNewParam", "value");
```

### 原子操作保證

PARAMS 寫入使用以下機制確保原子性：

```cpp
// params.cc:613-650
int Params::put(const char* key, const char* value, size_t value_size) {
  // 1. 建立臨時檔案
  std::string tmp_path = params_path + "/.tmp_value_XXXXXX";
  int tmp_fd = mkstemp((char*)tmp_path.c_str());

  // 2. 寫入資料
  write(tmp_fd, value, value_size);

  // 3. 同步到磁碟
  fsync(tmp_fd);

  // 4. 原子重命名
  rename(tmp_path.c_str(), getParamPath(key).c_str());

  // 5. 同步目錄
  fsync_dir(getParamPath());

  close(tmp_fd);
}
```

**關鍵特性**：
- ✅ 寫入過程不會損壞現有資料
- ✅ 斷電後資料完整（已 fsync）
- ✅ 多進程安全

### FrogPilot 多層 PARAMS

```python
# frogpilot/common/frogpilot_variables.py
params = Params()                          # 主系統參數 (/data/params)
params_cache = Params("/cache/params")    # 快取參數（重啟保留）
params_memory = Params("/dev/shm/params") # 記憶體參數（重啟清除）
```

**用途**：
- `params`: 全系統持久化，重啟和斷電保留
- `params_cache`: 快取資料，重啟保留但可被清除
- `params_memory`: 記憶體中，快速存取，重啟清除

### 常用參數清單

#### 時間相關參數

| 參數名 | 類型 | 清除規則 | 用途 |
|--------|------|---------|------|
| `LastUpdateTime` | PERSISTENT | - | 上次系統更新時間（ISO 格式）|
| `LastGPSPosition` | PERSISTENT | - | 最後 GPS 位置（JSON，含時間戳）|
| `Timezone` | PERSISTENT | - | 時區設定 |
| `LastAthenaPingTime` | CLEAR_ON_MANAGER_START | 管理器啟動 | 最後 Athena 連線時間 |

#### 系統管理參數

| 參數名 | 類型 | 用途 |
|--------|------|------|
| `DoShutdown` | CLEAR_ON_MANAGER_START | 觸發關機 |
| `DoReboot` | CLEAR_ON_MANAGER_START | 觸發重啟 |
| `DoUninstall` | CLEAR_ON_MANAGER_START | 觸發卸載 |
| `DisablePowerDown` | PERSISTENT | 禁用自動關機 |
| `ForcePowerDown` | PERSISTENT | 強制關機 |

#### FrogPilot 特定參數

| 參數名 | 類型 | 預設值 | 用途 |
|--------|------|--------|------|
| `DeviceShutdown` | PERSISTENT | "9" | 關機計時器（0-33）|
| `LowVoltageShutdown` | PERSISTENT | "11.8" | 低電壓關機閾值 |
| `FrogPilotToggles` | MEMORY | - | FrogPilot 功能開關 |
| `FrogPilotStats` | PERSISTENT | - | FrogPilot 統計數據 |

---

## 訊息傳遞系統

### 架構

FrogPilot 使用 **cereal + msgq** 進行進程間通訊（IPC）。

### 訊息定義位置

- **協議定義**: `cereal/*.capnp`
- **主要定義**: `cereal/log.capnp`

### Event 結構

```capnp
# cereal/log.capnp:2302
struct Event {
  logMonoTime @0 :UInt64;  # 單調時間戳（納秒）
  valid @67 :Bool = true;

  union {
    initData @1 :InitData;
    deviceState @4 :DeviceState;
    gpsLocation @21 :GpsLocationData;
    clocks @35 :Clocks;
    # ... 更多訊息類型
  }
}
```

### Python 使用範例

```python
import cereal.messaging as messaging

# Publisher（發布者）
pm = messaging.PubMaster(['myTopic'])
msg = messaging.new_message('myTopic')
msg.myTopic.field = value
pm.send('myTopic', msg)

# Subscriber（訂閱者）
sm = messaging.SubMaster(['myTopic'])
while True:
    sm.update(timeout=1000)  # 等待 1000ms

    if sm.updated['myTopic']:
        data = sm['myTopic']
        timestamp = sm.logMonoTime['myTopic']  # 納秒時間戳
```

### 自動時間戳

所有訊息自動附帶時間戳：

```cpp
// cereal/messaging/messaging.h:54-62
cereal::Event::Builder initEvent(bool valid = true) {
  cereal::Event::Builder event = initRoot<cereal::Event>();

  struct timespec t;
  clock_gettime(CLOCK_BOOTTIME, &t);
  uint64_t current_time = t.tv_sec * 1000000000ULL + t.tv_nsec;

  event.setLogMonoTime(current_time);  // 自動設定
  event.setValid(valid);
  return event;
}
```

---

## 進程管理

### Manager 架構

**位置**: `system/manager/manager.py`

### 進程配置

**位置**: `system/manager/process_config.py`

### 啟動流程

```
launch_openpilot.sh → launch_chffrplus.sh
  ↓
[AGNOS] 初始化 GPU 和設備
  ↓
[AGNOS] 版本檢查和更新
  ↓
檢查 Overlay 更新
  ↓
執行 manager.py
  ├─ manager_init()     # 初始化
  └─ manager_thread()   # 主迴圈
       ↓
       啟動所有進程（timed, loggerd, controlsd, etc.）
       ↓
       監控進程狀態和關機條件
```

### manager_init() 功能

```python
# system/manager/manager.py:25-123
def manager_init() -> None:
    # 1. 保存啟動日誌
    save_bootlog()

    # 2. 初始化 Params
    params = Params()

    # 3. 清除特定參數
    params.clear_all(ParamKeyType.CLEAR_ON_MANAGER_START)
    params.clear_all(ParamKeyType.CLEAR_ON_ONROAD_TRANSITION)
    params.clear_all(ParamKeyType.CLEAR_ON_OFFROAD_TRANSITION)

    # 4. FrogPilot 初始化
    setup_frogpilot(build_metadata)
    convert_params(params_cache)

    # 5. 設定預設參數
    default_params = [...]
    if not PC:
        default_params.append(
            ("LastUpdateTime", datetime.datetime.utcnow().isoformat())
        )

    # 6. 註冊設備
    reg_res = register(show_spinner=True)

    # 7. 預載所有進程
    for p in managed_processes.values():
        p.prepare()
```

### 進程生命週期

```python
# system/manager/process.py
class ManagerProcess:
    def start(self):
        # 啟動進程

    def stop(self, sig=SIGINT):
        # 1. 發送 SIGINT（優雅終止）
        # 2. 等待 5 秒
        # 3. 如果未退出，發送 SIGKILL（強制終止）

    def signal(self, sig):
        # 發送信號到進程
```

---

## 時間系統

## 時間取得機制

### 核心時間函式庫

**位置**: `common/timing.h`

### C++ 時間函數

```cpp
#include "common/timing.h"

// 單調時間（不受系統時鐘調整影響）
uint64_t nanos_since_boot()      // 納秒，使用 CLOCK_BOOTTIME
double millis_since_boot()       // 毫秒
double seconds_since_boot()      // 秒

// 系統時間（實際時鐘時間）
uint64_t nanos_since_epoch()     // 納秒，使用 CLOCK_REALTIME
double seconds_since_epoch()     // 秒

// 備用單調時間
uint64_t nanos_monotonic()       // 使用 CLOCK_MONOTONIC
uint64_t nanos_monotonic_raw()   // 使用 CLOCK_MONOTONIC_RAW
```

### 時鐘類型說明

| 時鐘類型 | 用途 | 受系統調整影響 | 使用場景 |
|---------|------|---------------|---------|
| `CLOCK_BOOTTIME` | 主要計時 | 否 | 控制循環、事件排序 |
| `CLOCK_MONOTONIC` | 備用單調 | 否 | Python time.monotonic() |
| `CLOCK_REALTIME` | 系統時間 | 是 | 日誌時間戳、GPS 同步 |

### Python 時間工具

**位置**: `common/realtime.py`

```python
from openpilot.common.realtime import Ratekeeper, DT_CTRL

# 創建速率保持器（100 Hz）
rk = Ratekeeper(100)

while True:
    # ... 控制邏輯 ...

    # 維持循環週期
    lagged = rk.keep_time()
    if lagged:
        print("Loop is lagging!")
```

### 預定義時間步長

```python
# common/realtime.py
DT_CTRL = 0.01   # controlsd - 100 Hz (10ms)
DT_MDL = 0.05    # model - 20 Hz (50ms)
DT_HW = 0.5      # hardware/manager - 2 Hz
DT_DMON = 0.05   # driver monitoring - 20 Hz
```

### 時間驗證

**位置**: `common/time.py`

```python
from openpilot.common.time import system_time_valid

if system_time_valid():
    # 系統時間晚於 2024-03-30 或 systemd 建置時間 + 1 天
    print("System time is valid")
else:
    print("System time is invalid, need to sync")
```

---

## 日誌記錄時間

### Cloudlog 系統

**位置**:
- Python: `common/swaglog.py`
- C++: `common/swaglog.cc`, `common/swaglog.h`

### Python 日誌使用

```python
from openpilot.common.swaglog import cloudlog

cloudlog.debug("Debug message")
cloudlog.info("Info message")
cloudlog.warning("Warning message")
cloudlog.error("Error message")
cloudlog.critical("Critical message")
cloudlog.exception("Exception occurred")
```

### C++ 日誌使用

```cpp
#include "common/swaglog.h"

cloudlog("Info message");
cloudlog_warn("Warning message");
cloudlog_err("Error message");

// 速率限制日誌（避免洪水）
cloudlog_rl(10, 1000, "Rate limited message");
// 10 次爆發，1000ms 內
```

### 日誌時間戳

**C++ 實作**：
```cpp
// swaglog.cc:94
{"created", seconds_since_epoch()}  // 日誌建立時間

// swaglog.cc:129
{"time", std::to_string(nanos_since_boot())}  // 事件時間戳
```

**Python 實作**：
```python
# swaglog.py:64
record_dict['created'] = record.created  # Python logging 的時間戳（秒）
```

### Loggerd 服務

**位置**: `system/loggerd/`

**功能**：
- 記錄所有 cereal 訊息到檔案
- 60 秒一個段（segment）
- 壓縮和上傳日誌

**時間戳設定**：

```cpp
// logger.cc:16
uint64_t wall_time = nanos_since_epoch();
init.setWallTimeNanos(wall_time);
```

### 日誌檔案命名

**格式**: `{route_name}--{segment_number}`

**範例**: `000001a3--c20ba54385--0`
- `000001a3--c20ba54385` = 路由名稱
- `0` = 段號

---

## 時間同步服務

### timed.py 服務

**位置**: `system/timed.py`

**職責**：
1. 從 GPS 獲取精確時間
2. 從 GPS 座標查詢時區
3. 設定系統時間和時區

### 主要流程

```python
def main() -> NoReturn:
    params = Params()

    # 1. 恢復時區
    tz = params.get("Timezone", encoding='utf8')
    if tz is not None:
        set_timezone(tz)

    # 2. 發布時鐘訊息
    pm = messaging.PubMaster(['clocks'])
    sm = messaging.SubMaster(['liveLocationKalman'])

    while True:
        sm.update(1000)

        # 發布系統時鐘狀態
        msg = messaging.new_message('clocks')
        msg.valid = system_time_valid()
        msg.clocks.wallTimeNanos = time.time_ns()
        pm.send('clocks', msg)

        # 3. 從 GPS 同步時間
        llk = sm['liveLocationKalman']
        if llk.gpsOK:
            gps_time = datetime.fromtimestamp(llk.unixTimestampMillis / 1000.)
            set_time(gps_time)

            # 4. 從 GPS 座標查詢時區
            pos = llk.positionGeodetic.value
            gps_timezone = tf.timezone_at(lat=pos[0], lng=pos[1])
            set_timezone(gps_timezone)
            params.put_nonblocking("Timezone", gps_timezone)

        time.sleep(10)
```

### 時區設定

**AGNOS 系統**：
```python
def set_timezone(timezone):
    if AGNOS:
        tzpath = os.path.join("/usr/share/zoneinfo/", timezone)
        subprocess.check_call(
            f'sudo su -c "ln -snf {tzpath} /data/etc/tmptime && '
            f'mv /data/etc/tmptime /data/etc/localtime"',
            shell=True
        )
```

**其他系統**：
```python
subprocess.check_call(f'sudo timedatectl set-timezone {timezone}', shell=True)
```

---

## 電源管理

## 關機流程

### 完整流程圖

```
拔掉車輛電源
  ↓
內建電池供電開始
  ↓
hardwared.py 監測關機條件（每秒）
  ├─ 離線時間 > device_shutdown_time
  ├─ 電壓 < low_voltage_cutoff
  └─ 電池容量耗盡
  ↓
params.put_bool("DoShutdown", True)
  ↓
manager.py 偵測到 DoShutdown
  ↓
跳出 manager_thread() 主迴圈
  ↓
manager_cleanup()
  ├─ 發送 SIGINT 到所有進程
  ├─ 等待 5 秒
  └─ 未退出者發送 SIGKILL
  ↓
HARDWARE.shutdown()
  ↓
執行 "sudo poweroff"
```

### 關機觸發條件

**位置**: `system/hardware/power_monitoring.py:110-128`

```python
def should_shutdown(self, ignition, in_car, offroad_timestamp, started_seen, frogpilot_toggles):
    offroad_time = (now - offroad_timestamp)

    # 計算各種關機條件
    low_voltage_shutdown = (
        self.car_voltage_mV < (frogpilot_toggles.low_voltage_shutdown * 1e3)
        and offroad_time > 60
    )

    should_shutdown = False
    should_shutdown |= offroad_time > frogpilot_toggles.device_shutdown_time  # 時間到
    should_shutdown |= low_voltage_shutdown                                    # 低電壓
    should_shutdown |= (self.car_battery_capacity_uWh <= 0)                   # 沒電
    should_shutdown &= not ignition                                            # 未行駛
    should_shutdown &= (not self.params.get_bool("DisablePowerDown"))         # 允許關機
    should_shutdown &= in_car                                                  # 在車上
    should_shutdown &= offroad_time > DELAY_SHUTDOWN_TIME_S                   # 延遲 5 分鐘
    should_shutdown |= self.params.get_bool("ForcePowerDown")                 # 強制關機
    should_shutdown &= started_seen or (now > MIN_ON_TIME_S)                  # 最小開機時間

    return should_shutdown
```

### 關機常數

```python
# power_monitoring.py
DELAY_SHUTDOWN_TIME_S = 300              # 5 分鐘保護期
VOLTAGE_SHUTDOWN_MIN_OFFROAD_TIME_S = 60 # 低壓關機至少等 60 秒
MAX_TIME_OFFROAD_S = 30*3600             # 最大離線 30 小時
MIN_ON_TIME_S = 3600                     # 最小開機時間 1 小時
```

### 信號處理

**進程停止流程**：

```python
# process.py:112-142
def stop(self, sig=signal.SIGINT):
    if self.proc.exitcode is None:
        # 1. 發送信號（預設 SIGINT）
        self.signal(sig)

        # 2. 等待 5 秒
        join_process(self.proc, 5)

        # 3. 如果還沒退出，發送 SIGKILL
        if self.proc.exitcode is None:
            cloudlog.info(f"killing {self.name} with SIGKILL")
            self.signal(signal.SIGKILL)
            self.proc.join()
```

### Manager 退出處理

```python
# manager.py:223-242
signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(1))

try:
    manager_thread()
except Exception:
    traceback.print_exc()
    sentry.capture_exception()
finally:
    manager_cleanup()  # 確保清理

# 執行關機操作
params = Params()
if params.get_bool("DoUninstall"):
    uninstall_frogpilot()
elif params.get_bool("DoReboot"):
    HARDWARE.reboot()
elif params.get_bool("DoShutdown"):
    HARDWARE.shutdown()
```

---

## 電源監控

### PowerMonitoring 類別

**位置**: `system/hardware/power_monitoring.py`

### 電池容量計算

```python
class PowerMonitoring:
    CAR_BATTERY_CAPACITY_uWh = 30e6  # 30Wh
    CAR_CHARGING_RATE_W = 45         # 45W
    VBATT_PAUSE_CHARGING = 11.8      # 11.8V

    def calculate(self, voltage, ignition):
        # 低通濾波電壓
        self.car_voltage_mV = (
            (voltage * CAR_VOLTAGE_LOW_PASS_K) +
            (self.car_voltage_mV * (1 - CAR_VOLTAGE_LOW_PASS_K))
        )

        if ignition:
            # 行駛中：累積充電
            self.car_battery_capacity_uWh += (CAR_CHARGING_RATE_W * 1e6 * integration_time_h)
        else:
            # 離線中：減少電量
            current_power = HARDWARE.get_current_power_draw()
            power_used = (current_power * 1000000) * integration_time_h
            self.car_battery_capacity_uWh -= power_used

        # 每 10 秒保存一次
        if now - self.last_save_time >= 10:
            self.params.put_nonblocking("CarBatteryCapacity", str(int(self.car_battery_capacity_uWh)))
```

---

## Comma 3 電池管理

### Device Shutdown Timer

**參數**: `DeviceShutdown`（0-33）

**計算方式**：
```python
# frogpilot_variables.py:739
if device_shutdown_setting >= 4:
    device_shutdown_time = (device_shutdown_setting - 3) * 3600  # 小時
else:
    device_shutdown_time = device_shutdown_setting * (60 * 15)   # 15分鐘
```

**設定對照表**：

| 設定值 | 實際時間 |
|-------|---------|
| 0 | 立即關機 |
| 1 | 15 分鐘 |
| 2 | 30 分鐘 |
| 3 | 45 分鐘 |
| 4 | 1 小時 |
| 5 | 2 小時 |
| 9（預設）| 6 小時 |
| 33 | 30 小時 |

### Low Voltage Cutoff

**參數**: `LowVoltageShutdown`（11.8 - 12.5V）

**電壓對照**：
- 12.6V = 滿電（100%）
- 12.4V = 75% 電量
- 12.2V = 50% 電量
- 12.0V = 25% 電量
- 11.8V = 接近放電極限（預設值）

**說明**: 設定過低會導致電池過度放電，建議保持 11.8-12.0V 之間。

---

## FrogPilot 模式系統

FrogPilot 提供三種主要模式來控制駕駛行為，理解這些模式對於開發和調整非常重要。

## OpenPilot 縱向控制

### 概述

OpenPilot 縱向控制是指系統接管車輛的油門與煞車控制，是使用進階功能（如實驗模式）的前提。

### 啟用條件

**位置**: `selfdrive/car/volkswagen/interface.py:86-93`

```python
ret.experimentalLongitudinalAvailable = ret.networkLocation == NetworkLocation.gateway or docs

if experimental_long:
    ret.openpilotLongitudinalControl = True
    ret.safetyConfigs[0].safetyParam |= Panda.FLAG_VOLKSWAGEN_LONG_CONTROL
    if ret.transmissionType == TransmissionType.manual:
        ret.minEnableSpeed = 4.5

ret.pcmCruise = not ret.openpilotLongitudinalControl
```

### 關鍵參數

| 參數 | 值 | 說明 |
|------|-----|------|
| `openpilotLongitudinalControl` | `bool` | 是否啟用 OP 縱向控制 |
| `pcmCruise` | `bool` | 原廠定速（與 OP 縱向互斥）|
| `radarUnavailable` | `bool` | VW 車輛預設為 `True` |
| `networkLocation` | `enum` | 必須為 `gateway` 才能啟用縱向控制 |

### 控制特性

- ✅ 完全接管油門與煞車
- ✅ 停車控制 (`stoppingControl = True`)
- ✅ 自動恢復（視變速箱類型）
- ❌ VW 車輛無雷達支援（依賴視覺模型）

---

## 實驗模式 (Experimental Mode)

### 概述

實驗模式啟用進階駕駛輔助功能，包含自動變道、紅綠燈辨識、彎道速度控制等。

### 條件式實驗模式 (CEM)

**位置**: `frogpilot/controls/lib/conditional_experimental_mode.py`

**核心變數**：
```python
# frogpilot/common/frogpilot_variables.py:633
toggle.conditional_experimental_mode = toggle.openpilot_longitudinal and \
    params.get_bool("ConditionalExperimental")
```

### 觸發條件

**位置**: `conditional_experimental_mode.py:36-70`

```python
def check_conditions(self, v_ego, sm, frogpilot_toggles):
    # 1. 速度條件
    below_speed = frogpilot_toggles.conditional_limit > v_ego >= 1

    # 2. 打方向燈且車道不足
    if 方向燈 and not lane_available:
        return True

    # 3. 導航接近路口/轉彎
    if frogpilot_toggles.conditional_navigation and approaching_maneuver:
        return True

    # 4. 偵測到彎道
    if frogpilot_toggles.conditional_curves and self.curve_detected:
        return True

    # 5. 前方有慢速車輛
    if frogpilot_toggles.conditional_lead and self.slow_lead_detected:
        return True

    # 6. 偵測到紅綠燈/停止標誌
    if frogpilot_toggles.conditional_model_stop_time != 0 and self.stop_light_detected:
        return True
```

### CEStatus 狀態值

| 狀態值 | 觸發條件 | 說明 |
|-------|---------|------|
| 0 | 無 | 未啟用 |
| 1 | 手動暫停 | 使用者手動關閉 |
| 2 | 手動啟用 | 使用者手動開啟 |
| 3 | 低速無前車 | 速度低於設定值 |
| 4 | 低速有前車 | 跟車且速度低 |
| 5 | 打方向燈 | 車道寬度不足 |
| 6 | 接近路口 | 導航觸發 |
| 7 | 接近轉彎 | 導航觸發 |
| 8 | 偵測彎道 | 曲率觸發 |
| 9 | 前車停止 | 前車速度 < 1 m/s |
| 10 | 前車緩慢 | 前車減速中 |
| 11 | 紅綠燈 | 模型偵測停止點 |
| 12 | 強制停止 | SLC 觸發停止 |
| 13 | 速限控制 | SLC 實驗模式 |

### 程式位置

- **主控制**: `frogpilot/controls/lib/conditional_experimental_mode.py`
- **整合**: `frogpilot/controls/frogpilot_planner.py:62-66`
- **UI 控制**: `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc`

---

## 輕鬆模式 (Relaxed Mode)

### 概述

輕鬆模式是一種個性化駕駛風格設定，提供更長的跟車距離和更平順的加減速。

### 核心參數

**位置**: `frogpilot/common/frogpilot_variables.py:320-326`

```python
("RelaxedFollow", "1.75", 2, "1.75")                    # 跟車時間 1.75秒
("RelaxedJerkAcceleration", "100", 3, "100")            # 加速Jerk 100%
("RelaxedJerkDanger", "100", 3, "100")                  # 危險Jerk 100%
("RelaxedJerkDeceleration", "100", 3, "100")            # 減速Jerk 100%
("RelaxedJerkSpeed", "100", 3, "100")                   # 速度Jerk 100%
("RelaxedJerkSpeedDecrease", "100", 3, "100")           # 速度減少Jerk 100%
```

### 個性化對比

| 參數 | Aggressive | Standard | Relaxed | Traffic |
|------|------------|----------|---------|---------|
| **跟車時間** | 1.25s | 1.45s | **1.75s** | 0.5s |
| **加速 Jerk** | 50% | 100% | **100%** | 50% |
| **減速 Jerk** | 50% | 100% | **100%** | 50% |
| **危險 Jerk** | 100% | 100% | **100%** | 100% |
| **速度 Jerk** | 50% | 100% | **100%** | 50% |

> **Jerk**: 加速度的變化率，數值越低越激進，越高越平順

### 程式位置

**跟車邏輯**: `frogpilot/controls/lib/frogpilot_following.py:34-55`

```python
self.base_acceleration_jerk, self.base_danger_jerk, self.base_speed_jerk = get_jerk_factor(
    frogpilot_toggles.aggressive_jerk_acceleration,
    frogpilot_toggles.standard_jerk_acceleration,
    frogpilot_toggles.relaxed_jerk_acceleration,  # 輕鬆模式參數
    frogpilot_toggles.custom_personalities,
    sm["controlsState"].personality  # 當前選擇的個性
)

self.t_follow = get_T_FOLLOW(
    frogpilot_toggles.aggressive_follow,     # 1.25s
    frogpilot_toggles.standard_follow,       # 1.45s
    frogpilot_toggles.relaxed_follow,        # 1.75s
    frogpilot_toggles.custom_personalities,
    sm["controlsState"].personality
)
```

---

## 除錯與診斷

系統性的除錯流程能大幅提升開發效率，以下是完整的除錯指南。

## 查看 Process 錯誤

### 方法一：使用 TMux（最直接）

```bash
# 1. SSH 連線到 Comma 3
ssh comma@192.168.1.xxx

# 2. 進入 tmux session
tmux attach -t comma

# 3. 查看 process 輸出
# 按 Ctrl+B 然後按 [ 進入滾動模式
# 使用方向鍵滾動，按 / 搜尋關鍵字
# 按 q 退出滾動模式

# 4. 退出 tmux
# 按 Ctrl+B 然後按 D
```

### 方法二：使用 Journalctl

```bash
# 查看最近日誌
journalctl -u comma --no-pager -n 100

# 只看錯誤
journalctl -u comma --no-pager | grep -i error | tail -50

# 即時監控
journalctl -u comma -f

# 查看特定 process
journalctl -u comma --no-pager | grep timed | tail -50
```

### 方法三：手動運行 Process

```bash
# 停止服務
sudo systemctl stop comma

# 手動運行查看錯誤
cd /data/openpilot
export PYTHONPATH=/data/openpilot
python system/timed.py  # 替換成要測試的 process

# 重啟服務
sudo systemctl start comma
```

---

## 卡 LOGO 診斷流程

當系統卡在 LOGO 畫面時的標準診斷流程。

### 初步診斷

```bash
# 1. 查看 tmux session
tmux attach -t comma

# 2. 檢查系統日誌
journalctl -u comma --no-pager -n 100

# 3. 手動測試 manager
sudo systemctl stop comma
cd /data/openpilot
python system/manager/manager.py
```

### 檢查模組載入

```bash
cd /data/openpilot

# 測試關鍵模組
python -c "from system.manager import manager; print('✓ manager.py OK')"
python -c "from system.manager import process_config; print('✓ process_config.py OK')"
python -c "from selfdrive.controls import controlsd; print('✓ controlsd.py OK')"
```

### Git 回復

```bash
# 查看最近變更
git log --oneline -5

# 回復到前一個 commit
git checkout HEAD~1
sudo reboot

# 回復特定檔案
git checkout HEAD~1 -- path/to/problematic_file.py
git add path/to/problematic_file.py
git commit -m "Revert problematic file"
sudo reboot
```

### 快速診斷腳本

```bash
cat > /data/diagnose.sh << 'EOF'
#!/bin/bash
echo "=== OpenPilot Diagnostic ==="
echo "Date: $(date)"
echo ""

echo "=== Git Status ==="
cd /data/openpilot
git log --oneline -3
echo ""

echo "=== Python Module Check ==="
python -c "from system.manager import manager; print('✓')" 2>&1
echo ""

echo "=== Recent Errors ==="
journalctl -u comma --no-pager -n 50 | grep -i error | tail -5
echo ""
EOF

chmod +x /data/diagnose.sh
/data/diagnose.sh
```

---

## TMux 操作

### 基本操作

```bash
# 查看所有 sessions
tmux ls

# 進入 comma session
tmux attach -t comma

# 創建新 session
tmux new -s debug
```

### 快捷鍵（先按 Ctrl+B）

```
D         - 分離 session（退出但不關閉）
[         - 進入滾動/複製模式
]         - 貼上
C         - 創建新視窗
N         - 下一個視窗
P         - 上一個視窗
W         - 列出所有視窗
?         - 顯示所有快捷鍵
```

### 滾動模式操作

```
方向鍵      - 上下左右滾動
Page Up/Down - 快速翻頁
/            - 向下搜尋
?            - 向上搜尋
N            - 下一個匹配
Shift+N      - 上一個匹配
Space        - 開始選取
Enter        - 完成選取並複製
Q 或 Esc     - 退出滾動模式
```

---

## Git 工作流程

## 分支管理

### 當前分支結構

```
main (本地主分支)
  └─ hfop (開發分支)
       ↓
  upstream/FrogPilot (上游)
```

### 基本操作

```bash
# 查看分支
git branch -a

# 切換分支
git checkout hfop

# 創建新分支
git checkout -b feature/new-feature

# 查看狀態
git status

# 查看最近 commits
git log --oneline -5
```

---

## 上游更新合併

### 方法一：Merge（推薦新手）

```bash
# 1. 確保在 hfop 分支
git checkout hfop

# 2. 獲取上游更新
git fetch upstream

# 3. 查看更新
git log --oneline HEAD..upstream/FrogPilot

# 4. 合併
git merge upstream/FrogPilot

# 5. 推送
git push origin hfop
```

**優點**：
- 保留完整歷史
- 較容易理解
- 衝突較容易解決

**缺點**：
- 有合併提交
- 歷史較複雜

### 方法二：Rebase（進階用戶）

```bash
# 1. 切換到 hfop
git checkout hfop

# 2. Rebase 到上游
git rebase upstream/FrogPilot

# 3. 推送（需要 force）
git push origin hfop --force-with-lease
```

**優點**：
- 線性歷史
- 更清晰

**缺點**：
- 需要逐個解決衝突
- 改寫歷史

### 處理衝突

```bash
# 查看衝突
git status

# 編輯衝突檔案（尋找 <<<<<<< HEAD 標記）

# 標記衝突已解決
git add [衝突的檔案]

# 繼續合併
git merge --continue  # Merge 模式
# 或
git rebase --continue  # Rebase 模式
```

---

## 回復與修復

### 回復到前一個版本

```bash
# 方法一：暫存當前變更
git stash
sudo reboot

# 方法二：回到前一個 commit
git checkout HEAD~1
sudo reboot

# 方法三：重置（會遺失變更）
git reset --hard HEAD~1
sudo reboot
```

### 回復特定檔案

```bash
# 回復單一檔案
git checkout HEAD~1 -- path/to/file.py

# 提交修復
git add path/to/file.py
git commit -m "Revert file to fix issue"
```

### 清理並重新編譯

```bash
cd /data/openpilot

# 清理編譯檔案
find . -name "*.pyc" -delete
find . -name "__pycache__" -type d -delete

# 重新編譯
scons -j4

sudo reboot
```

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

### 案例 1：新增持久化參數

**需求**: 保存最後已知的系統時間

**步驟**:

1. **定義參數** (`common/params.cc`):
```cpp
{"LastKnownGoodTime", PERSISTENT},
```

2. **讀取參數** (`system/timed.py`):
```python
params = Params()
last_time_str = params.get("LastKnownGoodTime", encoding='utf8')
if last_time_str:
    last_time = datetime.fromisoformat(last_time_str)
```

3. **寫入參數** (關機時):
```python
import signal

def signal_handler(signum, frame):
    if system_time_valid():
        params.put("LastKnownGoodTime", datetime.now().isoformat())
    sys.exit(0)

signal.signal(signal.SIGTERM, signal_handler)
signal.signal(signal.SIGINT, signal_handler)
```

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

## 附錄

### 重要檔案位置索引

| 功能 | 檔案路徑 |
|------|---------|
| 參數系統 C++ | `common/params.h`, `common/params.cc` |
| 參數系統 Python | `common/params.py`, `common/params_pyx.pyx` |
| 時間工具 | `common/timing.h` |
| 日誌系統 | `common/swaglog.*` |
| 訊息定義 | `cereal/log.capnp` |
| 進程管理 | `system/manager/manager.py` |
| 進程配置 | `system/manager/process_config.py` |
| 時間同步 | `system/timed.py` |
| 電源監控 | `system/hardware/power_monitoring.py` |
| 硬體抽象 | `system/hardware/hw.py` |
| 日誌記錄器 | `system/loggerd/logger.cc` |
| FrogPilot 變數 | `frogpilot/common/frogpilot_variables.py` |

### 訊息類型索引

| 訊息 | 發布者 | 訂閱者 | 頻率 |
|------|-------|-------|------|
| `clocks` | timed.py | 多個 | 1 Hz |
| `deviceState` | hardwared.py | manager, controlsd | 1 Hz |
| `liveLocationKalman` | locationd | timed, planner | 20 Hz |
| `carParams` | car interface | controlsd, planner | 啟動時 |
| `controlsState` | controlsd | ui, loggerd | 100 Hz |
| `managerState` | manager | ui | 1 Hz |

### 常用常數

```python
# 時間
DT_CTRL = 0.01      # 10ms
DT_MDL = 0.05       # 50ms

# 電源
VBATT_PAUSE_CHARGING = 11.8              # 11.8V
CAR_BATTERY_CAPACITY_uWh = 30e6          # 30Wh
DELAY_SHUTDOWN_TIME_S = 300              # 5 分鐘

# 路徑
PARAMS_PATH = "/data/params/d/"          # 參數路徑
PERSIST_PATH = "/persist/"               # 持久化路徑
```

---

## 變更紀錄

| 日期 | 版本 | 變更內容 |
|------|------|---------|
| 2025-10-22 | v1.1 | 新增：知識庫使用指南、FrogPilot 模式系統、除錯與診斷、Git 工作流程 |
| 2025-10-22 | v1.0 | 初始版本，包含時間系統、PARAMS、電源管理研究成果 |

---

**文件維護指南**：

1. 每次深入研究後，將結果整合到對應章節
2. 保持實作案例的更新
3. 記錄所有 API 變更
4. 維護檔案位置索引的準確性
5. 添加實際遇到的問題和解決方案

**下次研究主題建議**：
- [ ] 車輛介面（selfdrive/car）架構
- [ ] UI 系統（Qt/QML）結構
- [ ] 模型推理流程（modeld）
- [ ] CAN 通訊機制
- [ ] 日誌上傳和下載流程
