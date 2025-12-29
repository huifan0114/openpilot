# FrogPilot Navigation

FrogPilot 導航系統，支援多種導航提供商和速限資料來源。

## 🗺️ 支援的導航提供商

### 1. **Mapbox** (預設)
- 全球路線規劃與導航
- 即時交通資訊
- 速限查詢
- 每月 100,000 次免費請求

### 2. **Google Maps** ⭐ 新功能
- 全球路線規劃與導航
- 即時交通資訊 (Google 交通數據)
- 速限查詢 (Roads API)
- 詳細的轉向指引
- 每月約 $200 免費額度

📖 **詳細設定指南**: [GOOGLE_MAPS_GUIDE.md](GOOGLE_MAPS_GUIDE.md)

### 3. **Map Data (離線地圖)**
- 基於 OpenStreetMap (OSM)
- 離線速限查詢
- 不需要網路連線
- 無使用限制

## 🚀 快速開始

### 設定 Google Maps

```bash
# 1. 設定 API Key
/data/params/d/GoogleMapsKey "AIza..."

# 2. 切換到 Google Maps
/data/params/d/NavigationProvider "google"
```

### 使用 Python API

```python
from openpilot.frogpilot.navigation.navigation_config import NavigationConfig

config = NavigationConfig()

# 切換提供商
config.set_provider("google")  # 或 "mapbox"

# 檢查可用提供商
print(config.get_available_providers())

# 設定 API Key
config.set_api_key("google", "YOUR_API_KEY")
```

## 📊 功能對比

| 功能 | Mapbox | Google Maps | Map Data |
|------|--------|-------------|----------|
| 路線規劃 | ✅ | ✅ | ❌ |
| 即時交通 | ✅ | ✅ | ❌ |
| 速限查詢 | ✅ | ✅ | ✅ |
| 離線使用 | ❌ | ❌ | ✅ |
| 路口偵測 | ✅ | ✅ | ❌ |
| 轉向指引 | ✅ | ✅ | ❌ |
| 多語言 | ✅ | ✅ | 部分 |
| 免費額度 | 100k/月 | $200/月 | 無限制 |

## 🔧 檔案說明

### 主要檔案

- **`mapd.py`** - 離線地圖管理服務
  - 自動下載與更新 OSM 地圖資料
  - 提供離線速限查詢

- **`navigation_config.py`** - 導航配置管理
  - 統一管理多個導航提供商
  - API Key 配置與驗證
  - 提供商切換邏輯

- **`test_google_maps.py`** - Google Maps 測試工具
  - 驗證 API 配置
  - 測試路線規劃功能
  - 測試速限查詢功能

### 核心整合

- **`../../selfdrive/navd/navd.py`** - 主導航引擎
  - `calculate_route_mapbox()` - Mapbox 路線計算
  - `calculate_route_google()` - Google Maps 路線計算
  - `send_instruction()` - 發送導航指令
  - `decode_polyline()` - 解碼 Google polyline 格式

- **`../controls/lib/speed_limit_controller.py`** - 速限控制器
  - `get_mapbox_speed_limit()` - Mapbox 速限查詢
  - `get_google_maps_speed_limit()` - Google Maps 速限查詢
  - 自動選擇最佳速限資料來源

## 📱 使用方式

### 路線導航

1. **設定目的地** (透過 The Pond 或命令列)
2. **選擇導航提供商** (Mapbox 或 Google Maps)
3. **系統自動規劃路線**
4. **開始導航**

### 速限控制

系統會自動從多個來源獲取速限，並根據道路類型自動調整優先順序：

#### 🛣️ 高速公路
- **優先級模式**: `Highest` - 選擇所有來源中的最高速限

#### 🚗 快速道路、市區、鄉村
- **優先順序**: `Map Data` → `Navigation` → `Dashboard`
  1. Map Data (離線地圖)
  2. Navigation (導航 API - Google Maps/Mapbox)
  3. Dashboard (儀表板)
  4. Vision (視覺識別)

## 🛠️ 測試

### 運行 Google Maps 測試

```bash
cd /data/openpilot/frogpilot/navigation
python test_google_maps.py
```

測試項目：
- ✅ 配置檢查
- ✅ Directions API 連接
- ✅ Roads API 速限查詢
- ✅ 提供商切換功能

## 📖 相關文檔

- [Google Maps 整合指南](GOOGLE_MAPS_GUIDE.md) - 完整的 Google Maps 設定教學
- [Mapbox 文檔](https://docs.mapbox.com/) - Mapbox API 官方文檔
- [OpenStreetMap](https://www.openstreetmap.org/) - 離線地圖資料來源

## ⚠️ 注意事項

### API 使用限制

- **Mapbox**: 每月 100,000 次免費請求
- **Google Maps**:
  - Directions API: ~40,000 次/月 (免費額度)
  - Roads API: ~40,000 次/月 (免費額度)

### 隱私考量

- 使用導航 API 會將位置資料發送到第三方服務
- 離線地圖不會傳送任何資料
- 建議根據個人隱私偏好選擇合適的提供商

## 🤝 貢獻

歡迎提交 Issue 和 Pull Request！

## 📄 授權

遵循 FrogPilot 主專案授權條款。
