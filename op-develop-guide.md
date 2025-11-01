# FrogPilot / openpilot 開發指南

> **文件版本**: v1.9
> **最後更新**: 2025-10-23
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
6. [控制系統](#控制系統)
   - [控制啟用/禁用架構](#控制啟用禁用架構)
   - [油門與煞車的控制邏輯](#油門與煞車的控制邏輯)
   - [Panda Safety 層](#panda-safety-層)
   - [controlsd 狀態機](#controlsd-狀態機)
7. [FrogPilot 模式系統](#frogpilot-模式系統)
   - [OpenPilot 縱向控制](#openpilot-縱向控制)
   - [實驗模式 (Experimental Mode)](#實驗模式-experimental-mode)
   - [輕鬆模式 (Relaxed Mode)](#輕鬆模式-relaxed-mode)
8. [FrogPilot 導航與地圖系統](#frogpilot-導航與地圖系統)
   - [mapd 服務架構](#mapd-服務架構)
   - [啟動流程](#啟動流程-1)
   - [PARAMS 寫入機制](#params-寫入機制)
   - [故障診斷](#故障診斷)
9. [FrogPilot UI 系統](#frogpilot-ui-系統)
   - [UI 架構概述](#ui-架構概述)
   - [彎道圖標系統](#彎道圖標系統)
   - [UI 位置計算系統](#ui-位置計算系統)
   - [實作案例：移動彎道圖標](#實作案例移動彎道圖標)
10. [除錯與診斷](#除錯與診斷)
   - [查看 Process 錯誤](#查看-process-錯誤)
   - [卡 LOGO 診斷流程](#卡-logo-診斷流程)
   - [TMux 操作](#tmux-操作)
11. [Git 工作流程](#git-工作流程)
   - [分支管理](#分支管理)
   - [上游更新合併](#上游更新合併)
   - [回復與修復](#回復與修復)
12. [API 快速參考](#api-快速參考)
13. [開發流程與最佳實踐](#開發流程與最佳實踐)
14. [實作案例](#實作案例)

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

## 控制系統

控制系統是 openpilot/FrogPilot 的核心，負責決定何時啟用/禁用駕駛輔助功能。系統採用**雙層架構**：硬體層（Panda Safety）和軟體層（controlsd 狀態機）。

### 控制啟用/禁用架構

#### 雙層架構概覽

```
┌─────────────────────────────────────────┐
│   軟體層：controlsd.py 狀態機            │
│   - enabled / disabled / overriding     │
│   - 根據事件類型轉換狀態                 │
│   - 處理 UI 交互、模式切換               │
└─────────────┬───────────────────────────┘
              │ 讀取 controls_allowed
              ↓
┌─────────────────────────────────────────┐
│   硬體層：Panda Safety (safety.h)       │
│   - controls_allowed 旗標               │
│   - 硬體級安全檢查                       │
│   - 最後防線，無法繞過                   │
└─────────────┬───────────────────────────┘
              │ 讀取 CAN 訊號
              ↓
┌─────────────────────────────────────────┐
│   車輛 CAN Bus                          │
│   - 油門/煞車踏板狀態                    │
│   - ACC 狀態 (cruise_engaged)           │
│   - 車速、轉向角度等                     │
└─────────────────────────────────────────┘
```

**關鍵變數**：
- `controls_allowed` (Panda): 硬體層控制旗標，`false` 時**強制禁止**所有控制指令
- `state` (controlsd): 軟體層狀態機，決定 UI 顯示和邏輯行為

---

### 油門與煞車的控制邏輯

#### 核心問題

**現象**：
- OP 控制啟動時踩油門 → 控制禁用 → 放掉油門 → **自動恢復**
- OP 控制啟動時踩煞車 → 控制禁用 → 放掉煞車 → **無法自動恢復**

**為什麼？**

| 操作 | Panda 層影響 | 原廠 ACC 影響 | 恢復方式 |
|------|-------------|--------------|---------|
| **油門** | `get_longitudinal_allowed() = false` | **Cruise 保持啟用** | 自動（油門放開） |
| **煞車** | ~~無直接影響（FrogPilot 已移除）~~ | **Cruise 關閉** (`cruise_engaged = false`) | 需重新按 SET/RESUME |

---

### Panda Safety 層

#### 關鍵函數 1：`get_longitudinal_allowed()`

**檔案**：`panda/board/safety.h:90-92`

```c
bool get_longitudinal_allowed(void) {
  return controls_allowed && !gas_pressed_prev;
}
```

**作用**：
- 即使 `controls_allowed = true`，如果**油門踩下**，縱向控制仍被禁止
- 橫向控制（轉向）不受影響
- 這就是為什麼**油門只影響加速/減速，不影響轉向**

**應用**：
- 所有縱向控制檢查（油門、煞車、加速度指令）都必須通過此函數
- 例如：`longitudinal_accel_checks()` 中會呼叫此函數

---

#### 關鍵函數 2：`generic_rx_checks()`

**檔案**：`panda/board/safety.h:258-281`

```c
void generic_rx_checks(bool stock_ecu_detected) {
  // exit controls on rising edge of gas press
  if (gas_pressed && !gas_pressed_prev && !(alternative_experience & ALT_EXP_DISABLE_DISENGAGE_ON_GAS)) {
    controls_allowed = false;  // ← 油門踩下會禁用（可透過旗標關閉）
  }
  gas_pressed_prev = gas_pressed;

  // exit controls on rising edge of brake press
  if (brake_pressed && (!brake_pressed_prev || vehicle_moving)) {
    controls_allowed = controls_allowed;  // ← FrogPilot 已移除煞車禁用邏輯！
  }
  brake_pressed_prev = brake_pressed;

  // exit controls on rising edge of regen paddle
  if (regen_braking && (!regen_braking_prev || vehicle_moving)) {
    controls_allowed = false;  // ← Regen 煞車仍會禁用
  }
  regen_braking_prev = regen_braking;
  ...
}
```

**重要發現**：
- **FrogPilot 0.9.7 已將煞車邏輯改為 `controls_allowed = controls_allowed`（什麼都不做）**
- 原始 openpilot：`controls_allowed = false`
- 煞車不再直接禁用控制！

**Git 歷史**：
```bash
# FrogPilot 0.9.7 commit 73821d94
-    controls_allowed = false;
+    controls_allowed = controls_allowed;
```

---

#### 關鍵函數 3：`pcm_cruise_check()`

**檔案**：`panda/board/safety.h:700-709`

```c
void pcm_cruise_check(bool cruise_engaged) {
  // Enter controls on rising edge of stock ACC, exit controls if stock ACC disengages
  if (!cruise_engaged) {
    controls_allowed = false;  // ← Cruise 關閉時禁用
  }
  if (cruise_engaged && !cruise_engaged_prev) {
    controls_allowed = true;   // ← Cruise 開啟時啟用
  }
  cruise_engaged_prev = cruise_engaged;
}
```

**作用**：
- 這才是**真正控制 `controls_allowed` 的地方**
- 當原廠 ACC 的 `cruise_engaged` 狀態改變時更新 `controls_allowed`
- **煞車會讓原廠 ACC 關閉，從而觸發此函數**

---

#### VW 特定實作

**檔案**：`panda/board/safety/safety_volkswagen_mqb.h:141-156`

```c
if (addr == MSG_TSK_06) {
  // TSK_06 訊息包含 ACC 狀態
  // Signal: TSK_06.TSK_Status
  int acc_status = (GET_BYTE(to_push, 3) & 0x7U);
  bool cruise_engaged = (acc_status == 3) || (acc_status == 4) || (acc_status == 5);
  acc_main_on = cruise_engaged || (acc_status == 2);

  if (!volkswagen_longitudinal) {
    pcm_cruise_check(cruise_engaged);  // ← 使用原廠 ACC 時呼叫
  }

  if (!acc_main_on) {
    controls_allowed = false;  // ← ACC 主開關關閉也會禁用
  }
}
```

**ACC 狀態值**：
- `2` = ACC 待命（主開關開啟，未設定速度）
- `3` = ACC 啟用（跟車中）
- `4` = ACC 啟用（定速中）
- `5` = ACC 啟用（其他狀態）

**邏輯**：
- 踩煞車 → 原廠 ACC `acc_status` 變為非 3/4/5
- `cruise_engaged = false`
- `pcm_cruise_check(false)` 被呼叫
- `controls_allowed = false`

---

#### Alternative Experience 旗標

**檔案**：`panda/board/safety_declarations.h:255-277`

```c
// 可透過 USB 命令設定的替代體驗旗標
#define ALT_EXP_DISABLE_DISENGAGE_ON_GAS 1        // 禁用油門解除控制
#define ALT_EXP_DISABLE_STOCK_AEB 2               // 禁用原廠 AEB
#define ALT_EXP_RAISE_LONGITUDINAL_LIMITS_TO_ISO_MAX 8  // 提高縱向限制
#define ALT_EXP_ALLOW_AEB 16                      // 允許 AEB 指令
#define ALT_EXP_ALWAYS_ON_LATERAL 32              // Always On Lateral (AOL)

int alternative_experience = 0;
```

**應用**：
```c
// 如果設定了 ALT_EXP_DISABLE_DISENGAGE_ON_GAS，油門不會禁用控制
if (gas_pressed && !gas_pressed_prev &&
    !(alternative_experience & ALT_EXP_DISABLE_DISENGAGE_ON_GAS)) {
  controls_allowed = false;
}
```

**對應 Python 參數**：
- `DisengageOnAccelerator` (selfdrive/car/card.py:58)
- `false` 時會設定 `ALT_EXP_DISABLE_DISENGAGE_ON_GAS`

---

### controlsd 狀態機

#### 狀態定義

**檔案**：`selfdrive/controls/controlsd.py:524-609`

```python
class State:
  disabled = 0       # OP 完全禁用
  preEnabled = 1     # 預啟用（準備中）
  enabled = 2        # OP 完全啟用
  softDisabling = 3  # 軟禁用（3秒倒數）
  overriding = 4     # 駕駛者接管中（保持部分控制）
```

**狀態轉換圖**：
```
       ┌─────────────┐
       │  disabled   │
       └──────┬──────┘
              │ ET.ENABLE
              ↓
       ┌─────────────┐
   ┌──→│ preEnabled  │
   │   └──────┬──────┘
   │          │ 準備完成
   │          ↓
   │   ┌─────────────┐     ET.OVERRIDE_LONGITUDINAL
   │   │   enabled   │────────────────────────────┐
   │   └──────┬──────┘                            │
   │          │ ET.SOFT_DISABLE                   ↓
   │          ↓                            ┌─────────────┐
   │   ┌─────────────┐                    │ overriding  │
   │   │softDisabling│                    └──────┬──────┘
   │   └──────┬──────┘                           │ 放掉油門
   │          │ 3秒後                             │
   └──────────┴──────────────────────────────────┘
              ↓
       ET.USER_DISABLE
```

---

#### 油門邏輯（OVERRIDE 模式）

**事件生成**：`selfdrive/car/interfaces.py:384-385`
```python
if cs_out.gasPressed:
    events.add(EventName.gasPressedOverride)
```

**事件定義**：`selfdrive/controls/lib/events.py:715-721`
```python
EventName.gasPressedOverride: {
    ET.OVERRIDE_LONGITUDINAL: Alert(
        "", "",
        AlertStatus.normal, AlertSize.none,
        Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1),
},
```

**狀態轉換**：`selfdrive/controls/controlsd.py:577-586`
```python
# OVERRIDING 狀態
elif self.state == State.overriding:
    if self.contains_event_type(ET.SOFT_DISABLE):
        self.state = State.softDisabling
    elif not self.contains_event_type(ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL):
        self.state = State.enabled  # ← 油門放掉自動恢復！
    else:
        self.current_alert_types += [ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL]
```

**流程**：
```
enabled 狀態
  ↓
油門踩下 → gasPressed = True
  ↓
events.add(gasPressedOverride)  # ET.OVERRIDE_LONGITUDINAL
  ↓
self.state = State.overriding
  ↓
油門放掉 → gasPressed = False
  ↓
gasPressedOverride 事件消失
  ↓
not self.contains_event_type(ET.OVERRIDE_*)  # True
  ↓
self.state = State.enabled  ← 自動恢復！
```

---

#### 煞車邏輯（USER_DISABLE 模式）

**事件生成**：`selfdrive/car/card.py:154-157`
```python
if (CS.gasPressed and not self.CS_prev.gasPressed and self.disengage_on_accelerator) or \
   (CS.brakePressed and (not self.CS_prev.brakePressed or not CS.standstill)) or \
   (CS.regenBraking and (not self.CS_prev.regenBraking or not CS.standstill)):
    self.events.add(EventName.pedalPressed)
```

**事件定義**：`selfdrive/controls/lib/events.py:701-705`
```python
EventName.pedalPressed: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("Pedal Pressed",
                              visual_alert=VisualAlert.brakePressed),
},
```

**狀態轉換**：`selfdrive/controls/controlsd.py:588-602`
```python
# DISABLED 狀態
elif self.state == State.disabled:
    if self.contains_event_type(ET.ENABLE):  # ← 需要明確的 ENABLE 事件
        if self.contains_event_type(ET.NO_ENTRY):
            self.current_alert_types.append(ET.NO_ENTRY)
        else:
            # 進入 enabled / overriding / preEnabled
            ...
```

**流程**：
```
enabled 狀態
  ↓
煞車踩下 → brakePressed = True
  ↓
原廠 ACC: cruise_engaged = false (CAN 訊息改變)
  ↓
Panda: pcm_cruise_check(false) → controls_allowed = false
  ↓
events.add(pedalPressed)  # ET.USER_DISABLE
  ↓
self.state = State.disabled
  ↓
煞車放掉 → brakePressed = False
  ↓
pedalPressed 事件消失，BUT cruise_engaged 仍是 false
  ↓
controls_allowed 仍是 false
  ↓
無法自動恢復！需要駕駛者按 SET/RESUME 重新啟用 ACC
```

---

### 煞車自動回復功能實作（已修正）

**實作日期**：2025-10-23
**修正日期**：2025-10-23
**Git Commit**：59d37cca (初版), 待提交 (修正版)

#### 功能需求

**目標**：讓煞車行為更智慧、更安全
- ✅ **行駛中踩煞車**（v_ego > 0.5 m/s）：放開後**自動回復** OP 控制
- ✅ **靜止時踩煞車**（v_ego ≤ 0.5 m/s）：放開後**不自動回復**（更安全）

**原因**：
- 行駛中踩煞車通常是臨時減速，放開後希望繼續由 OP 控制
- 靜止時踩煞車通常是要停車、下車等，需要明確操作重新啟動

#### 實作方式

**修改檔案**：
1. `selfdrive/car/interfaces.py:386-389`
2. `selfdrive/car/card.py:153-159`
3. `panda/board/safety.h:91` (保持原有修改)

**核心邏輯**：

```
踩煞車 (brakePressed = True)
    ↓
檢查車速 v_ego
    ↓
┌───────────────────┴───────────────────┐
│                                       │
v_ego > 0.5 m/s                    v_ego ≤ 0.5 m/s
(行駛中)                            (靜止/慢速)
│                                       │
↓                                       ↓
interfaces.py                       card.py
gasPressedOverride                  pedalPressed
(ET.OVERRIDE_LONGITUDINAL)          (ET.USER_DISABLE)
│                                       │
↓                                       ↓
State: enabled → overriding         State: enabled → disabled
│                                       │
↓ (放開煞車)                          ↓ (放開煞車)
State: overriding → enabled         State: disabled (保持)
✅ 自動回復                          ❌ 需要按 SET/RESUME
```

#### 程式碼修改

**1. interfaces.py (Line 386-389)**：
```python
if cs_out.gasPressed:
    events.add(EventName.gasPressedOverride)
# Brake override only when moving (v_ego > 0.5 m/s)
# When stopped, brake should disable OP completely for safety
if cs_out.brakePressed and cs_out.vEgo > 0.5:
    events.add(EventName.gasPressedOverride)
```

**觸發條件**：
- 煞車踩下 **且** 車速 > 0.5 m/s
- 產生 `gasPressedOverride` 事件（跟油門一樣的行為）

**2. card.py (Line 153-159)**：
```python
# Disable on rising edge of accelerator or regen
# Brake: when moving (v_ego > 0.5 m/s) -> override mode (handled in interfaces.py)
#        when stopped or slow (v_ego <= 0.5 m/s) -> disable completely (safety)
if (CS.gasPressed and not self.CS_prev.gasPressed and self.disengage_on_accelerator) or \
  (CS.regenBraking and (not self.CS_prev.regenBraking or not CS.standstill)) or \
  (CS.brakePressed and CS.vEgo <= 0.5 and (not self.CS_prev.brakePressed or not CS.standstill)):
    self.events.add(EventName.pedalPressed)
```

**煞車邏輯分解**：
- `CS.brakePressed`：煞車踩下
- `CS.vEgo <= 0.5`：車速 ≤ 0.5 m/s（**關鍵判斷**）
- `not self.CS_prev.brakePressed`：煞車剛踩下（rising edge）
- `or not CS.standstill`：或者車輛在移動中

**3. safety.h (Line 91)** - 保持原有修改：
```c
bool get_longitudinal_allowed(void) {
  return controls_allowed && !gas_pressed_prev && !brake_pressed_prev;
}
```

**作用**：煞車踩下時禁止縱向控制（加速/減速指令）

#### 測試情境

**情境 1：高速公路 60 km/h 踩煞車**
```
v_ego = 16.67 m/s (> 0.5)
  ↓ 踩煞車
interfaces.py: gasPressedOverride ✅
  ↓
State: enabled → overriding
  ↓ 放開煞車
State: overriding → enabled
✅ 自動回復控制
```

**情境 2：紅燈前減速到 1.8 km/h (0.5 m/s)**
```
v_ego = 0.5 m/s (邊界值)
  ↓ 踩煞車
card.py: pedalPressed ✅
  ↓
State: enabled → disabled
  ↓ 放開煞車
State: disabled (保持)
❌ 需要按 SET/RESUME
```

**情境 3：完全靜止 (紅燈停車)**
```
v_ego = 0 m/s
standstill = True
  ↓ 踩煞車
card.py: pedalPressed ✅
  ↓
State: enabled → disabled
  ↓ 放開煞車
需要按 SET/RESUME
❌ 更安全
```

**情境 4：行駛中 30 km/h 踩煞車**
```
v_ego = 8.33 m/s (> 0.5)
  ↓ 踩煞車
interfaces.py: gasPressedOverride ✅
  ↓
State: enabled → overriding
  ↓ 放開煞車
State: overriding → enabled
✅ 自動回復
```

#### 關鍵設計決策

**為什麼選擇 0.5 m/s (1.8 km/h) 作為閾值？**

1. **VW standstill 定義**：
   ```python
   # selfdrive/car/volkswagen/carstate.py:51
   ret.standstill = ret.vEgoRaw == 0
   ```
   - 完全靜止判定為 `vEgoRaw == 0`
   - 但實際上微小速度（如 0.1 m/s）也應視為停止

2. **安全考量**：
   - `< 0.5 m/s` = `< 1.8 km/h`：幾乎靜止
   - 這個速度下踩煞車通常是要停車
   - 自動回復容易造成誤觸

3. **其他車廠參考**：
   - Hyundai: `STANDSTILL_THRESHOLD = 12 * 0.03125 * CV.KPH_TO_MS` ≈ 0.1 m/s
   - 我們使用 0.5 m/s 更保守

#### 狀態機影響

**OVERRIDE 模式** (gasPressedOverride)：
```python
# controlsd.py:577-586
elif self.state == State.overriding:
    if not self.contains_event_type(ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL):
        self.state = State.enabled  # ← 自動恢復
```

**DISABLED 模式** (pedalPressed)：
```python
# controlsd.py:588-602
elif self.state == State.disabled:
    if self.contains_event_type(ET.ENABLE):  # ← 需要明確的 ENABLE 事件
        # 進入 enabled
```

#### 測試結果

- ✅ **2025-10-23 實車測試成功**
- ✅ 行駛中煞車自動回復正常
- ✅ 靜止時煞車不會誤觸回復

#### 相關檔案索引

| 檔案 | 行數 | 說明 |
|------|------|------|
| `selfdrive/car/interfaces.py` | 386-389 | 行駛中煞車 → OVERRIDE 事件 |
| `selfdrive/car/card.py` | 153-159 | 靜止時煞車 → USER_DISABLE 事件 |
| `panda/board/safety.h` | 91 | 煞車禁止縱向控制 |
| `selfdrive/controls/controlsd.py` | 577-586, 588-602 | 狀態機邏輯 |

---

#### DisengageOnAccelerator 參數

**檔案**：`selfdrive/car/card.py:58-61`

```python
self.disengage_on_accelerator = self.params.get_bool("DisengageOnAccelerator")
self.CP.alternativeExperience = 0
if not self.disengage_on_accelerator:
    self.CP.alternativeExperience |= ALTERNATIVE_EXPERIENCE.DISABLE_DISENGAGE_ON_GAS
```

**作用**：
- `DisengageOnAccelerator = True`：油門會產生 `pedalPressed` 事件（如煞車）
- `DisengageOnAccelerator = False`：油門只產生 `gasPressedOverride` 事件

**影響**：
```python
# card.py:154
if (CS.gasPressed and not self.CS_prev.gasPressed and self.disengage_on_accelerator):
    self.events.add(EventName.pedalPressed)  # ← 油門變得跟煞車一樣
```

---

### 完整流程總結

#### 🟢 油門踩下/放開完整流程

```
【硬體層】
1. 油門踩下
2. Panda 讀取 CAN → gas_pressed = true
3. get_longitudinal_allowed() 返回 false
   (controls_allowed 仍是 true，但縱向控制被禁)

【軟體層】
4. interfaces.py 檢測到 gasPressed
5. events.add(gasPressedOverride)  # ET.OVERRIDE_LONGITUDINAL
6. controlsd: state = State.overriding

【使用者體驗】
7. 轉向繼續工作（橫向控制）
8. 無法控制油門/煞車（縱向控制禁用）
9. 使用者自己控制加速

【恢復】
10. 油門放開
11. gas_pressed = false
12. get_longitudinal_allowed() 返回 true
13. gasPressedOverride 事件消失
14. controlsd: state = State.enabled  ← 自動恢復！
```

---

#### 🔴 煞車踩下/放開完整流程

```
【車輛層】
1. 煞車踩下
2. 原廠 ACC 接收煞車信號 → 主動關閉 ACC

【CAN 訊息】
3. TSK_06.TSK_Status 改變
4. acc_status 變為非 3/4/5
5. cruise_engaged = false

【硬體層】
6. Panda 讀取 CAN → brake_pressed = true
7. Panda 讀取 TSK_06 → cruise_engaged = false
8. pcm_cruise_check(false) 被呼叫
9. controls_allowed = false  ← 硬體層完全禁用！

【軟體層】
10. card.py 檢測到 brakePressed
11. events.add(pedalPressed)  # ET.USER_DISABLE
12. controlsd: state = State.disabled

【使用者體驗】
13. 所有控制（轉向+加速）全部關閉
14. 使用者完全接管車輛

【嘗試恢復】
15. 煞車放開
16. brake_pressed = false
17. pedalPressed 事件消失
18. BUT: cruise_engaged 仍是 false（原廠 ACC 未重啟）
19. controls_allowed 仍是 false
20. 無法自動恢復！

【手動恢復】
21. 駕駛者按下方向盤 SET 或 RESUME 按鈕
22. 原廠 ACC 重新啟用 → cruise_engaged = true
23. pcm_cruise_check(true) → controls_allowed = true
24. 駕駛者再按 OP 啟用按鈕 → ET.ENABLE 事件
25. controlsd: state = State.enabled  ← 恢復！
```

---

### 關鍵檔案索引

#### Panda Safety 層（C/C++）

| 檔案 | 行號 | 說明 |
|------|------|------|
| `panda/board/safety.h` | 90-92 | `get_longitudinal_allowed()` - 油門影響縱向控制 |
| `panda/board/safety.h` | 258-281 | `generic_rx_checks()` - 踏板檢測（FrogPilot 已修改煞車邏輯） |
| `panda/board/safety.h` | 700-709 | `pcm_cruise_check()` - Cruise engaged 控制 `controls_allowed` |
| `panda/board/safety.h` | 518-545 | 縱向控制檢查函數（都會呼叫 `get_longitudinal_allowed()`） |
| `panda/board/safety_declarations.h` | 255-277 | Alternative Experience 旗標定義 |
| `panda/board/safety/safety_volkswagen_mqb.h` | 141-156 | VW TSK_06 訊息解析（ACC 狀態） |

#### openpilot 控制層（Python）

| 檔案 | 行號 | 說明 |
|------|------|------|
| `selfdrive/controls/controlsd.py` | 524-609 | 完整狀態機邏輯 |
| `selfdrive/controls/controlsd.py` | 577-586 | `overriding` 狀態（油門） |
| `selfdrive/controls/controlsd.py` | 588-602 | `disabled` 狀態（煞車） |
| `selfdrive/controls/lib/events.py` | 38-48 | 事件類型定義 (ET 類) |
| `selfdrive/controls/lib/events.py` | 701-705 | `pedalPressed` 事件（USER_DISABLE） |
| `selfdrive/controls/lib/events.py` | 715-721 | `gasPressedOverride` 事件（OVERRIDE_LONGITUDINAL） |
| `selfdrive/car/interfaces.py` | 384-385 | `gasPressed` 檢測 |
| `selfdrive/car/card.py` | 58-61 | `DisengageOnAccelerator` 參數設定 |
| `selfdrive/car/card.py` | 154-157 | `brakePressed` 檢測 |

---

### 設計理念

| 操作 | 語義 | 安全等級 | 恢復方式 |
|------|------|---------|---------|
| **油門** | 駕駛者想要加速（暫時接管） | 中等 | 自動（放掉油門） |
| **煞車** | 緊急制動（明確禁用） | 最高 | 手動（重啟 ACC + OP） |
| **Regen** | 能量回收煞車（類似煞車） | 高 | 手動 |
| **ACC 關閉** | 駕駛者關閉巡航（明確禁用） | 最高 | 手動（重啟 ACC） |

**核心原則**：
1. **油門 = 臨時干預**：保持 OP 啟用狀態，只暫停縱向控制
2. **煞車 = 緊急情況**：完全禁用 OP，需要駕駛者確認恢復
3. **硬體優先**：Panda 的 `controls_allowed` 是最後防線
4. **雙重保護**：硬體層 + 軟體層確保安全

---

### volkswagen_longitudinal 模式差異

VW 車輛有兩種縱向控制模式，影響控制邏輯：

#### 模式 1：使用原廠 ACC (`volkswagen_longitudinal = false`)

**特徵**：
- `ret.pcmCruise = True`
- 使用原廠 ACC 系統控制油門/煞車
- OP 只負責橫向控制（轉向）

**控制邏輯**：
```c
// panda/board/safety/safety_volkswagen_mqb.h:149-151
if (!volkswagen_longitudinal) {
  pcm_cruise_check(cruise_engaged);  // ← 會呼叫此函數
}
```

**行為**：
- `pcm_cruise_check(cruise_engaged)` 會被呼叫
- 當 `cruise_engaged = false` 時（煞車或 CANCEL），`controls_allowed = false`
- 煞車會完全禁用 OP

---

#### 模式 2：使用 OP 縱向控制 (`volkswagen_longitudinal = true`)

**特徵**：
- `ret.pcmCruise = False`
- `ret.openpilotLongitudinalControl = True`
- OP 完全接管油門和煞車控制
- 需要 Panda ALLOW_DEBUG 韌體

**控制邏輯**：
```c
// panda/board/safety/safety_volkswagen_mqb.h:149-151
if (!volkswagen_longitudinal) {
  pcm_cruise_check(cruise_engaged);
}
// ← volkswagen_longitudinal = true 時，這段完全不執行！
```

**行為**：
- **`pcm_cruise_check` 完全不會被呼叫**
- 踩煞車時原廠 ACC 可能關閉，但 Panda 不會因為 `cruise_engaged = false` 禁用控制
- 真正的禁用機制在其他地方

---

### 使用 OP 縱向控制時的煞車邏輯 (`volkswagen_longitudinal = true`)

#### 硬體層（Panda）

**1. `generic_rx_checks()`** (`panda/board/safety.h:265-269`)
```c
// exit controls on rising edge of brake press
if (brake_pressed && (!brake_pressed_prev || vehicle_moving)) {
  controls_allowed = controls_allowed;  // ← FrogPilot 修改：不做任何事
}
brake_pressed_prev = brake_pressed;
```
- **原始 openpilot**：`controls_allowed = false`（煞車會禁用）
- **FrogPilot 修改**：改為 `controls_allowed = controls_allowed`（煞車不禁用）

---

**2. VW ACC 主開關檢查** (`panda/board/safety/safety_volkswagen_mqb.h:153-155`)
```c
if (!acc_main_on) {
  controls_allowed = false;  // ← ACC 主開關關閉時禁用
}
```

**`acc_main_on` 定義**（第 147 行）：
```c
acc_main_on = cruise_engaged || (acc_status == 2);
```
- `acc_status = 0/1`: ACC 關閉 → `acc_main_on = false`
- `acc_status = 2`: ACC 待命（主開關開，未設定速度）→ `acc_main_on = true`
- `acc_status = 3/4/5`: ACC 啟用中 → `acc_main_on = true`

**重點**：
- **踩煞車時，`acc_main_on` 通常保持 `true`**（只要 ACC 主開關還開著）
- 只有關閉 ACC 主開關或按 CANCEL 時，`acc_main_on` 才變 `false`
- 因此**煞車不會在硬體層禁用 `controls_allowed`**

---

**3. `get_longitudinal_allowed()`** (`panda/board/safety.h:90-92`)
```c
bool get_longitudinal_allowed(void) {
  return controls_allowed && !gas_pressed_prev;  // ← 只檢查油門！
}
```

**問題**：
- 只檢查 `!gas_pressed_prev`
- **不檢查 `!brake_pressed_prev`**
- 因此踩煞車時，縱向控制指令**不會**被硬體層阻擋

---

#### 軟體層（openpilot）

**1. 煞車事件生成** (`selfdrive/car/card.py:154-157`)
```python
if (CS.gasPressed and not self.CS_prev.gasPressed and self.disengage_on_accelerator) or \
   (CS.brakePressed and (not self.CS_prev.brakePressed or not CS.standstill)) or \
   (CS.regenBraking and (not self.CS_prev.regenBraking or not CS.standstill)):
    self.events.add(EventName.pedalPressed)
```
- 煞車產生 `pedalPressed` 事件（`ET.USER_DISABLE`）

**2. 油門事件生成** (`selfdrive/car/interfaces.py:384-385`)
```python
if cs_out.gasPressed:
    events.add(EventName.gasPressedOverride)
```
- 油門產生 `gasPressedOverride` 事件（`ET.OVERRIDE_LONGITUDINAL`）

**3. 狀態機處理** (`selfdrive/controls/controlsd.py`)

**油門**：
```python
# overriding 狀態（577-586 行）
elif self.state == State.overriding:
    if not self.contains_event_type(ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL):
        self.state = State.enabled  # ← 事件消失自動恢復
```

**煞車**：
```python
# disabled 狀態（588-602 行）
elif self.state == State.disabled:
    if self.contains_event_type(ET.ENABLE):  # ← 需要 ENABLE 事件
        ...  # 進入 enabled
```

**差異**：
- 油門：`OVERRIDE_LONGITUDINAL` → `overriding` 狀態 → 自動恢復
- 煞車：`USER_DISABLE` → `disabled` 狀態 → 需手動恢復

---

### 使用 OP 縱向控制時的完整流程

#### 🟢 油門踩下/放開

```
【硬體層】
1. 油門踩下 → gas_pressed = true
2. generic_rx_checks: 可能設 controls_allowed = false（如果沒有 ALT_EXP_DISABLE_DISENGAGE_ON_GAS）
3. get_longitudinal_allowed() = false（因為 gas_pressed_prev = true）
4. 縱向控制指令被硬體層阻擋

【軟體層】
5. interfaces.py → gasPressedOverride 事件
6. ET.OVERRIDE_LONGITUDINAL → state = overriding
7. 橫向控制繼續，縱向控制暫停

【恢復】
8. 油門放開 → gas_pressed = false
9. gasPressedOverride 事件消失
10. state = enabled ← 自動恢復
```

---

#### 🔴 煞車踩下/放開

```
【硬體層】
1. 煞車踩下 → brake_pressed = true
2. generic_rx_checks:267 → controls_allowed = controls_allowed（FrogPilot 已改，不做事）
3. get_longitudinal_allowed() = controls_allowed && !gas_pressed_prev
   → 返回 true（因為沒檢查 brake_pressed_prev）
4. 縱向控制指令「沒有」被硬體層阻擋！
5. acc_main_on 保持 true（ACC 主開關還開著）
6. controls_allowed 保持 true

【軟體層】
7. card.py → pedalPressed 事件
8. ET.USER_DISABLE → state = disabled
9. 所有控制（橫向+縱向）在軟體層禁用

【嘗試恢復】
10. 煞車放開 → brake_pressed = false
11. pedalPressed 事件消失
12. BUT: state 仍是 disabled
13. 需要 ET.ENABLE 事件（手動按 SET/RESUME）
14. 無法自動恢復
```

**關鍵問題**：
1. 硬體層：`get_longitudinal_allowed()` 沒檢查 `brake_pressed_prev`
2. 軟體層：煞車產生 `USER_DISABLE` 而非 `OVERRIDE_LONGITUDINAL`

---

#### ⚫ CANCEL 按鍵

```
【硬體層】
1. CANCEL 按下（GRA_ACC_01.GRA_Abbrechen）
2. safety_volkswagen_mqb.h:172 → controls_allowed = acc_main_on
3. 如果 ACC 主開關仍開著，controls_allowed 保持 true
4. 但按 CANCEL 通常會關閉 ACC → acc_main_on = false
5. 第 153 行檢查：if (!acc_main_on) → controls_allowed = false

【軟體層】
6. state = disabled

【結果】
完全禁用，需手動重啟
```

---

### 當前煞車、油門與 CANCEL 按鈕的核心差異

| 比較項目 | 油門 | 煞車 (行駛中) | 煞車 (靜止時) | CANCEL 按鈕 |
|---------|------|--------------|--------------|-------------|
| **硬體層** | | | | |
| `controls_allowed` 影響 | 可能設為 `false`（如果沒旗標） | 保持 `true`（FrogPilot 已改） | 保持 `true` | N/A |
| `get_longitudinal_allowed()` | 檢查 `!gas_pressed_prev` ✅ | **不檢查** `!brake_pressed_prev` ❌ | 同左 | N/A |
| 縱向指令阻擋 | ✅ 被阻擋 | ❌ 不被阻擋 | 同左 | N/A |
| **軟體層** | | | | |
| 事件名稱 | `gasPressedOverride` | `gasPressedOverride` | `pedalPressed` | `buttonCancel` |
| 事件類型 | `OVERRIDE_LONGITUDINAL` | `OVERRIDE_LONGITUDINAL` | `USER_DISABLE` | `USER_DISABLE` + `NO_ENTRY` |
| 狀態機 | `overriding` | `overriding` | `disabled` | `disabled` |
| 恢復方式 | ✅ 自動 | ✅ 自動 | ❌ 需手動 | ❌ 需手動 |

---

## 控制恢復機制詳解

### 問題:踩煞車停止或按 CANCEL 後,能用油門恢復控制嗎?

#### 答案總結

| 情境 | 能否用油門恢復 | 說明 |
|------|---------------|------|
| **行駛中踩煞車** (v_ego > 0.5 m/s) | ✅ **可以** | 放開煞車或踩油門都能恢復 |
| **靜止時踩煞車** (v_ego ≤ 0.5 m/s) | ❌ **不能** | 必須按 SET/RESUME 重新啟動 |
| **按 CANCEL 按鈕** | ❌ **不能** | 必須按 SET/RESUME 重新啟動 |

#### 詳細分析

**1. 行駛中踩煞車 (v_ego > 0.5 m/s)**

**流程**:
```
踩煞車 → gasPressedOverride 事件 (ET.OVERRIDE_LONGITUDINAL)
       → State.overriding
       → 放開煞車或踩油門 → 自動恢復 State.enabled
```

**程式碼位置**:
- `selfdrive/car/interfaces.py:386-389` - 產生 `gasPressedOverride` 事件
- `selfdrive/controls/lib/events.py:715-721` - 事件定義為 `OVERRIDE_LONGITUDINAL`
- `selfdrive/controls/controlsd.py:578-586` - `overriding` 狀態處理

**關鍵邏輯** (`controlsd.py:583-584`):
```python
elif not self.contains_event_type(ET.OVERRIDE_LONGITUDINAL):
    self.state = State.enabled  # ← 油門放開後自動恢復
```

**結論**: ✅ **可以用油門恢復**,也可以放開煞車自動恢復

---

**2. 靜止時踩煞車 (v_ego ≤ 0.5 m/s)**

**流程**:
```
踩煞車 → pedalPressed 事件 (ET.USER_DISABLE)
       → State.disabled
       → 必須按 SET/RESUME 才能回到 State.enabled
```

**程式碼位置**:
- `selfdrive/car/card.py:154-159` - 產生 `pedalPressed` 事件的條件
- `selfdrive/controls/lib/events.py:701-705` - 事件定義為 `USER_DISABLE`

**判斷條件** (`card.py:158`):
```python
(CS.brakePressed and CS.vEgo <= 0.5 and (not self.CS_prev.brakePressed or not CS.standstill))
```

**為什麼不能自動恢復?**
- 產生 `USER_DISABLE` 事件 → 進入 `disabled` 狀態
- `disabled` 狀態需要 `ENABLE` 事件才能恢復 (`controlsd.py:590`)
- 油門會產生 `gasPressedOverride`,但**不是** `ENABLE` 事件
- 只有按 SET/RESUME 才能產生 `ENABLE` 事件

**結論**: ❌ **不能用油門恢復**,必須按 SET/RESUME

---

**3. 按 CANCEL 按鈕**

**流程**:
```
按 CANCEL → buttonCancel 事件 (ET.USER_DISABLE + ET.NO_ENTRY)
         → State.disabled
         → 必須按 SET/RESUME 才能回到 State.enabled
```

**程式碼位置**:
- `selfdrive/car/interfaces.py:396-398` - 偵測 CANCEL 按鈕
- `selfdrive/controls/lib/events.py:686-689` - 事件定義

**按鈕偵測** (`interfaces.py:397-398`):
```python
if b.type == ButtonType.cancel:
    events.add(EventName.buttonCancel)
```

**事件定義** (`events.py:686-689`):
```python
EventName.buttonCancel: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("Cancel Pressed"),
}
```

**為什麼不能自動恢復?**
- 同靜止煞車,產生 `USER_DISABLE` → `disabled` 狀態
- 還額外產生 `NO_ENTRY`,防止立即重新啟動
- 油門產生的 `OVERRIDE_LONGITUDINAL` 不是 `ENABLE` 事件

**結論**: ❌ **不能用油門恢復**,必須按 SET/RESUME

---

### 恢復邏輯的設計理念

**為什麼行駛中煞車可以自動恢復?**
- 行駛中踩煞車通常是**臨時減速**（避讓、減速等）
- 使用 `OVERRIDE` 事件類型,表示駕駛者暫時接管
- 放開後自動恢復,符合駕駛習慣

**為什麼靜止時煞車不能自動恢復?**
- 靜止時踩煞車通常是**明確停車意圖**（停紅燈、停車等）
- 使用 `USER_DISABLE` 事件類型,表示駕駛者明確禁用
- 需要手動重啟,更安全

**為什麼 CANCEL 按鈕不能自動恢復?**
- CANCEL 按鈕代表**明確取消控制**的意圖
- 如果能用油門恢復,違反 CANCEL 的語義
- 必須按 SET/RESUME,確保駕駛者有意重啟

---

### 相關程式碼位置總覽

| 功能 | 檔案 | 行數 | 說明 |
|------|------|------|------|
| 行駛中煞車事件生成 | `selfdrive/car/interfaces.py` | 386-389 | 產生 `gasPressedOverride` |
| 靜止時煞車事件生成 | `selfdrive/car/card.py` | 154-159 | 產生 `pedalPressed` |
| CANCEL 按鈕事件生成 | `selfdrive/car/interfaces.py` | 396-398 | 產生 `buttonCancel` |
| `gasPressedOverride` 定義 | `selfdrive/controls/lib/events.py` | 715-721 | `OVERRIDE_LONGITUDINAL` |
| `pedalPressed` 定義 | `selfdrive/controls/lib/events.py` | 701-705 | `USER_DISABLE` |
| `buttonCancel` 定義 | `selfdrive/controls/lib/events.py` | 686-689 | `USER_DISABLE` + `NO_ENTRY` |
| `overriding` 狀態邏輯 | `selfdrive/controls/controlsd.py` | 578-586 | 自動恢復邏輯 |
| `disabled` 狀態邏輯 | `selfdrive/controls/controlsd.py` | 589-602 | 需要 `ENABLE` 事件 |

---

## 踩油門恢復控制功能實作方案

### ✅ 實作狀態

**已實作完成** (2025-11-01)

**修改檔案**:
1. `selfdrive/controls/lib/events.py:721` - 新增 `ET.ENABLE` 到 `gasPressedOverride` 事件
2. `selfdrive/controls/lib/drive_helpers.py:160-168` - 優化初始速度邏輯,確保安全的最小巡航速度
3. `selfdrive/controls/lib/drive_helpers.py:82-83, 117-118` - 允許在 disabled 狀態調整定速速度

**效果**:
- ✅ 靜止時踩煞車後,可用油門恢復控制
- ✅ 按 CANCEL 按鈕後,可用油門恢復控制
- ✅ 按 CANCEL 後可調整定速速度,恢復時使用新速度
- ✅ 油門啟動時,初始速度設為 max(當前速度, 40 kph),確保安全
- ✅ 所有原有功能完全不受影響

**安全保護**:
- 車輛靜止 (0 km/h) → 踩油門啟動 → 巡航速度 40 kph
- 車輛慢速 (20 km/h) → 踩油門啟動 → 巡航速度 40 kph
- 車輛正常 (60 km/h) → 踩油門啟動 → 巡航速度 60 kph
- 避免在低速時設定過低的巡航速度,確保安全性

**使用情境範例**:
```
情境 1: 高速公路減速下交流道
1. 高速公路定速 120 kph
2. 即將下交流道,按 CANCEL 讓車滑行減速
3. 滑行中使用調速按鈕預先調整定速到 60 kph
4. 到達匝道後踩油門恢復控制
5. 系統以 60 kph 而非 120 kph 恢復,避免向前衝

情境 2: 紅燈停車後起步
1. 車輛定速行駛
2. 遇紅燈,車輛自動煞車到完全靜止
3. 綠燈時直接踩油門
4. 系統以 40 kph 安全速度恢復控制
```

---

### 目標需求

讓以下兩種情境可以透過踩油門恢復控制:
1. **靜止時踩煞車** (v_ego ≤ 0.5 m/s) → 解除控制 → **踩油門恢復**
2. **按 CANCEL 按鈕** → 解除控制 → **踩油門恢復**

### 當前問題分析 (實作前)

**問題根源**:
- 這兩種情境都會產生 `USER_DISABLE` 事件 → 進入 `disabled` 狀態
- `disabled` 狀態需要 `ENABLE` 事件才能恢復
- 油門踩下只會產生 `gasPressedOverride` 事件 (事件類型: `OVERRIDE_LONGITUDINAL`)
- `gasPressedOverride` **沒有** `ENABLE` 事件類型

**事件類型對照**:
```python
# events.py:678-680
EventName.buttonEnable: {
    ET.ENABLE: EngagementAlert(AudibleAlert.none),  # ← SET/RESUME 按鈕有 ENABLE
}

# events.py:715-721
EventName.gasPressedOverride: {
    ET.OVERRIDE_LONGITUDINAL: Alert(...)  # ← 油門只有 OVERRIDE,沒有 ENABLE
}
```

### 解決方案

#### 方案 1: 修改 `gasPressedOverride` 事件定義 ✅ (已採用)

**優點**: 簡單、直接、符合邏輯
**缺點**: 改變事件語義,可能影響其他邏輯 (經驗證無影響)

**修改位置**: `selfdrive/controls/lib/events.py:715-722`

**修改前**:
```python
EventName.gasPressedOverride: {
    ET.OVERRIDE_LONGITUDINAL: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1),
},
```

**修改後**:
```python
EventName.gasPressedOverride: {
    ET.OVERRIDE_LONGITUDINAL: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1),
    ET.ENABLE: EngagementAlert(AudibleAlert.none),  # ← 新增 ENABLE 事件類型
},
```

**邏輯說明 - 為什麼 ENABLE 和 OVERRIDE 不衝突?**

一個事件可以同時有多個事件類型,狀態機會根據當前狀態選擇對應的處理邏輯:

**在 `enabled` 狀態** (`controlsd.py:554-556`):
```python
elif self.contains_event_type(ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL):
    self.state = State.overriding  # ← 檢測到 OVERRIDE → overriding
```
- 油門踩下 → 有 `OVERRIDE_LONGITUDINAL` → 進入 `overriding` 狀態
- `ENABLE` 事件類型被忽略 (因為已經是 enabled,不需要 enable)

**在 `disabled` 狀態** (`controlsd.py:590-602`):
```python
if self.contains_event_type(ET.ENABLE):              # ← 檢查有 ENABLE
    if self.contains_event_type(ET.NO_ENTRY):
        # 阻擋
    else:
        if self.contains_event_type(ET.PRE_ENABLE):
            self.state = State.preEnabled
        elif self.contains_event_type(ET.OVERRIDE_LONGITUDINAL):  # ← 有 OVERRIDE
            self.state = State.overriding  # ← 優先進入 overriding!
        else:
            self.state = State.enabled
```
- 油門踩下 → 有 `ENABLE` + `OVERRIDE_LONGITUDINAL`
- 進入 if (因為有 ENABLE)
- 檢查到有 `OVERRIDE_LONGITUDINAL` → 進入 `overriding` 狀態 ✅

**在 `overriding` 狀態** (`controlsd.py:583-584`):
```python
elif not self.contains_event_type(ET.OVERRIDE_LONGITUDINAL):
    self.state = State.enabled  # ← 沒有 OVERRIDE → enabled
```
- 放開油門 → 沒有 `OVERRIDE` → 回到 `enabled` 狀態 ✅

**結論**: `ENABLE` 的作用是「開啟進入 enabled/overriding 狀態的大門」,`OVERRIDE` 決定「進入 overriding 還是 enabled」。兩者不衝突,反而是完美配合!

**影響範圍**:
- ✅ `disabled` → 踩油門 → `overriding` (新功能)
- ✅ `enabled` → 踩油門 → `overriding` (原功能不變)
- ✅ `overriding` → 放油門 → `enabled` (原功能不變)

**測試要點**:
1. 靜止時踩煞車 → 放開 → 踩油門 → 檢查是否恢復控制
2. 按 CANCEL → 踩油門 → 檢查是否恢復控制
3. 行駛中踩油門 → 檢查是否仍正常進入 `overriding`
4. 檢查是否有其他依賴 `gasPressedOverride` 只有 `OVERRIDE` 的邏輯

---

#### 方案 2: 在 `controlsd.py` 中特殊處理油門

**優點**: 不改變事件定義,影響範圍最小
**缺點**: 邏輯分散,不夠直觀

**修改位置**: `selfdrive/controls/controlsd.py:589-602`

**修改前**:
```python
# DISABLED
elif self.state == State.disabled:
  if self.contains_event_type(ET.ENABLE):
    if self.contains_event_type(ET.NO_ENTRY):
      self.current_alert_types.append(ET.NO_ENTRY)
    else:
      if self.contains_event_type(ET.PRE_ENABLE):
        self.state = State.preEnabled
      elif self.contains_event_type(ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL):
        self.state = State.overriding
      else:
        self.state = State.enabled
      self.current_alert_types.append(ET.ENABLE)
      self.v_cruise_helper.initialize_v_cruise(...)
```

**修改後**:
```python
# DISABLED
elif self.state == State.disabled:
  # 新增: 油門踩下時,視為 ENABLE 事件
  has_gas_override = self.contains_event_type(ET.OVERRIDE_LONGITUDINAL) and \
                     EventName.gasPressedOverride in self.events.names

  if self.contains_event_type(ET.ENABLE) or has_gas_override:
    if self.contains_event_type(ET.NO_ENTRY):
      self.current_alert_types.append(ET.NO_ENTRY)
    else:
      if self.contains_event_type(ET.PRE_ENABLE):
        self.state = State.preEnabled
      elif self.contains_event_type(ET.OVERRIDE_LATERAL, ET.OVERRIDE_LONGITUDINAL):
        self.state = State.overriding
      else:
        self.state = State.enabled
      if self.contains_event_type(ET.ENABLE):
        self.current_alert_types.append(ET.ENABLE)
      self.v_cruise_helper.initialize_v_cruise(...)
```

**問題**: 這個方案有個潛在問題 - `NO_ENTRY` 檢查可能會阻擋恢復。

**NO_ENTRY 事件持續性分析**:
- `buttonCancel` 產生 `NO_ENTRY` 事件,但**不是** static 事件
- 每個控制循環結束時會調用 `events.clear()`,清除非 static 事件
- CANCEL 按鈕放開後,下一個循環 `NO_ENTRY` 就會被清除
- 因此踩油門時 `NO_ENTRY` 已經不存在,不會阻擋恢復 ✅

**結論**: 方案 2 理論上可行,但邏輯比方案 1 複雜,不建議。

---

#### 方案 3: 新增專用事件 (最安全但最複雜)

**優點**: 完全不影響現有邏輯,語義清晰
**缺點**: 修改點較多,需要更多測試

**步驟**:
1. 在 `events.py` 新增 `gasPressedEnable` 事件
2. 在 `interfaces.py` 判斷:如果在 `disabled` 狀態,產生 `gasPressedEnable` 而非 `gasPressedOverride`
3. `gasPressedEnable` 同時有 `ENABLE` 和 `OVERRIDE_LONGITUDINAL`

這個方案過於複雜,不建議使用。

---

### ❓ 常見疑問解答

#### Q1: ENABLE 和 OVERRIDE 不是相反的動作嗎?同時存在會衝突嗎?

**A**: 不會衝突!這是狀態機設計的巧妙之處。

**誤解**: 認為 ENABLE = 啟用控制, OVERRIDE = 接管控制,兩者互斥
**真相**:
- `ENABLE` = "允許從 disabled 進入控制狀態" (開門鑰匙)
- `OVERRIDE` = "駕駛者正在接管,但仍在控制狀態內" (房間內的一種狀態)

**類比**:
```
disabled 狀態 = 門鎖住了,人在門外
ENABLE 事件 = 拿到鑰匙開門
OVERRIDE 事件 = 進門後選擇「手動模式」房間而非「自動模式」房間

同時有 ENABLE + OVERRIDE = 開門後直接進入「手動模式」房間 ✅
```

**實際流程**:
```
1. CANCEL 按鈕 → disabled 狀態 (門鎖住)
2. 踩油門 → 產生 ENABLE (開門) + OVERRIDE (選擇手動房間)
3. 狀態機:
   - 檢查有 ENABLE → 可以進入 ✅
   - 檢查有 OVERRIDE → 進入 overriding 而非 enabled ✅
4. 放開油門 → 沒有 OVERRIDE → 從「手動房間」回到「自動房間」(enabled) ✅
```

**程式碼證明** (`controlsd.py:590-598`):
```python
if self.contains_event_type(ET.ENABLE):              # ← ENABLE 開門
    if self.contains_event_type(ET.NO_ENTRY):        # ← 檢查是否被阻擋
        # 保持 disabled
    else:
        elif self.contains_event_type(ET.OVERRIDE_LONGITUDINAL):  # ← OVERRIDE 選房間
            self.state = State.overriding  # ← 進入「手動房間」
```

**結論**: `ENABLE` + `OVERRIDE` 是完美配合,不是衝突!

---

#### Q2: CANCEL 按鈕的 NO_ENTRY 會不會一直阻擋油門恢復?

**A**: 不會!`NO_ENTRY` 事件會在下一個循環被清除。

**事件生命週期**:
```
T0: CANCEL 按下
  → buttonCancel 事件產生 (USER_DISABLE + NO_ENTRY)
  → state = disabled
  → 本循環結束

T1: CANCEL 放開
  → events.clear() 被調用 (events.py:77-79)
  → buttonCancel 不是 static 事件 → 被清除
  → NO_ENTRY 消失! ✅

T2: 踩油門
  → gasPressedOverride 產生 (OVERRIDE + ENABLE)
  → contains_event_type(ET.NO_ENTRY) → ❌ False
  → 不會被阻擋 → 進入 overriding ✅
```

**程式碼證明** (`events.py:77-79`):
```python
def clear(self) -> None:
    self.event_counters = {k: (v + 1 if k in self.events else 0) for k, v in self.event_counters.items()}
    self.events = self.static_events.copy()  # ← 只保留 static 事件
```

**如何確認 buttonCancel 不是 static**:
```python
# interfaces.py:398 - 產生事件時沒有 static=True
events.add(EventName.buttonCancel)  # ← 沒有 static=True 參數
```

**結論**: `NO_ENTRY` 只在 CANCEL 按下的那一個循環存在,放開後立即消失,不會阻擋油門恢復!

---

#### Q3: 如果 CANCEL 按鈕一直按住不放,油門能恢復嗎?

**A**: 不能!只要 CANCEL 按住,`NO_ENTRY` 就會持續存在。

**流程**:
```
T0-Tn: CANCEL 一直按住
  → 每個循環都產生 buttonCancel 事件
  → NO_ENTRY 持續存在
  → 踩油門 → 有 ENABLE 但被 NO_ENTRY 阻擋
  → 保持 disabled ❌

Tn+1: CANCEL 放開
  → buttonCancel 事件消失
  → NO_ENTRY 消失
  → 踩油門 → 恢復成功 ✅
```

**設計理念**:
- CANCEL 按住 = 駕駛者明確表達「不要控制」
- 符合安全邏輯,避免意外恢復

**結論**: 必須先放開 CANCEL,才能用油門恢復!

---

### 建議實作方案

**推薦使用方案 1**,原因:
1. ✅ 修改點單一 (只需修改一個檔案的一個地方)
2. ✅ 邏輯清晰直觀
3. ✅ 符合「油門可以恢復控制」的使用者預期
4. ✅ 對現有功能影響最小 (只是新增事件類型,不刪除原有的)

### 實作步驟

**1. 修改事件定義**

檔案: `selfdrive/controls/lib/events.py`

```python
# 第 715-721 行
EventName.gasPressedOverride: {
    ET.OVERRIDE_LONGITUDINAL: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .1),
    ET.ENABLE: EngagementAlert(AudibleAlert.none),  # ← 新增這一行
},
```

**2. 測試驗證**

測試場景:
```
場景 1: 靜止時煞車恢復
1. 啟動 OP LONG
2. 車輛減速到停止
3. 踩煞車 (v_ego ≤ 0.5) → 檢查進入 disabled 狀態
4. 放開煞車
5. 踩油門 → 檢查是否進入 overriding 狀態
6. 放開油門 → 檢查是否自動恢復到 enabled 狀態

場景 2: CANCEL 後油門恢復
1. 啟動 OP LONG
2. 按 CANCEL 按鈕 → 檢查進入 disabled 狀態
3. 踩油門 → 檢查是否進入 overriding 狀態
4. 放開油門 → 檢查是否自動恢復到 enabled 狀態

場景 3: 原有功能不受影響
1. 啟動 OP LONG (enabled 狀態)
2. 行駛中踩油門 → 檢查是否進入 overriding 狀態
3. 放開油門 → 檢查是否自動恢復到 enabled 狀態

場景 4: 行駛中煞車恢復
1. 啟動 OP LONG
2. 行駛中 (v_ego > 0.5) 踩煞車 → 檢查進入 overriding 狀態
3. 放開煞車 → 檢查是否自動恢復到 enabled 狀態
```

**3. 可能的副作用檢查**

檢查以下是否有依賴 `gasPressedOverride` 只有 `OVERRIDE_LONGITUDINAL` 的邏輯:
```bash
# 搜尋所有使用 gasPressedOverride 的地方
grep -r "gasPressedOverride" selfdrive/
```

預期結果: 應該只有產生事件的地方 (`interfaces.py`) 和事件定義的地方 (`events.py`)。

**4. 文檔更新**

更新以下文檔章節:
- 「控制恢復機制詳解」→ 更新所有表格,標記油門可以恢復
- 「當前煞車、油門與 CANCEL 按鈕的核心差異」→ 更新恢復方式欄位
- 新增「修改紀錄」章節說明此次修改

---

### 進階選項: 新增 UI 設定開關

如果想讓使用者自行選擇是否啟用此功能,可以新增一個 PARAMS:

**步驟**:
1. 在 `frogpilot_variables.py` 新增參數 `ResumeOnGas`
2. 在 UI 設定頁面新增開關
3. 在 `events.py` 中根據此參數決定是否新增 `ENABLE` 事件類型

**實作範例**:
```python
# frogpilot_variables.py
"ResumeOnGas": {
    "type": "toggle",
    "default": True,
    "description": "允許踩油門恢復控制 (靜止煞車/CANCEL後)",
}

# events.py (需要讀取 params)
# 這需要將 events.py 改成動態讀取,較複雜,不建議
```

**建議**: 先實作基本功能,確認穩定後再考慮是否需要 UI 開關。

---

### 風險評估

| 風險項目 | 風險等級 | 說明 | 緩解措施 |
|---------|---------|------|---------|
| 意外恢復控制 | 中 | 使用者可能不預期踩油門會恢復 | 充分測試,觀察使用者反饋 |
| 與其他功能衝突 | 低 | `gasPressedOverride` 使用範圍小 | 程式碼搜尋確認 |
| 安全性影響 | 低 | 油門恢復符合駕駛習慣 | 硬體層仍有保護機制 |
| 狀態機邏輯錯誤 | 低 | 可能產生非預期狀態轉換 | 完整的測試場景覆蓋 |

---

### 相關程式碼位置

| 功能 | 檔案 | 行數 | 說明 |
|------|------|------|------|
| **修改點** | `selfdrive/controls/lib/events.py` | 715-721 | `gasPressedOverride` 事件定義 |
| 油門事件產生 | `selfdrive/car/interfaces.py` | 384-385 | 產生 `gasPressedOverride` |
| 靜止煞車事件產生 | `selfdrive/car/card.py` | 156-159 | 產生 `pedalPressed` |
| CANCEL 事件產生 | `selfdrive/car/interfaces.py` | 397-398 | 產生 `buttonCancel` |
| 狀態機邏輯 | `selfdrive/controls/controlsd.py` | 589-602 | `disabled` 狀態處理 |
| DisengageOnAccelerator | `selfdrive/car/card.py` | 58-61 | 油門是否產生 `pedalPressed` |

---

## 為什麼啟動後必須先按 SET 鍵才能啟用 OP 控制?

### 問題描述

車輛啟動後,即使 openpilot 系統已運行,也無法直接用 RESUME 或油門啟動控制,必須先按 SET 鍵建立初始巡航速度。

### 核心原因

**巡航速度未初始化保護機制** (`v_cruise_initialized`)

### 完整邏輯分析

#### 1. 巡航速度初始化狀態

**檔案**: `selfdrive/controls/lib/drive_helpers.py`

**初始狀態** (第 47-48 行):
```python
self.v_cruise_kph = V_CRUISE_UNSET  # 255 kph (未設定標記)
self.v_cruise_cluster_kph = V_CRUISE_UNSET
```

**檢查函數** (第 54-55 行):
```python
@property
def v_cruise_initialized(self):
    return self.v_cruise_kph != V_CRUISE_UNSET  # ← 檢查是否已設定速度
```

---

#### 2. RESUME 按鈕被阻擋機制

**檔案**: `selfdrive/controls/controlsd.py:255-258`

```python
# Block resume if cruise never previously enabled
resume_pressed = any(be.type in (ButtonType.accelCruise, ButtonType.resumeCruise) for be in CS.buttonEvents)
if not self.CP.pcmCruise and not self.v_cruise_helper.v_cruise_initialized and resume_pressed:
    self.events.add(EventName.resumeBlocked)
```

**邏輯**:
1. 檢測到 RESUME 或 accelCruise 按鈕按下
2. 檢查 `v_cruise_initialized` → False (巡航速度未初始化)
3. 產生 `resumeBlocked` 事件 → `ET.NO_ENTRY`
4. 顯示訊息: **"Press Set to Engage"** (第 738 行)

**事件定義** (`events.py:737-739`):
```python
EventName.resumeBlocked: {
    ET.NO_ENTRY: NoEntryAlert("Press Set to Engage"),
},
```

---

#### 3. SET 按鈕如何初始化速度

**VW 車輛的啟用按鈕配置** (`volkswagen/interface.py:109`):
```python
enable_buttons=(ButtonType.setCruise, ButtonType.resumeCruise)
```

**按鈕事件產生** (`interfaces.py:393-395`):
```python
# Enable OP long on falling edge of enable buttons
if not self.CP.pcmCruise and (b.type in enable_buttons and not b.pressed):
    events.add(EventName.buttonEnable)  # ← 產生 ENABLE 事件
```

**速度初始化** (`drive_helpers.py:146-162`):
```python
def initialize_v_cruise(self, CS, experimental_mode: bool, desired_speed_limit, frogpilot_toggles) -> None:
    # 當 buttonEnable 事件被處理時調用

    # 如果按的是 RESUME 且之前有設定過速度
    if any(b.type in (ButtonType.accelCruise, ButtonType.resumeCruise) for b in CS.buttonEvents) and self.v_cruise_kph_last < 250:
        self.v_cruise_kph = self.v_cruise_kph_last  # ← 恢復上次速度
    else:
        # 如果按的是 SET 或第一次啟動
        if desired_speed_limit != 0 and frogpilot_toggles.set_speed_limit:
            self.v_cruise_kph = int(round(desired_speed_limit * CV.MS_TO_KPH))
        else:
            # 使用當前車速或預設速度
            initial = V_CRUISE_INITIAL_EXPERIMENTAL_MODE if experimental_mode else V_CRUISE_INITIAL
            self.v_cruise_kph = int(round(clip(CS.vEgo * CV.MS_TO_KPH, initial, V_CRUISE_MAX)))
```

**調用位置** (`controlsd.py:602`):
```python
self.v_cruise_helper.initialize_v_cruise(CS, self.experimental_mode,
    self.sm['frogpilotPlan'].slcSpeedLimit + self.sm['frogpilotPlan'].slcSpeedLimitOffset,
    self.frogpilot_toggles)
```

---

### 完整流程圖

#### 情境 1: 車輛啟動後直接按 RESUME

```
啟動 openpilot
  ↓
v_cruise_kph = 255 (UNSET)
  ↓
按 RESUME 按鈕
  ↓
controlsd.py:256 檢測到 resume_pressed = True
  ↓
檢查 v_cruise_initialized → False (255 != UNSET)
  ↓
產生 resumeBlocked 事件 (ET.NO_ENTRY)
  ↓
狀態機: disabled 狀態 + NO_ENTRY
  ↓
contains_event_type(ET.NO_ENTRY) → True
  ↓
保持 disabled ❌
  ↓
UI 顯示: "Press Set to Engage"
```

#### 情境 2: 車輛啟動後按 SET

```
啟動 openpilot
  ↓
v_cruise_kph = 255 (UNSET)
  ↓
按 SET 按鈕 (setCruise)
  ↓
interfaces.py:394 - setCruise in enable_buttons → True
  ↓
產生 buttonEnable 事件 (ET.ENABLE)
  ↓
狀態機: disabled 狀態 + ENABLE 事件
  ↓
controlsd.py:590 - contains_event_type(ET.ENABLE) → True
  ↓
controlsd.py:602 - 調用 initialize_v_cruise()
  ↓
drive_helpers.py:160 - 設定初始速度
  ↓
v_cruise_kph = 當前車速 (或 40/105 kph)
  ↓
v_cruise_initialized → True ✅
  ↓
進入 enabled 狀態 ✅
```

#### 情境 3: 按過 SET 後再按 RESUME

```
已經按過 SET
  ↓
v_cruise_kph = 60 kph (已設定)
v_cruise_kph_last = 60 kph
  ↓
按 CANCEL 取消控制
  ↓
v_cruise_kph 保持 60 kph (不會重置)
  ↓
按 RESUME 按鈕
  ↓
controlsd.py:256 檢測到 resume_pressed = True
  ↓
檢查 v_cruise_initialized → True (60 != 255) ✅
  ↓
**不會**產生 resumeBlocked 事件
  ↓
interfaces.py:394 - resumeCruise in enable_buttons → True
  ↓
產生 buttonEnable 事件 (ET.ENABLE)
  ↓
drive_helpers.py:154-155 - 恢復上次速度
  ↓
v_cruise_kph = v_cruise_kph_last = 60 kph
  ↓
進入 enabled 狀態 ✅
```

---

### 設計理念

**為什麼需要這個機制?**

1. **安全性**: 防止在不知道目標速度的情況下啟動控制
   - 如果沒有初始速度,系統不知道要維持多快
   - 可能導致意外加速或減速

2. **使用者體驗**: 符合傳統 ACC 的使用習慣
   - 傳統 ACC 也需要先 SET 設定速度
   - RESUME 是「恢復」已設定的速度,不是初始設定

3. **明確性**: 強制使用者明確設定巡航速度
   - SET = 設定新的巡航速度
   - RESUME = 恢復上次的巡航速度

---

### 相關程式碼位置

| 功能 | 檔案 | 行數 | 說明 |
|------|------|------|------|
| v_cruise 初始化 | `drive_helpers.py` | 47-48 | 設為 V_CRUISE_UNSET (255) |
| v_cruise_initialized 檢查 | `drive_helpers.py` | 54-55 | 判斷是否已初始化 |
| resumeBlocked 檢查 | `controlsd.py` | 256-258 | 阻擋未初始化的 RESUME |
| resumeBlocked 事件定義 | `events.py` | 737-739 | NO_ENTRY: "Press Set to Engage" |
| buttonEnable 產生 | `interfaces.py` | 393-395 | SET/RESUME 按鈕放開時 |
| VW 啟用按鈕配置 | `volkswagen/interface.py` | 109 | setCruise + resumeCruise |
| initialize_v_cruise | `drive_helpers.py` | 146-162 | 設定初始巡航速度 |
| VW 按鈕定義 | `volkswagen/values.py` | 88-89 | GRA_Tip_Setzen, GRA_Tip_Wiederaufnahme |

---

### 實驗:如何跳過 SET 按鈕要求

**⚠️ 警告**: 以下修改僅供研究,**不建議**用於實車,存在安全風險!

如果想讓車輛啟動後可以直接用 RESUME 或油門啟動控制,需要:

**方法 1: 移除 resumeBlocked 檢查**

修改 `controlsd.py:256-258`:
```python
# Block resume if cruise never previously enabled
resume_pressed = any(be.type in (ButtonType.accelCruise, ButtonType.resumeCruise) for be in CS.buttonEvents)
# if not self.CP.pcmCruise and not self.v_cruise_helper.v_cruise_initialized and resume_pressed:
#     self.events.add(EventName.resumeBlocked)  # ← 註解掉這行
```

**問題**: RESUME 按鈕會觸發 `initialize_v_cruise`,但 `v_cruise_kph_last` 是 0,會使用預設速度。

**方法 2: 預設初始化巡航速度**

修改 `drive_helpers.py:47`:
```python
# 原本
self.v_cruise_kph = V_CRUISE_UNSET

# 改為
self.v_cruise_kph = V_CRUISE_INITIAL  # 40 kph (或其他預設值)
```

**問題**: 啟動後立即有預設速度,可能不符合當前車速,造成意外加速/減速。

**方法 3: 使用當前車速作為預設** (較安全)

修改 `drive_helpers.py:47-48`:
```python
def __init__(self, CP):
    self.CP = CP
    self.v_cruise_kph = 0  # 改為 0 而非 UNSET
    self.v_cruise_cluster_kph = 0
    # ... 後續在 update 時設定為當前車速
```

並修改 `v_cruise_initialized`:
```python
@property
def v_cruise_initialized(self):
    return self.v_cruise_kph > 0  # 改為檢查 > 0
```

**建議**: 保持原有設計,這是合理的安全機制。

---

### GRA_ACC_01 按鍵處理（FrogPilot 修改）

**檔案**：`panda/board/safety/safety_volkswagen_mqb.h:169-173`

```c
// Always exit controls on rising edge of Cancel
// Signal: GRA_ACC_01.GRA_Abbrechen
if (GET_BIT(to_push, 13U)) {
  controls_allowed = acc_main_on;  // ← FrogPilot 修改
}
```

**原始 openpilot**：
```c
controls_allowed = false;  // 直接禁用
```

**FrogPilot 修改後**：
- `controls_allowed = acc_main_on`
- 只有當 ACC 主開關關閉（`acc_main_on = false`）時才真正禁用
- 配合第 153-155 行的檢查確保禁用

**設計理念**：
- 讓 ACC 主開關狀態決定是否禁用
- 按 CANCEL 通常會關閉 ACC，從而觸發禁用
- 如果 ACC 主開關保持開啟，OP 可以繼續控制

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

### 實驗模式縱向控制差異

**前提說明**: 本節討論的是在 **OP Longitudinal Control (OP LONG) 已開啟** 的情況下，實驗模式開關對縱向控制行為的影響。如果 OP LONG 關閉，則使用車輛原廠 ACC，以下內容不適用。

實驗模式對 OP LONG 的控制行為（加速/減速）有顯著影響，主要體現在初始速度設定和 MPC 求解模式的不同。

#### 1. 初始巡航速度

**位置**: `selfdrive/controls/lib/drive_helpers.py:13-17,151`

```python
V_CRUISE_MIN = 8                               # 最小定速 8 km/h
V_CRUISE_MAX = 145                             # 最大定速 145 km/h
V_CRUISE_UNSET = 255                           # 未設定標記
V_CRUISE_INITIAL = 40                          # 普通模式初始速度 40 km/h
V_CRUISE_INITIAL_EXPERIMENTAL_MODE = 105       # 實驗模式初始速度 105 km/h

def initialize_v_cruise(v_ego, button_events, v_cruise_last, experimental_mode, frogpilot_toggles):
    initial = (V_CRUISE_INITIAL_EXPERIMENTAL_MODE if experimental_mode and
               not frogpilot_toggles.conditional_experimental_mode
               else V_CRUISE_INITIAL)
```

**差異**:
- **普通模式**: 啟動定速時，系統預設設定 **40 km/h**
- **實驗模式**: 啟動定速時，系統預設設定 **105 km/h**
- **FrogPilot CEM**: 如果啟用 `conditional_experimental_mode`，即使在實驗模式下也使用 40 km/h（由條件式實驗模式動態控制）

#### 2. MPC 求解模式

**重要前提**: 此處討論的是 **OP Longitudinal Control (OP LONG) 已開啟** 的情況下，實驗模式對 MPC 求解器的影響。

**邏輯層次**:
```
OP LONG 關閉 → 使用車輛原廠 ACC (openpilot 不控制油門煞車)
    └─ 實驗模式開關無影響

OP LONG 開啟 → openpilot 控制油門煞車
    ├─ 實驗模式關閉 → MPC 使用 'acc' 模式
    └─ 實驗模式開啟 → MPC 使用 'blended' 模式
```

**位置**: `selfdrive/controls/lib/longitudinal_planner.py:105-107`

```python
def update(self, sm, classic_longitudinal, frogpilot_toggles):
    mode = 'blended' if sm['controlsState'].experimentalMode else 'acc'
    if classic_longitudinal:  # classic_longitudinal 表示使用 OP LONG
        self.mpc.mode = mode
```

**兩種 MPC 模式** (都是 OP LONG 控制，只是演算法不同):
- **`acc` 模式** (實驗模式關閉): OP LONG 使用傳統 ACC 演算法邏輯，只跟隨前車或維持設定速度
- **`blended` 模式** (實驗模式開啟): OP LONG 融合駕駛模型預測，考慮道路曲率、紅綠燈、路口等因素

**關鍵區別**:
- 不是「車輛 ACC vs OP LONG」的區別
- 而是「OP LONG 的兩種不同演算法模式」的區別

#### 3. 加速度限制策略

**位置**: `selfdrive/controls/lib/longitudinal_planner.py:129-136`

```python
if mode == 'acc':
    # ACC 模式: 使用 FrogPilot 自訂加速度限制
    accel_clip = [sm['frogpilotPlan'].minAcceleration,
                  sm['frogpilotPlan'].maxAcceleration]

    # 在彎道中限制加速
    steer_angle_without_offset = (sm['carState'].steeringAngleDeg -
                                   sm['liveParameters'].angleOffsetDeg)
    if not sm['frogpilotPlan'].cscControllingSpeed:
        accel_clip = limit_accel_in_turns(v_ego, steer_angle_without_offset,
                                          accel_clip, self.CP)
else:
    # Blended 模式: 使用全範圍加速度限制
    accel_clip = [ACCEL_MIN, ACCEL_MAX]
```

**差異**:
- **ACC 模式** (`mode == 'acc'`):
  - 最大加速度由個性化設定和速度決定（見下表）
  - 在彎道中會額外限制加速度
  - 受 FrogPilot 加速度配置影響

- **Blended 模式** (`mode == 'blended'`):
  - 使用全範圍限制 `[ACCEL_MIN=-3.5, ACCEL_MAX=2.0]`
  - 更激進的加速能力
  - 不受個性化設定限制（由模型直接控制）

#### 4. 加速到定速速度的影響因素

完整的加速行為由以下多個層級的因素共同決定：

##### Level 1: 最大加速度基礎值

**位置**: `selfdrive/controls/lib/longitudinal_planner.py:20-32`

```python
A_CRUISE_MAX_VALS = [1.6, 1.2, 0.8, 0.6]  # m/s²
A_CRUISE_MAX_BP = [0., 10.0, 25., 40.]    # m/s

def get_max_accel(v_ego):
    return float(np.interp(v_ego, A_CRUISE_MAX_BP, A_CRUISE_MAX_VALS))
```

**基礎加速度表** (僅適用於 ACC 模式):

| 速度 (m/s) | 速度 (km/h) | 最大加速度 (m/s²) |
|-----------|------------|------------------|
| 0         | 0          | 1.6              |
| 10        | 36         | 1.2              |
| 25        | 90         | 0.8              |
| 40+       | 144+       | 0.6              |

##### Level 2: FrogPilot 個性化設定

**位置**: `frogpilot/controls/lib/frogpilot_acceleration.py:39-83`

根據不同的 **加速檔位** (acceleration_profile) 和 **變速箱模式**:

**加速度配置**:
```python
# Eco 模式
A_CRUISE_MAX_BP_CUSTOM = [0.0, 5., 10., 15., 20., 25., 40.]  # m/s
A_CRUISE_MAX_VALS_ECO = [2.0, 1.5, 1.0, 0.8, 0.6, 0.4, 0.2]   # m/s²

# Sport 模式
A_CRUISE_MAX_VALS_SPORT = [3.0, 2.5, 2.0, 1.5, 1.0, 0.8, 0.6]  # m/s²

# ISO 15622:2018 標準
get_max_allowed_accel = [4.0, 4.0, 2.0]  # 對應速度 [0, 5, 20] m/s
```

**減速度配置**:
```python
# CRUISE_MIN_ACCEL 來自 MPC (通常約 -1.2 m/s²)
A_CRUISE_MIN_ECO = CRUISE_MIN_ACCEL / 2     # 約 -0.6 m/s²
A_CRUISE_MIN_SPORT = CRUISE_MIN_ACCEL * 2   # 約 -2.4 m/s²
```

**個性化設定優先級**:
1. **Traffic Mode**: 使用標準 `get_max_accel(v_ego)`
2. **Eco/Sport Gear** (如果啟用 `map_acceleration`):
   - Eco Gear → `A_CRUISE_MAX_VALS_ECO`
   - Sport Gear + profile=2 → `A_CRUISE_MAX_VALS_SPORT`
   - Sport Gear + profile≠2 → ISO 標準
3. **Acceleration Profile** (一般情況):
   - 0: Standard → `get_max_accel(v_ego)`
   - 1: Eco → `A_CRUISE_MAX_VALS_ECO`
   - 2: Sport → `A_CRUISE_MAX_VALS_SPORT`
   - 3: Max → ISO 標準

##### Level 3: Human-like 加速調整

**位置**: `frogpilot/controls/lib/frogpilot_acceleration.py:63-65`

如果啟用 `human_acceleration`:

```python
# 低速時降低加速度 (模擬人類平順起步)
get_max_accel_low_speeds:
    [0, CITY_SPEED_LIMIT/2, CITY_SPEED_LIMIT] → [max_accel/4, max_accel/2, max_accel]

# 接近目標速度時漸降加速度 (避免超速衝刺)
get_max_accel_ramp_off:
    (v_cruise - v_ego) 從 [0, 1, 5] m/s → [0, 0.5, max_accel]
```

**效果**:
- 速度低於城市限速一半時: 最大加速度降為 1/4
- 速度達到城市限速時: 恢復到完整加速度
- 接近目標速度 1 m/s 內: 加速度降為 0
- 距離目標速度 5 m/s: 恢復到完整加速度

##### Level 4: 彎道加速限制

**位置**: `selfdrive/controls/lib/longitudinal_planner.py:38-53`

```python
def limit_accel_in_turns(v_ego, angle_steers, a_target, CP):
    """根據橫向加速度限制縱向加速度"""
    # 總加速度限制 (速度依賴)
    _A_TOTAL_MAX_V = [1.7, 3.2]   # m/s²
    _A_TOTAL_MAX_BP = [20., 40.]  # m/s

    a_total_max = np.interp(v_ego, _A_TOTAL_MAX_BP, _A_TOTAL_MAX_V)

    # 計算橫向加速度
    a_y = v_ego² × angle_steers × DEG_TO_RAD / (steerRatio × wheelbase)

    # 如果橫向加速度顯著 (> MINIMUM_LATERAL_ACCELERATION = 0.35 m/s²)
    if abs(a_y) > 0.35:
        # 計算剩餘可用縱向加速度: a_x = √(a_total² - a_y²)
        a_x_allowed = sqrt(a_total_max² - a_y²)

    return [a_target[0], min(a_target[1], a_x_allowed)]
```

**彎道限制表**:

| 速度 (km/h) | 最大總加速度 (m/s²) | 橫向加速度 0.5 m/s² 時可用縱向加速度 |
|------------|--------------------|---------------------------------|
| 72         | 1.7                | 1.62                            |
| 108        | 2.45               | 2.40                            |
| 144+       | 3.2                | 3.16                            |

**說明**:
- 只在 **ACC 模式** 且 **非 CSC 控制** 時啟用
- 使用向量合成原理: `a_total² = a_x² + a_y²`
- 橫向加速度越大，可用縱向加速度越小
- 保證總加速度不超過舒適度限制

##### Level 5: Personality 跟車距離

**位置**: `selfdrive/controls/lib/longitudinal_mpc_lib/long_mpc.py:86-104`

```python
def get_T_FOLLOW(personality):
    if personality == log.LongitudinalPersonality.aggressive:
        return 1.25  # 秒
    elif personality == log.LongitudinalPersonality.standard:
        return 1.45  # 秒
    else:  # relaxed
        return 1.75  # 秒
```

**影響**:
- 跟車距離越短 (aggressive)，加速意願越強
- 跟車距離越長 (relaxed)，保持更保守的加速

##### Level 6: Personality Jerk 因子

**位置**: `selfdrive/controls/lib/longitudinal_mpc_lib/long_mpc.py:62-83`

```python
def get_jerk_factor(personality, v_ego, v_lead, t_follow, jerk_tuning):
    if personality == aggressive:
        jerk_factor = 0.5   # 更激進的加速度變化
    else:  # standard or relaxed
        jerk_factor = 1.0   # 平順的加速度變化

    # 根據速度和跟車情況進一步調整
    # Jerk 越小，加速越激進；越大，加速越平順
```

##### Level 7: PID 控制器

**位置**: `selfdrive/controls/lib/longcontrol.py:102-131`

```python
def update(self, active, CS, a_target, should_stop, accel_limits, frogpilot_toggles):
    """縱向 PID 控制器"""
    # 設定 PID 限制
    self.pid.neg_limit = accel_limits[0]  # 最小加速度 (減速)
    self.pid.pos_limit = accel_limits[1]  # 最大加速度 (加速)

    if self.long_control_state == LongCtrlState.starting:
        # 起步狀態: 使用固定起步加速度
        output_accel = frogpilot_toggles.startAccel if not human_acceleration else a_target

    elif self.long_control_state == LongCtrlState.pid:
        # PID 狀態: 追蹤目標加速度
        error = a_target - CS.aEgo
        output_accel = self.pid.update(error, speed=CS.vEgo, feedforward=a_target)

    # 最終輸出限制在 accel_limits 範圍內
    self.last_output_accel = clip(output_accel, accel_limits[0], accel_limits[1])
```

**PID 參數來源**:
- `CP.longitudinalTuning.kpBP`, `kpV`: 比例增益
- `CP.longitudinalTuning.kiBP`, `kiV`: 積分增益
- `CP.longitudinalTuning.kf`: 前饋增益
- 這些參數在 `selfdrive/car/[manufacturer]/values.py` 中為每個車型定義

##### Level 8: 車輛特定限制

**位置**: `selfdrive/car/interfaces.py:123-124`

```python
@staticmethod
def get_pid_accel_limits(CP, current_speed, cruise_speed):
    return ACCEL_MIN, ACCEL_MAX  # 預設: -3.5, 2.0 m/s²
```

某些車廠會覆寫此方法提供特定限制，例如:
- **Toyota**: 根據當前速度和目標速度動態調整
- **Honda**: 考慮變速箱狀態
- **GM**: 考慮 ACC 狀態

#### 5. 實驗模式開關對比總結

**前提**: 以下對比皆在 **OP LONG 已開啟** 的情況下進行

| 項目 | OP LONG + 實驗模式關閉 (`acc`) | OP LONG + 實驗模式開啟 (`blended`) |
|------|-------------------------------|----------------------------------|
| **縱向控制權** | ✅ OP 控制油門煞車 | ✅ OP 控制油門煞車 |
| **初始定速** | 40 km/h | 105 km/h (CEM 啟用時為 40) |
| **MPC 模式** | `acc` (傳統 ACC 邏輯) | `blended` (融合模型預測) |
| **加速度限制** | 個性化 + 速度表 + 彎道限制 | 全範圍 `[-3.5, 2.0]` |
| **彎道加速限制** | ✅ 啟用 | ❌ 不啟用 |
| **駕駛模型影響** | ❌ 僅跟車/定速 | ✅ 融合道路預測 |
| **紅綠燈識別** | ❌ | ✅ |
| **路口減速** | ❌ | ✅ |
| **彎道預判減速** | ❌ | ✅ |
| **控制平順度** | 較平順 (受個性化限制) | 較激進 (由模型主導) |

**注意**: 如果 OP LONG 關閉，則使用車輛原廠 ACC，上述所有 OP 控制邏輯都不適用。

#### 6. 加速流程完整圖示

```
使用者按下 SET 啟動定速
    ↓
┌─────────────────────────────────────┐
│ 初始速度設定                          │
│ - 實驗模式: 105 km/h                 │
│ - 普通模式: 40 km/h                  │
│ - CEM 啟用: 40 km/h (任何模式)        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ MPC 計算目標速度/加速度               │
│ - ACC 模式: 跟車或維持速度            │
│ - Blended 模式: 融合模型預測         │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 1: 基礎加速度限制               │
│ 速度 0→40 m/s: 1.6→0.6 m/s²         │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 2: FrogPilot 個性化            │
│ - Standard / Eco / Sport / ISO       │
│ - Traffic Mode 優先                  │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 3: Human Acceleration (選用)   │
│ - 低速時降低 → 1/4 加速度             │
│ - 接近目標速度時漸降                  │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 4: 彎道限制 (僅 ACC 模式)      │
│ a_x = √(a_total² - a_y²)            │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 5: Personality 跟車距離        │
│ Aggressive(1.25s) / Relaxed(1.75s)  │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 6: Jerk 平順度控制             │
│ Aggressive(0.5) / Standard(1.0)     │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 7: PID 控制器執行              │
│ 追蹤目標加速度 a_target              │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Level 8: 車輛特定限制                │
│ get_pid_accel_limits()              │
└─────────────────────────────────────┘
    ↓
最終輸出加速度指令到 CAN
```

#### 7. 關鍵檔案位置

| 檔案 | 功能 |
|------|------|
| `selfdrive/controls/lib/drive_helpers.py` | 初始速度、巡航速度管理 |
| `selfdrive/controls/lib/longitudinal_planner.py` | MPC 整合、彎道限制、模式選擇 |
| `selfdrive/controls/lib/longitudinal_mpc_lib/long_mpc.py` | MPC 求解器、T_FOLLOW、Jerk 計算 |
| `frogpilot/controls/lib/frogpilot_acceleration.py` | FrogPilot 個性化加速度 |
| `selfdrive/controls/lib/longcontrol.py` | PID 控制器、狀態機 |
| `selfdrive/controls/controlsd.py` | 主控制循環 |
| `selfdrive/car/interfaces.py` | 車輛介面基類 |
| `selfdrive/car/[manufacturer]/values.py` | 車型特定參數 |

---

### FrogPilot 縱向控制參數完整對照表

本節列出所有 FrogPilot UI 中可調整的縱向控制相關參數，說明每個參數控制的內部變數、程式位置、以及調整後的實際影響。

#### 參數分類概覽

FrogPilot 縱向控制參數分為以下幾大類：
1. **基礎啟用**: 控制 OP LONG 的開關
2. **個性化設定** (Personalities): Aggressive / Standard / Relaxed / Traffic 四種模式
3. **加速度檔位**: Eco / Standard / Sport / Max 四種加速度配置
4. **進階調整**: 起步/停止/Human-like 行為調整
5. **速限控制器** (SLC): 自動速限跟隨
6. **條件式實驗模式** (CEM): 智慧切換實驗模式
7. **彎道速度控制器** (CSC): 彎道減速

---

#### 1. 基礎啟用參數

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 | 調高影響 | 調低影響 |
|------------|-------|---------|---------|---------|---------|---------|
| `ExperimentalLongitudinalEnabled` | 關閉 | `CP.openpilotLongitudinalControl` | `frogpilot_variables.py:562` | 啟用 OP LONG (油門煞車控制) | 啟用 OP 縱向控制 | 使用車輛原廠 ACC |
| `DisableOpenpilotLongitudinal` | 關閉 | `toggle.disable_openpilot_long` | `frogpilot_variables.py:548` | 強制關閉 OP LONG | 即使車輛支援也關閉 | 按車輛支援狀態 |

**說明**:
- 必須先啟用 `ExperimentalLongitudinalEnabled` 才能使用以下所有縱向控制參數
- `DisableOpenpilotLongitudinal` 用於臨時關閉 OP LONG（例如某些情況下需要使用原廠 ACC）

---

#### 2. 個性化設定 (Personalities)

FrogPilot 提供四種駕駛個性，每種個性控制 **跟車距離 (T_FOLLOW)** 和 **Jerk 平順度**。

##### 2.1 Aggressive (激進模式)

| UI 參數名稱 | 預設值 | 範圍 | 程式變數 | 程式位置 | 功能說明 | 調高影響 | 調低影響 |
|------------|-------|------|---------|---------|---------|---------|---------|
| `AggressiveFollow` | 1.25s | 1.0-3.0s | `toggle.aggressive_follow` | `frogpilot_variables.py:672`<br>`frogpilot_following.py:50-55` | 跟車時間間距 | 跟車距離變遠，更保守 | 跟車距離變近，更激進 |
| `AggressiveJerkAcceleration` | 50% | 25-200% | `toggle.aggressive_jerk_acceleration` | `frogpilot_variables.py:667`<br>`long_mpc.py:62-83` | 加速時的 Jerk 平順度 | 加速更平順溫和 | 加速更猛烈急促 |
| `AggressiveJerkDeceleration` | 50% | 25-200% | `toggle.aggressive_jerk_deceleration` | `frogpilot_variables.py:668` | 減速時的 Jerk 平順度 | 煞車更平順溫和 | 煞車更猛烈急促 |
| `AggressiveJerkDanger` | 100% | 25-200% | `toggle.aggressive_jerk_danger` | `frogpilot_variables.py:669` | 危險情況的 Jerk (通常不調整) | 緊急煞車更平順 | 緊急煞車更急促 |
| `AggressiveJerkSpeed` | 50% | 25-200% | `toggle.aggressive_jerk_speed` | `frogpilot_variables.py:670` | 速度變化的 Jerk (加速時) | 速度變化更平順 | 速度變化更快速 |
| `AggressiveJerkSpeedDecrease` | 50% | 25-200% | `toggle.aggressive_jerk_speed_decrease` | `frogpilot_variables.py:671` | 速度變化的 Jerk (減速時) | 速度變化更平順 | 速度變化更快速 |

##### 2.2 Standard (標準模式)

| UI 參數名稱 | 預設值 | 範圍 | 程式變數 | 程式位置 | 功能說明 | 調高影響 | 調低影響 |
|------------|-------|------|---------|---------|---------|---------|---------|
| `StandardFollow` | 1.45s | 1.0-3.0s | `toggle.standard_follow` | `frogpilot_variables.py:679` | 跟車時間間距 | 跟車距離變遠 | 跟車距離變近 |
| `StandardJerkAcceleration` | 100% | 25-200% | `toggle.standard_jerk_acceleration` | `frogpilot_variables.py:674` | 加速時的 Jerk 平順度 | 加速更平順 | 加速更猛烈 |
| `StandardJerkDeceleration` | 100% | 25-200% | `toggle.standard_jerk_deceleration` | `frogpilot_variables.py:675` | 減速時的 Jerk 平順度 | 煞車更平順 | 煞車更猛烈 |
| `StandardJerkDanger` | 100% | 25-200% | `toggle.standard_jerk_danger` | `frogpilot_variables.py:676` | 危險情況的 Jerk | 緊急煞車更平順 | 緊急煞車更急促 |
| `StandardJerkSpeed` | 100% | 25-200% | `toggle.standard_jerk_speed` | `frogpilot_variables.py:677` | 速度變化的 Jerk (加速時) | 速度變化更平順 | 速度變化更快速 |
| `StandardJerkSpeedDecrease` | 100% | 25-200% | `toggle.standard_jerk_speed_decrease` | `frogpilot_variables.py:678` | 速度變化的 Jerk (減速時) | 速度變化更平順 | 速度變化更快速 |

##### 2.3 Relaxed (輕鬆模式)

| UI 參數名稱 | 預設值 | 範圍 | 程式變數 | 程式位置 | 功能說明 | 調高影響 | 調低影響 |
|------------|-------|------|---------|---------|---------|---------|---------|
| `RelaxedFollow` | 1.75s | 1.0-3.0s | `toggle.relaxed_follow` | `frogpilot_variables.py:686` | 跟車時間間距 | 跟車距離變遠 | 跟車距離變近 |
| `RelaxedJerkAcceleration` | 100% | 25-200% | `toggle.relaxed_jerk_acceleration` | `frogpilot_variables.py:681` | 加速時的 Jerk 平順度 | 加速更平順 | 加速更猛烈 |
| `RelaxedJerkDeceleration` | 100% | 25-200% | `toggle.relaxed_jerk_deceleration` | `frogpilot_variables.py:682` | 減速時的 Jerk 平順度 | 煞車更平順 | 煞車更猛烈 |
| `RelaxedJerkDanger` | 100% | 25-200% | `toggle.relaxed_jerk_danger` | `frogpilot_variables.py:683` | 危險情況的 Jerk | 緊急煞車更平順 | 緊急煞車更急促 |
| `RelaxedJerkSpeed` | 100% | 25-200% | `toggle.relaxed_jerk_speed` | `frogpilot_variables.py:684` | 速度變化的 Jerk (加速時) | 速度變化更平順 | 速度變化更快速 |
| `RelaxedJerkSpeedDecrease` | 100% | 25-200% | `toggle.relaxed_jerk_speed_decrease` | `frogpilot_variables.py:685` | 速度變化的 Jerk (減速時) | 速度變化更平順 | 速度變化更快速 |

##### 2.4 Traffic (塞車模式)

| UI 參數名稱 | 預設值 | 範圍 | 程式變數 | 程式位置 | 功能說明 | 調高影響 | 調低影響 |
|------------|-------|------|---------|---------|---------|---------|---------|
| `TrafficFollow` | 0.5s | 0.5-3.0s | `toggle.traffic_follow` | `frogpilot_variables.py:690` | 跟車時間間距 | 跟車距離變遠 | 跟車距離變近 |
| `TrafficJerkAcceleration` | 50% | 25-200% | `toggle.traffic_jerk_acceleration` | `frogpilot_variables.py:687` | 加速時的 Jerk 平順度 | 加速更平順 | 加速更猛烈 |
| `TrafficJerkDeceleration` | 50% | 25-200% | `toggle.traffic_jerk_deceleration` | `frogpilot_variables.py:688` | 減速時的 Jerk 平順度 | 煞車更平順 | 煞車更猛烈 |
| `TrafficJerkDanger` | 100% | 25-200% | `toggle.traffic_jerk_danger` | `frogpilot_variables.py:692` | 危險情況的 Jerk | 緊急煞車更平順 | 緊急煞車更急促 |
| `TrafficJerkSpeed` | 50% | 25-200% | `toggle.traffic_jerk_speed` | `frogpilot_variables.py:689` | 速度變化的 Jerk (加速時) | 速度變化更平順 | 速度變化更快速 |
| `TrafficJerkSpeedDecrease` | 50% | 25-200% | `toggle.traffic_jerk_speed_decrease` | `frogpilot_variables.py:691` | 速度變化的 Jerk (減速時) | 速度變化更平順 | 速度變化更快速 |

**Jerk 參數說明**:
- **Jerk**: 加速度的變化率 (m/s³)，數值越小越激進，越大越平順
- **100% = 1.0**: 標準值，使用 openpilot 原始設定
- **50% = 0.5**: 一半的 Jerk cost，加速更激進、快速
- **200% = 2.0**: 兩倍的 Jerk cost，加速更平順、溫和
- **Acceleration vs Speed Jerk**:
  - `JerkAcceleration`: 控制加速度本身的變化 (a → a')
  - `JerkSpeed`: 控制速度變化導致的 Jerk (v → v' → a')
- **加速 vs 減速**:
  - 系統根據 `carState.aEgo` 判斷當前是加速還是減速
  - `aEgo >= 0` → 使用 Acceleration/Speed 參數
  - `aEgo < 0` → 使用 Deceleration/SpeedDecrease 參數

**T_FOLLOW (跟車距離) 說明**:
- 計算公式: `desired_distance = v_ego * T_FOLLOW + STOP_DISTANCE`
- 例如: 時速 100 km/h (27.8 m/s)，T_FOLLOW=1.45s → 距離 ≈ 40 公尺
- Traffic 模式在低速時使用更短的跟車距離（塞車時更緊跟）
- 在高速時會根據速度插值到正常跟車距離

---

#### 3. 加速度檔位配置

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 | 選項說明 |
|------------|-------|---------|---------|---------|---------|
| `AccelerationProfile` | Sport | `toggle.acceleration_profile` | `frogpilot_variables.py:816`<br>`frogpilot_acceleration.py:54-61` | 加速度檔位 | 0=Standard, 1=Eco, 2=Sport, 3=Max |
| `DecelerationProfile` | Standard | `toggle.deceleration_profile` | `frogpilot_variables.py:817`<br>`frogpilot_acceleration.py:77-82` | 減速度檔位 | 0=Standard, 1=Eco, 2=Sport |

**加速度檔位對照表** (`frogpilot_acceleration.py`):

| 速度 (m/s) | 速度 (km/h) | Standard | Eco | Sport | Max (ISO) |
|-----------|------------|----------|-----|-------|-----------|
| 0         | 0          | 1.6      | 2.0 | 3.0   | 4.0       |
| 5         | 18         | 1.4      | 1.5 | 2.5   | 4.0       |
| 10        | 36         | 1.2      | 1.0 | 2.0   | 3.0       |
| 15        | 54         | 1.0      | 0.8 | 1.5   | 2.5       |
| 20        | 72         | 0.8      | 0.6 | 1.0   | 2.0       |
| 25        | 90         | 0.8      | 0.4 | 0.8   | 2.0       |
| 40        | 144        | 0.6      | 0.2 | 0.6   | 2.0       |

**減速度配置**:
- **Eco**: `CRUISE_MIN_ACCEL / 2` ≈ -0.6 m/s² (輕柔減速，滑行為主)
- **Standard**: `CRUISE_MIN_ACCEL` ≈ -1.2 m/s² (標準減速)
- **Sport**: `CRUISE_MIN_ACCEL * 2` ≈ -2.4 m/s² (積極減速)

**Map Gears (變速箱映射)**:
- `MapAcceleration`: 根據車輛變速箱檔位自動選擇 Eco/Sport 加速度
- `MapDeceleration`: 根據車輛變速箱檔位自動選擇 Eco/Sport 減速度
- 需要車輛支援變速箱狀態回報 (`ecoGear`, `sportGear`)

---

#### 4. Human-like 行為調整

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 | 啟用影響 |
|------------|-------|---------|---------|---------|---------|
| `HumanAcceleration` | 開啟 | `toggle.human_acceleration` | `frogpilot_variables.py:818`<br>`frogpilot_acceleration.py:63-65` | 模擬人類加速行為 | 低速時降低加速度，接近目標速度時漸降 |
| `HumanFollowing` | 開啟 | `toggle.human_following` | `frogpilot_variables.py:819`<br>`frogpilot_following.py:76-95` | 模擬人類跟車行為 | 動態調整跟車距離和 Jerk |

**HumanAcceleration 行為**:
1. **低速平順起步**:
   ```python
   # 速度低於城市限速 (55 mph / 88 km/h) 時降低加速度
   [0, CITY_SPEED_LIMIT/2, CITY_SPEED_LIMIT] → [max_accel/4, max_accel/2, max_accel]
   ```
   - 速度 < 44 km/h: 最大加速度降為 1/4
   - 速度 44-88 km/h: 線性恢復到完整加速度

2. **接近目標速度時減速**:
   ```python
   # 距離目標速度 (v_cruise - v_ego) 決定加速度
   (v_cruise - v_ego) 從 [0, 1, 5] m/s → [0, 0.5, max_accel]
   ```
   - 距離 < 1 m/s (3.6 km/h): 加速度降為 0
   - 距離 1-5 m/s: 線性恢復到完整加速度
   - 距離 > 5 m/s: 使用完整加速度

**HumanFollowing 行為**:
1. **接近更快前車時**: 降低 Jerk 和 T_FOLLOW，更積極加速追上
   ```python
   accelerating_offset = clip(STOP_DISTANCE - v_ego, 1, distance_factor)
   acceleration_jerk /= accelerating_offset
   t_follow /= accelerating_offset
   ```

2. **接近更慢前車時**: 增加 T_FOLLOW，提早減速保持距離
   ```python
   braking_offset = clip(min(v_ego - v_lead, v_lead) - COMFORT_BRAKE, 1, distance_factor)
   t_follow /= braking_offset + far_lead_offset
   ```

---

#### 5. 進階調整參數

需要啟用 `AdvancedLongitudinalTune` 才能調整以下參數:

| UI 參數名稱 | 預設值 | 範圍 | 程式變數 | 程式位置 | 功能說明 | 調高影響 | 調低影響 |
|------------|-------|------|---------|---------|---------|---------|---------|
| `MaxDesiredAcceleration` | 2.0 m/s² | 0.1-4.0 | `toggle.max_desired_acceleration` | `frogpilot_variables.py:610`<br>`controlsd.py:670,672` | PID 控制器最大輸出 | 更高的加速上限 | 更保守的加速 |
| `StartAccel` | 車型預設 | 0-4 m/s² | `toggle.startAccel` | `frogpilot_variables.py:611`<br>`longcontrol.py:122` | 起步時的固定加速度 | 起步更快速 | 起步更平順 |
| `StopAccel` | 車型預設 | -4-0 m/s² | `toggle.stopAccel` | `frogpilot_variables.py:612`<br>`longcontrol.py:116` | 停止前的最小加速度 | (接近0) 煞車更輕柔 | (接近-4) 煞車更果斷 |
| `StoppingDecelRate` | 車型預設 | 0.001-1 | `toggle.stoppingDecelRate` | `frogpilot_variables.py:613`<br>`longcontrol.py:118` | 停止時的減速率 | 煞車減速更快 | 煞車減速更慢 |
| `VEgoStarting` | 車型預設 | 0.01-1 m/s | `toggle.vEgoStarting` | `frogpilot_variables.py:614`<br>`longcontrol.py:21,63` | 判定為"已起步"的速度 | 更高速度才認為起步 | 更低速度就認為起步 |
| `VEgoStopping` | 車型預設 | 0.01-1 m/s | `toggle.vEgoStopping` | `frogpilot_variables.py:615`<br>`longcontrol.py:52,55` | 判定為"停止"的速度 | 更高速度就認為停止 | 更低速度才認為停止 |
| `LongitudinalActuatorDelay` | 車型預設 | 0-1s | `toggle.longitudinalActuatorDelay` | `frogpilot_variables.py:609`<br>`longcontrol.py:146` | 縱向執行器延遲補償 | 補償更大延遲 | 補償更小延遲 |

**特殊車型調整**:
- **GM ExperimentalGMTune**: `StoppingDecelRate=0.3, VEgoStarting=0.15, VEgoStopping=0.15`
- **Toyota FrogsGoMoosTweak**: `StoppingDecelRate=0.01, VEgoStarting=0.1, VEgoStopping=0.5`

---

#### 6. QOL (Quality of Life) 縱向控制參數

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 | 啟用影響 |
|------------|-------|---------|---------|---------|---------|
| `CustomCruise` | 1 km/h | `toggle.cruise_increase` | `frogpilot_variables.py:886` | 短按速度調整按鈕的增減量 | 自訂每次調整幅度 |
| `CustomCruiseLong` | 5 km/h | `toggle.cruise_increase_long` | `frogpilot_variables.py:887` | 長按速度調整按鈕的增減量 | 自訂長按調整幅度 |
| `SetSpeedOffset` | 0 km/h | `toggle.set_speed_offset` | `frogpilot_variables.py:894` | 定速設定速度偏移 | 自動在設定速度上加/減此值 |
| `ReverseCruise` | 關閉 | `toggle.reverse_cruise_increase` | `frogpilot_variables.py:893` | 反轉定速調整方向 (Toyota PCM) | 上鍵減速、下鍵加速 |
| `ForceStops` | 關閉 | `toggle.force_stops` | `frogpilot_variables.py:888` | 強制停止在停止線 | 即使前方無車也會停止 |
| `IncreasedStoppedDistance` | 0 ft | `toggle.increase_stopped_distance` | `frogpilot_variables.py:889`<br>`frogpilot_planner.py:148` | 增加停止距離 | 停得離停止線/前車更遠 |

---

#### 7. 條件式實驗模式 (CEM) 參數

詳見「條件式實驗模式 (CEM)」章節，這裡列出與縱向控制直接相關的參數:

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 |
|------------|-------|---------|---------|---------|
| `ConditionalExperimental` | 開啟 | `toggle.conditional_experimental_mode` | `frogpilot_variables.py:638` | 啟用條件式實驗模式 |
| `CESpeed` | 0 mph | `toggle.conditional_limit` | `frogpilot_variables.py:644` | 速度低於此值自動啟用實驗模式 |
| `CESpeedLead` | 0 mph | `toggle.conditional_limit_lead` | `frogpilot_variables.py:645` | 有前車且速度低於此值啟用實驗模式 |

---

#### 8. 速限控制器 (SLC) 參數

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 |
|------------|-------|---------|---------|---------|
| `SpeedLimitController` | 開啟 | `toggle.speed_limit_controller` | `frogpilot_variables.py:916` | 啟用速限控制器 |
| `SetSpeedLimit` | 關閉 | `toggle.set_speed_limit` | `frogpilot_variables.py:920` | 自動設定定速為速限 |
| `SLCLookaheadHigher` | 0s | `toggle.map_speed_lookahead_higher` | `frogpilot_variables.py:918` | 提前多久看到更高速限 |
| `SLCLookaheadLower` | 0s | `toggle.map_speed_lookahead_lower` | `frogpilot_variables.py:919` | 提前多久看到更低速限 |
| `SLCFallback` | Set Speed | `toggle.slc_fallback_*` | `frogpilot_variables.py:922-925` | 無速限時的備用方案 |

**SLC Fallback 選項**:
- 0: 使用當前設定速度
- 1: 切換到實驗模式
- 2: 使用上一個已知速限

---

#### 9. 彎道速度控制器 (CSC) 參數

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 |
|------------|-------|---------|---------|---------|
| `CurveSpeedController` | 開啟 | `toggle.curve_speed_controller` | `frogpilot_variables.py:655` | 啟用彎道速度控制器 |

**功能**: 根據模型預測的道路曲率自動調整速度，在彎道前減速。

---

#### 10. 其他縱向控制相關參數

| UI 參數名稱 | 預設值 | 程式變數 | 程式位置 | 功能說明 |
|------------|-------|---------|---------|---------|
| `LeadDetectionThreshold` | 50% | `toggle.lead_detection_probability` | `frogpilot_variables.py:820` | 前車偵測機率門檻 (降低會更早偵測前車) |
| `TacoTune` | 關閉 | `toggle.taco_tune` | `frogpilot_variables.py:821`<br>`longitudinal_planner.py:92-96` | Taco 彎道速度調整 (根據曲率限制模型速度) |

---

### 參數調整建議與注意事項

#### 新手建議
1. **保持預設個性化設定**: Aggressive/Standard/Relaxed 的預設值已經過良好調校
2. **優先調整 T_FOLLOW**: 如果覺得跟車太近/太遠，調整各個性的 Follow 參數即可
3. **謹慎調整 Jerk**: Jerk 參數影響乘坐舒適度，過低會造成乘客不適

#### 進階調整
1. **加速度檔位**: 根據個人喜好選擇 Eco/Sport，不建議長期使用 Max (耗能且不舒適)
2. **Human-like 功能**: 建議保持開啟，提供更自然的駕駛感受
3. **進階參數**: 只有在遇到特定問題時才調整 (例如起步過慢、停止不順等)

#### 安全注意事項
⚠️ **警告**:
- **不當調整可能影響行車安全**
- T_FOLLOW 過小會導致跟車過近，緊急情況反應時間不足
- 加速度過高會增加輪胎打滑風險
- 減速度過低會導致煞車距離過長
- 務必在安全環境下測試調整效果

#### 測試建議
1. **每次只調整一個參數**: 方便判斷影響
2. **記錄調整前後數值**: 方便回復
3. **選擇熟悉的道路測試**: 更容易感受差異
4. **注意 rlog 記錄**: 可以事後分析加速度曲線

---

### 條件式實驗模式 (CEM)

**位置**: `frogpilot/controls/lib/conditional_experimental_mode.py`

**核心變數**：
```python
# frogpilot/common/frogpilot_variables.py:638
toggle.conditional_experimental_mode = toggle.openpilot_longitudinal and \
    params.get_bool("ConditionalExperimental")
```

### 啟動機制

**初始化流程**：
```
system/manager 啟動
    ↓
啟動 selfdrive/controls/plannerd.py
    ↓
引入 frogpilot_planner.py
    ↓
FrogPilotPlanner.__init__() (line 23)
    ↓
self.cem = ConditionalExperimentalMode(self) (line 24)
    ↓ 【實例始終被創建，無論開關狀態】
每個循環週期調用 FrogPilotPlanner.update()
```

**呼叫邏輯** (`frogpilot_planner.py:62-66`)：
```python
if sm["controlsState"].enabled and frogpilot_toggles.conditional_experimental_mode:
    self.cem.update(v_ego, sm, frogpilot_toggles)  # 開啟時完整更新
else:
    self.cem.curve_detected = False                 # 關閉時清除狀態
    self.cem.stop_sign_and_light(v_ego, sm, PLANNER_TIME - 2)
```

### 關閉後行為

| 項目 | 開關開啟 | 開關關閉 |
|------|---------|---------|
| **實例創建** | ✅ | ✅ |
| **佔用記憶體** | ✅ | ✅ |
| **主要邏輯** | ✅ 完整執行 | ❌ 不執行 |
| **條件檢測** | ✅ 全部條件 | ❌ 不檢測 |
| **stop_sign_and_light** | ✅ | ✅ 仍執行 |
| **CPU 負載** | 較高 | 極低 |

**關閉後仍執行的部分**：
- `self.cem.stop_sign_and_light()` - 保持紅綠燈檢測狀態清除
- 不會觸發實驗模式切換
- CPU 負載極低（只清除標誌）

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

## FrogPilot 導航與地圖系統

FrogPilot 使用 OpenStreetMap (OSM) 離線地圖數據提供道路名稱、速限等資訊。

### mapd 服務架構

**核心組件**：
- **mapd 二進制執行檔**：獨立程式，查詢 OSM 數據並寫入 PARAMS
- **mapd.py 管理器**：下載、更新和監控 mapd 執行檔

**程式位置**：
- 管理器：`frogpilot/navigation/mapd.py`
- 執行檔路徑：`/data/media/0/osm/mapd`
- 離線地圖：`/data/media/0/osm/offline/`
- 進程配置：`system/manager/process_config.py:119`

### 啟動流程

```
system/manager 啟動
    ↓
讀取 process_config.py
    ↓
啟動 PythonProcess("mapd", "frogpilot.navigation.mapd", always_run)
    ↓
mapd.py 主循環 (mapd_thread)
    ├─ 檢查 /data/media/0/osm/mapd 是否存在
    ├─ 檢查版本（/data/media/0/osm/mapd_version）
    ├─ 如需更新 → 從 GitHub 下載最新版本
    └─ 設置執行權限
    ↓
subprocess.Popen("/data/media/0/osm/mapd")
    ↓
mapd 執行檔運行
    ├─ 讀取 GPS 位置（liveLocationKalman）
    ├─ 查詢 OSM 離線數據庫
    └─ 寫入 params_memory
        ├─ MapSpeedLimit（當前道路速限）
        ├─ RoadName（道路名稱）
        └─ NextMapSpeedLimit（下一個速限）
    ↓
如 mapd 崩潰 → mapd.py 自動重啟
```

### mapd.py 關鍵函數

**下載函數** (`mapd.py:44-75`)：
```python
def download():
    Path(MAPD_PATH).parent.mkdir(parents=True, exist_ok=True)

    latest_version = get_latest_version()
    urls = [
        f"https://github.com/pfeiferj/openpilot-mapd/releases/download/{latest_version}/mapd",
        f"https://gitlab.com/{RESOURCES_REPO}/-/raw/Mapd/{latest_version}"
    ]

    # 下載二進制檔
    with urllib.request.urlopen(url) as response:
        with tempfile.NamedTemporaryFile("wb", delete=False) as temp_file:
            shutil.copyfileobj(response, temp_file)

    # 設置執行權限
    os.chmod(temp_file_path, os.stat(temp_file_path).st_mode | stat.S_IEXEC)
    os.rename(temp_file_path, MAPD_PATH)
```

**啟動函數** (`mapd.py:123-141`)：
```python
def mapd_thread():
    while True:
        cleanup_temp_files()

        # 確保 mapd 是最新版本
        while not update_mapd():
            time.sleep(60)
            continue

        # 啟動 mapd 執行檔
        process = subprocess.Popen(str(MAPD_PATH))
        process.wait()  # 等待執行檔運行
```

### mapd 執行檔與地圖資料

**mapd 主執行檔**：
- 位置：`/data/media/0/osm/mapd`
- 類型：Go 語言編譯的二進制執行檔（約 9.3 MB）
- 來源：從 GitHub `pfeiferj/openpilot-mapd` 下載
- 版本檔：`/data/media/0/osm/mapd_version`

**離線地圖資料**：
- 位置：`/data/media/0/osm/offline/`
- 結構：`{緯度}/{經度}/{地圖檔案}`
- 檔案命名格式：`{緯度起點}_{經度起點}_{緯度終點}_{經度終點}`

**範例（台灣地圖）**：
```
/data/media/0/osm/offline/
├── 20/
│   └── 120/
│       └── 20.000000_120.000000_20.250000_120.250000
├── 22/
│   └── 120/
│       └── 22.000000_120.000000_22.250000_120.250000
└── 24/
    └── 120/
        └── 24.750000_121.250000_25.000000_121.500000  (台北地區)
```

**執行機制**：
```
mapd 主執行檔運行
    ↓
讀取 GPS 位置 (例如: 24.9°N, 121.4°E)
    ↓
計算對應地圖檔案 (24/120/24.750000_121.250000_...)
    ↓
載入該地圖檔案（資料檔，不是執行檔）
    ↓
查詢 OSM 資料：道路名稱、速限、幾何資訊等
    ↓
寫入 PARAMS
```

**日誌範例**：
```json
{"level":"info","filename":"/data/media/0/osm/offline/24/120/24.750000_121.250000_25.000000_121.500000","message":"Loading bounds file"}
{"level":"info","message":"Starting main loop with stability improvements and distance smoothing"}
```

### PARAMS 寫入機制

**參數定義** (`common/params.cc`)：
```cpp
{"MapSpeedLimit", CLEAR_ON_MANAGER_START},      // Line 396
{"NextMapSpeedLimit", CLEAR_ON_MANAGER_START},  // Line 409
{"RoadName", CLEAR_ON_MANAGER_START},           // Line 461
```

**重要：這些 PARAMS 存在記憶體中**

**存放位置**：
- ✅ `/dev/shm/params/` （記憶體參數，params_memory）
- ❌ **不在** `/data/params/d/` （持久化參數）
- ⚠️ 重開機後會消失

**正確的檢查方式**：
```bash
# 方法 1：直接查看記憶體參數目錄
ls -la /dev/shm/params/d/
cat /dev/shm/params/d/MapSpeedLimit
cat /dev/shm/params/d/RoadName

# 方法 2：使用 Python
cd /data/openpilot
python -c "
from openpilot.frogpilot.common.frogpilot_variables import params_memory
print('MapSpeedLimit:', params_memory.get('MapSpeedLimit', encoding='utf-8'))
print('RoadName:', params_memory.get('RoadName', encoding='utf-8'))
print('NextMapSpeedLimit:', params_memory.get('NextMapSpeedLimit', encoding='utf-8'))
"
```

**寫入來源**：
- ❌ 不是由 Python 代碼寫入
- ❌ 不是由 FrogPilot C++ 代碼寫入
- ✅ 由 **mapd 二進制執行檔** 直接寫入到 params_memory

**數據流程**：
```
GPS (liveLocationKalman)
    ↓
mapd 執行檔
    ↓
載入離線地圖檔案
    ↓
OSM 資料解析
    ↓
params_memory.put():
  - MapSpeedLimit (float, m/s)
  - RoadName (string)
  - NextMapSpeedLimit (json)
    ↓
speed_limit_controller.py 讀取
    ↓
顯示在 UI
```

### 讀取這些參數的代碼

**速限控制器** (`frogpilot/controls/lib/speed_limit_controller.py:307-333`)：
```python
def update_map_speed_limit(self, gps_position, v_ego):
    # 讀取當前道路速限
    self.map_speed_limit = params_memory.get_float("MapSpeedLimit")

    # 讀取下一個速限（含位置）
    next_map_speed_limit = json.loads(params_memory.get("NextMapSpeedLimit") or "{}")
    self.next_speed_limit = next_map_speed_limit.get("speedlimit", 0)

    # 計算距離並決定是否提前切換
    if self.next_speed_limit:
        distance_to_upcoming = calculate_distance_to_point(...)
        if distance_to_upcoming < max_lookahead:
            self.map_speed_limit = self.next_speed_limit
```

**速限填充器** (`frogpilot/system/speed_limit_filler.py:220-227`)：
```python
map_speed = params_memory.get_float("MapSpeedLimit")
road_name = params_memory.get("RoadName", encoding="utf-8")
```

**UI 顯示** (`frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc`)：
```cpp
// Line 746: 顯示道路名稱
QString roadName = QString::fromStdString(params_memory.get("RoadName"));

// Line 860: 顯示地圖速限
drawSource(mapDataRect, mapDataIcon, "Map Data",
           frogpilotPlan.getSlcMapSpeedLimit() * speedConversion);
```

### 手動測試 mapd

**1. 停止現有 mapd 進程**：
```bash
pkill -f mapd
```

**2. 直接執行 mapd**：
```bash
/data/media/0/osm/mapd
```

**預期輸出**：
```json
{"level":"info","filename":"/data/media/0/osm/offline/24/120/24.750000_121.250000_25.000000_121.500000","message":"Loading bounds file"}
{"level":"info","message":"Starting main loop with stability improvements and distance smoothing"}
```

**說明**：
- mapd 會持續運行（不會自動退出）
- 按 `Ctrl+C` 可以停止
- 如果沒有輸出或報錯，表示有問題

**3. 背景執行並查看日誌**：
```bash
/data/media/0/osm/mapd > /tmp/mapd.log 2>&1 &
tail -f /tmp/mapd.log
```

**4. 檢查 params_memory**：
```bash
# 開啟另一個 SSH 連線（不要關閉 mapd）
cd /data/openpilot
python -c "
from openpilot.frogpilot.common.frogpilot_variables import params_memory
import time

for i in range(10):
    print(f'--- Check {i+1} ---')
    print('MapSpeedLimit:', params_memory.get('MapSpeedLimit', encoding='utf-8'))
    print('RoadName:', params_memory.get('RoadName', encoding='utf-8'))
    time.sleep(2)
"
```

**5. 測試完成後重啟 mapd 服務**：
```bash
# 停止手動執行的 mapd
pkill -f mapd

# 讓 manager 重新啟動 mapd
# 不需要額外操作，manager 會自動重啟
```

### 故障診斷

**問題：MapSpeedLimit 沒有顯示**

可能原因：
1. **mapd 執行檔未運行**
   ```bash
   ps aux | grep mapd
   # 應該看到：/data/media/0/osm/mapd
   ```

2. **執行檔不存在或損壞**
   ```bash
   ls -lh /data/media/0/osm/mapd
   # 應該約 9.3 MB

   cat /data/media/0/osm/mapd_version
   # 顯示版本號

   # 檢查執行權限
   file /data/media/0/osm/mapd
   # 應該顯示：ELF 64-bit LSB executable
   ```

3. **離線地圖數據未下載**
   ```bash
   ls -lh /data/media/0/osm/offline/
   # 應該有對應地區的目錄（如 20/, 22/, 24/）

   # 檢查台灣地圖
   find /data/media/0/osm/offline/ -name "*.000000"
   ```

4. **GPS 定位問題**
   ```bash
   cd /data/openpilot
   python -c "
   from cereal import messaging
   import time

   sm = messaging.SubMaster(['liveLocationKalman'])
   for i in range(5):
       sm.update(1000)
       if sm.updated['liveLocationKalman']:
           loc = sm['liveLocationKalman']
           lat = loc.positionGeodetic.value[0]
           lon = loc.positionGeodetic.value[1]
           valid = loc.positionGeodetic.valid
           print(f'GPS: lat={lat:.6f}, lon={lon:.6f}, valid={valid}')
       else:
           print('No GPS data')
       time.sleep(1)
   "
   ```

5. **OSM 數據中缺少速限標記**
   - RoadName (`name` 標籤) 較常見
   - MapSpeedLimit (`maxspeed` 標籤) 可能缺失
   - 取決於地區的 OSM 數據完整度

6. **PARAMS 位置錯誤**
   - ❌ 不要查看 `/data/params/d/`
   - ✅ 應該查看 `/dev/shm/params/d/`

**正確檢查 PARAMS 值**：
```bash
# 檢查 MapSpeedLimit
cat /dev/shm/params/d/MapSpeedLimit | od -A n -t f8

# 檢查 RoadName
cat /dev/shm/params/d/RoadName

# 檢查 NextMapSpeedLimit
cat /dev/shm/params/d/NextMapSpeedLimit

# 或使用 Python
cd /data/openpilot
python -c "
from openpilot.frogpilot.common.frogpilot_variables import params_memory
print('MapSpeedLimit:', params_memory.get_float('MapSpeedLimit'))
print('RoadName:', params_memory.get('RoadName', encoding='utf-8'))
"
```

### 相關檔案索引

| 檔案 | 行數 | 說明 |
|------|------|------|
| `frogpilot/navigation/mapd.py` | 1-150 | mapd 管理器 |
| `frogpilot/common/frogpilot_variables.py` | 71 | MAPD_PATH 定義 |
| `system/manager/process_config.py` | 119 | 進程配置 |
| `common/params.cc` | 396, 409, 461 | PARAMS 定義 |
| `frogpilot/controls/lib/speed_limit_controller.py` | 307-333 | 速限更新邏輯 |
| `frogpilot/system/speed_limit_filler.py` | 220, 227 | 讀取參數 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 746, 860 | UI 顯示 |

---

## 車輛速限識別系統 (Dashboard Speed Limit)

部分車輛配備 **Traffic Sign Recognition (TSR)** 功能，使用攝影機辨識道路速限標誌，並透過 CAN bus 廣播給車內其他模組使用。FrogPilot 可以從 CAN bus 讀取這些資訊作為速限來源之一。

### 系統概述

**資料來源**：
- 車輛原廠攝影機視覺辨識系統
- 透過 CAN bus 廣播（通常在 Camera bus 或主 bus）
- 即時辨識道路速限標誌

**用途**：
- 作為 Speed Limit Controller (SLC) 的速限來源之一
- 補充或驗證地圖速限資料（MapSpeedLimit）
- 提供更即時準確的速限資訊

**資料流程**：
```
車輛攝影機辨識速限標誌
    ↓
CAN bus 廣播 (Camera bus/Main bus)
    ↓
carstate.py 讀取 CAN 訊息
    ↓
calculate_speed_limit() 解析並轉換單位
    ↓
fp_ret.dashboardSpeedLimit (m/s)
    ↓
speed_limit_filler.py 記錄（用於群眾外包資料收集）
frogpilot_vcruise.py 使用（速限控制）
```

### 已實作車廠

#### 1. Toyota（豐田）

**實作位置**：`selfdrive/car/toyota/carstate.py`

**CAN 訊息**：`RSA1` (Road Sign Assist) - Camera bus (bus 2)

**實作代碼** (Lines 29-38, 223)：
```python
def calculate_speed_limit(cp_cam, frogpilot_toggles):
  speed_limit_unit = cp_cam.vl["RSA1"]["TSGN1"]      # 單位：1=km/h, 36=mph
  speed_limit_value = cp_cam.vl["RSA1"]["SPDVAL1"]  # 速限數值

  if speed_limit_unit == 1 and not frogpilot_toggles.force_mph_dashboard:
    return speed_limit_value * CV.KPH_TO_MS
  elif speed_limit_unit == 36 or frogpilot_toggles.force_mph_dashboard:
    return speed_limit_value * CV.MPH_TO_MS
  else:
    return 0

# Line 223: 寫入 FrogPilotCarState
fp_ret.dashboardSpeedLimit = calculate_speed_limit(cp_cam, frogpilot_toggles)
```

**CAN Parser 註冊** (Line 332-334)：
```python
def get_cam_can_parser(CP, FPCP):
    messages = [
      ("RSA1", 0),  # Road Sign Assist message
      ("RSA2", 0),
    ]
```

#### 2. Hyundai（現代）

**實作位置**：`selfdrive/car/hyundai/carstate.py`

**CAN 訊息**：
- **CAN FD 車款**：`CLUSTER_SPEED_LIMIT` → `SPEED_LIMIT_1`
- **傳統 CAN**：`LKAS12` → `CF_Lkas_TsrSpeed_Display_Clu`
- **備用來源**：`Navi_HU` → `SpeedLim_Nav_Clu`（導航速限）

**實作代碼** (Lines 20-38)：
```python
def calculate_speed_limit(CP, FPCP, cp, cp_cam):
  if CP.carFingerprint in CANFD_CAR:
    # CAN FD 車款
    if CP.flags & HyundaiFlags.CANFD_HDA2:
      speed_limit_bus = cp           # HDA2 從主 bus 讀取
    else:
      speed_limit_bus = cp_cam       # 其他從 camera bus 讀取
    speed_limit = speed_limit_bus.vl["CLUSTER_SPEED_LIMIT"]["SPEED_LIMIT_1"]
  else:
    # 傳統 CAN 車款
    if FPCP.fpFlags & HyundaiFrogPilotFlags.LKAS12:
      speed_limit = cp_cam.vl["LKAS12"]["CF_Lkas_TsrSpeed_Display_Clu"]
    else:
      speed_limit = 0
    # 如果沒有 TSR，嘗試從導航讀取
    if speed_limit in (0, 255) and FPCP.fpFlags & HyundaiFrogPilotFlags.NAV_MSG:
      speed_limit = cp.vl["Navi_HU"]["SpeedLim_Nav_Clu"]

  if speed_limit not in (0, 255):
    return speed_limit
  else:
    return 0

# Lines 204, 306: 寫入 FrogPilotCarState
fp_ret.dashboardSpeedLimit = calculate_speed_limit(self.CP, self.FPCP, cp, cp_cam) * speed_conv
```

**特點**：
- 支援多種速限來源（TSR、儀表板、導航）
- 區分 CAN FD 和傳統 CAN 架構
- 透過 FrogPilotFlags 控制啟用哪種來源

#### 3. Ford（福特）

**實作位置**：`selfdrive/car/ford/carstate.py`

**CAN 訊息**：`Traffic_RecognitnData` (Traffic Recognition Data) - Camera bus

**實作代碼** (Lines 14-23, 126)：
```python
def calculate_speed_limit(cp_cam):
  speed_limit_unit = cp_cam.vl["Traffic_RecognitnData"]["TsrVlUnitMsgTxt_D_Rq"]  # 單位
  speed_limit_value = cp_cam.vl["Traffic_RecognitnData"]["TsrVLim1MsgTxt_D_Rq"]  # 速限值

  if speed_limit_unit == 1:
    return speed_limit_value * CV.KPH_TO_MS
  elif speed_limit_unit == 2:
    return speed_limit_value * CV.MPH_TO_MS
  else:
    return 0

# Line 126: 只有 CAN FD 車款支援
if self.CP.flags & FordFlags.CANFD:
  fp_ret.dashboardSpeedLimit = calculate_speed_limit(cp_cam)
```

**CAN Parser 註冊** (Lines 189-192)：
```python
@staticmethod
def get_cam_can_parser(CP, FPCP):
    if CP.flags & FordFlags.CANFD:
      messages += [
        ("Traffic_RecognitnData", 1),
      ]
```

**限制**：僅支援 CAN FD 車款

### VW (Volkswagen) 車輛

**DBC 定義**：`opendbc/vw_mqb.dbc`

**CAN 訊息 ID 804 (ACC_04)** - Line 1411-1425：

| 訊號名稱 | 位置 | 長度 | 說明 |
|---------|------|------|------|
| `ACC_Tempolimit` | Bit 51 | 5 bits | 速限值（編碼） |
| `ACC_Texte_Sekundaeranz` | Bit 12 | 4 bits | 次要顯示狀態 |

**速限值編碼** (Line 1706)：
```
0  = 無顯示
1  = 5 km/h
7  = 30 km/h
11 = 50 km/h
17 = 80 km/h
21 = 100 km/h
23 = 120 km/h
31 = 速限結束
```

**狀態值** (Line 1698)：
```
7 = "Tempolimit_erkannt" (速限已識別)
4 = "Tempolimit_voraus" (前方速限)
```

**目前狀態**：❌ **未實作**

**問題分析**：
1. `selfdrive/car/volkswagen/carstate.py` 沒有 `calculate_speed_limit()` 函數
2. CAN parser 沒有訂閱 `ACC_04` 訊息
3. 沒有設定 `fp_ret.dashboardSpeedLimit`
4. VW 目前僅讀取 `ACC_02`、`ACC_06`、`ACC_10`，未讀取 `ACC_04`

**可行性**：
- ✅ DBC 已定義完整訊號
- ✅ 其他車廠有參考實作
- ✅ 訊息存在於 CAN bus（需實車驗證）
- ⚠️ 需要確認車輛是否真的廣播此訊息

### speed_limit_filler.py 功能

**檔案位置**：`frogpilot/system/speed_limit_filler.py`

**核心功能**：群眾外包速限資料收集和驗證系統

**不是用來顯示速限，而是用來改善速限資料庫**

#### 運作機制

**1. 記錄速限資料** (Lines 201-252)：

```python
def log_speed_limit(self):
    # 讀取當前位置
    current_latitude = self.sm["liveLocationKalman"].positionGeodetic.value[0]
    current_longitude = self.sm["liveLocationKalman"].positionGeodetic.value[1]

    # 獲取速限來源（優先順序排序）
    current_speed_source = self.get_speed_limit_source()
    # 來源：Navigation API > Mapbox > Dashboard

    # 讀取 mapd 提供的速限
    map_speed = params_memory.get_float("MapSpeedLimit")
    road_name = params_memory.get("RoadName", encoding="utf-8")

    # 比對：如果 mapd 速限與其他來源差距 > 1，標記為可能錯誤
    is_incorrect_limit = bool(map_speed > 0 and valid_sources and
                             all(abs(map_speed - source) > 1 for source in valid_sources))

    # 記錄到資料集
    self.dataset_additions.append({
      "bearing": ...,
      "end_coordinates": {"latitude": current_latitude, "longitude": current_longitude},
      "incorrect_limit": is_incorrect_limit,
      "road_name": road_name,
      "road_width": ...,
      "source": source,  # "NOO" / "Mapbox" / "Dashboard"
      "speed_limit": speed_limit,
      "start_coordinates": self.previous_coordinates,
    })
```

**速限來源優先順序** (Lines 84-93)：
```python
def get_speed_limit_source(self):
    sources = [
      (self.sm["frogpilotNavigation"].navigationSpeedLimit, "NOO"),     # 1. Navigation (OSM Overpass)
      (self.sm["frogpilotPlan"].slcMapboxSpeedLimit, "Mapbox"),         # 2. Mapbox API
      (self.sm["frogpilotCarState"].dashboardSpeedLimit, "Dashboard"),  # 3. 車輛 TSR
    ]
    for speed_limit, source in sources:
      if speed_limit > 0:
        return speed_limit, source
    return None
```

**2. 處理和驗證資料** (Lines 300-322)：

停車後，如果有 WiFi/Ethernet 連線：
```python
def process_speed_limits(self):
    # 查詢 Overpass API (OpenStreetMap) 驗證速限
    self.process_new_entries(dataset, filtered_dataset)

    # 比對 OSM 資料庫與記錄的速限
    # 如果 OSM 沒有 maxspeed 標籤，但我們有記錄 → 可能需要更新 OSM
    # 如果 OSM maxspeed 與記錄不符 → 標記為需要審核
```

**3. 定期審查** (Lines 339-372)：

每 7 天重新驗證：
```python
def vet_entries(self, filtered_dataset):
    for entry in dataset_list:
      last_vetted_time = datetime.fromisoformat(entry["last_vetted"])
      if datetime.now(timezone.utc) - last_vetted_time < timedelta(days=7):
        continue  # 還在有效期內

      # 重新查詢 OSM，檢查速限是否已更新
      current_maxspeed = self.cached_segments.get(entry["segment_id"])
      if current_maxspeed is None or entry.get("incorrect_limit"):
        # 保留記錄，OSM 仍未修正
        vetted_entries.append(entry)
```

#### 資料儲存

**PARAMS 儲存**：
- `SpeedLimits`：待處理的原始記錄
- `SpeedLimitsFiltered`：已驗證的速限資料
- `OverpassRequests`：API 使用量統計

**API 限制** (Lines 17-20)：
```python
MAX_ENTRIES = 1_000_000
MAX_OVERPASS_DATA_BYTES = 1_073_741_824  # 1GB/天
MAX_OVERPASS_REQUESTS = 10_000           # 10000 次/天
```

#### 使用情境

```
運行中：
    車輛行駛
    ↓
    mapd 說：這裡速限 80 km/h
    Dashboard/Mapbox/Navigation 說：這裡速限 100 km/h
    ↓
    記錄到 dataset_additions（標記為 incorrect_limit）

停車後（有網路）：
    查詢 OpenStreetMap Overpass API
    ↓
    OSM 資料庫顯示：maxspeed=100
    ↓
    確認 mapd 資料有誤
    ↓
    儲存到 SpeedLimitsFiltered
    ↓
    （未來可能）上傳到 FrogPilot 伺服器
    ↓
    改善全球 FrogPilot 使用者的速限資料品質
```

### 實作模式總結

所有車廠實作都遵循相同模式：

**步驟 1**：在 `carstate.py` 中定義 `calculate_speed_limit()` 函數
```python
def calculate_speed_limit(cp_cam):
    speed_limit_unit = cp_cam.vl["MESSAGE_NAME"]["UNIT_SIGNAL"]
    speed_limit_value = cp_cam.vl["MESSAGE_NAME"]["VALUE_SIGNAL"]

    if speed_limit_unit == UNIT_KPH:
        return speed_limit_value * CV.KPH_TO_MS
    elif speed_limit_unit == UNIT_MPH:
        return speed_limit_value * CV.MPH_TO_MS
    else:
        return 0
```

**步驟 2**：在 `update()` 函數中呼叫並寫入
```python
fp_ret.dashboardSpeedLimit = calculate_speed_limit(cp_cam)
```

**步驟 3**：在 `get_cam_can_parser()` 中註冊 CAN 訊息
```python
def get_cam_can_parser(CP, FPCP):
    messages = [
        ("MESSAGE_NAME", frequency),
    ]
    return CANParser(DBC[CP.carFingerprint]["pt"], messages, CANBUS.camera)
```

### 相關檔案索引

| 檔案 | 行數 | 說明 |
|------|------|------|
| `selfdrive/car/toyota/carstate.py` | 29-38, 223, 332-334 | Toyota TSR 實作 |
| `selfdrive/car/hyundai/carstate.py` | 20-38, 204, 306, 399-400 | Hyundai TSR 實作 |
| `selfdrive/car/ford/carstate.py` | 14-23, 126, 189-192 | Ford TSR 實作 |
| `selfdrive/car/volkswagen/carstate.py` | - | VW 未實作 |
| `opendbc/vw_mqb.dbc` | 1411-1425, 1698, 1706 | VW ACC_04 訊息定義 |
| `frogpilot/system/speed_limit_filler.py` | 1-406 | 速限資料收集系統 |
| `frogpilot/controls/lib/frogpilot_vcruise.py` | - | 使用 dashboardSpeedLimit |
| `cereal/custom.capnp` | - | FrogPilotCarState 定義 |

---

## FrogPilot UI 系統

FrogPilot 使用 Qt 5.15.2 + QML 建構使用者介面，主要的 onroad 介面由 C++ 繪製。

### UI 架構概述

**技術棧**：
- **框架**：Qt 5.15.2
- **語言**：C++ (onroad 即時繪製), QML (設定介面)
- **繪製**：QPainter（直接繪製到 OpenGL 紋理）

**主要組件**：
- **AnnotatedCameraWidget**: 基礎相機覆蓋層（openpilot）
- **FrogPilotAnnotatedCameraWidget**: FrogPilot 擴充的覆蓋層

### 主要繪製函數

**位置**: `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc`

| 函數 | 功能 | 行數 |
|------|------|------|
| `paintEvent()` | 主繪製函數 | ~150-250 |
| `paintCEMStatus()` | CEM 狀態圖標 | 329-375 |
| `paintCurveSpeedControl()` | 彎道速度控制 | 470-512 |
| `paintSmartControllerTraining()` | CSC 訓練模式 | 791-845 |
| `paintCompass()` | 指南針 | 377-467 |
| `paintRoadName()` | 道路名稱 | 745-771 |
| `paintSpeedLimitSources()` | 速限來源 | 847-920 |

---

## 彎道圖標系統

FrogPilot 有兩個獨立的彎道相關圖標系統。

### 1. 彎道速度控制圖標（Curve Speed Control）

**圖標資源**：
```cpp
// frogpilot_annotated_camera.cc:9
curveSpeedIcon = loadPixmap("../../frogpilot/assets/other_images/curve_speed.png",
                            {btn_size, btn_size});
```

**檔案資訊**：
- **路徑**: `frogpilot/assets/other_images/curve_speed.png`
- **大小**: 15KB
- **格式**: PNG (200x200, RGBA)
- **外觀**: 黃色菱形道路標誌，黑色彎道箭頭

**顯示條件** (`frogpilot_annotated_camera.cc:192-204`)：
```cpp
if (!frogpilot_scene.map_open &&
    !frogpilotPlan.getSpeedLimitChanged() &&
    !(signalStyle == "static" && carState.getLeftBlinker()) &&
    frogpilot_toggles.value("csc_status").toBool()) {

    if (frogpilotPlan.getCscTraining()) {
        paintSmartControllerTraining(p, frogpilotPlan, frogpilot_scene);
    } else if (isCruiseSet && frogpilotPlan.getCscControllingSpeed()) {
        paintCurveSpeedControl(p, frogpilotPlan, frogpilot_scene);
    }
}
```

**必要條件**：
1. ✅ 地圖未開啟
2. ✅ 速限未變化
3. ✅ `csc_status` 設定開啟
4. ✅ 定速已設定
5. ✅ CSC 正在控制速度

**速度計算** (`frogpilot_annotated_camera.cc:144`):
```cpp
cscSpeedStr = QString::number(
    std::nearbyint(fmin(speed, frogpilotPlan.getCscSpeed() * speedConversion))
) + speedUnit;
```

**視覺樣式**：
- **圖標框**: 黑色半透明背景，藍色邊框（10px）
- **速度框**: 藍色半透明背景，100px 高
- **文字**: 白色，45pt 粗體，顯示目標速度
- **方向**: 根據 `getRoadCurvature()` 自動翻轉（左轉/右轉）

### 2. CEM 彎道偵測圖標

**圖標資源**：
```cpp
// frogpilot_annotated_camera.cc:20
loadGif("../../frogpilot/assets/other_images/curve_icon.gif",
        cemCurveIcon, QSize(btn_size / 2, btn_size / 2), this);
```

**檔案資訊**：
- **路徑**: `frogpilot/assets/other_images/curve_icon.gif`
- **大小**: 486KB
- **格式**: GIF 動畫
- **外觀**: 藍色/白色彎道弧線符號

**顯示條件** (`frogpilot_annotated_camera.cc:178-179, 364-365`)：
```cpp
if (frogpilot_toggles.value("cem_status").toBool()) {
    paintCEMStatus(...);
}

// 在 paintCEMStatus 內部
if (frogpilot_scene.conditional_status == 8) {
    icon = cemCurveIcon;  // 偵測到彎道
}
```

**CEStatus 圖標對照**：

| conditional_status | 圖標 | 說明 |
|-------------------|------|------|
| 0 | 無 | 未啟用 |
| 1 | chillModeIcon | 手動暫停 |
| 2 | experimentalModeIcon | 手動啟用 |
| 3, 4 | cemSpeedIcon | 低速觸發 |
| 5, 7 | cemTurnIcon | 轉彎/方向燈 |
| 6, 11, 12 | cemStopIcon | 紅綠燈/停止 |
| **8** | **cemCurveIcon** | **偵測到彎道** |
| 9, 10 | cemLeadIcon | 前車緩慢 |
| 13 | experimentalModeIcon | SLC 實驗模式 |

---

## UI 位置計算系統

### 主要錨點

FrogPilot UI 使用多個錨點來定位元素：

```cpp
QPoint dmIconPosition;              // 駕駛監控圖標位置
QPoint experimentalButtonPosition;  // 實驗模式按鈕位置
QPoint cemStatusPosition;           // CEM 狀態位置
QPoint compassPosition;             // 指南針位置
QPoint lateralPausedPosition;       // 橫向暫停圖標位置
QPoint longitudinalIconPosition;    // 縱向圖標位置
```

### CEStatus 位置計算邏輯

**計算公式** (`frogpilot_annotated_camera.cc:336-338`):
```cpp
cemStatusPosition.rx() = dmIconPosition.x();
cemStatusPosition.ry() = dmIconPosition.y() - widget_size / 2;
cemStatusPosition.rx() += (rightHandDM ? -img_size - widget_size : widget_size) /
                          (frogpilot_scene.map_open ? 1.25 : 1);
```

**邏輯說明**：
1. **X 座標**：
   - 基準：DM 圖標的 X 座標
   - 左駕：向右偏移 `widget_size`
   - 右駕：向左偏移 `img_size + widget_size`
   - 地圖開啟時：偏移量除以 1.25（更靠近 DM 圖標）

2. **Y 座標**：
   - 基準：DM 圖標的 Y 座標
   - 向上偏移：`widget_size / 2`

**常數定義** (`frogpilot_annotated_camera.h:9`):
```cpp
const int widget_size = img_size + (UI_BORDER_SIZE / 2);
```

---

## 訓練模式 vs 控制模式

### Smart Controller Training（訓練模式）

**觸發條件**：`getCscTraining() == true`

**視覺特徵**：
- **發光效果**：藍色邊框脈動（2 秒週期）
- **文字顯示**：`"Training..."`
- **文字框高度**：50px（較矮）

**發光動畫** (`paintSmartControllerTraining`):
```cpp
qreal phase = (glowTimer.elapsed() % 2000) / 2000.0 * 2 * M_PI;
qreal alphaFactor = 0.5 + 0.5 * sin(phase);

QColor glowColor = blueColor();
glowColor.setAlphaF(0.3 + 0.7 * alphaFactor);

int glowWidth = 8 + static_cast<int>(2 * alphaFactor);
p.setPen(QPen(glowColor, glowWidth));
```

### Curve Speed Control（控制模式）

**觸發條件**：`getCscControllingSpeed() == true`

**視覺特徵**：
- **固定邊框**：藍色，10px 寬
- **文字顯示**：目標速度（例如：`"80 km/h"`）
- **文字框高度**：100px（較高）

---

## 實作案例：移動彎道圖標

**需求**：將彎道速度控制圖標從最高時速右邊移到 CEStatus 位置

**原因**：原位置在螢幕頂部，容易被後照鏡遮擋

**修改檔案**：
- `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.h` (Line 70, 77)
- `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` (Line 470-512, 791-845, 199)

**修改前位置**（最高時速右邊）：
```cpp
QRect curveSpeedRect(
    QPoint(setSpeedRect.right() + UI_BORDER_SIZE, setSpeedRect.top()),
    QSize(defaultSize.width() * 1.25, defaultSize.width() * 1.25)
);
```

**修改後位置**（CEStatus 位置）：
```cpp
QPoint curveSpeedPosition;
curveSpeedPosition.rx() = dmIconPosition.x();
curveSpeedPosition.ry() = dmIconPosition.y() - widget_size / 2;
curveSpeedPosition.rx() += (rightHandDM ? -img_size - widget_size : widget_size) /
                          (frogpilot_scene.map_open ? 1.25 : 1);

QRect curveSpeedRect(curveSpeedPosition, QSize(widget_size, widget_size));
```

**效果**：
- ✅ 圖標移至 DM 圖標旁邊
- ✅ 不會被後照鏡遮擋
- ✅ CEM 關閉時可用該位置
- ✅ 保持所有原有功能（方向翻轉、速度顯示等）

---

## 相關檔案索引

| 檔案 | 行數 | 說明 |
|------|------|------|
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.h` | 67-81 | 繪製函數宣告 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.h` | 104, 119 | 圖標資源定義 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 8-26 | 圖標載入 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 144 | 彎道速度計算 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 178-204 | 主繪製邏輯 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 329-375 | CEStatus 繪製 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 470-512 | 彎道速度控制繪製 |
| `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc` | 791-845 | 訓練模式繪製 |
| `frogpilot/assets/other_images/curve_speed.png` | - | 彎道速度圖標（PNG）|
| `frogpilot/assets/other_images/curve_icon.gif` | - | CEM 彎道圖標（GIF）|

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
| 2025-11-01 | v2.5 | **✅ 實作**：油門啟動控制的安全初始速度（修改 `drive_helpers.py:160-164`）- 使用 max(當前速度, 40 kph) 作為初始巡航速度,配合油門恢復功能確保安全 |
| 2025-11-01 | v2.4 | **新增**：為什麼啟動後必須先按 SET 鍵才能啟用 OP 控制（v_cruise_initialized 機制、resumeBlocked 邏輯、3種情境流程圖、設計理念、跳過方法研究） |
| 2025-11-01 | v2.3 | **✅ 實作**：踩油門恢復控制功能（修改 `events.py:721` 新增 `ET.ENABLE` 到 `gasPressedOverride`）- 靜止煞車和 CANCEL 後可用油門恢復控制 |
| 2025-11-01 | v2.2 | **新增**：常見疑問解答（ENABLE/OVERRIDE 為何不衝突、NO_ENTRY 事件持續性分析、CANCEL 按住行為）；**優化**：詳細的狀態機邏輯說明、事件生命週期分析 |
| 2025-11-01 | v2.1 | **新增**：踩油門恢復控制功能完整實作方案（3種方案對比、推薦方案、實作步驟、測試場景、風險評估、進階選項） |
| 2025-11-01 | v2.0 | **新增**：控制恢復機制詳解（煞車/CANCEL 後能否用油門恢復）、事件類型與狀態機完整分析、設計理念說明；**更新**：煞車與油門差異表格新增 CANCEL 按鈕欄位 |
| 2025-10-23 | v1.9 | 實作：煞車自動回復功能修正版（依車速判斷：> 0.5 m/s 自動回復，≤ 0.5 m/s 需重新啟動），包含完整邏輯說明、測試情境、設計決策 |
| 2025-10-23 | v1.8 | 新增：mapd 執行檔與地圖資料詳細說明、params_memory 位置說明、手動測試 mapd 流程、完整故障診斷 |
| 2025-10-23 | v1.7 | 新增：車輛速限識別系統 (Dashboard Speed Limit)，包含 Toyota/Hyundai/Ford 實作、VW 未實作分析、speed_limit_filler.py 完整功能說明 |
| 2025-10-23 | v1.6 | 新增：FrogPilot UI 系統（彎道圖標、位置計算、實作案例）；實作：移動彎道速度控制圖標到 CEStatus 位置 |
| 2025-10-23 | v1.5 | 新增：FrogPilot 導航與地圖系統（mapd 架構、MapSpeedLimit/RoadName PARAMS）、CEM 啟動機制與關閉行為分析 |
| 2025-10-22 | v1.4 | 新增：控制系統詳細研究（煞車踏板修改、安全層邏輯、狀態機流程）|
| 2025-10-22 | v1.2 | 實作：時間持久化功能（`common/params.cc`, `system/timed.py`）|
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
