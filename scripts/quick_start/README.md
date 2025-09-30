# 🚀 OCR クイックスタートガイド

24時間以内に STM32N6 上で OCR モデルを動かすための最速手順

**関連Issue:** [#13 - OCRモデル24時間実装ガイド](../../issues/13)

---

## 📋 必要なもの

### ハードウェア
- STM32N6570-DK 開発ボード
- USB ケーブル
- PC (Linux/Windows/Mac)

### ソフトウェア
- Python 3.8+
- STM32Cube.AI 10.0+
- ARM GCC Toolchain
- ST-Link ツール

---

## ⚡ 最速実装（6ステップ）

### Step 1: モデル変換（ONNX）

```bash
cd scripts/quick_start

# 必要なパッケージインストール
pip install paddlepaddle paddle2onnx onnx

# Paddle → ONNX 変換
python 01_convert_paddle_to_onnx.py
```

**出力:** `ocr_model.onnx` (約 2-3 MB)

### Step 2: STM32Cube.AI 変換

```bash
# STM32 用に変換
bash 02_convert_to_stm32.sh
```

**出力:** `../../models/stm32_ocr/` ディレクトリ
- `ocr_net.c`
- `ocr_net.h`
- `ocr_net_data.c`
- `ocr_net_data.h`

### Step 3: コードをプロジェクトに統合

```bash
# 生成ファイルをコピー
cp ../../models/stm32_ocr/*.{c,h} ../../src/ai/

# 最小実装コードをコピー
cp 03_minimal_ocr_template.c ../../src/minimal_ocr.c
```

### Step 4: Makefile 更新

`../../Makefile` に以下を追加:

```makefile
# OCR 関連ソース
C_SOURCES += \
  src/ai/ocr_net.c \
  src/ai/ocr_net_data.c \
  src/minimal_ocr.c

# インクルードパス
C_INCLUDES += -Isrc/ai

# コンパイラフラグ
CFLAGS += -DMINIMAL_OCR_TEST
```

### Step 5: ビルド

```bash
cd ../../

# クリーンビルド
make clean
make -j8

# サイズ確認
arm-none-eabi-size build/utron-ocr.elf
```

### Step 6: フラッシュと実行

```bash
# フラッシュ
st-flash write build/utron-ocr.bin 0x08000000

# シリアル出力確認
screen /dev/ttyUSB0 115200
```

---

## ✅ 期待される出力

```
====================================
  STM32N6 Minimal OCR Test
====================================

[OCR] Initializing OCR network...
[OCR] ✓ Network created
[OCR] ✓ Network initialized
[OCR] Network Info:
[OCR]   Model name: ocr_net
[OCR]   Input size: 4096 bytes
[OCR]   Output size: 512 bytes
[OCR]   Activation size: 2048 KB
[OCR] ✓ OCR ready!

🚀 Starting inference test loop...

Test 1: [OCR] Inference completed in 5 ms
✓ Success
  Output (first 5): 0.02 -0.15 0.31 -0.08 0.45
Test 2: [OCR] Inference completed in 5 ms
✓ Success
...

=== OCR Statistics ===
Total inferences: 10
Successful: 10
Failed: 0
Inference time:
  Min: 5 ms
  Max: 6 ms
  Avg: 5 ms
  Success rate: 100.00%
======================

✅ Test completed!
====================================
```

---

## 🔧 トラブルシューティング

### ❌ メモリ不足エラー

**症状:**
```
Error: Region 'RAM' overflowed
```

**解決方法:**
```bash
# 量子化版を使用
cd scripts/quick_start
bash 02_convert_to_stm32.sh
# 質問に "y" と答える
```

### ❌ リンクエラー

**症状:**
```
undefined reference to `ai_ocr_net_create'
```

**解決方法:**
```makefile
# Makefile にライブラリパス追加
LIBS += -L$(STM32CUBE_AI_PATH)/Lib/GCC -lNetworkRuntime
```

### ❌ 実行時クラッシュ

**症状:** ハードフォルトやスタックオーバーフロー

**解決方法:**
```assembly
; startup_stm32n6xx.s を編集
Stack_Size EQU 0x4000  ; 16KB に増加
Heap_Size  EQU 0x4000  ; 16KB に増加
```

---

## 📊 性能目標

| 指標 | 目標 | 初期値 |
|------|------|--------|
| 推論時間 | < 8ms | ~5ms ✅ |
| メモリ | < 2.5MB | ~2.1MB ✅ |
| 精度 | > 90% | TBD 🔄 |

---

## 🎯 次のステップ

1. **実画像でテスト** ([Issue #11](../../issues/11))
   - カメラ統合
   - 実際の文字認識

2. **精度向上** ([Issue #11](../../issues/11))
   - 量子化最適化
   - 前処理追加

3. **NPU最適化** ([Issue #12](../../issues/12))
   - 利用率向上
   - 速度改善

4. **エンドツーエンド統合** ([Issue #10](../../issues/10))
   - μTRON OS 統合
   - 全機能連携

---

## 💬 サポート

詰まったら:
1. エラーメッセージをコピー
2. [Issue #13](../../issues/13) にコメント
3. 完全なログを添付

---

**目標:** 24時間以内に動作する OCR システム 🚀

**頑張ってください！** 💪
