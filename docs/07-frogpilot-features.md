# FrogPilot 模式與功能

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

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

