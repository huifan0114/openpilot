#!/usr/bin/env python3
"""
Google Maps 設定工具
快速設定 Google Maps API Key 和導航提供商
"""

import sys
from openpilot.common.params import Params
from openpilot.frogpilot.navigation.navigation_config import NavigationConfig

def setup_google_maps():
    """交互式設定 Google Maps"""
    print("=" * 60)
    print("Google Maps 導航設定工具")
    print("=" * 60)
    print()

    params = Params()
    config = NavigationConfig()

    # 檢查當前配置
    current_provider = config.get_provider()
    current_key = config.get_api_key("google")

    print(f"當前導航提供商: {current_provider}")
    if current_key:
        print(f"已設定 Google Maps API Key: {current_key[:10]}...{current_key[-4:]}")
    else:
        print("尚未設定 Google Maps API Key")
    print()

    # 詢問是否要設定 API Key
    if not current_key or input("是否要更新 Google Maps API Key? (y/n): ").lower() == 'y':
        print()
        print("請輸入 Google Maps API Key")
        print("(格式應為: AIza... 開頭，約 39 字元)")
        print("取得方式: https://console.cloud.google.com/")
        print()

        api_key = input("API Key: ").strip()

        if not api_key:
            print("❌ API Key 不能為空")
            return False

        if not api_key.startswith("AIza"):
            print("⚠️  警告: API Key 格式可能不正確（應以 AIza 開頭）")
            if input("是否仍要繼續? (y/n): ").lower() != 'y':
                return False

        # 設定 API Key
        config.set_api_key("google", api_key)
        print(f"✅ API Key 已設定: {api_key[:10]}...{api_key[-4:]}")

    print()

    # 詢問是否要切換到 Google Maps
    print("導航提供商選項:")
    print("  1. Google Maps - 使用 Google 路線規劃和速限")
    print("  2. Mapbox - 使用 Mapbox 路線規劃和速限")
    print("  3. 保持當前設定")
    print()

    choice = input(f"請選擇 (1-3) [當前: {current_provider}]: ").strip()

    if choice == "1":
        if config.set_provider("google"):
            print("✅ 已切換到 Google Maps")
            print()
            print("注意事項:")
            print("  - 確保已在 Google Cloud Console 啟用以下 API:")
            print("    • Directions API (路線規劃)")
            print("    • Roads API (速限查詢)")
            print("  - 請注意 API 使用配額和計費")
        else:
            print("❌ 切換失敗")
            return False

    elif choice == "2":
        if config.set_provider("mapbox"):
            print("✅ 已切換到 Mapbox")
        else:
            print("❌ 切換失敗")
            return False

    else:
        print(f"⏭️  保持當前設定: {current_provider}")

    print()
    print("=" * 60)
    print("設定完成")
    print("=" * 60)
    print()

    # 顯示最終配置
    final_provider = config.get_provider()
    available = config.get_available_providers()

    print(f"當前提供商: {final_provider}")
    print(f"可用提供商: {', '.join(available)}")
    print()

    # 建議運行測試
    print("💡 建議運行測試以驗證設定:")
    print("   python test_google_maps.py")
    print()

    return True

def quick_setup(api_key, provider="google"):
    """非交互式快速設定"""
    config = NavigationConfig()

    if api_key:
        config.set_api_key("google", api_key)
        print(f"✅ API Key 已設定")

    if provider:
        if config.set_provider(provider):
            print(f"✅ 已切換到 {provider}")
        else:
            print(f"❌ 無效的提供商: {provider}")
            return False

    return True

def show_status():
    """顯示當前配置狀態"""
    config = NavigationConfig()

    print("=" * 60)
    print("導航配置狀態")
    print("=" * 60)
    print()

    current = config.get_provider()
    available = config.get_available_providers()

    print(f"當前提供商: {current}")
    print(f"可用提供商: {', '.join(available)}")
    print()

    # 顯示各提供商詳細資訊
    for provider_name in ["mapbox", "google", "mapd"]:
        info = config.get_provider_info(provider_name)
        key = config.get_api_key(provider_name)
        is_available = config.is_provider_available(provider_name)

        status = "✅" if is_available else "❌"
        print(f"{status} {info['name']}")

        if key:
            print(f"   API Key: {key[:10]}...{key[-4:]}")
        elif info['key_param']:
            print(f"   API Key: 未設定")
        else:
            print(f"   不需要 API Key")

        print(f"   交通資訊: {'支援' if info['supports_traffic'] else '不支援'}")
        print(f"   速限查詢: {'支援' if info['supports_speed_limits'] else '不支援'}")
        print()

def main():
    if len(sys.argv) > 1:
        command = sys.argv[1]

        if command == "status":
            show_status()

        elif command == "set" and len(sys.argv) >= 3:
            api_key = sys.argv[2]
            provider = sys.argv[3] if len(sys.argv) > 3 else "google"
            quick_setup(api_key, provider)

        elif command == "switch" and len(sys.argv) >= 3:
            provider = sys.argv[2]
            config = NavigationConfig()
            if config.set_provider(provider):
                print(f"✅ 已切換到 {provider}")
            else:
                print(f"❌ 無效的提供商: {provider}")

        else:
            print("用法:")
            print("  python setup_google_maps.py              # 交互式設定")
            print("  python setup_google_maps.py status       # 顯示當前狀態")
            print("  python setup_google_maps.py set <key>    # 快速設定 API Key")
            print("  python setup_google_maps.py switch <provider>  # 切換提供商")
            print()
            print("範例:")
            print("  python setup_google_maps.py set AIza... google")
            print("  python setup_google_maps.py switch mapbox")

    else:
        # 交互式模式
        setup_google_maps()

if __name__ == "__main__":
    main()
