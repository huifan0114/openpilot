#!/usr/bin/bash
# 完整的預編譯和優化腳本
# 執行此腳本以建立優化過的版本

set -e

BASEDIR="/data/openpilot"
if [ -z "$BASEDIR" ]; then
  BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../.."
fi

cd "$BASEDIR"

echo "=========================================="
echo "開始編譯和優化 openpilot"
echo "=========================================="

# 1. 編譯 C/C++ 程式碼
echo ""
echo "[1/4] 編譯 C/C++ 程式碼..."
cd system/manager
./build.py

# 2. 預編譯 Python 模組
echo ""
echo "[2/4] 預編譯 Python 模組..."
cd "$BASEDIR"
python3 system/manager/precompile_python.py

# 3. 清理不必要的檔案以減少儲存空間
echo ""
echo "[3/4] 清理開發檔案..."
find . -name "*.o" -delete 2>/dev/null || true
find . -name "*.os" -delete 2>/dev/null || true
find . -name "*.a" -delete 2>/dev/null || true
find . -name ".sconsign.dblite" -delete 2>/dev/null || true

# 清理測試檔案
find . -type d -name "test" -o -name "tests" 2>/dev/null | while read dir; do
  if [[ "$dir" != *"/third_party/"* ]]; then
    echo "  清理: $dir"
    rm -rf "$dir" || true
  fi
done

# 4. 建立 prebuilt 標記
echo ""
echo "[4/4] 建立預編譯標記..."
touch "$BASEDIR/prebuilt"

echo ""
echo "=========================================="
echo "✓ 編譯和優化完成!"
echo "=========================================="
echo ""
echo "下次開機時將跳過編譯步驟,加快啟動速度"
echo ""
echo "如需重新編譯,請刪除 prebuilt 檔案:"
echo "  rm $BASEDIR/prebuilt"
echo ""
