#!/usr/bin/env python3
"""
AR HUD Dashboard Server
使用 bodyteleop 代理模式轉發 WebRTC 請求到 webrtcd
"""

import json
import dataclasses
from pathlib import Path
from aiohttp import web, ClientSession


# WebRTC 流請求結構
@dataclasses.dataclass
class StreamRequestBody:
    sdp: str
    cameras: list[str]
    test_sounds: list[str]
    push_uids: list[str]


# 靜態文件目錄
STATIC_DIR = Path(__file__).parent / "static"


async def index(request: web.Request):
    """返回主頁面"""
    return web.FileResponse(STATIC_DIR / "index.html")


async def offer(request: web.Request):
    """
    處理 WebRTC SDP offer，轉發到 webrtcd

    接收的 cereal 訊息:
    - modelV2: 車道模型 (車道線、路徑)
    - liveCalibration: 相機校正
    - carState: 車輛狀態 (速度等)
    - controlsState: 控制狀態 (cruise 等)
    - selfdriveState: 自駕狀態
    """
    try:
        params = await request.json()

        # 構建請求體
        body = StreamRequestBody(
            sdp=params["sdp"],
            cameras=[],  # Data Channel only，不需要視訊
            test_sounds=[],  # 不需要聲音
            push_uids=["modelV2", "liveCalibration", "carState", "controlsState", "selfdriveState"]
        )

        # 轉發到 webrtcd (port 5001)
        async with ClientSession() as session:
            async with session.post(
                "http://localhost:5001/stream",
                data=json.dumps(dataclasses.asdict(body)),
                headers={"Content-Type": "application/json"}
            ) as resp:
                answer = await resp.json()
                return web.json_response(answer)

    except Exception as e:
        print(f"Error handling offer: {e}")
        return web.json_response({"error": str(e)}, status=500)


def create_app():
    """創建 aiohttp 應用"""
    app = web.Application()

    # 路由
    app.router.add_get('/', index)
    app.router.add_post('/offer', offer)
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
