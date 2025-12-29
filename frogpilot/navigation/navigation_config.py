#!/usr/bin/env python3
"""
FrogPilot Navigation Configuration
Manages navigation provider selection and API keys
"""

from openpilot.common.params import Params

class NavigationConfig:
  PROVIDERS = {
    "mapbox": {
      "name": "Mapbox",
      "host": "https://api.mapbox.com",
      "key_param": "MapboxSecretKey",
      "env_var": "MAPBOX_TOKEN",
      "supports_traffic": True,
      "supports_speed_limits": True,
    },
    "google": {
      "name": "Google Maps",
      "host": "https://maps.googleapis.com",
      "key_param": "GoogleMapsKey",
      "env_var": "GOOGLE_MAPS_KEY",
      "supports_traffic": True,
      "supports_speed_limits": True,
    },
    "mapd": {
      "name": "Map Data (Offline)",
      "host": None,
      "key_param": None,
      "env_var": None,
      "supports_traffic": False,
      "supports_speed_limits": True,
    }
  }

  def __init__(self):
    self.params = Params()

  def get_provider(self):
    """獲取當前選擇的導航提供商"""
    provider = self.params.get("NavigationProvider", encoding='utf8')
    return provider if provider in self.PROVIDERS else "mapbox"

  def set_provider(self, provider):
    """設定導航提供商"""
    if provider in self.PROVIDERS:
      self.params.put("NavigationProvider", provider)
      return True
    return False

  def get_api_key(self, provider=None):
    """獲取指定提供商的 API key"""
    if provider is None:
      provider = self.get_provider()

    if provider not in self.PROVIDERS:
      return None

    config = self.PROVIDERS[provider]
    if config["key_param"]:
      return self.params.get(config["key_param"], encoding='utf8')
    return None

  def set_api_key(self, provider, key):
    """設定 API key"""
    if provider not in self.PROVIDERS:
      return False

    config = self.PROVIDERS[provider]
    if config["key_param"]:
      self.params.put(config["key_param"], key)
      return True
    return False

  def is_provider_available(self, provider):
    """檢查提供商是否可用（有 API key）"""
    if provider not in self.PROVIDERS:
      return False

    config = self.PROVIDERS[provider]
    if config["key_param"] is None:
      return True  # 離線地圖不需要 API key

    return bool(self.get_api_key(provider))

  def get_available_providers(self):
    """獲取所有可用的提供商"""
    return [
      provider for provider in self.PROVIDERS
      if self.is_provider_available(provider)
    ]

  def get_provider_info(self, provider):
    """獲取提供商資訊"""
    return self.PROVIDERS.get(provider, {})

if __name__ == "__main__":
  # 測試用途
  config = NavigationConfig()
  print(f"Current provider: {config.get_provider()}")
  print(f"Available providers: {config.get_available_providers()}")
