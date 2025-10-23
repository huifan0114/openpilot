# FrogPilot / openpilot 開發指南

> **文件版本**: v1.8
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

### 當前煞車與油門的核心差異

| 比較項目 | 油門 | 煞車 |
|---------|------|------|
| **硬體層** | | |
| `controls_allowed` 影響 | 可能設為 `false`（如果沒旗標） | 保持 `true`（FrogPilot 已改） |
| `get_longitudinal_allowed()` | 檢查 `!gas_pressed_prev` ✅ | **不檢查** `!brake_pressed_prev` ❌ |
| 縱向指令阻擋 | ✅ 被阻擋 | ❌ 不被阻擋 |
| **軟體層** | | |
| 事件類型 | `OVERRIDE_LONGITUDINAL` | `USER_DISABLE` |
| 狀態機 | `overriding` | `disabled` |
| 恢復方式 | ✅ 自動 | ❌ 需手動 |

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

2. **開機時恢復時間** (`system/timed.py:92-102`):
```python
# 如果系統時間無效，嘗試從 PARAMS 恢復
if not system_time_valid():
    last_good_timestamp_str = params.get("LastKnownGoodTime", encoding='utf8')
    if last_good_timestamp_str is not None:
        try:
            last_good_timestamp = float(last_good_timestamp_str)
            last_good_time = datetime.datetime.fromtimestamp(last_good_timestamp)
            cloudlog.info(f"Restoring system time from LastKnownGoodTime: {last_good_time}")
            set_time(last_good_time)
        except (ValueError, TypeError) as e:
            cloudlog.error(f"Failed to parse LastKnownGoodTime: {e}")
```

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
