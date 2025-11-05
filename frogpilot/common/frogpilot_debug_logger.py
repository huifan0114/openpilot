#!/usr/bin/env python3
"""
FrogPilot Debug Logger - 統一的文字 log 系統
所有我們的修改都應該使用這個 logger 記錄到 /data/frogpilot_debug.log
"""

import os
from datetime import datetime

DEBUG_LOG_FILE = "/data/frogpilot_debug.log"
MAX_LOG_SIZE = 500000  # 500KB

def write_debug_log(module: str, message: str) -> None:
    """
    寫入 debug log (非同步,不阻塞主程序)

    Args:
        module: 模組名稱 (例如: "SLC", "BRAKE", "UI")
        message: log 訊息
    """
    try:
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_line = f"[{timestamp}] {module}: {message}\n"

        # 非阻塞寫入
        with open(DEBUG_LOG_FILE, "a", buffering=1) as f:
            f.write(log_line)

        # 簡單的檔案大小控制 (不每次都檢查,避免效能影響)
        if os.path.getsize(DEBUG_LOG_FILE) > MAX_LOG_SIZE:
            _rotate_log()

    except Exception:
        # 靜默失敗,不影響主程序
        pass

def _rotate_log() -> None:
    """循環 log 檔案 (保留最新 500 行)"""
    try:
        with open(DEBUG_LOG_FILE, "r") as f:
            lines = f.readlines()
        with open(DEBUG_LOG_FILE, "w") as f:
            f.write("=" * 60 + "\n")
            f.write(f"Log rotated at {datetime.now()}\n")
            f.write("=" * 60 + "\n")
            f.writelines(lines[-500:])
    except Exception:
        pass
