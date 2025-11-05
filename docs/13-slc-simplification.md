# SLC 簡化實作文檔

**日期**: 2025-11-03
**狀態**: ✅ 實作完成
**修改檔案**: 5 個

---

## 需求說明

### 原始需求
簡化 Speed Limit Controller (SLC) 功能:
1. ✅ **只在速限變化時設定 MAX SPEED** - 一次性設定,不持續控制
2. ✅ **只使用 Map Data** - 移除 Dashboard/Navigation 速限來源
3. ✅ **60 km/h 雙向差距檢查** - 差距過大時忽略變更
4. ✅ **自動設定** - 無需按鈕確認
5. ✅ **UI 簡化** - 主速限顯示 Map Data,下方只顯示 Upcoming
6. ✅ **Map Data 速限修正** - 四捨五入到最接近的 10 的倍數

### 核心差異 vs 原設計

| 項目 | 原設計 | 簡化後 |
|-----|--------|--------|
| **控制方式** | 持續用 min() 限制 v_cruise | **只在變化時設定一次 MAX SPEED** |
| **速限來源** | Dashboard/Map/Nav 三選一 | **只用 Map Data** |
| **確認機制** | 需要按鈕確認 (accel/decel) | **自動設定** |
| **加速處理** | 不允許 | **允許 (差距 <= 60)** |
| **差距檢查** | 無 | **60 km/h 雙向檢查** |
| **UI 顯示** | 四個來源 + 選擇框 | **只顯示 Upcoming 標誌** |
| **速限數值** | 原始值 (88.5 km/h) | **四捨五入到 10 的倍數 (90 km/h)** |

---

## 實作修改

### 修改 1: speed_limit_controller.py - Map Data 速限修正

**檔案**: `frogpilot/controls/lib/speed_limit_controller.py`
**位置**: Line 311-317
**修改**: 6 行

**修改前**:
```python
self.map_speed_limit = params_memory.get_float("MapSpeedLimit")

next_map_speed_limit = json.loads(params_memory.get("NextMapSpeedLimit") or "{}")
self.next_speed_limit = next_map_speed_limit.get("speedlimit", 0)
```

**修改後**:
```python
# Round to nearest 10 (fix mile-to-km conversion issues)
raw_speed_limit = params_memory.get_float("MapSpeedLimit")
self.map_speed_limit = round(raw_speed_limit * CV.MS_TO_KPH / 10) * 10 * CV.KPH_TO_MS if raw_speed_limit > 0 else 0

next_map_speed_limit = json.loads(params_memory.get("NextMapSpeedLimit") or "{}")
raw_next_speed = next_map_speed_limit.get("speedlimit", 0)
self.next_speed_limit = round(raw_next_speed * CV.MS_TO_KPH / 10) * 10 * CV.KPH_TO_MS if raw_next_speed > 0 else 0
```

**處理邏輯**:
```python
# 範例: mapd 提供 55 mph → 88.514 km/h (24.587 m/s)
# 1. 轉換 m/s → km/h: 24.587 * 3.6 = 88.514
# 2. 除以 10: 88.514 / 10 = 8.8514
# 3. 四捨五入: round(8.8514) = 9
# 4. 乘以 10: 9 * 10 = 90 km/h
# 5. 轉回 m/s: 90 / 3.6 = 25.0 m/s
```

**轉換範例**:

| mapd 原始 (mph) | 原始 m/s | 原始 km/h | **修正後 km/h** | 修正後 m/s |
|----------------|---------|----------|--------------|-----------|
| 35 mph | 15.646 | 56.3 | **60** | 16.667 |
| 45 mph | 20.117 | 72.4 | **70** | 19.444 |
| 55 mph | 24.587 | 88.5 | **90** | 25.000 |
| 65 mph | 29.058 | 104.6 | **100** | 27.778 |
| 75 mph | 33.528 | 120.7 | **120** | 33.333 |

**原因**:
- mapd 提供的 OSM 資料可能是英里轉公里的原始值
- 實際道路速限都是 10 的倍數
- 在來源處修正,所有使用處自動正確

---

### 修改 2: controlsd.py - 自動設定 MAX SPEED

**檔案**: `selfdrive/controls/controlsd.py`
**位置**: Line 529-554 (在原 Line 527 後新增)
**新增**: 26 行

```python
# SLC: Auto set MAX SPEED when speed limit changes
if (self.frogpilot_toggles.speed_limit_controller and
    self.enabled and
    self.sm['frogpilotPlan'].speedLimitChanged):

  map_speed_limit = self.sm['frogpilotPlan'].slcMapSpeedLimit
  slc_offset = self.sm['frogpilotPlan'].slcSpeedLimitOffset

  if map_speed_limit > 0:  # Valid speed limit detected
    new_speed_ms = map_speed_limit + slc_offset  # m/s
    new_speed_kph = new_speed_ms * CV.MS_TO_KPH  # Convert to km/h
    current_speed_kph = self.v_cruise_helper.v_cruise_kph

    # Bidirectional 60 km/h difference check
    diff = abs(new_speed_kph - current_speed_kph)

    if diff <= 60:
      # Auto set MAX SPEED
      self.v_cruise_helper.v_cruise_kph = new_speed_kph
      cloudlog.info(f"SLC: Auto set MAX SPEED to {new_speed_kph:.1f} km/h "
                   f"(MapData={map_speed_limit*CV.MS_TO_KPH:.1f}, "
                   f"offset={slc_offset*CV.MS_TO_KPH:.1f}, diff={diff:.1f})")
    else:
      cloudlog.warning(f"SLC: Ignored speed limit "
                      f"(new={new_speed_kph:.1f}, current={current_speed_kph:.1f}, "
                      f"diff={diff:.1f} > 60)")
```

**核心邏輯**:
- **觸發條件**: `speedLimitChanged` 事件 (速限變化 >= 1 m/s)
- **速限來源**: `slcMapSpeedLimit` (Map Data)
- **差距檢查**: `abs(new - current) <= 60` (雙向)
- **直接設定**: `v_cruise_helper.v_cruise_kph` (MAX SPEED)
- **日誌輸出**: 成功/忽略都有詳細記錄

---

### 修改 3: annotated_camera.cc - UI 顯示 Map Data

**檔案**: `selfdrive/ui/qt/onroad/annotated_camera.cc`
**位置**: Line 76-80
**修改**: 5 行

**修改前**:
```cpp
speedLimit = frogpilotPlan.getSlcOverriddenSpeed() != 0 ?
             frogpilotPlan.getSlcOverriddenSpeed() :
             frogpilotPlan.getSlcSpeedLimit();  // ❌ 使用選擇後的速限
if (frogpilotPlan.getSlcOverriddenSpeed() == 0 && !frogpilot_toggles.value("show_speed_limit_offset").toBool()) {
  speedLimit += frogpilotPlan.getSlcSpeedLimitOffset();
}
```

**修改後**:
```cpp
// Use Map Data directly (not selected source)
speedLimit = frogpilotPlan.getSlcMapSpeedLimit();  // ✅ 直接用 Map Data
if (speedLimit > 0 && !frogpilot_toggles.value("show_speed_limit_offset").toBool()) {
  speedLimit += frogpilotPlan.getSlcSpeedLimitOffset();
}
```

**修復問題**:
- ❌ **之前**: 使用 `slcSpeedLimit` (選擇後的速限,可能來自 Dashboard/Navigation)
- ✅ **現在**: 使用 `slcMapSpeedLimit` (Map Data 原始速限)
- ✅ **移除**: Override 邏輯

---

### 修改 4: frogpilot_annotated_camera.cc - Upcoming 速限標誌

**檔案**: `frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc`
**位置**: Line 842-878
**修改**: 完全重寫 (刪除 40 行文字標籤,新增 37 行速限標誌)

**修改前** (文字標籤樣式):
```cpp
drawSource(dashboardRect, dashboardIcon, "Dashboard", ...);    // 刪除
drawSource(mapDataRect, mapDataIcon, "Map Data", ...);         // 刪除
drawSource(navigationRect, navigationIcon, "Navigation", ...); // 刪除
drawSource(nextLimitRect, nextMapsIcon, "Upcoming", ...);      // 改為速限標誌
```

**修改後** (速限標誌樣式):
```cpp
// Draw upcoming speed limit sign (same style as main speed limit sign, different color)
float upcomingSpeedLimit = frogpilotPlan.getSlcNextSpeedLimit() * speedConversion;

if (upcomingSpeedLimit > 0) {
  const int upcoming_sign_size = viennaSpeedLimit ? 140 : 150;
  QRect upcomingRect(speedLimitRect.x() + (speedLimitRect.width() - upcoming_sign_size) / 2,
                     speedLimitRect.y() + speedLimitRect.height() + UI_BORDER_SIZE,
                     upcoming_sign_size, upcoming_sign_size);

  QString upcomingStr = QString::number(std::nearbyint(upcomingSpeedLimit));

  if (viennaSpeedLimit) {
    // EU (Vienna style) - Circle with blue border
    p.setPen(Qt::NoPen);
    p.setBrush(whiteColor());
    p.drawEllipse(upcomingRect);
    p.setPen(QPen(QColor(0x00, 0x7A, 0xFF), 16));  // Blue border
    p.drawEllipse(upcomingRect.adjusted(12, 12, -12, -12));

    p.setPen(blackColor());
    p.setFont(InterFont((upcomingStr.size() >= 3) ? 50 : 60, QFont::Bold));
    p.drawText(upcomingRect, Qt::AlignCenter, upcomingStr);
  } else {
    // US (MUTCD style) - Rectangle with blue border
    p.setPen(Qt::NoPen);
    p.setBrush(whiteColor());
    p.drawRoundedRect(upcomingRect, 20, 20);
    p.setPen(QPen(QColor(0x00, 0x7A, 0xFF), 5));  // Blue border
    p.drawRoundedRect(upcomingRect.adjusted(7, 7, -7, -7), 14, 14);

    p.setPen(blackColor());
    p.setFont(InterFont(24, QFont::DemiBold));
    p.drawText(upcomingRect.adjusted(0, 18, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("NEXT"));
    p.setFont(InterFont(60, QFont::Bold));
    p.drawText(upcomingRect.adjusted(0, 45, 0, 0), Qt::AlignTop | Qt::AlignHCenter, upcomingStr);
  }
}
```

**視覺改進**:
- ✅ **相同樣式**: 跟主速限標誌一樣 (圓形 EU / 方形 US)
- ✅ **藍色邊框**: 用顏色區分 (主速限: 紅色 EU / 黑色 US)
- ✅ **無文字標籤**: 不顯示 "Upcoming:",只顯示速限數字
- ✅ **US 加 "NEXT"**: 方形標誌上方有小字 "NEXT"
- ✅ **位置居中**: 主速限標誌正下方,水平居中
- ✅ **只在有值時顯示**: `if (upcomingSpeedLimit > 0)`

---

### 修改 5: frogpilot_vcruise.py - 移除 SLC 控制

**檔案**: `frogpilot/controls/lib/frogpilot_vcruise.py`
**位置**: Line 100-103
**刪除**: 3 行

**修改前**:
```python
targets = [self.braking_target, self.csc_target, v_cruise]
if frogpilot_toggles.speed_limit_controller:
    targets.append(max(self.slc.overridden_speed, self.slc_target + self.slc_offset) - v_ego_diff)
    # ↑ 這行會持續用 min() 限制 v_cruise

v_cruise = min([target if target > CRUISING_SPEED else v_cruise for target in targets])
```

**修改後**:
```python
# SLC no longer controls v_cruise (MAX SPEED is set directly in controlsd.py)
targets = [self.braking_target, self.csc_target, v_cruise]

v_cruise = min([target if target > CRUISING_SPEED else v_cruise for target in targets])
```

**原因**:
- SLC 不再持續控制 `v_cruise` (臨時巡航速度)
- 改為在 controlsd.py 直接設定 `v_cruise_kph` (MAX SPEED)
- 避免持續 `min()` 限制的設計

---

## 測試場景

### 場景 0: Map Data 速限修正
```
前置: mapd 提供 55 mph 速限 (88.5 km/h)
處理:
  - 原始值: 24.587 m/s
  - 轉換: 24.587 * 3.6 = 88.514 km/h
  - 四捨五入: round(88.514 / 10) * 10 = 90 km/h
  - 轉回: 90 / 3.6 = 25.0 m/s
結果: ✅ 所有地方顯示 90 km/h (不是 88.5 km/h)
```

### 場景 1: 正常減速 (120 → 90)
```
前置: MAX SPEED = 120 km/h
動作: 進入 80 km/h 速限區 (offset = +10)
計算: new = 90, diff = |90-120| = 30 <= 60
結果: ✅ MAX SPEED 自動變為 90 km/h
Log:  "SLC: Auto set MAX SPEED to 90.0 km/h (MapData=80.0, offset=10.0, diff=30.0)"
```

### 場景 2: 差距過大忽略 (120 → 50)
```
前置: MAX SPEED = 120 km/h
動作: 進入 40 km/h 速限區 (offset = +10)
計算: new = 50, diff = |50-120| = 70 > 60
結果: ⚠️ 保持 120 km/h (忽略變更)
Log:  "SLC: Ignored speed limit (new=50.0, current=120.0, diff=70.0 > 60)"
```

### 場景 3: 允許加速 (80 → 130)
```
前置: MAX SPEED = 80 km/h
動作: 進入 120 km/h 速限區 (offset = +10)
計算: new = 130, diff = |130-80| = 50 <= 60
結果: ✅ MAX SPEED 自動變為 130 km/h (允許加速!)
Log:  "SLC: Auto set MAX SPEED to 130.0 km/h (MapData=120.0, offset=10.0, diff=50.0)"
```

### 場景 4: 拒絕極端加速 (50 → 130)
```
前置: MAX SPEED = 50 km/h
動作: 進入 120 km/h 速限區 (offset = +10)
計算: new = 130, diff = |130-50| = 80 > 60
結果: ⚠️ 保持 50 km/h (忽略變更)
Log:  "SLC: Ignored speed limit (new=130.0, current=50.0, diff=80.0 > 60)"
```

### 場景 5: UI 顯示
```
主速限標誌:
  - 顯示 Map Data 速限 (80 或 90,取決於 show_speed_limit_offset 設定)
  - 紅色圓框 (EU) 或 黑色方框 (US)

Upcoming 標誌:
  - 主速限標誌正下方
  - 藍色圓框 (EU) 或 藍色方框 + "NEXT" (US)
  - 只顯示速限數字 (例如: "110")
  - 只在有 upcoming 速限時顯示
```

---

## 技術細節

### Map Data 速限修正邏輯

**位置**: `frogpilot/controls/lib/speed_limit_controller.py:307-317`

```python
def update_map_speed_limit(self, gps_position, v_ego):
    # Round to nearest 10 (fix mile-to-km conversion issues)
    raw_speed_limit = params_memory.get_float("MapSpeedLimit")
    self.map_speed_limit = round(raw_speed_limit * CV.MS_TO_KPH / 10) * 10 * CV.KPH_TO_MS if raw_speed_limit > 0 else 0

    next_map_speed_limit = json.loads(params_memory.get("NextMapSpeedLimit") or "{}")
    raw_next_speed = next_map_speed_limit.get("speedlimit", 0)
    self.next_speed_limit = round(raw_next_speed * CV.MS_TO_KPH / 10) * 10 * CV.KPH_TO_MS if raw_next_speed > 0 else 0
```

**影響範圍**:
- ✅ `self.map_speed_limit` - 所有 SLC 邏輯使用
- ✅ `self.next_speed_limit` - Upcoming 顯示
- ✅ 自動應用到 controlsd.py 和 UI

### speedLimitChanged 事件

**來源**: `frogpilot/controls/lib/speed_limit_controller.py:299-304`

```python
# 速限變化檢測
if abs(desired_target - self.previous_target) >= 1:  # 差距 >= 1 m/s
    self.handle_limit_change(desired_source, desired_target, sm)
else:
    self.speed_limit_changed_timer = 0

# 事件發布 (frogpilot_planner.py:169)
frogpilotPlan.speedLimitChanged = self.frogpilot_vcruise.slc.speed_limit_changed_timer > DT_MDL
```

**觸發條件**: 當速限變化 >= 1 m/s 且持續超過 DT_MDL 時間

### cereal 訊息欄位

**使用欄位** (`cereal/custom.capnp`):
```capnp
slcMapSpeedLimit @19 :Float32;        # Line 203 - Map Data 速限 (m/s)
slcNextSpeedLimit @21 :Float32;       # Line 205 - Upcoming 速限 (m/s)
slcSpeedLimitOffset @24 :Float32;     # Line 208 - 速限偏移 (m/s)
speedLimitChanged @28 :Bool;          # Line 212 - 速限變化事件
```

### v_cruise_helper.v_cruise_kph

**位置**: `selfdrive/controls/lib/drive_helpers.py`
**類別**: `VCruiseHelper`
**變數**: `self.v_cruise_kph` (float, km/h)

這是駕駛手動設定的 MAX SPEED,顯示在 UI 的 "MAX XXX" 框中。

### Map Data 來源

**服務**: `mapd` (Go binary)
**位置**: `third_party/mapd/` (需要編譯)
**資料**: 離線 OpenStreetMap (OSM) 資料
**參數**:
- `MapSpeedLimit` - 當前道路速限
- `NextMapSpeedLimit` - 下一個速限 (含距離/座標)

---

## cloudlog 輸出範例

### 成功設定
```
INFO: SLC: Auto set MAX SPEED to 90.0 km/h (MapData=80.0, offset=10.0, diff=30.0)
```

### 忽略變更
```
WARNING: SLC: Ignored speed limit (new=50.0, current=120.0, diff=70.0 > 60)
```

### 無速限
```
(無輸出,保持當前 MAX SPEED)
```

---

## 修改總結

| 檔案 | 修改類型 | 行數變化 |
|-----|---------|---------|
| frogpilot/controls/lib/speed_limit_controller.py | 修改 | ~6 (速限四捨五入) |
| selfdrive/controls/controlsd.py | 新增 | +26 |
| selfdrive/ui/qt/onroad/annotated_camera.cc | 修改 | ~5 |
| frogpilot/ui/qt/onroad/frogpilot_annotated_camera.cc | 重寫 | -40 +37 |
| frogpilot/controls/lib/frogpilot_vcruise.py | 刪除 | -3 |

**總計**: 5 個檔案,淨增加約 25 行

---

## 後續維護

### 相關檔案
- `frogpilot/controls/lib/speed_limit_controller.py` - SLC 核心邏輯 (未修改,保留變化檢測)
- `frogpilot/controls/frogpilot_planner.py` - 發布 cereal 訊息 (未修改)
- `docs/08-speed-limit-nav.md` - 速限系統研究文檔

### 未來可選清理
1. 簡化 `speed_limit_controller.py` - 移除確認機制相關代碼
2. 移除 Dashboard/Navigation 速限來源的處理代碼
3. 簡化參數設定 UI (只保留 Map Data offset 設定)

---

**文檔版本**: 1.0
**最後更新**: 2025-11-03
