# 除錯與診斷工具

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

## 除錯與診斷

系統性的除錯流程能大幅提升開發效率，以下是完整的除錯指南。

## 查看 Process 錯誤

### 方法一：使用 TMux（最直接）

```bash
# 1. SSH 連線到 Comma 3
ssh comma@192.168.1.xxx

# 2. 進入 tmux session
tmux attach -t comma

# 3. 查看 process 輸出
# 按 Ctrl+B 然後按 [ 進入滾動模式
# 使用方向鍵滾動，按 / 搜尋關鍵字
# 按 q 退出滾動模式

# 4. 退出 tmux
# 按 Ctrl+B 然後按 D
```

### 方法二：使用 Journalctl

```bash
# 查看最近日誌
journalctl -u comma --no-pager -n 100

# 只看錯誤
journalctl -u comma --no-pager | grep -i error | tail -50

# 即時監控
journalctl -u comma -f

# 查看特定 process
journalctl -u comma --no-pager | grep timed | tail -50
```

### 方法三：手動運行 Process

```bash
# 停止服務
sudo systemctl stop comma

# 手動運行查看錯誤
cd /data/openpilot
export PYTHONPATH=/data/openpilot
python system/timed.py  # 替換成要測試的 process

# 重啟服務
sudo systemctl start comma
```

---

## 卡 LOGO 診斷流程

當系統卡在 LOGO 畫面時的標準診斷流程。

### 初步診斷

```bash
# 1. 查看 tmux session
tmux attach -t comma

# 2. 檢查系統日誌
journalctl -u comma --no-pager -n 100

# 3. 手動測試 manager
sudo systemctl stop comma
cd /data/openpilot
python system/manager/manager.py
```

### 檢查模組載入

```bash
cd /data/openpilot

# 測試關鍵模組
python -c "from system.manager import manager; print('✓ manager.py OK')"
python -c "from system.manager import process_config; print('✓ process_config.py OK')"
python -c "from selfdrive.controls import controlsd; print('✓ controlsd.py OK')"
```

### Git 回復

```bash
# 查看最近變更
git log --oneline -5

# 回復到前一個 commit
git checkout HEAD~1
sudo reboot

# 回復特定檔案
git checkout HEAD~1 -- path/to/problematic_file.py
git add path/to/problematic_file.py
git commit -m "Revert problematic file"
sudo reboot
```

### 快速診斷腳本

```bash
cat > /data/diagnose.sh << 'EOF'
#!/bin/bash
echo "=== OpenPilot Diagnostic ==="
echo "Date: $(date)"
echo ""

echo "=== Git Status ==="
cd /data/openpilot
git log --oneline -3
echo ""

echo "=== Python Module Check ==="
python -c "from system.manager import manager; print('✓')" 2>&1
echo ""

echo "=== Recent Errors ==="
journalctl -u comma --no-pager -n 50 | grep -i error | tail -5
echo ""
EOF

chmod +x /data/diagnose.sh
/data/diagnose.sh
```

---

## TMux 操作

### 基本操作

```bash
# 查看所有 sessions
tmux ls

# 進入 comma session
tmux attach -t comma

# 創建新 session
tmux new -s debug
```

### 快捷鍵（先按 Ctrl+B）

```
D         - 分離 session（退出但不關閉）
[         - 進入滾動/複製模式
]         - 貼上
C         - 創建新視窗
N         - 下一個視窗
P         - 上一個視窗
W         - 列出所有視窗
?         - 顯示所有快捷鍵
```

### 滾動模式操作

```
方向鍵      - 上下左右滾動
Page Up/Down - 快速翻頁
/            - 向下搜尋
?            - 向上搜尋
N            - 下一個匹配
Shift+N      - 上一個匹配
Space        - 開始選取
Enter        - 完成選取並複製
Q 或 Esc     - 退出滾動模式
```

---

