# STM32Cube.AI モデル変換実践ガイド

## 概要

本章では、TensorFlow/PyTorchで訓練されたAIモデルをSTM32N6のNeural-ART NPUで動作させるための変換プロセスを解説します。

---

## 1. モデル変換の全体フロー

```
訓練済みモデル (TensorFlow/PyTorch)
    ↓
中間フォーマット変換 (ONNX/TFLite)
    ↓
STM32Cube.AI変換 (X-CUBE-AI)
    ↓
Neural-ART NPU最適化コード
    ↓
STM32N6実装
```

---

## 2. サポートされる入力フォーマット

### STM32Cube.AI v10.0がサポートする形式

| フォーマット | 拡張子 | フレームワーク | 推奨度 |
|------------|--------|--------------|--------|
| TensorFlow Lite | `.tflite` | TensorFlow | ⭐⭐⭐⭐⭐ |
| ONNX | `.onnx` | PyTorch, TensorFlow | ⭐⭐⭐⭐ |
| Keras | `.h5` | Keras | ⭐⭐⭐ |

**推奨**: TensorFlow Lite形式 (`.tflite`) を使用すると、量子化とNeural-ART最適化が最も効率的です。

---

## 3. PyTorchモデルからの変換

### 3.1 PyTorch → ONNX

```python
import torch
import torch.onnx

# モデルのロード
model = YourModel()
model.load_state_dict(torch.load('model.pth'))
model.eval()

# ダミー入力の作成
dummy_input = torch.randn(1, 3, 320, 240)  # (batch, channels, height, width)

# ONNX変換
torch.onnx.export(
    model,
    dummy_input,
    'model.onnx',
    export_params=True,
    opset_version=11,  # Neural-ARTは opset 11-13をサポート
    do_constant_folding=True,
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={
        'input': {0: 'batch_size'},
        'output': {0: 'batch_size'}
    }
)
```

### 3.2 ONNX → TFLite (推奨経路)

```python
import onnx
from onnx_tf.backend import prepare
import tensorflow as tf

# ONNXモデルのロード
onnx_model = onnx.load('model.onnx')
tf_rep = prepare(onnx_model)

# TensorFlowモデルとして保存
tf_rep.export_graph('model_tf')

# TFLite変換
converter = tf.lite.TFLiteConverter.from_saved_model('model_tf')
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.float16]  # まずFP16で変換
tflite_model = converter.convert()

with open('model.tflite', 'wb') as f:
    f.write(tflite_model)
```

---

## 4. TensorFlowモデルからの変換

### 4.1 Keras (H5) → TFLite

```python
import tensorflow as tf

# Kerasモデルのロード
model = tf.keras.models.load_model('model.h5')

# TFLite変換
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open('model.tflite', 'wb') as f:
    f.write(tflite_model)
```

### 4.2 SavedModel → TFLite

```python
import tensorflow as tf

converter = tf.lite.TFLiteConverter.from_saved_model('saved_model_dir')
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open('model.tflite', 'wb') as f:
    f.write(tflite_model)
```

---

## 5. STM32Cube.AIでの変換

### 5.1 STM32CubeMXを使用した変換

**手順**:

1. **STM32CubeMXを起動**
2. **Software Packs** → **Select Components** → **X-CUBE-AI** を有効化
3. **Middleware** → **X-CUBE-AI** → **Core Settings**:
   - Model Path: `model.tflite` を指定
   - Target Device: `STM32N6570`
   - Optimization: `Balanced` または `Time`

4. **Analyze** ボタンをクリック:
   - メモリ使用量の推定
   - 推論時間の予測
   - レイヤー互換性チェック

5. **Validation** タブで精度検証:
   - テストデータで推論実行
   - 元モデルとの精度比較

6. **Generate Code** でコード生成

### 5.2 コマンドラインツール (stedgeai-cli)

```bash
# モデルの分析
stedgeai analyze \
  --model model.tflite \
  --target stm32n6 \
  --optimization balanced

# コード生成
stedgeai generate \
  --model model.tflite \
  --target stm32n6 \
  --optimization time \
  --output ./generated \
  --name ocr_model
```

---

## 6. Neural-ART向け最適化オプション

### 6.1 コンパイラ最適化レベル

| オプション | メモリ | 速度 | 精度 | 用途 |
|-----------|--------|------|------|------|
| `Balanced` | 中 | 中 | 高 | 汎用 |
| `Time` | 高 | **最速** | 高 | リアルタイム |
| `RAM` | **最小** | 低 | 高 | メモリ制約 |

**OCRプロジェクトでの推奨**: `Time` - 5ms推論を実現

### 6.2 Neural-ART固有の最適化

```c
// X-CUBE-AI生成コードの設定例
#define AI_NETWORK_ACTIVATIONS_ALIGNMENT  8
#define AI_NETWORK_WEIGHTS_ALIGNMENT      8
#define AI_NETWORK_USE_NPU                1  // Neural-ART NPU有効化
#define AI_NETWORK_NPU_CLOCK_FREQ         1000000000  // 1GHz
```

---

## 7. モデル検証とデバッグ

### 7.1 精度検証

```python
import numpy as np
import tensorflow as tf

# TFLiteモデルのロード
interpreter = tf.lite.Interpreter(model_path='model.tflite')
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# テストデータでの推論
test_data = np.load('test_images.npy')
for img in test_data:
    interpreter.set_tensor(input_details[0]['index'], img)
    interpreter.invoke()
    output = interpreter.get_tensor(output_details[0]['index'])
    # 精度評価...
```

### 7.2 レイヤー互換性チェック

**Neural-ARTでサポートされる主要レイヤー**:
- ✅ Conv2D, DepthwiseConv2D
- ✅ MaxPool2D, AvgPool2D
- ✅ Dense (Fully Connected)
- ✅ BatchNormalization
- ✅ ReLU, ReLU6, LeakyReLU
- ✅ Concatenate, Add
- ⚠️ カスタムレイヤー（CPU fallback）

**確認方法**:
```bash
stedgeai validate \
  --model model.tflite \
  --target stm32n6 \
  --verbosity 2
```

---

## 8. よくある問題と解決策

### 問題1: Unsupported Layer エラー

**原因**: Neural-ARTが未対応のレイヤー使用

**解決策**:
```python
# TensorFlowモデルで対応レイヤーのみ使用
model = tf.keras.Sequential([
    tf.keras.layers.Conv2D(32, 3, activation='relu'),  # OK
    # tf.keras.layers.Lambda(custom_op)  # NG - カスタム演算は避ける
    tf.keras.layers.Dense(10, activation='softmax')  # OK
])
```

### 問題2: メモリ不足エラー

**原因**: アクティベーションメモリが2.5MBを超過

**解決策**:
1. 入力解像度を下げる (640x480 → 320x240)
2. モデルの幅を削減 (filters数を減らす)
3. Depth-Separable Convolutionを使用

### 問題3: 推論時間が遅い

**原因**: INT8量子化未実施、NPU未活用

**解決策**: 次章「量子化」を参照してINT8変換

---

## 9. OCRプロジェクトでの実装例

### 9.1 EAST Text Detectorの変換

```python
# PyTorch EAST → ONNX
torch.onnx.export(
    east_model,
    dummy_input,
    'east_detector.onnx',
    input_names=['input_image'],
    output_names=['score_map', 'geometry_map'],
    opset_version=11
)

# ONNX → TFLite (INT8量子化は次章)
# ...
```

### 9.2 CRNN Text Recognizerの変換

```python
# PyTorch CRNN → ONNX
torch.onnx.export(
    crnn_model,
    dummy_input,
    'crnn_recognizer.onnx',
    input_names=['input_text_region'],
    output_names=['character_probabilities'],
    opset_version=11
)
```

**実測結果**:
- EAST変換後サイズ: 1.2MB (FP32) → 320KB (INT8)
- CRNN変換後サイズ: 1.8MB (FP32) → 480KB (INT8)
- 合計: 800KB (Flash領域に格納)

---

## 10. ベストプラクティス

### ✅ 推奨事項

1. **TFLite形式を使用** - 最高の互換性
2. **モデル検証を必ず実施** - 精度劣化の早期発見
3. **小さなモデルから始める** - 段階的な最適化
4. **Neural-ART制約を事前確認** - レイヤー互換性

### ❌ 避けるべき事項

1. **カスタムレイヤーの多用** - CPU fallbackで遅延
2. **過度に大きなモデル** - メモリ制約違反
3. **検証なしでの量子化** - 精度劣化リスク
4. **古いopsetバージョン** - 互換性問題

---

## 11. 次のステップ

- **次章**: [04-quantization.md](04-quantization.md) - INT8量子化でさらに高速化
- **関連**: [05-npu-optimization.md](05-npu-optimization.md) - NPU最適化テクニック
- **実例**: [08-ocr-case-study.md](08-ocr-case-study.md) - 本プロジェクトでの実装

---

**更新日**: 2025年9月30日  
**関連Issue**: #4 - Neural-ART NPU統合
