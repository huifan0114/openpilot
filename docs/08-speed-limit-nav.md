# 速限、導航與地圖系統

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

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


---

## SLC 速限控制邏輯深度解析

**版本**: v1.0  
**日期**: 2025-11-02  
**研究重點**: SLC 如何調整 v_cruise,以及與用戶設定的 MAX SPEED 關係

---

### 核心問題

1. **SLC 偵測到速限後,會不會改變用戶設定的 MAX SPEED?**
2. **用戶設定 80,進入 120 速限區,車子會不會自動加速?**
3. **用戶設定 120,進入 80 速限區,車子會怎麼做?**

---

### 核心發現: SLC 只降速,不加速

**關鍵程式碼**: `frogpilot/controls/lib/frogpilot_vcruise.py:104`

```python
v_cruise = min([target if target > CRUISING_SPEED else v_cruise for target in targets])
```

**targets 包含**:
- `v_cruise`: 用戶設定的 MAX SPEED (來自 VCruiseHelper)
- `self.slc_target + self.slc_offset`: SLC 速限目標
- `self.braking_target`: 煞車目標
- `self.csc_target`: 彎道速度目標

**min() 的意義**:
- 永遠選擇**最小值**
- SLC 只會**限制**速度,不會**提高**速度

---

### 資料流程詳解

```
[用戶設定 MAX SPEED]
  用戶按 +/- 鍵
  ↓
  VCruiseHelper.v_cruise_kph = 120  ← 保存在 VCruiseHelper
  ↓
  發布: controlsState.vCruise = 120

[SLC 讀取並限制]
  FrogPilotVCruise 讀取: v_cruise = 120
  ↓
  SLC 偵測速限: slc_target = 80
  ↓
  targets = [120 (用戶), 80 (速限), ...]
  ↓
  v_cruise = min(120, 80, ...) = 80  ← 臨時降低,不改變 MAX SPEED
  ↓
  發布: frogpilotPlan.vCruise = 80

[MPC 執行]
  使用 80 km/h 巡航
```

---

### 情境分析

#### 情境 1: 用戶設定 120,進入 80 速限區

```
用戶 MAX SPEED = 120 km/h
  ↓
SLC 偵測速限 = 80 km/h
  ↓
targets = [120, 80]
  ↓
v_cruise = min(120, 80) = 80
  ↓
結果: 車子巡航 80 km/h

VCruiseHelper.v_cruise_kph 還是 120 (沒變!)
```

**重點**:
- ✅ SLC 臨時降低巡航速度到 80
- ✅ 用戶的 MAX SPEED 120 **完全沒變**
- ✅ 離開速限區後,自動恢復到 120

---

#### 情境 2: 用戶設定 80,進入 120 速限區

```
用戶 MAX SPEED = 80 km/h
  ↓
SLC 偵測速限 = 120 km/h
  ↓
targets = [80, 120]
  ↓
v_cruise = min(80, 120) = 80
  ↓
結果: 車子還是巡航 80 km/h (不變!)

VCruiseHelper.v_cruise_kph 還是 80
```

**重點**:
- ✅ SLC **不會**自動加速
- ✅ 保持用戶設定的 80 km/h
- ✅ 尊重駕駛的速度選擇

**如何加速到 120?**
- 必須**手動按 + 鍵**
- VCruiseHelper 更新 MAX SPEED: 80 → 90 → 100 → 110 → 120
- 然後 SLC: `min(120, 120) = 120`

---

### 完整情境對比表

| 用戶 MAX SPEED | 速限 | targets | min() 結果 | 實際巡航 | MAX SPEED 變化 | 說明 |
|---------------|------|---------|-----------|---------|--------------|------|
| 120 km/h | 80 | [120, 80] | 80 | 80 km/h | 保持 120 | ✅ 自動降速避免超速 |
| 80 km/h | 120 | [80, 120] | 80 | 80 km/h | 保持 80 | ✅ 不自動加速 |
| 100 km/h | 100 | [100, 100] | 100 | 100 km/h | 保持 100 | ✅ 剛好相同 |
| 60 km/h | 80 | [60, 80] | 60 | 60 km/h | 保持 60 | ✅ 尊重駕駛想開慢 |
| 120 km/h | 無 | [120] | 120 | 120 km/h | 保持 120 | ✅ 無速限區 |

---

### 設計理念

#### 為什麼 SLC 只降速不加速?

這是**安全優先**的設計:

1. **自動降速 OK** ✅
   - 防止超速罰單
   - 提高安全性
   - 符合法規要求

2. **自動加速 NO** ❌
   - 可能路況不好 (濕滑、能見度差)
   - 可能天氣不好 (下雨、起霧)
   - 可能駕駛想省油
   - 可能駕駛不熟悉路段
   - **必須由駕駛決定**

#### 類比: 智慧限速器

```
SLC 就像智慧限速器:
- 會幫你"踩煞車" (降速) ✅
- 不會幫你"踩油門" (加速) ❌
- 加速永遠是駕駛的決定!
```

---

### 程式碼分析

#### VCruiseHelper: 保存 MAX SPEED

**位置**: `selfdrive/controls/lib/drive_helpers.py:136`

```python
self.v_cruise_kph = clip(round(self.v_cruise_kph, 1), V_CRUISE_MIN, V_CRUISE_MAX)
```

**何時會改變**:
- ✅ 用戶按 +/- 鍵
- ✅ 用戶按 SET/RESUME 鍵
- ❌ SLC **不會改變** 這個值

#### FrogPilotVCruise: 臨時調整

**位置**: `frogpilot/controls/lib/frogpilot_vcruise.py:100-106`

```python
targets = [self.braking_target, self.csc_target, v_cruise]
if frogpilot_toggles.speed_limit_controller:
    targets.append(max(self.slc.overridden_speed, self.slc_target + self.slc_offset) - v_ego_diff)

v_cruise = min([target if target > CRUISING_SPEED else v_cruise for target in targets])

return v_cruise
```

**特點**:
- 讀取用戶的 `v_cruise` (MAX SPEED)
- 加入 SLC 目標 `self.slc_target`
- 使用 `min()` 選擇最小值
- **回傳調整後的值,不修改原始 MAX SPEED**

---

### 速限解除後的恢復

#### 自動恢復流程

```
在 80 速限區巡航 80
  ↓
離開速限區
  ↓
SLC: slc_target = 無速限
  ↓
targets = [120 (MAX SPEED), 無限制]
  ↓
v_cruise = min(120, 無限制) = 120
  ↓
自動恢復到 MAX SPEED 120 km/h
```

**注意**:
- ✅ 完全自動,不需要手動操作
- ✅ 平滑恢復,不會突然加速
- ✅ MAX SPEED 從來沒變過

---

### 與其他控制器的交互

SLC 與其他速度控制器共同作用:

```python
targets = [
    self.braking_target,      # 煞車目標 (急煞車時)
    self.csc_target,          # 彎道速度控制
    v_cruise,                 # 用戶 MAX SPEED
    slc_target + slc_offset   # SLC 速限目標
]

v_cruise = min(targets)  # 選最小值,最保守
```

**優先順序** (由 min() 決定):
1. 最保守的控制器獲勝
2. 例如: 彎道 60 < 速限 80 < MAX 120 → 使用 60
3. 安全優先原則

---

### VW Dashboard 無訊號修正

**問題**: VW 車輛 `dashboardSpeedLimit = 0`,導致 SLC 無法使用 Map Data

**解決**: 修改 `speed_limit_controller.py:245`

```python
# 修改前
limits = {
    "Dashboard": dashboard_speed_limit,  # VW 車輛 = 0
    "Map Data": self.map_speed_limit,
    "Navigation": navigation_speed_limit
}

# 修改後
limits = {
    "Dashboard": self.map_speed_limit,   # 直接使用 Map Data 覆蓋
    "Map Data": self.map_speed_limit,
    "Navigation": navigation_speed_limit
}
```

**結果**:
- ✅ Dashboard 和 Map Data 現在永遠相同
- ✅ SLC 優先級系統正常運作
- ✅ VW 車輛可以正常使用 SLC

---

### 總結

#### 關鍵要點

1. **MAX SPEED 不會被 SLC 改變**
   - 保存在 VCruiseHelper
   - 只有用戶按鍵能改變

2. **SLC 只臨時調整巡航速度**
   - 使用 `min()` 限制速度
   - 不會修改 MAX SPEED

3. **只降速不加速**
   - 120 進 80 區 → 自動降到 80
   - 80 進 120 區 → 保持 80 (不自動加速)
   - 加速必須手動按 +

4. **自動恢復**
   - 離開速限區自動恢復 MAX SPEED
   - 完全自動,平滑過渡

#### 設計哲學

- **安全第一**: 降速可自動,加速需手動
- **尊重駕駛**: 不會強制改變駕駛的速度選擇
- **智慧限制**: 多個控制器取最小值,最保守

---

**相關檔案**:
- `frogpilot/controls/lib/frogpilot_vcruise.py:100-106` - min() 邏輯
- `selfdrive/controls/lib/drive_helpers.py:110-297` - VCruiseHelper
- `frogpilot/controls/lib/speed_limit_controller.py` - SLC 實作

