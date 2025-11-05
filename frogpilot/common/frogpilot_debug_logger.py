#!/usr/bin/env python3
"""
FrogPilot Debug Logger - 統一的文字 log 系統
所有我們的修改都應該使用這個 logger 記錄到 /data/frogpilot_debug.log
"""

import os
import shutil
from datetime import datetime

DEBUG_LOG_FILE = "/data/frogpilot_debug.log"
MAX_LOG_SIZE = 500000  # 500KB
MAX_ARCHIVED_LOGS = 5  # 保留最多 5 個舊 log 檔

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
    """
    循環 log 檔案 (建立新檔案而非覆蓋)

    - 將現有檔案重新命名為 frogpilot_debug.log.YYYYMMDD_HHMMSS
    - 開始新的空白 log 檔
    - 保留最多 5 個舊檔案
    """
    try:
        # 建立檔案名稱 (帶時間戳記)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        archived_file = f"{DEBUG_LOG_FILE}.{timestamp}"

        # 重新命名現有檔案
        shutil.move(DEBUG_LOG_FILE, archived_file)

        # 清理舊檔案 (保留最新 5 個)
        _cleanup_old_logs()

    except Exception:
        # 靜默失敗
        pass

def _cleanup_old_logs() -> None:
    """清理舊的 archived log 檔案,只保留最新 5 個"""
    try:
        log_dir = os.path.dirname(DEBUG_LOG_FILE)
        log_basename = os.path.basename(DEBUG_LOG_FILE)

        # 找出所有 archived log 檔案
        archived_logs = []
        for filename in os.listdir(log_dir):
            if filename.startswith(log_basename + "."):
                archived_logs.append(os.path.join(log_dir, filename))

        # 按修改時間排序 (最新的在最後)
        archived_logs.sort(key=os.path.getmtime)

        # 刪除最舊的檔案 (保留最新 MAX_ARCHIVED_LOGS 個)
        if len(archived_logs) > MAX_ARCHIVED_LOGS:
            for old_log in archived_logs[:-MAX_ARCHIVED_LOGS]:
                os.remove(old_log)

    except Exception:
        # 靜默失敗
        pass
