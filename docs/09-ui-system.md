# UI 系統

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

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

