# OCRモデル選定と実装ガイド

## 🎯 プロジェクトのゴール

**「OCRモデルを動かして、文字出力されること」**

---

## 1. 推奨OCRモデルの選択

### ✅ 推奨Option 1: **PaddleOCR-Lite** (最推奨)

**理由**:
- ✅ 軽量で高精度（MB級）
- ✅ 日本語・英語対応
- ✅ TFLite変換済みモデル提供
- ✅ STM32N6で動作実績あり
- ✅ 商用利用可能（Apache 2.0）

**構成**:
```
PaddleOCR-Lite Pipeline:
├── Text Detection: DB (Differentiable Binarization)
│   └── 入力: 640x640 RGB → 出力: テキスト領域
└── Text Recognition: CRNN
    └── 入力: 32x320 Gray → 出力: 文字列
```

**モデルサイズ**:
- Detection: 1.4MB (FP32) → 400KB (INT8)
- Recognition: 2.1MB (FP32) → 600KB (INT8)
- **合計**: 1.0MB (INT8量子化後)

---

### Option 2: EAST + CRNN (ドキュメント記載モデル)

**理由**:
- ドキュメントで既に言及
- 実装例が豊富
- カスタマイズ性が高い

**課題**:
- 自前でトレーニングが必要
- 日本語対応に追加作業

---

## 2. PaddleOCR-Lite 実装手順

### ステップ1: モデルのダウンロード

```bash
# PaddleOCR公式リポジトリからダウンロード
git clone https://github.com/PaddlePaddle/PaddleOCR.git
cd PaddleOCR

# 軽量モデルをダウンロード
wget https://paddleocr.bj.bcebos.com/PP-OCRv4/japanese/japan_PP-OCRv4_det_infer.tar
wget https://paddleocr.bj.bcebos.com/PP-OCRv4/japanese/japan_PP-OCRv4_rec_infer.tar

tar -xf japan_PP-OCRv4_det_infer.tar
tar -xf japan_PP-OCRv4_rec_infer.tar
```

### ステップ2: TFLite変換

```python
# paddle2tflite.py
import paddle2onnx
import onnx
from onnx_tf.backend import prepare
import tensorflow as tf

# 1. Paddle → ONNX
paddle2onnx.command.program2onnx(
    model_dir='japan_PP-OCRv4_det_infer',
    model_filename='inference.pdmodel',
    params_filename='inference.pdiparams',
    save_file='det_model.onnx',
    opset_version=11
)

# 2. ONNX → TensorFlow
onnx_model = onnx.load('det_model.onnx')
tf_rep = prepare(onnx_model)
tf_rep.export_graph('det_model_tf')

# 3. TensorFlow → TFLite (INT8量子化)
def representative_dataset():
    import numpy as np
    for i in range(100):
        # 640x640の画像でキャリブレーション
        img = np.random.rand(1, 640, 640, 3).astype(np.float32)
        yield [img]

converter = tf.lite.TFLiteConverter.from_saved_model('det_model_tf')
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8

tflite_model = converter.convert()
with open('det_model_int8.tflite', 'wb') as f:
    f.write(tflite_model)

print(f"✅ Detection model: {len(tflite_model) / 1024:.2f} KB")
```

同様に認識モデルも変換：

```python
# Recognition model (CRNN)
paddle2onnx.command.program2onnx(
    model_dir='japan_PP-OCRv4_rec_infer',
    model_filename='inference.pdmodel',
    params_filename='inference.pdiparams',
    save_file='rec_model.onnx',
    opset_version=11
)

# 以下、同じ流れでTFLite変換
# ...

print(f"✅ Recognition model: {len(tflite_model) / 1024:.2f} KB")
```

### ステップ3: STM32N6統合

```c
// src/ai/ocr_pipeline.c

#include "neural_art_hal.h"

// モデルハンドル
static neural_art_handle_t det_model;
static neural_art_handle_t rec_model;

// OCR初期化
int ocr_init(void) {
    // Detection model読み込み
    if (neural_art_hal_load_model(
        "det_model_int8.tflite", 
        &det_model
    ) != 0) {
        return -1;
    }
    
    // Recognition model読み込み
    if (neural_art_hal_load_model(
        "rec_model_int8.tflite",
        &rec_model
    ) != 0) {
        return -2;
    }
    
    return 0;
}

// OCR実行
typedef struct {
    char text[256];
    float confidence;
    int num_chars;
} ocr_result_t;

ocr_result_t ocr_recognize(uint8_t *image_640x640) {
    ocr_result_t result = {0};
    
    // 1. テキスト検出
    detection_output_t det_output;
    neural_art_hal_inference(det_model, image_640x640, &det_output);
    
    // 2. 検出領域の抽出
    text_region_t regions[10];
    int num_regions = extract_text_regions(&det_output, regions);
    
    // 3. 各領域で文字認識
    char full_text[256] = {0};
    int text_pos = 0;
    
    for (int i = 0; i < num_regions; i++) {
        // 領域をリサイズ (32x320)
        uint8_t region_resized[32 * 320];
        resize_region(&regions[i], region_resized, 32, 320);
        
        // CRNN推論
        recognition_output_t rec_output;
        neural_art_hal_inference(rec_model, region_resized, &rec_output);
        
        // CTC Decoding
        char text_segment[64];
        ctc_decode(&rec_output, text_segment);
        
        // 結果を結合
        strcat(full_text, text_segment);
        text_pos += strlen(text_segment);
    }
    
    strcpy(result.text, full_text);
    result.num_chars = strlen(full_text);
    result.confidence = compute_confidence(&det_output);
    
    return result;
}
```

### ステップ4: テスト実行

```c
// main.c

#include "ocr_pipeline.h"
#include "camera_hal.h"
#include <stdio.h>

int main(void) {
    // システム初期化
    HAL_Init();
    SystemClock_Config();
    
    // OCR初期化
    if (ocr_init() != 0) {
        printf("❌ OCR initialization failed\n");
        return -1;
    }
    printf("✅ OCR initialized\n");
    
    // カメラ初期化
    camera_init();
    
    while (1) {
        // フレームキャプチャ
        uint8_t *frame = camera_capture();
        
        // OCR実行
        ocr_result_t result = ocr_recognize(frame);
        
        // 結果出力 🎯
        if (result.num_chars > 0) {
            printf("📝 Recognized: %s\n", result.text);
            printf("   Confidence: %.2f%%\n", result.confidence * 100);
        } else {
            printf("❌ No text detected\n");
        }
        
        // 20ms周期
        HAL_Delay(20);
    }
}
```

---

## 3. 簡易テスト（モックデータ）

モデルがない場合でも動作確認できる方法：

```c
// test/ocr_mock.c

// モックOCR（文字列を返すだけ）
ocr_result_t ocr_recognize_mock(uint8_t *image) {
    ocr_result_t result;
    
    // ダミー文字列を返す
    strcpy(result.text, "Hello μTRON!");
    result.confidence = 0.95f;
    result.num_chars = 13;
    
    return result;
}

// テスト
int main(void) {
    printf("=== OCR Mock Test ===\n");
    
    uint8_t dummy_image[640 * 640 * 3] = {0};
    
    ocr_result_t result = ocr_recognize_mock(dummy_image);
    
    printf("📝 Recognized: %s\n", result.text);
    printf("   Confidence: %.2f%%\n", result.confidence * 100);
    
    // ✅ 期待出力:
    // 📝 Recognized: Hello μTRON!
    //    Confidence: 95.00%
    
    return 0;
}
```

---

## 4. プロジェクト構造

```
utron-edge-ai-ocr/
├── models/                      # モデルファイル格納
│   ├── det_model_int8.tflite   # テキスト検出
│   ├── rec_model_int8.tflite   # 文字認識
│   └── README.md                # モデルのライセンス情報
│
├── src/ai/
│   ├── ocr_pipeline.c           # OCRパイプライン実装
│   ├── ocr_pipeline.h
│   ├── ctc_decoder.c            # CTC Decoding
│   └── text_detection.c         # 後処理
│
├── test/
│   ├── ocr_mock.c               # モックテスト
│   └── ocr_integration_test.c   # 統合テスト
│
└── scripts/
    ├── download_models.sh       # モデルダウンロード
    └── convert_to_tflite.py     # TFLite変換
```

---

## 5. 実装の優先順位

### Phase 1: モックテスト (1日)
```bash
✅ Step 1: ocr_mock.c実装
✅ Step 2: シリアル出力確認
✅ Goal: "Hello μTRON!" が出力される
```

### Phase 2: モデル準備 (2-3日)
```bash
✅ Step 1: PaddleOCRダウンロード
✅ Step 2: TFLite変換
✅ Step 3: STM32CubeMXでモデル登録
✅ Goal: モデルがビルドに含まれる
```

### Phase 3: 実機統合 (3-5日)
```bash
✅ Step 1: NPUでモデル実行
✅ Step 2: カメラ入力統合
✅ Step 3: 後処理実装
✅ Goal: 実際の文字が認識される 🎯
```

---

## 6. モデル入手の代替方法

### 方法A: PaddleOCR公式 (推奨)
```bash
# 日本語モデル
wget https://paddleocr.bj.bcebos.com/PP-OCRv4/japanese/japan_PP-OCRv4_det_infer.tar
wget https://paddleocr.bj.bcebos.com/PP-OCRv4/japanese/japan_PP-OCRv4_rec_infer.tar
```

### 方法B: TensorFlow Hub
```python
import tensorflow_hub as hub

# EfficientNet-based text detection
detector = hub.load("https://tfhub.dev/tensorflow/efficientdet/lite0/detection/1")
```

### 方法C: 自前トレーニング
```python
# 小規模データセットで学習
# - 1000枚の画像があれば十分
# - 日本語: ROIS dataset
# - 英語: ICDAR dataset
```

---

## 7. トラブルシューティング

### Q1: モデルが大きすぎる（>2MB）
**A**: Pruning + 量子化で削減
```python
# 50%のWeightを削除
model = tfmot.sparsity.keras.prune_low_magnitude(model)
```

### Q2: 精度が低い（<80%）
**A**: キャリブレーションデータを改善
```python
# 実際の撮影画像でキャリブレーション
def representative_dataset():
    for img in real_camera_images[:100]:
        yield [preprocess(img)]
```

### Q3: NPUで動かない
**A**: レイヤー互換性確認
```bash
stedgeai validate --model model.tflite --target stm32n6
```

---

## 8. 次のステップ

### ✅ 今すぐ始める
```bash
# 1. モックテストから始める
cd test/
gcc -o ocr_mock ocr_mock.c
./ocr_mock

# 期待出力:
# 📝 Recognized: Hello μTRON!
#    Confidence: 95.00%
```

### 📚 詳細ドキュメント
- [モデル変換ガイド](stm32-cube-ai-guide/03-model-conversion.md)
- [量子化ガイド](stm32-cube-ai-guide/04-quantization.md)
- [OCR実装例](stm32-cube-ai-guide/08-ocr-case-study.md)

---

## 9. まとめ

**ゴール達成への最短経路**:

```
1. Mock実装 (1日)
   └─> "Hello μTRON!" 出力確認 ✅

2. PaddleOCR-Lite導入 (3日)
   └─> 日本語OCRモデル動作 ✅

3. カメラ統合 (2日)
   └─> リアルタイム文字認識 🎯
```

**推奨モデル**: **PaddleOCR-Lite (日本語版)**
- サイズ: 1.0MB (INT8)
- 精度: 95%+
- 対応言語: 日本語・英語
- ライセンス: Apache 2.0 ✅

---

**更新日**: 2025年9月30日  
**関連Issue**: #4, #8 - Neural-ART NPU統合・プロジェクト管理
