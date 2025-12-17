# Simple Dash 主題開發指南

## 目錄結構

```
tools/simple_dash/
├── server.py
├── shared/                    # 共用資源（所有主題共用）
│   ├── js/
│   │   ├── dashboard.js
│   │   ├── main.js
│   │   ├── webrtc.js
│   │   └── ...
│   ├── manifest.json
│   ├── icon-192.png
│   └── icon-512.png
└── themes/
    ├── HFOP-GTI-NAVDASH/     # 主題 1
    ├── SIMPLE-DASH/           # 主題 2
    └── [新主題名稱]/          # 你的新主題
```

---

## 新增主題步驟

### 步驟 1：建立主題目錄

```bash
mkdir -p tools/simple_dash/themes/[新主題名稱]/css
mkdir -p tools/simple_dash/themes/[新主題名稱]/assets
```

### 步驟 2：複製範本

```bash
# 複製現有主題作為範本
cp tools/simple_dash/themes/SIMPLE-DASH/index.html tools/simple_dash/themes/[新主題名稱]/
cp tools/simple_dash/themes/SIMPLE-DASH/css/style.css tools/simple_dash/themes/[新主題名稱]/css/
```

### 步驟 3：準備素材

將主題專用的圖片放入 `assets/` 目錄：
- 背景圖 (如 `bg.png`)
- 圖示 (如 `img_experimental_white.svg`)
- 其他素材

### 步驟 4：確認 index.html 路徑

**必須使用以下路徑格式：**

| 類型 | 路徑格式 | 說明 |
|------|----------|------|
| CSS | `/theme/css/style.css` | 主題專用 |
| 主題圖片 | `/theme/assets/xxx.png` | 主題專用 |
| JS | `/shared/js/xxx.js` | 共用 |
| manifest | `/shared/manifest.json` | 共用 |
| icon | `/shared/icon-192.png` | 共用 |

**index.html 範例：**

```html
<!DOCTYPE html>
<html lang="zh-TW">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>FrogPilot Dashboard</title>

  <!-- 主題專用 CSS -->
  <link rel="stylesheet" href="/theme/css/style.css">

  <!-- 共用資源 -->
  <link rel="manifest" href="/shared/manifest.json">
  <link rel="apple-touch-icon" href="/shared/icon-192.png">
</head>
<body>
  <!-- 主題專用圖片 -->
  <img src="/theme/assets/my_image.png">

  <!-- 共用 JS -->
  <script src="/shared/js/webrtc.js" type="module"></script>
  <script src="/shared/js/dashboard.js" type="module"></script>
  <script src="/shared/js/main.js" type="module"></script>
</body>
</html>
```

### 步驟 5：修改 style.css

如果有背景圖，使用 `/theme/assets/` 路徑：

```css
body {
  background: #000 url('/theme/assets/my_bg.png') no-repeat center center;
  background-size: cover;
}
```

### 步驟 6：更新 C++ 設定

**檔案：** `frogpilot/ui/qt/offroad/visual_settings.cc`

**位置：** 約第 339 行

```cpp
// 在 themeList 加入新主題名稱
QStringList themeList{"HFOP-GTI-NAVDASH", "SIMPLE-DASH", "[新主題名稱]"};
```

### 步驟 7：重新編譯

```bash
scons -j$(nproc)
```

---

## 最終檔案結構

```
themes/[新主題名稱]/
├── index.html          # 主頁面（路徑必須正確）
├── css/
│   └── style.css       # 樣式
└── assets/
    ├── bg.png          # 背景圖（選用）
    └── ...             # 其他素材
```

---

## 路徑對應表

| 請求路徑 | 實際檔案位置 |
|----------|--------------|
| `/` | `themes/{當前主題}/index.html` |
| `/theme/css/*` | `themes/{當前主題}/css/*` |
| `/theme/assets/*` | `themes/{當前主題}/assets/*` |
| `/shared/js/*` | `shared/js/*` |
| `/shared/*` | `shared/*` |

---

## 設定位置

FrogPilot UI：**Settings → Visuals → Quality of Life → Simple Dash Theme**

---

## API

| 端點 | 方法 | 說明 |
|------|------|------|
| `/api/themes` | GET | 取得可用主題列表 |
| `/api/theme` | POST | 設定主題 `{"theme": "主題名稱"}` |

---

## 常見問題

| 問題 | 原因 | 解決方法 |
|------|------|----------|
| CSS 沒載入 | 用了 `/static/` 舊路徑 | 改成 `/theme/css/style.css` |
| 圖片沒顯示 | 路徑錯誤 | 改成 `/theme/assets/xxx.png` |
| JS 錯誤 | 路徑錯誤 | 改成 `/shared/js/xxx.js` |
| 設定沒出現 | 沒更新 themeList | 修改 `visual_settings.cc` |
| 重開機設定消失 | params 沒註冊 | 已在 `params.cc` 註冊 `SimpleDashTheme` |

---

## 相關檔案

| 檔案 | 說明 |
|------|------|
| `tools/simple_dash/server.py` | 多主題路由伺服器 |
| `common/params.cc` | 參數定義 (`SimpleDashTheme`) |
| `frogpilot/ui/qt/offroad/visual_settings.cc` | UI 主題選擇器 |
| `frogpilot/ui/qt/offroad/visual_settings.h` | UI 設定定義 |
