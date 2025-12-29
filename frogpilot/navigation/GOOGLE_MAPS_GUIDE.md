# Google Maps 導航整合指南

## 概述

FrogPilot 現已支援 Google Maps 作為導航提供商，與 Mapbox 並行使用。Google Maps 提供：
- 全球路線規劃
- 即時交通資訊
- 速限查詢 (透過 Roads API)
- 多語言支援

## 設定步驟

### 1. 獲取 Google Maps API Key

1. 前往 [Google Cloud Console](https://console.cloud.google.com/)
2. 創建新專案或選擇現有專案
3. 啟用以下 API：
   - **Directions API** - 用於路線規劃
   - **Roads API** - 用於速限查詢
   - **Geocoding API** (可選) - 用於地址搜尋

4. 創建 API 金鑰：
   - 導航至「憑證」頁面
   - 點擊「建立憑證」→「API 金鑰」
   - 複製生成的金鑰（格式：`AIza...`）

5. 設定 API 金鑰限制（建議）：
   - 應用程式限制：選擇「IP 地址」
   - API 限制：僅限上述三個 API

### 2. 在 FrogPilot 中配置

#### 方法 A: 透過 The Pond 網頁介面（推薦） ⭐
1. 開啟 The Pond 介面（瀏覽器訪問 `http://YOUR_DEVICE_IP:8082`）
2. 前往左側選單 **Navigation** → **Manage Keys**
3. 在 **Google Maps Key** 區域輸入您的 API Key
4. 點擊 💾 儲存
5. 在頁面頂部的 **Navigation Provider** 區域
6. 點擊 **Google Maps** 按鈕切換提供商

#### 方法 B: 透過參數設定
```bash
# SSH 連線到設備後執行
echo -n "YOUR_API_KEY_HERE" > /data/params/d/GoogleMapsKey
echo -n "google" > /data/params/d/NavigationProvider
```

或使用設定腳本：
```bash
cd /data/openpilot/frogpilot/navigation
python setup_google_maps.py
```

#### 方法 C: 透過 Python 程式設定
```python
from openpilot.common.params import Params
from openpilot.frogpilot.navigation.navigation_config import NavigationConfig

params = Params()
config = NavigationConfig()

# 設定 API Key
config.set_api_key("google", "YOUR_API_KEY_HERE")

# 切換到 Google Maps
config.set_provider("google")
```

#### 方法 D: 透過環境變數（開發用）
```bash
export GOOGLE_MAPS_KEY="YOUR_API_KEY_HERE"
```

### 3. 切換導航提供商

可以隨時在 Mapbox 和 Google Maps 之間切換：

```python
from openpilot.frogpilot.navigation.navigation_config import NavigationConfig

config = NavigationConfig()
config.set_provider("google")  # 或 "mapbox"
```

## 功能對比

| 功能 | Mapbox | Google Maps | Map Data (離線) |
|------|--------|-------------|----------------|
| 路線規劃 | ✅ | ✅ | ❌ |
| 即時交通 | ✅ | ✅ | ❌ |
| 速限查詢 | ✅ | ✅ | ✅ |
| 離線使用 | ❌ | ❌ | ✅ |
| 多語言 | ✅ | ✅ | 部分 |
| 免費額度 | 100k/月 | 見下方 | 無限制 |

## Google Maps API 定價

### 免費額度（每月）
- **Directions API**: $200 免費額度 = ~40,000 次請求
- **Roads API**: $200 免費額度 = ~40,000 次請求

### 使用建議
- 正常導航使用通常不會超過免費額度
- 建議設定預算警報
- 可以在 Google Cloud Console 監控使用量

## 速限資料來源優先順序

在 [frogpilot_planner.py](../controls/frogpilot_planner.py) 中，速限來源會根據道路類型自動調整：

### 道路類型優先順序規則

#### 🛣️ 高速公路（`高速`、`國道`）
- **優先級模式**: `Highest` - 選擇所有來源中的最高速限
- 確保高速公路以最高合法速度行駛

#### 🚗 快速道路、交流道、市區、鄉村（其他所有道路）
- **優先順序**: `Map Data` → `Navigation` → `Dashboard`
1. **Map Data** - 離線地圖速限（最優先）
2. **Navigation** - Google Maps 或 Mapbox 導航速限
3. **Dashboard** - 車輛儀表板速限

這樣的設計確保：
- ✅ 高速公路使用最高合法速限以保持車流
- ✅ 市區和鄉村道路優先信任可靠的地圖資料
- ✅ 統一且合理的速限來源優先順序

## 技術細節

### Google Maps API 呼叫

#### Directions API
```
GET https://maps.googleapis.com/maps/api/directions/json
Parameters:
  - origin: 起點座標
  - destination: 終點座標
  - mode: driving
  - departure_time: now
  - alternatives: true
  - language: zh-TW
  - key: YOUR_API_KEY
```

#### Roads API (Speed Limits)
```
GET https://maps.googleapis.com/maps/api/roads/speedLimits
Parameters:
  - path: 當前座標
  - key: YOUR_API_KEY
```

### 程式碼位置

- 導航路線計算: [selfdrive/navd/navd.py](../../selfdrive/navd/navd.py)
  - `calculate_route_google()` - Google Maps 路線規劃
  - `parse_google_route()` - 解析 Google 路線資料
  - `decode_polyline()` - 解碼 Google polyline 格式

- 速限查詢: [frogpilot/controls/lib/speed_limit_controller.py](../controls/lib/speed_limit_controller.py)
  - `get_google_maps_speed_limit()` - Google Roads API 速限查詢

- 配置管理: [frogpilot/navigation/navigation_config.py](navigation_config.py)
  - 統一管理導航提供商設定

## 疑難排解

### 問題：無法取得路線
- 檢查 API key 是否正確
- 確認已啟用 Directions API
- 檢查網路連線
- 查看日誌：`/data/tmux_logs/navd.log`

### 問題：無速限資訊
- 確認已啟用 Roads API
- 部分地區 Google Maps 可能無速限資料
- 嘗試切換回 Mapbox 或使用離線地圖

### 問題：API 配額超限
- 檢查 Google Cloud Console 使用量
- 考慮增加預算或優化請求頻率
- 臨時切換至 Mapbox 或離線地圖

## 貢獻與回饋

如有問題或建議，請在 GitHub 提交 Issue：
https://github.com/FrogAi/FrogPilot/issues

## 參考資料

- [Google Maps Platform Documentation](https://developers.google.com/maps/documentation)
- [Directions API Guide](https://developers.google.com/maps/documentation/directions)
- [Roads API Guide](https://developers.google.com/maps/documentation/roads)
- [API Key Best Practices](https://developers.google.com/maps/api-security-best-practices)
