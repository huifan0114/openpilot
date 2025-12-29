#!/usr/bin/env python3
"""
測試 Google Maps 導航整合
用於驗證 API key 配置和基本功能
"""

import json
import requests
from openpilot.common.params import Params
from openpilot.frogpilot.navigation.navigation_config import NavigationConfig

def test_google_maps_directions():
    """測試 Google Maps Directions API"""
    config = NavigationConfig()
    api_key = config.get_api_key("google")

    if not api_key:
        print("❌ Google Maps API key 未設定")
        print("   請先設定: params.put('GoogleMapsKey', 'YOUR_KEY')")
        return False

    print(f"✅ 找到 API Key: {api_key[:10]}...")

    # 測試路線：台北101 -> 台北車站
    params = {
        'origin': '25.0330,121.5654',  # 台北101
        'destination': '25.0478,121.5170',  # 台北車站
        'key': api_key,
        'mode': 'driving',
        'language': 'zh-TW',
    }

    url = 'https://maps.googleapis.com/maps/api/directions/json'

    try:
        print("🔄 測試 Directions API...")
        response = requests.get(url, params=params, timeout=10)
        response.raise_for_status()

        data = response.json()

        if data.get('status') == 'OK':
            route = data['routes'][0]
            leg = route['legs'][0]
            print(f"✅ Directions API 正常")
            print(f"   距離: {leg['distance']['text']}")
            print(f"   時間: {leg['duration']['text']}")
            print(f"   步驟數: {len(leg['steps'])}")
            return True
        else:
            print(f"❌ API 回應錯誤: {data.get('status')}")
            if 'error_message' in data:
                print(f"   錯誤訊息: {data['error_message']}")
            return False

    except requests.exceptions.RequestException as e:
        print(f"❌ 網路請求失敗: {e}")
        return False

def test_google_maps_speed_limits():
    """測試 Google Maps Roads API (Speed Limits)"""
    config = NavigationConfig()
    api_key = config.get_api_key("google")

    if not api_key:
        print("❌ Google Maps API key 未設定")
        return False

    # 測試位置：台北市某條道路
    params = {
        'path': '25.0330,121.5654',
        'key': api_key,
    }

    url = 'https://maps.googleapis.com/maps/api/roads/speedLimits'

    try:
        print("🔄 測試 Roads API (Speed Limits)...")
        response = requests.get(url, params=params, timeout=10)
        response.raise_for_status()

        data = response.json()

        if 'speedLimits' in data:
            if data['speedLimits']:
                speed_limit = data['speedLimits'][0]
                print(f"✅ Roads API 正常")
                print(f"   速限: {speed_limit.get('speedLimit')} {speed_limit.get('units', 'KPH')}")
                return True
            else:
                print("⚠️  Roads API 正常但該位置無速限資料")
                return True
        else:
            print(f"❌ API 回應異常: {data}")
            return False

    except requests.exceptions.RequestException as e:
        print(f"❌ 網路請求失敗: {e}")
        return False

def test_navigation_config():
    """測試導航配置"""
    print("🔄 測試導航配置...")
    config = NavigationConfig()

    current = config.get_provider()
    available = config.get_available_providers()

    print(f"✅ 導航配置正常")
    print(f"   當前提供商: {current}")
    print(f"   可用提供商: {', '.join(available)}")

    # 測試切換
    if 'google' in available:
        print("✅ Google Maps 可用")
    else:
        print("⚠️  Google Maps 不可用（缺少 API key）")

    return True

def test_provider_switch():
    """測試切換提供商"""
    print("🔄 測試切換提供商...")
    config = NavigationConfig()

    original = config.get_provider()
    print(f"   原始提供商: {original}")

    # 切換到 Google
    if config.set_provider("google"):
        print(f"   ✅ 切換到 Google Maps")
        current = config.get_provider()
        print(f"   確認: {current}")

        # 切換回原始
        config.set_provider(original)
        print(f"   ✅ 恢復為 {original}")
        return True
    else:
        print(f"   ❌ 切換失敗")
        return False

def main():
    print("=" * 60)
    print("Google Maps 導航整合測試")
    print("=" * 60)
    print()

    results = {
        "配置測試": test_navigation_config(),
        "Directions API": test_google_maps_directions(),
        "Roads API": test_google_maps_speed_limits(),
        "提供商切換": test_provider_switch(),
    }

    print()
    print("=" * 60)
    print("測試結果摘要")
    print("=" * 60)

    for test_name, result in results.items():
        status = "✅ 通過" if result else "❌ 失敗"
        print(f"{test_name}: {status}")

    all_passed = all(results.values())
    print()
    if all_passed:
        print("🎉 所有測試通過！Google Maps 整合正常運作。")
    else:
        print("⚠️  部分測試失敗，請檢查上方錯誤訊息。")

    return all_passed

if __name__ == "__main__":
    import sys
    success = main()
    sys.exit(0 if success else 1)
