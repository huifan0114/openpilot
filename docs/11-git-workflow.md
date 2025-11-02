# Git 工作流程

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

## Git 工作流程

## 分支管理

### 當前分支結構

```
main (本地主分支)
  └─ hfop (開發分支)
       ↓
  upstream/FrogPilot (上游)
```

### 基本操作

```bash
# 查看分支
git branch -a

# 切換分支
git checkout hfop

# 創建新分支
git checkout -b feature/new-feature

# 查看狀態
git status

# 查看最近 commits
git log --oneline -5
```

---

## 上游更新合併

### 方法一：Merge（推薦新手）

```bash
# 1. 確保在 hfop 分支
git checkout hfop

# 2. 獲取上游更新
git fetch upstream

# 3. 查看更新
git log --oneline HEAD..upstream/FrogPilot

# 4. 合併
git merge upstream/FrogPilot

# 5. 推送
git push origin hfop
```

**優點**：
- 保留完整歷史
- 較容易理解
- 衝突較容易解決

**缺點**：
- 有合併提交
- 歷史較複雜

### 方法二：Rebase（進階用戶）

```bash
# 1. 切換到 hfop
git checkout hfop

# 2. Rebase 到上游
git rebase upstream/FrogPilot

# 3. 推送（需要 force）
git push origin hfop --force-with-lease
```

**優點**：
- 線性歷史
- 更清晰

**缺點**：
- 需要逐個解決衝突
- 改寫歷史

### 處理衝突

```bash
# 查看衝突
git status

# 編輯衝突檔案（尋找 <<<<<<< HEAD 標記）

# 標記衝突已解決
git add [衝突的檔案]

# 繼續合併
git merge --continue  # Merge 模式
# 或
git rebase --continue  # Rebase 模式
```

---

## 回復與修復

### 回復到前一個版本

```bash
# 方法一：暫存當前變更
git stash
sudo reboot

# 方法二：回到前一個 commit
git checkout HEAD~1
sudo reboot

# 方法三：重置（會遺失變更）
git reset --hard HEAD~1
sudo reboot
```

### 回復特定檔案

```bash
# 回復單一檔案
git checkout HEAD~1 -- path/to/file.py

# 提交修復
git add path/to/file.py
git commit -m "Revert file to fix issue"
```

### 清理並重新編譯

```bash
cd /data/openpilot

# 清理編譯檔案
find . -name "*.pyc" -delete
find . -name "__pycache__" -type d -delete

# 重新編譯
scons -j4

sudo reboot
```

---

