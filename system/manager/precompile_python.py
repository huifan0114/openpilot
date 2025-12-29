#!/usr/bin/env python3
"""
預編譯所有 Python 模組以加快開機速度
"""
import compileall
import py_compile
import os
import sys
from pathlib import Path

BASEDIR = Path(__file__).resolve().parent.parent.parent

def precompile_python():
    """預編譯所有 Python 檔案為字節碼"""
    print("開始預編譯 Python 模組...")
    
    # 需要編譯的目錄
    dirs_to_compile = [
        BASEDIR / "selfdrive",
        BASEDIR / "system",
        BASEDIR / "common",
        BASEDIR / "cereal",
        BASEDIR / "frogpilot",
        BASEDIR / "openpilot",
    ]
    
    compiled_count = 0
    error_count = 0
    
    for directory in dirs_to_compile:
        if not directory.exists():
            print(f"跳過不存在的目錄: {directory}")
            continue
            
        print(f"編譯目錄: {directory}")
        
        # 使用 compileall 批量編譯
        # force=True: 強制重新編譯
        # quiet=1: 只顯示錯誤
        # optimize=2: 生成優化的字節碼
        try:
            result = compileall.compile_dir(
                directory,
                maxlevels=10,
                ddir=directory,
                force=True,
                quiet=1,
                optimize=2,
                workers=os.cpu_count() or 1
            )
            if result:
                compiled_count += 1
            else:
                error_count += 1
                print(f"  警告: {directory} 編譯時有錯誤")
        except Exception as e:
            error_count += 1
            print(f"  錯誤: {directory} 編譯失敗 - {e}")
    
    print(f"\n預編譯完成!")
    print(f"  成功: {compiled_count} 個目錄")
    print(f"  錯誤: {error_count} 個目錄")
    
    # 清理舊的未優化的 .pyc 檔案
    print("\n清理未優化的字節碼檔案...")
    removed_count = 0
    for directory in dirs_to_compile:
        if not directory.exists():
            continue
        for pyc_file in directory.rglob("*.pyc"):
            # 只保留 .opt-2.pyc 檔案
            if not pyc_file.name.endswith(".opt-2.pyc"):
                try:
                    pyc_file.unlink()
                    removed_count += 1
                except Exception:
                    pass
    
    print(f"已清理 {removed_count} 個未優化的字節碼檔案")
    return error_count == 0

if __name__ == "__main__":
    success = precompile_python()
    sys.exit(0 if success else 1)
