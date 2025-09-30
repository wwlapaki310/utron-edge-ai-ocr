#!/bin/bash
# STM32Cube.AI を使って ONNX モデルを STM32 用に変換

set -e  # エラーで停止

echo "=========================================="
echo "  STM32Cube.AI 変換スクリプト"
echo "=========================================="
echo

# 設定
MODEL_FILE="ocr_model.onnx"
OUTPUT_DIR="../../models/stm32_ocr"
NETWORK_NAME="ocr_net"
TARGET="stm32n6"

# モデルファイル確認
if [ ! -f "$MODEL_FILE" ]; then
    echo "❌ エラー: $MODEL_FILE が見つかりません"
    echo "先に 01_convert_paddle_to_onnx.py を実行してください"
    exit 1
fi

# STM32Cube.AI確認
if ! command -v stm32ai &> /dev/null; then
    echo "❌ エラー: stm32ai コマンドが見つかりません"
    echo ""
    echo "STM32Cube.AI をインストールしてください:"
    echo "  https://www.st.com/en/embedded-software/x-cube-ai.html"
    echo ""
    echo "または STM32CubeIDE から使用してください"
    exit 1
fi

echo "✅ STM32Cube.AI が見つかりました"
stm32ai --version
echo

# 出力ディレクトリ作成
mkdir -p "$OUTPUT_DIR"

echo "📋 変換設定:"
echo "  入力モデル: $MODEL_FILE"
echo "  出力先: $OUTPUT_DIR"
echo "  ネットワーク名: $NETWORK_NAME"
echo "  ターゲット: $TARGET"
echo

# モデルサイズ確認
MODEL_SIZE=$(du -h "$MODEL_FILE" | cut -f1)
echo "📏 モデルサイズ: $MODEL_SIZE"
echo

# まず分析（量子化なし）
echo "🔍 Step 1: モデル分析中..."
stm32ai analyze \
  --model "$MODEL_FILE" \
  --type onnx \
  --target "$TARGET" \
  --compression none \
  --verbosity 1

echo
echo "質問: メモリ使用量が 2.5MB を超えていますか? (y/N)"
read -r quantize_response

if [[ "$quantize_response" =~ ^[Yy]$ ]]; then
    echo
    echo "🔄 Step 2: 量子化変換中 (INT8)..."
    echo "  (これには数分かかる場合があります)"
    
    stm32ai generate \
      --model "$MODEL_FILE" \
      --type onnx \
      --target "$TARGET" \
      --optimization balanced \
      --compression high \
      --quantize int8 \
      --verbosity 3 \
      --output "${OUTPUT_DIR}_int8" \
      --name "${NETWORK_NAME}"
    
    OUTPUT_DIR="${OUTPUT_DIR}_int8"
    echo "✅ 量子化変換完了: $OUTPUT_DIR"
else
    echo
    echo "🔄 Step 2: 標準変換中..."
    
    stm32ai generate \
      --model "$MODEL_FILE" \
      --type onnx \
      --target "$TARGET" \
      --optimization balanced \
      --compression medium \
      --verbosity 3 \
      --output "$OUTPUT_DIR" \
      --name "$NETWORK_NAME"
    
    echo "✅ 標準変換完了: $OUTPUT_DIR"
fi

echo
echo "📊 変換結果:"
if [ -f "${OUTPUT_DIR}/network_report.txt" ]; then
    echo
    echo "--- メモリ使用量 ---"
    grep -A 5 "Memory" "${OUTPUT_DIR}/network_report.txt" || true
    echo
    echo "--- 推論時間 (推定) ---"
    grep -A 3 "time" "${OUTPUT_DIR}/network_report.txt" || true
    echo
    
    # レポート全体を表示するか確認
    echo "完全なレポートを表示しますか? (y/N)"
    read -r show_report
    if [[ "$show_report" =~ ^[Yy]$ ]]; then
        cat "${OUTPUT_DIR}/network_report.txt"
    fi
fi

echo
echo "=========================================="
echo "✅ 変換完了!"
echo "=========================================="
echo
echo "生成されたファイル:"
ls -lh "$OUTPUT_DIR"
echo
echo "次のステップ:"
echo "  1. 生成されたファイルをプロジェクトにコピー:"
echo "     cp ${OUTPUT_DIR}/*.{c,h} ../../src/ai/"
echo
echo "  2. 最小実装コードを使用:"
echo "     cp 03_minimal_ocr_template.c ../../src/minimal_ocr.c"
echo
echo "  3. ビルド:"
echo "     cd ../../"
echo "     make clean && make -j8"
echo
