#!/usr/bin/env python3
"""
AR HUD Dashboard Server
提供靜態文件服務（模仿 DASHY 架構）
前端直接連接 webrtcd，不需要後端代理
"""

from pathlib import Path
from aiohttp import web


# 靜態文件目錄
STATIC_DIR = Path(__file__).parent / "static"


async def index(request: web.Request):
    """返回主頁面"""
    return web.FileResponse(STATIC_DIR / "index.html")


async def vm_page(request: web.Request):
    """返回視覺監控頁面"""
    return web.FileResponse(STATIC_DIR / "vm.html")


# 移除 offer 函數 - 前端直接連接 webrtcd (模仿 DASHY 架構)


def create_app():
    """創建 aiohttp 應用"""
    app = web.Application()

    # 路由（只提供靜態文件服務，模仿 DASHY）
    app.router.add_get('/', index)
    app.router.add_get('/vm.html', vm_page)
    # 移除 /offer - 前端直接連接 webrtcd (port 5001)
    app.router.add_static('/static/', path=STATIC_DIR, name='static')

    return app


def main():
    """進程管理器調用的主函數"""
    app = create_app()
    web.run_app(app, host='0.0.0.0', port=8000)


if __name__ == '__main__':
    app = create_app()

    print("=" * 60)
    print("🚗 AR HUD Dashboard")
    print("=" * 60)
    print("Server starting on http://0.0.0.0:8000")
    print()
    print("在 Android 平板上打開瀏覽器，輸入:")
    print("  http://<設備IP>:8000")
    print()
    print("確保 webrtcd 已經運行在 port 5001")
    print("=" * 60)

    web.run_app(app, host='0.0.0.0', port=8000)
