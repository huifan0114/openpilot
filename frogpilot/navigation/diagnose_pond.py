#!/usr/bin/env python3
"""
The Pond 網頁介面診斷工具
用於檢查 The Pond 服務狀態和連線問題
"""

import os
import socket
import subprocess
import sys
from pathlib import Path

def check_port(port=8082):
    """檢查端口是否被佔用"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            result = s.connect_ex(('127.0.0.1', port))
            if result == 0:
                print(f"✅ 端口 {port} 正在監聽")
                return True
            else:
                print(f"❌ 端口 {port} 未監聽")
                return False
    except Exception as e:
        print(f"❌ 檢查端口時出錯: {e}")
        return False

def check_process():
    """檢查 The Pond 進程是否運行"""
    try:
        # 檢查 Python 進程
        result = subprocess.run(
            ['ps', 'aux'],
            capture_output=True,
            text=True,
            timeout=5
        )

        pond_processes = [line for line in result.stdout.split('\n') if 'the_pond.py' in line]

        if pond_processes:
            print("✅ The Pond 進程正在運行:")
            for proc in pond_processes:
                print(f"   {proc}")
            return True
        else:
            print("❌ 未找到 The Pond 進程")
            return False
    except Exception as e:
        print(f"❌ 檢查進程時出錯: {e}")
        return False

def check_files():
    """檢查關鍵檔案是否存在"""
    base_path = Path("/data/openpilot/frogpilot/system/the_pond")

    files_to_check = [
        base_path / "the_pond.py",
        base_path / "assets" / "components" / "navigation" / "navigation_keys.js",
        base_path / "assets" / "components" / "navigation" / "navigation_keys.css",
    ]

    all_exist = True
    for file_path in files_to_check:
        if file_path.exists():
            print(f"✅ {file_path.name} 存在")
        else:
            print(f"❌ {file_path.name} 不存在: {file_path}")
            all_exist = False

    return all_exist

def check_python_syntax():
    """檢查 Python 語法"""
    the_pond_path = "/data/openpilot/frogpilot/system/the_pond/the_pond.py"

    try:
        result = subprocess.run(
            ['python3', '-m', 'py_compile', the_pond_path],
            capture_output=True,
            text=True,
            timeout=5
        )

        if result.returncode == 0:
            print("✅ the_pond.py 語法正確")
            return True
        else:
            print("❌ the_pond.py 語法錯誤:")
            print(result.stderr)
            return False
    except Exception as e:
        print(f"❌ 檢查語法時出錯: {e}")
        return False

def get_ip_addresses():
    """獲取設備 IP 地址"""
    try:
        result = subprocess.run(
            ['hostname', '-I'],
            capture_output=True,
            text=True,
            timeout=5
        )

        ips = result.stdout.strip().split()
        if ips:
            print("✅ 設備 IP 地址:")
            for ip in ips:
                print(f"   http://{ip}:8082")
            return ips
        else:
            print("❌ 無法獲取 IP 地址")
            return []
    except Exception as e:
        print(f"❌ 獲取 IP 時出錯: {e}")
        return []

def check_logs():
    """檢查 The Pond 日誌"""
    log_path = Path("/data/tmux_logs/the_pond.log")

    if log_path.exists():
        print("✅ 找到日誌檔案，最後 20 行:")
        print("-" * 60)
        try:
            with open(log_path, 'r') as f:
                lines = f.readlines()
                for line in lines[-20:]:
                    print(line.rstrip())
        except Exception as e:
            print(f"❌ 讀取日誌時出錯: {e}")
        print("-" * 60)
    else:
        print(f"⚠️  未找到日誌檔案: {log_path}")

def main():
    print("=" * 60)
    print("The Pond 網頁介面診斷工具")
    print("=" * 60)
    print()

    # 1. 檢查檔案
    print("📁 檢查關鍵檔案...")
    files_ok = check_files()
    print()

    # 2. 檢查語法
    print("🔍 檢查 Python 語法...")
    syntax_ok = check_python_syntax()
    print()

    # 3. 檢查進程
    print("⚙️  檢查服務進程...")
    process_running = check_process()
    print()

    # 4. 檢查端口
    print("🔌 檢查網路端口...")
    port_listening = check_port()
    print()

    # 5. 獲取 IP
    print("🌐 獲取設備 IP...")
    ips = get_ip_addresses()
    print()

    # 6. 檢查日誌
    print("📋 檢查服務日誌...")
    check_logs()
    print()

    # 總結
    print("=" * 60)
    print("診斷結果總結")
    print("=" * 60)

    if all([files_ok, syntax_ok, process_running, port_listening]):
        print("✅ 所有檢查通過！")
        print()
        print("請嘗試訪問以下地址:")
        for ip in ips:
            print(f"   http://{ip}:8082")
    else:
        print("❌ 發現問題，請參考以下建議:")
        print()

        if not files_ok:
            print("📁 檔案問題:")
            print("   - 檢查檔案是否正確上傳")
            print("   - 確認路徑是否正確")

        if not syntax_ok:
            print("🔍 語法問題:")
            print("   - 查看上方錯誤訊息")
            print("   - 修正 Python 語法錯誤")

        if not process_running:
            print("⚙️  服務未運行:")
            print("   - 重啟 The Pond: sudo systemctl restart frogpilot-the-pond")
            print("   - 或手動啟動: cd /data/openpilot && python frogpilot/system/the_pond/the_pond.py")

        if not port_listening:
            print("🔌 端口未監聽:")
            print("   - 檢查是否有其他程式佔用 8082 端口")
            print("   - 查看服務日誌以了解啟動失敗原因")

    print()
    print("🔧 快速修復命令:")
    print("   # 重啟服務")
    print("   sudo systemctl restart frogpilot-the-pond")
    print()
    print("   # 查看即時日誌")
    print("   tail -f /data/tmux_logs/the_pond.log")
    print()
    print("   # 檢查端口佔用")
    print("   sudo netstat -tulpn | grep 8082")

if __name__ == "__main__":
    main()
