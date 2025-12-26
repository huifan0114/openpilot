#!/usr/bin/env python3
"""
Simple Dash - 多主題 Dashboard Server
支援透過 params 切換不同主題風格

主題目錄結構：
  themes/
    HFOP-GTI-NAVDASH/
      index.html
      css/style.css
      assets/
    [其他主題]/
      ...
  shared/
    js/           # 共用 JS
    manifest.json
    icon-*.png
"""

import os
from pathlib import Path
from aiohttp import web

# 目錄設定
BASE_DIR = Path(__file__).parent
THEMES_DIR = BASE_DIR / "themes"
SHARED_DIR = BASE_DIR / "shared"

# 預設主題
DEFAULT_THEME = "HFOP-GTI-NAVDASH"

# 有效主題列表（自動掃描 themes 目錄）
def get_valid_themes():
    if not THEMES_DIR.exists():
        return [DEFAULT_THEME]
    return [d.name for d in THEMES_DIR.iterdir() if d.is_dir()]


def get_current_theme():
    """從 params 讀取當前主題設定"""
    try:
        from openpilot.common.params import Params
        params = Params()
        theme = params.get("SimpleDashTheme")
        if theme:
            theme = theme.decode('utf-8').strip()
            if theme in get_valid_themes():
                return theme
    except Exception:
        pass
    return DEFAULT_THEME


async def index(request: web.Request):
    """返回當前主題的主頁面"""
    theme = get_current_theme()
    theme_index = THEMES_DIR / theme / "index.html"
    if theme_index.exists():
        return web.FileResponse(theme_index)
    # 回退到預設主題
    return web.FileResponse(THEMES_DIR / DEFAULT_THEME / "index.html")


async def theme_static(request: web.Request):
    """處理主題靜態文件（css, assets）"""
    theme = get_current_theme()
    path = request.match_info['path']

    # 先在當前主題目錄找
    theme_file = THEMES_DIR / theme / path
    if theme_file.exists() and theme_file.is_file():
        return web.FileResponse(theme_file)

    # 回退到預設主題
    default_file = THEMES_DIR / DEFAULT_THEME / path
    if default_file.exists() and default_file.is_file():
        return web.FileResponse(default_file)

    raise web.HTTPNotFound()


async def shared_static(request: web.Request):
    """處理共用靜態文件（js, manifest, icons）"""
    path = request.match_info['path']
    shared_file = SHARED_DIR / path
    if shared_file.exists() and shared_file.is_file():
        return web.FileResponse(shared_file)
    raise web.HTTPNotFound()


async def get_themes_api(request: web.Request):
    """API: 取得可用主題列表"""
    themes = get_valid_themes()
    current = get_current_theme()
    return web.json_response({
        "themes": themes,
        "current": current
    })


async def set_theme_api(request: web.Request):
    """API: 設定主題"""
    try:
        data = await request.json()
        theme = data.get("theme", "").strip()

        if theme not in get_valid_themes():
            return web.json_response({"error": "Invalid theme"}, status=400)

        from openpilot.common.params import Params
        params = Params()
        params.put("SimpleDashTheme", theme)

        return web.json_response({"success": True, "theme": theme})
    except Exception as e:
        return web.json_response({"error": str(e)}, status=500)


def create_app():
    """創建 aiohttp 應用"""
    app = web.Application()

    # 路由
    app.router.add_get('/', index)

    # API 路由
    app.router.add_get('/api/themes', get_themes_api)
    app.router.add_post('/api/theme', set_theme_api)

    # 靜態文件路由
    # /theme/ - 主題專用文件（css, assets）
    app.router.add_get('/theme/{path:.*}', theme_static)
    # /shared/ - 共用文件（js, manifest, icons）
    app.router.add_get('/shared/{path:.*}', shared_static)

    return app


def main():
    """進程管理器調用的主函數"""
    app = create_app()
    web.run_app(app, host='0.0.0.0', port=8000)


if __name__ == '__main__':
    app = create_app()

    print("=" * 60)
    print("🚗 Simple Dash - Multi-Theme Dashboard")
    print("=" * 60)
    print(f"Available themes: {get_valid_themes()}")
    print(f"Current theme: {get_current_theme()}")
    print()
    print("Server starting on http://0.0.0.0:8000")
    print()
    print("API:")
    print("  GET  /api/themes  - 取得主題列表")
    print("  POST /api/theme   - 設定主題 {\"theme\": \"...\"}")
    print("=" * 60)

    web.run_app(app, host='0.0.0.0', port=8000)
