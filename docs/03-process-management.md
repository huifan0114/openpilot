# 進程管理與時間系統

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

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

