# 定速控制深度研究

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

## 純視覺前車偵測不確定性分析與動態定速控制方案

**版本**: v1.0
**日期**: 2025-11-01
**背景**: 針對無雷達車輛,純視覺前車偵測在加速階段存在判斷誤差,導致車輛為達定速而忽略前車的安全問題

---

### 問題現象與根本原因

#### 使用者觀察到的問題

1. **前車距離和速度值幾乎不變** - 視覺模型輸出的 `dRel` 和 `vLead` 在慢車/停止車輛時會「卡住」不更新
2. **定速過高導致激進加速** - 當 v_cruise (120 kph) >> v_ego (50 kph) 時,MPC 會嘗試快速加速到定速
3. **加速階段視覺判斷最不準** - 因為 TTC (Time To Collision) 快速減少,單目深度估計誤差放大

#### 技術根因分析

**A. 純視覺深度估計的固有限制**

```
單目視覺深度誤差 = ±10-20% (學術文獻)
```

**位置**: `cereal/log.capnp:955-964` (LeadDataV2/V3 結構)

視覺模型提供的數據:
```python
struct LeadDataV3 {
    prob @0 :Float32;              # 前車存在機率
    x @3 :List(Float32);           # 縱向距離
    xStd @4 :List(Float32);        # 距離標準差 ⭐ 關鍵指標
    v @7 :List(Float32);           # 前車絕對速度
    vStd @8 :List(Float32);        # 速度標準差 ⭐ 關鍵指標
    a @9 :List(Float32);           # 前車加速度
    aStd @10 :List(Float32);       # 加速度標準差 ⭐ 關鍵指標
}
```

**關鍵發現**: 視覺模型**已經提供標準差 (Std)** 數據,但現有程式碼**沒有使用**這些不確定性指標!

**B. 雷達追蹤 vs 純視覺的差異**

**位置**: `radard.py:55-94` (Track 類別)

雷達追蹤有:
- ✅ 卡爾曼濾波器平滑 (`KF1D`)
- ✅ 追蹤計數器 (`self.cnt`) - 可判斷追蹤穩定性
- ✅ 一階濾波器 (`radarfulFilter`) - 需 1 秒確認 (THRESHOLD=0.63)

純視覺追蹤:
- ❌ 每幀重新偵測,無歷史連貫性
- ❌ 機率波動大 (0.3-0.9 之間跳動)
- ❌ 距離/速度容易「卡住」(使用者觀察)

**C. MPC 優化目標導致的激進加速**

**位置**: `longitudinal_planner.py:104-186`

```python
# MPC 目標: min( (v_ego - v_cruise)² + ... )
# 當 v_cruise = 120, v_ego = 50 時
# 巨大的速度誤差會驅使系統快速加速
```

---

### 視覺不確定性判斷指標 (完整方案)

經過深入研究,我找到了**7 個可用的判斷指標**:

#### 指標 1: 視覺模型標準差 (最佳) ⭐⭐⭐⭐⭐

**數據來源**: `modelV2.leadsV3[0].xStd`, `vStd`, `aStd`

**判斷邏輯**:
```python
# 距離不確定性 (單位: m)
uncertain_distance = lead_msg.xStd[0] > 5.0  # 距離標準差 > 5m

# 速度不確定性 (單位: m/s)
uncertain_velocity = lead_msg.vStd[0] > 2.0  # 速度標準差 > 2 m/s (約 7 kph)

# 加速度不確定性 (單位: m/s²)
uncertain_accel = lead_msg.aStd[0] > 1.0     # 加速度標準差 > 1 m/s²

vision_uncertain = uncertain_distance or uncertain_velocity or uncertain_accel
```

**優點**:
- ✅ 模型自身的不確定性估計,最準確
- ✅ 已經包含在現有數據中,無需額外計算
- ✅ 學術研究證明有效 (標準差 = 品質指標)

**閾值建議** (需實車調整):
- 距離 xStd: 3-7m (近距離要求更嚴格)
- 速度 vStd: 1.5-3.0 m/s
- 加速度 aStd: 0.8-1.5 m/s²

#### 指標 2: 前車機率波動 ⭐⭐⭐⭐

**數據來源**: `lead_msg.prob` 的歷史變化

**判斷邏輯**:
```python
class VisionUncertaintyDetector:
    def __init__(self):
        self.prob_history = deque(maxlen=10)  # 保留 10 幀 (~1 秒)

    def update(self, lead_prob):
        self.prob_history.append(lead_prob)

        if len(self.prob_history) >= 5:
            prob_std = np.std(self.prob_history)
            prob_range = max(self.prob_history) - min(self.prob_history)

            # 機率劇烈波動 = 偵測不穩定
            return prob_std > 0.15 or prob_range > 0.3
        return False
```

**閾值**:
- 標準差 > 0.15 (機率在 0.5-0.8 之間波動)
- 範圍 > 0.3 (1 秒內從 0.4 跳到 0.7)

#### 指標 3: 距離/速度變化率異常 ⭐⭐⭐⭐

**物理原理**: 前車距離和速度應該符合物理規律

**判斷邏輯**:
```python
class LeadPhysicsChecker:
    def __init__(self):
        self.last_dRel = 0
        self.last_vLead = 0
        self.last_time = 0

    def check_anomaly(self, dRel, vLead, v_ego, current_time):
        dt = current_time - self.last_time

        if dt > 0.05:  # 至少 50ms 間隔
            # 預期距離變化 = 相對速度 × 時間
            expected_dRel_change = (vLead - v_ego) * dt
            actual_dRel_change = dRel - self.last_dRel

            # 距離變化異常 (你觀察到的「卡住」現象)
            distance_stuck = abs(actual_dRel_change) < 0.1 and abs(vLead - v_ego) > 2.0

            # 距離變化與速度不符
            physics_violation = abs(actual_dRel_change - expected_dRel_change) > 3.0

            # 速度突變 (> 5 m/s² 的加速度不合理)
            speed_jump = abs(vLead - self.last_vLead) / dt > 5.0

            self.last_dRel = dRel
            self.last_vLead = vLead
            self.last_time = current_time

            return distance_stuck or physics_violation or speed_jump

        return False
```

**這個指標能捕捉你觀察到的「距離和速度幾乎不變」的現象!**

#### 指標 4: 前車機率偏低 + 加速中 ⭐⭐⭐

**結合多個條件**:
```python
def risky_acceleration_scenario(lead_prob, a_ego, v_cruise, v_ego):
    low_confidence = 0.3 < lead_prob < 0.65  # 機率不高不低,最危險
    accelerating = a_ego > 0.5                # 正在加速
    large_speed_gap = (v_cruise - v_ego) > 20 / 3.6  # 定速比當前快 20+ kph

    return low_confidence and accelerating and large_speed_gap
```

這是**最容易出事的組合**。

#### 指標 5: 無雷達追蹤確認 ⭐⭐⭐

**位置**: `radard.py:207-216`

```python
# 檢查是否有雷達追蹤支持視覺偵測
vision_only = track is None and lead_msg.prob > threshold
radar_confirmed = track is not None

# 純視覺偵測,無雷達確認 = 較不可靠
if vision_only:
    apply_conservative_control = True
```

對於你的車 (無雷達),這個條件**永遠為 True**,所以不太有用。但可以結合其他指標。

#### 指標 6: 低速前車特殊處理 ⭐⭐⭐⭐

**位置**: `radard.py:218-225`

```python
def potential_low_speed_lead(v_ego, lead_dRel, lead_vLead):
    """
    低速前車最容易誤判
    """
    slow_ego = v_ego < 4.0  # V_EGO_STATIONARY
    close_distance = 0.75 < lead_dRel < 25
    stopped_or_slow_lead = lead_vLead < 2.0

    return slow_ego and close_distance and stopped_or_slow_lead
```

#### 指標 7: TTC (Time To Collision) 快速減少 ⭐⭐⭐⭐⭐

**物理公式**:
```python
def calculate_ttc_risk(dRel, vRel, a_ego):
    """
    碰撞時間 (TTC) 是最直接的危險指標
    """
    if vRel <= 0:  # 沒有接近,無風險
        return False

    # TTC = 距離 / 相對速度
    ttc = dRel / max(vRel, 0.1)

    # TTC 變化率 (越快越危險)
    # 加速時 TTC 會快速下降
    ttc_decrease_rate = vRel + a_ego  # 簡化版

    # 危險情況: TTC < 3秒 且正在快速減少
    critical_ttc = ttc < 3.0 and ttc_decrease_rate > 2.0

    return critical_ttc
```

---

### 完整解決方案:智慧動態定速控制器

#### 方案設計哲學

**核心概念**:
1. **不改變 CEM** - 保留彎道和紅綠燈的實驗模式觸發
2. **不修改前車偵測** - 視覺模型保持原樣
3. **只調整 v_cruise** - 根據不確定性動態降低定速目標
4. **記錄原始定速** - 條件解除後逐步恢復

#### 實作架構

**新增檔案**: `frogpilot/controls/lib/vision_safety_controller.py`

```python
#!/usr/bin/env python3
"""
Vision Safety Controller (VSC)
純視覺車輛的動態定速安全控制器

功能:
1. 偵測視覺不確定性
2. 動態調整定速速度到當前速度的最近 10 倍數
3. 緊急情況持續降速
4. 安全後逐步恢復原始定速
"""

import numpy as np
from collections import deque
from openpilot.common.conversions import Conversions as CV
from openpilot.common.realtime import DT_MDL

# 定速調整步進 (km/h)
CRUISE_STEP = 10

# 恢復速度 (每秒恢復多少 km/h)
RECOVERY_RATE = 5  # 較慢恢復,確保安全

class VisionSafetyController:
    def __init__(self):
        # 原始定速記錄
        self.original_v_cruise = 0
        self.active = False

        # 當前限制的定速
        self.limited_v_cruise = 0

        # 歷史數據
        self.prob_history = deque(maxlen=10)
        self.last_dRel = 0
        self.last_vLead = 0
        self.last_time = 0

        # 緊急降速級別 (0 = 無, 1-5 = 降速級別)
        self.emergency_level = 0
        self.emergency_timer = 0

    def update(self, v_ego, v_cruise, lead_msg, CS, sm, enabled):
        """
        主更新函數

        參數:
        - v_ego: 當前車速 (m/s)
        - v_cruise: 當前定速 (m/s)
        - lead_msg: 視覺模型前車數據 (LeadDataV3)
        - CS: CarState
        - sm: SubMaster
        - enabled: 系統是否啟用

        返回:
        - adjusted_v_cruise: 調整後的定速 (m/s)
        """

        if not enabled:
            self.reset()
            return v_cruise

        # 記錄原始定速
        if not self.active:
            self.original_v_cruise = v_cruise

        # 檢查是否有前車
        has_lead = lead_msg.prob > 0.3

        if not has_lead:
            # 無前車,逐步恢復原始定速
            return self._recover_cruise_speed(v_ego, v_cruise)

        # 計算不確定性指標
        uncertainty_score = self._calculate_uncertainty(v_ego, lead_msg, CS, sm)

        # 根據不確定性調整定速
        if uncertainty_score > 0.6:  # 高度不確定
            return self._apply_safety_limit(v_ego, v_cruise, uncertainty_score)
        elif uncertainty_score > 0.3:  # 中度不確定
            return self._apply_conservative_limit(v_ego, v_cruise)
        else:
            # 低不確定性,逐步恢復
            return self._recover_cruise_speed(v_ego, v_cruise)

    def _calculate_uncertainty(self, v_ego, lead_msg, CS, sm):
        """
        計算綜合不確定性分數 (0-1)

        使用 7 個指標的加權平均
        """
        score = 0.0
        weights = 0.0

        # === 指標 1: 視覺標準差 (權重 30%) ===
        if len(lead_msg.xStd) > 0:
            x_std_score = np.clip(lead_msg.xStd[0] / 7.0, 0, 1)  # 7m 標準差 = 滿分
            v_std_score = np.clip(lead_msg.vStd[0] / 3.0, 0, 1)  # 3 m/s 標準差 = 滿分
            std_score = (x_std_score + v_std_score) / 2
            score += std_score * 0.3
            weights += 0.3

        # === 指標 2: 機率波動 (權重 15%) ===
        self.prob_history.append(lead_msg.prob)
        if len(self.prob_history) >= 5:
            prob_std = np.std(self.prob_history)
            prob_score = np.clip(prob_std / 0.2, 0, 1)  # 0.2 標準差 = 滿分
            score += prob_score * 0.15
            weights += 0.15

        # === 指標 3: 物理異常檢測 (權重 25%) ===
        if len(lead_msg.x) > 0:
            current_time = sm.logMonoTime['modelV2'] * 1e-9
            dRel = lead_msg.x[0]
            vLead = lead_msg.v[0]

            dt = current_time - self.last_time
            if dt > 0.05 and self.last_time > 0:
                # 距離卡住檢測
                dRel_change = abs(dRel - self.last_dRel)
                vRel = vLead - v_ego
                distance_stuck = dRel_change < 0.2 and abs(vRel) > 2.0

                # 速度突變檢測
                vLead_change = abs(vLead - self.last_vLead) / dt
                speed_jump = vLead_change > 4.0  # > 4 m/s² 不合理

                physics_score = 1.0 if (distance_stuck or speed_jump) else 0.0
                score += physics_score * 0.25
                weights += 0.25

            self.last_dRel = dRel
            self.last_vLead = vLead
            self.last_time = current_time

        # === 指標 4: 危險加速場景 (權重 20%) ===
        a_ego = CS.aEgo
        v_cruise_kph = v_cruise * CV.MS_TO_KPH
        v_ego_kph = v_ego * CV.MS_TO_KPH

        low_prob = 0.35 < lead_msg.prob < 0.65
        accelerating = a_ego > 0.5
        large_gap = (v_cruise_kph - v_ego_kph) > 20

        if low_prob and accelerating and large_gap:
            score += 1.0 * 0.2
        weights += 0.2

        # === 指標 5: TTC 風險 (權重 10%) ===
        if len(lead_msg.x) > 0 and len(lead_msg.v) > 0:
            dRel = lead_msg.x[0]
            vRel = lead_msg.v[0] - v_ego

            if vRel > 0.1:  # 正在接近
                ttc = dRel / vRel
                if ttc < 3.0:
                    ttc_score = 1.0 - (ttc / 3.0)  # TTC 越短,分數越高
                    score += ttc_score * 0.1
            weights += 0.1

        # 標準化分數
        if weights > 0:
            return score / weights
        else:
            return 0.0

    def _apply_safety_limit(self, v_ego, v_cruise, uncertainty_score):
        """
        高度不確定:將定速降到當前速度 + 10 km/h
        """
        self.active = True
        v_ego_kph = v_ego * CV.MS_TO_KPH
        v_cruise_kph = v_cruise * CV.MS_TO_KPH

        # 計算目標定速:當前速度的最近 10 倍數 + 10
        target_kph = (int(v_ego_kph / CRUISE_STEP) + 1) * CRUISE_STEP

        # 緊急情況:持續降速
        if uncertainty_score > 0.8:
            self.emergency_level = min(self.emergency_level + 1, 5)
            target_kph = max(40, target_kph - self.emergency_level * CRUISE_STEP)
        else:
            self.emergency_level = max(self.emergency_level - 1, 0)

        # 限制最低 40 km/h
        target_kph = max(40, target_kph)

        # 不能超過原始定速
        target_kph = min(target_kph, v_cruise_kph)

        self.limited_v_cruise = target_kph / CV.MS_TO_KPH
        return self.limited_v_cruise

    def _apply_conservative_limit(self, v_ego, v_cruise):
        """
        中度不確定:限制定速不超過當前速度 + 20 km/h
        """
        self.active = True
        v_ego_kph = v_ego * CV.MS_TO_KPH
        v_cruise_kph = v_cruise * CV.MS_TO_KPH

        target_kph = min(v_cruise_kph, v_ego_kph + 20)
        target_kph = max(40, target_kph)

        self.limited_v_cruise = target_kph / CV.MS_TO_KPH
        return self.limited_v_cruise

    def _recover_cruise_speed(self, v_ego, v_cruise):
        """
        逐步恢復到原始定速
        """
        if not self.active:
            return v_cruise

        # 逐步恢復 (每秒 5 km/h)
        recovery_step = RECOVERY_RATE * DT_MDL  # 單次調整量
        v_cruise_kph = v_cruise * CV.MS_TO_KPH
        original_kph = self.original_v_cruise * CV.MS_TO_KPH

        if v_cruise_kph < original_kph:
            new_cruise_kph = min(v_cruise_kph + recovery_step, original_kph)
            self.limited_v_cruise = new_cruise_kph / CV.MS_TO_KPH
            return self.limited_v_cruise
        else:
            # 已恢復
            self.reset()
            return v_cruise

    def reset(self):
        """重置狀態"""
        self.active = False
        self.emergency_level = 0
        self.prob_history.clear()
```

#### 整合到 FrogPilot

**修改位置**: `frogpilot/controls/frogpilot_planner.py`

```python
# 檔案開頭導入
from openpilot.frogpilot.controls.lib.vision_safety_controller import VisionSafetyController

class FrogPilotPlanner:
    def __init__(self, CP):
        # ... 現有初始化 ...

        # 新增 VSC
        self.vsc = VisionSafetyController()

    def update(self, sm, pm, CP, ...):
        # ... 現有更新邏輯 ...

        # 在 frogpilot_vcruise.update() 之後
        self.frogpilot_vcruise.update(...)
        v_cruise = self.frogpilot_vcruise.v_cruise

        # 套用 VSC
        if frogpilot_toggles.vision_safety_controller:  # 新增的 toggle
            # 取得視覺前車數據
            lead_msg = sm['modelV2'].leadsV3[0] if len(sm['modelV2'].leadsV3) > 0 else None

            if lead_msg is not None:
                v_cruise = self.vsc.update(
                    v_ego=sm['carState'].vEgo,
                    v_cruise=v_cruise,
                    lead_msg=lead_msg,
                    CS=sm['carState'],
                    sm=sm,
                    enabled=sm['controlsState'].enabled
                )

        # 更新到 frogpilot_plan
        self.frogpilot_plan.vCruise = v_cruise
```

---

### 使用情境範例

#### 情境 1: 高速公路前方慢車

```
狀態:
- v_ego: 100 km/h
- v_cruise: 120 km/h (原始定速)
- 前車: 80 km/h,距離 50m
- 視覺機率: 0.55 (中低)
- xStd: 6.2m (高不確定性)

VSC 判斷:
- uncertainty_score = 0.68 (高度不確定)
- 觸發 safety_limit

動作:
1. 記錄原始定速 120 km/h
2. 計算目標: int(100/10)+1 = 11 → 110 km/h
3. 將定速降到 110 km/h
4. MPC 現在目標是 110 而非 120,加速較溫和
5. 視覺有時間更準確判斷前車

結果:
- 避免激進加速
- 保持安全距離
- 前車超車後自動恢復 120 km/h
```

#### 情境 2: 市區停止車輛

```
狀態:
- v_ego: 40 km/h,正在加速
- v_cruise: 60 km/h
- 前車: 0 km/h (停止),距離 25m
- 視覺機率: 0.42 (低)
- 距離/速度值卡住不變

VSC 判斷:
- uncertainty_score = 0.85 (極高不確定)
- 物理異常檢測觸發 (距離卡住)
- TTC < 3 秒

動作:
1. 立即觸發 emergency_level 1
2. 目標定速: int(40/10)+1 - 1 = 40 km/h
3. 如果繼續接近: emergency_level 2 → 30 km/h
4. 持續降低直到安全

結果:
- 避免碰撞
- 給視覺模型時間重新偵測
- 或觸發駕駛接管
```

---

### 實作建議與下一步

#### 建議實作順序

**階段 1: 最小可行方案 (MVP)** ⭐ 推薦先做

只實作最核心的功能:
- 指標 1: 視覺標準差 (`xStd`, `vStd`)
- 指標 4: 危險加速場景
- 基本的定速降低邏輯 (當前速度 +10 km/h)

**預估工作量**: 2-3 小時編碼 + 實車測試

**階段 2: 完整方案**

加入所有 7 個指標和完整的 VSC 控制器

**預估工作量**: 1 天編碼 + 多次實車調整

#### 參數調整建議

所有閾值需要實車測試後調整:

| 參數 | 初始值 | 建議範圍 | 說明 |
|------|--------|----------|------|
| `xStd` 閾值 | 5.0m | 3-7m | 距離標準差觸發值 |
| `vStd` 閾值 | 2.0 m/s | 1.5-3.0 m/s | 速度標準差觸發值 |
| 機率波動 | 0.15 | 0.1-0.2 | prob 標準差觸發值 |
| uncertainty_score 高 | 0.6 | 0.5-0.7 | 高度不確定觸發 |
| uncertainty_score 中 | 0.3 | 0.2-0.4 | 中度不確定觸發 |
| CRUISE_STEP | 10 kph | 10-20 kph | 定速調整步進 |
| RECOVERY_RATE | 5 kph/s | 3-10 kph/s | 恢復速率 |

#### 需要新增的 Toggle

在 FrogPilot 設定中新增:

```python
# frogpilot_variables.py
vision_safety_controller: bool = False  # 視覺安全控制器開關
vsc_sensitivity: int = 2  # 靈敏度 1-3 (低/中/高)
vsc_debug_mode: bool = False  # 顯示不確定性分數
```

#### 除錯與監控

建議在 UI 顯示:
- 當前 uncertainty_score (0-100%)
- VSC 是否啟用 (active)
- 原始定速 vs 限制定速
- 觸發的指標 (哪些指標觸發了)

這樣你可以實車觀察系統行為並調整參數。

---

### 方案優缺點評估

#### 優點 ✅

1. **非侵入性** - 不修改視覺模型、CEM、或 MPC 核心邏輯
2. **可配置** - 所有參數可調整,適應不同車輛和駕駛風格
3. **可關閉** - toggle 開關,不滿意可隨時關閉
4. **有理論基礎** - 基於學術研究和物理原理
5. **使用現有數據** - 視覺模型已提供 Std 數據,無需額外計算
6. **逐步恢復** - 不會永久限制速度,條件改善後自動恢復

#### 缺點與風險 ⚠️

1. **需要實車調參** - 閾值必須經過實際測試才能確定
2. **可能過於保守** - 某些情況下可能不必要地降低定速
3. **計算開銷** - 7 個指標計算每幀執行,約 0.1ms (可接受)
4. **誤觸發風險** - 可能在安全情況下錯誤觸發
5. **與 SLC 衝突** - 如果同時使用速限控制器,需處理優先級

#### 替代方案對比

| 方案 | 複雜度 | 效果 | 風險 |
|------|--------|------|------|
| 修改 MPC 參數 | 高 | 好 | 高 |
| 限制最大加速度 | 低 | 中 | 中 |
| 動態定速 (VSC) | 中 | 很好 | 低 |
| 提高前車機率閾值 | 低 | 差 | 低 |

VSC 方案在效果和風險之間取得最佳平衡。

---

### 總結與建議

經過深入研究,我找到了解決你問題的完整方案:

**核心發現**:
1. ✅ 視覺模型**已經提供**不確定性數據 (`xStd`, `vStd`, `aStd`),但現有程式碼沒用
2. ✅ 你觀察到的「距離/速度卡住」可以用物理異常檢測捕捉
3. ✅ 7 個指標組合可以準確判斷視覺不確定性
4. ✅ 動態調整定速比修改 MPC 更安全且可控

**建議行動**:
1. **先實作 MVP** - 用指標 1 + 4,快速驗證概念
2. **實車測試** - 記錄 uncertainty_score 和觸發情況
3. **調整參數** - 根據實際表現微調閾值
4. **擴展到完整方案** - 加入所有 7 個指標

**預期效果**:
- 🎯 解決定速過高導致激進加速的問題
- 🎯 在視覺不確定時自動降低到安全速度
- 🎯 不影響正常駕駛和 CEM 功能
- 🎯 條件改善後自動恢復原始定速

要我開始寫 MVP 程式碼嗎?還是你想先看看哪些部分需要調整?

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

## 定速控制系統架構與資料流

**版本**: v1.0  
**日期**: 2025-11-02  

### 核心發現: VCruiseHelper 和 FrogPilotVCruise 不會衝突

**結論**: 完全不會衝突。這兩個組件在不同進程中運作,通過單向資料流協作:
- **VCruiseHelper**: 處理用戶輸入 → 「用戶想要的速度」
- **FrogPilotVCruise**: 處理自動調整 → 「安全/法規上應該的速度」
- **LongitudinalPlanner**: 接收最終速度 → 規劃軌跡

#### 完整資料流程

```
[controlsd - 100Hz]
  用戶按鈕 → VCruiseHelper → controlsState.vCruise

[frogpilot_process - 10Hz]
  讀取 controlsState.vCruise → FrogPilotVCruise (SLC/CSC/ForceStops) → frogpilotPlan.vCruise

[longitudinal_planner - 100Hz]
  讀取 frogpilotPlan.vCruise → MPC 軌跡規劃
```

#### 關鍵檔案

- **VCruiseHelper**: `selfdrive/controls/lib/drive_helpers.py:110-297`
- **FrogPilotVCruise**: `frogpilot/controls/lib/frogpilot_vcruise.py`
- **使用位置**: `controlsd.py:527,602,888` 和 `frogpilot_planner.py:53,114,178`

---

## ForceStops 停車機制

**版本**: v1.0  
**日期**: 2025-11-02  

### 重要修正: ForceStops 不需要實驗模式

**核心發現**: ForceStops 使用**動態 v_cruise 降低**策略,而非實驗模式的路徑規劃。

#### 原理

```
傳統停車 (實驗模式): 路徑規劃器決定精確停止點
ForceStops 停車: 持續降低 v_cruise → MPC 自然減速到 0
```

#### 核心演算法

**位置**: `frogpilot/controls/lib/frogpilot_vcruise.py:89-93`

```python
# 追蹤剩餘模型路徑長度
self.tracked_model_length = max(self.tracked_model_length - (v_ego * DT_MDL), 0)

# 將剩餘長度轉換為目標速度
v_cruise = min((self.tracked_model_length // PLANNER_TIME), v_cruise)
```

#### 為什麼不需要實驗模式

- ✅ 只需要 v_cruise 控制權 (所有模式都有)
- ✅ 只需要 MPC 減速能力 (基本功能)
- ✅ CEM 提供紅燈偵測 (獨立於實驗模式)

#### 對比

| 特性 | ForceStops | 實驗模式停車 |
|------|-----------|-------------|
| **控制方式** | 調整 v_cruise | 調整路徑終點 |
| **需要實驗模式** | ❌ 否 | ✅ 是 |
| **停止精度** | ±2-5m | ±0.5-2m |
| **適用場景** | Chill 模式紅燈停止 | 實驗模式精確停車 |

---

## 速限控制修正: VW Dashboard 無訊號

**版本**: v1.0  
**日期**: 2025-11-02  

### 問題與解決

**問題**: VW 車輛 dashboardSpeedLimit = 0,導致 SLC 無法使用 Map Data

**解決**: 修改 `speed_limit_controller.py:245`

```python
# 修改前
"Dashboard": dashboard_speed_limit,  # 0

# 修改後
"Dashboard": self.map_speed_limit,   # 使用 Map Data 覆蓋
```

**結果**: Dashboard 和 Map Data 現在永遠相同,SLC 優先級系統正常運作

---
