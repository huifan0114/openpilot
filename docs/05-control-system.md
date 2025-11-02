# 控制系統詳解

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

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

---

