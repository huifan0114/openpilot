# openpilot 預編譯優化指南

## 概述

本專案提供預編譯功能以大幅縮短開機啟動時間。預編譯會將 C/C++ 程式碼和 Python 模組提前編譯,避免每次開機時重新編譯。

## 開機速度優化方式

### 1. 使用預編譯版本 (最快)

如果您使用的是從 GitHub 下載的預編譯版本(FrogPilot-Staging 等),系統會自動跳過編譯:

- 預編譯版本包含 `prebuilt` 標記檔案
- 開機時會直接啟動,不執行編譯
- **預期開機時間**: 20-40秒 (取決於硬體)

### 2. 手動執行預編譯

如果您是從原始碼安裝,可以手動執行預編譯:

```bash
cd /data/openpilot
chmod +x precompile_all.sh
./precompile_all.sh
```

此腳本會:
1. 編譯所有 C/C++ 程式碼
2. 預編譯所有 Python 模組為優化的字節碼 (.opt-2.pyc)
3. 清理不必要的檔案
4. 建立 `prebuilt` 標記

完成後,重新開機即可享受快速啟動。

### 3. 自動預編譯 (開發版本)

如果您在開發或修改程式碼:

系統會在第一次開機時自動編譯並預編譯 Python 模組。build.py 會自動執行 Python 預編譯。

## 開機時間比較

| 模式 | 首次開機 | 後續開機 | 說明 |
|------|----------|----------|------|
| 原始版本 | 2-5 分鐘 | 2-5 分鐘 | 每次都重新編譯 |
| 有 prebuilt 標記 | - | 20-40 秒 | 跳過編譯,僅載入模組 |
| 預編譯 + Python 字節碼 | - | **15-30 秒** | 最快,所有程式碼都已編譯 |

## 預編譯檔案

預編譯會產生以下檔案:

```
selfdrive/
  __pycache__/
    *.opt-2.pyc          # 優化的 Python 字節碼
system/
  __pycache__/
    *.opt-2.pyc
common/
  __pycache__/
    *.opt-2.pyc
frogpilot/
  __pycache__/
    *.opt-2.pyc
```

這些檔案會在程式啟動時直接載入,不需要即時編譯。

## 重新編譯

如果您修改了程式碼,需要重新編譯:

```bash
# 方法 1: 刪除 prebuilt 標記 (會在下次開機時重新編譯)
rm /data/openpilot/prebuilt

# 方法 2: 手動執行預編譯腳本
cd /data/openpilot
./precompile_all.sh

# 方法 3: 只重新編譯 Python
python3 system/manager/precompile_python.py
```

## 磁碟空間

預編譯會增加少量磁碟使用:

- Python 字節碼: 約 50-100 MB
- C/C++ 編譯產物: 已包含在原始系統中

總額外空間: **約 50-100 MB**

## 疑難排解

### 開機仍然很慢

1. 確認 `prebuilt` 檔案存在:
   ```bash
   ls -la /data/openpilot/prebuilt
   ```

2. 檢查 Python 字節碼是否存在:
   ```bash
   find /data/openpilot -name "*.opt-2.pyc" | head -10
   ```

3. 查看啟動日誌:
   ```bash
   tail -f /data/media/0/log/launch_log_*.txt
   ```

### 修改程式碼後無效

刪除 `prebuilt` 檔案並重新執行預編譯腳本。

### Python 模組錯誤

刪除所有字節碼重新生成:
```bash
find /data/openpilot -name "*.pyc" -delete
find /data/openpilot -name "__pycache__" -type d -exec rm -rf {} +
python3 system/manager/precompile_python.py
```

## 技術細節

### C/C++ 編譯

使用 SCons 建置系統編譯:
- 使用快取 (`/data/scons_cache`) 加速後續編譯
- 支援平行編譯 (`-j` 參數)
- 編譯產物包含共享函式庫和可執行檔

### Python 字節碼

使用 Python 的 `compileall` 模組:
- 優化等級 2 (`-OO`): 移除 docstrings 和 asserts
- 平行編譯利用多核心
- 生成 `.opt-2.pyc` 檔案

### 啟動順序

1. `launch_openpilot.sh` → `launch_chffrplus.sh`
2. 檢查 `prebuilt` 檔案
3. 如不存在,執行 `build.py` (包含 C++ 編譯和 Python 預編譯)
4. 如存在,直接執行 `manager.py`
5. Python 自動載入 `.opt-2.pyc` 字節碼

## 授權

本優化方案遵循 openpilot 專案的 MIT 授權。
