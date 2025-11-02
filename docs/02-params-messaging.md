# 參數系統與訊息傳遞

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

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

