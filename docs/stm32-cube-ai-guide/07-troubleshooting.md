# トラブルシューティング完全ガイド

## 概要

STM32N6 Neural-ART NPU開発で遭遇する一般的な問題と、その解決策を体系的にまとめました。本プロジェクトで実際に発生した問題と解決プロセスも含めて解説します。

---

## 1. ビルド・コンパイルエラー

### 問題1.1: "undefined reference to ai_network_run"

**エラーメッセージ**:
```
undefined reference to `ai_network_run'
undefined reference to `ai_network_create'
```

**原因**:
X-CUBE-AIライブラリがリンカに正しく登録されていない

**解決策**:
```bash
# STM32CubeIDEでの設定
Project → Properties → C/C++ Build → Settings:

1. MCU GCC Linker → Libraries:
   - Libraries (-l): NetworkRuntime800_CM55_GCC
   - Library search path (-L):
     "${workspace_loc:/${ProjName}/X-CUBE-AI/Lib}"

2. ビルドをクリーン
   Project → Clean... → Clean all projects

3. 再ビルド
   Project → Build Project (Ctrl+B)
```

**検証**:
```bash
# リンカログで確認
arm-none-eabi-gcc ... -lNetworkRuntime800_CM55_GCC
# ↑このオプションが含まれていればOK
```

---

### 問題1.2: "region RAM overflowed"

**エラーメッセージ**:
```
region `RAM' overflowed by 524288 bytes
make: *** [makefile:64: utron-edge-ai-ocr.elf] Error 1
```

**原因**:
アクティベーションメモリがRAM容量を超過

**解決策**:

#### 方法1: リンカスクリプトでメモリ配置変更
```c
/* STM32N6570_FLASH.ld */

MEMORY
{
  RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 2560K
  FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 2048K
  NPU_RAM (xrw)  : ORIGIN = 0x30000000, LENGTH = 2560K  /* NPU専用 */
}

/* アクティベーションメモリをNPU_RAMに配置 */
.npu_section :
{
  . = ALIGN(8);
  *(.npu_activations)
  . = ALIGN(8);
} >NPU_RAM
```

#### 方法2: モデルサイズ削減
```python
# 入力解像度を下げる
model = create_model(input_shape=(320, 240, 3))  # 640x480 → 320x240

# レイヤー幅を削減
model.add(Conv2D(32, (3,3)))  # 64 → 32 filters
```

**検証**:
```bash
# メモリ使用量の確認
arm-none-eabi-size build/utron-edge-ai-ocr.elf

# 出力例:
   text    data     bss     dec     hex filename
 123456    5678  204800  333934   51876 utron-edge-ai-ocr.elf
#          ↑ bss (RAM使用量) が2.5MB以下であればOK
```

---

### 問題1.3: "Flash overflow"

**エラーメッセージ**:
```
region `FLASH' overflowed by 1048576 bytes
```

**原因**:
AIモデルのWeightがFlash容量（2MB）を超過

**解決策**:

```python
# モデル圧縮・量子化の徹底

# 1. INT8量子化（必須）
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

# 2. Weight共有
model = prune_low_magnitude(model, pruning_schedule)

# 3. 外部Flashの活用（最終手段）
# microSDカードにモデルを配置し、起動時にロード
```

---

## 2. NPU実行時エラー

### 問題2.1: "NPU initialization failed"

**エラーメッセージ**:
```
Neural-ART NPU: FAILED
Error code: NPU_CLOCK_ERROR
```

**原因**:
NPUクロックが正しく設定されていない

**解決策**:
```c
// STM32CubeMXでのクロック設定確認

// RCC設定:
HSE: 25 MHz (Crystal/Ceramic Resonator)
PLL Configuration:
  - PLLM: 4
  - PLLN: 160  // 25MHz * 160 / 4 = 1000MHz
  - PLLP: 1

System Clock Mux: PLLCLK
Neural-ART NPU Clock: 1000 MHz

// コードでの確認:
uint32_t npu_freq = HAL_RCC_GetNPUFreq();
if (npu_freq != 1000000000) {
    printf("NPU Clock Error: %lu Hz\n", npu_freq);
    // クロック再設定
}
```

**検証**:
```c
// NPUレジスタ直接読み取り
uint32_t npu_status = NPU->SR;
if (npu_status & NPU_SR_READY) {
    printf("NPU Ready\n");
} else {
    printf("NPU Not Ready: 0x%08lX\n", npu_status);
}
```

---

### 問題2.2: "Inference timeout"

**エラーメッセージ**:
```
NPU inference timeout after 1000ms
```

**原因**:
1. モデルがFP32のままでCPUフォールバック
2. NPUへのデータ転送が完了していない
3. NPUがハング状態

**解決策**:

#### 原因1: FP32モデルの確認
```bash
# TFLiteモデルの量子化状態確認
python -c "
import tensorflow as tf
interpreter = tf.lite.Interpreter('model.tflite')
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
print('Input dtype:', input_details[0]['dtype'])
# 出力が uint8 ならINT8量子化済み ✓
# 出力が float32 ならFP32（要量子化）✗
"
```

#### 原因2: DMA転送の確認
```c
// DMA完了待ち処理の追加
while (HAL_DMA_GetState(&hdma_npu) != HAL_DMA_STATE_READY) {
    // DMA転送完了待ち
}

// NPU入力バッファのキャッシュクリア
SCB_CleanDCache_by_Addr(npu_input_buffer, input_size);
```

#### 原因3: NPUリセット
```c
// タイムアウト時のNPUリセット処理
static void npu_reset_on_timeout(void) {
    __HAL_RCC_NPU_FORCE_RESET();
    HAL_Delay(10);
    __HAL_RCC_NPU_RELEASE_RESET();
    
    // NPU再初期化
    neural_art_hal_init();
}
```

---

### 問題2.3: "Unsupported layer type"

**エラーメッセージ**:
```
Error: Layer 5 (CustomOp) is not supported by Neural-ART
```

**原因**:
Neural-ARTが非対応の演算子を使用

**対応演算子リスト**:
```
✓ サポート済み:
  - Conv2D, DepthwiseConv2D
  - MaxPool2D, AvgPool2D, GlobalAvgPool2D
  - Dense (Fully Connected)
  - BatchNormalization
  - ReLU, ReLU6, LeakyReLU
  - Add, Concatenate
  - Reshape, Transpose

✗ 非サポート:
  - LSTM, GRU (RNN系)
  - Custom Lambda layers
  - 一部のAttention機構
```

**解決策**:
```python
# モデルの書き換え

# ✗ 非対応: LSTM
model.add(LSTM(128))

# ✓ 代替: 1D Convolution
model.add(Conv1D(128, kernel_size=3, padding='same'))
model.add(Conv1D(128, kernel_size=3, padding='same'))

# ✗ 非対応: Lambda (custom operation)
model.add(Lambda(lambda x: x * 2 + 1))

# ✓ 代替: 標準レイヤーで実装
model.add(tf.keras.layers.Rescaling(scale=2.0, offset=1.0))
```

---

## 3. メモリ関連の問題

### 問題3.1: "Memory allocation failed"

**エラーメッセージ**:
```
ai_memory_alloc: Out of memory
Requested: 524288 bytes
Available: 102400 bytes
```

**デバッグ手順**:
```c
// メモリ使用状況の詳細確認
void debug_memory_usage(void) {
    ai_memory_stats_t stats = ai_memory_get_stats();
    
    printf("=== Memory Stats ===\n");
    printf("Total:  %u KB\n", stats.total_size / 1024);
    printf("Used:   %u KB (%.1f%%)\n", 
           stats.current_used / 1024,
           (float)stats.current_used / stats.total_size * 100);
    printf("Peak:   %u KB\n", stats.peak_used / 1024);
    printf("Free:   %u KB\n", stats.free_size / 1024);
    printf("Fragmentation: %.1f%%\n", stats.fragmentation);
}
```

**解決策**:

#### 方法1: メモリの逐次実行
```c
// EAST + CRNN を同時実行せず、逐次実行

// 1. EAST実行
neural_art_hal_load_model(EAST_MODEL);
east_result = neural_art_hal_inference(input);
neural_art_hal_unload_model(EAST_MODEL);  // メモリ解放

// 2. CRNN実行（同じメモリ領域を再利用）
neural_art_hal_load_model(CRNN_MODEL);
for (region in east_result) {
    crnn_result = neural_art_hal_inference(region);
}
neural_art_hal_unload_model(CRNN_MODEL);
```

#### 方法2: メモリデフラグ
```c
// 定期的なメモリ整理
void ai_memory_defragment(void) {
    // 使用中のブロックを前方にコンパクト化
    memory_block_t *current = free_list;
    while (current != NULL) {
        if (!current->in_use && current->next != NULL && !current->next->in_use) {
            // 隣接する空きブロックを結合
            coalesce_blocks(current);
        }
        current = current->next;
    }
}
```

---

### 問題3.2: "Memory leak detected"

**検出方法**:
```c
// 起動時と定期的にメモリ使用量を記録
static ai_memory_stats_t initial_stats;

void memory_leak_check_start(void) {
    initial_stats = ai_memory_get_stats();
}

void memory_leak_check_periodic(void) {
    ai_memory_stats_t current_stats = ai_memory_get_stats();
    
    int32_t leak = current_stats.current_used - initial_stats.current_used;
    if (leak > 0) {
        printf("⚠️ Memory leak: %d bytes\n", leak);
    }
}
```

**よくあるリーク原因**:
```c
// ✗ 悪い例: free忘れ
void bad_function(void) {
    uint8_t *buffer = ai_memory_alloc(1024);
    // 処理...
    // free忘れ！
}

// ✓ 良い例: 確実にfree
void good_function(void) {
    uint8_t *buffer = ai_memory_alloc(1024);
    if (buffer == NULL) return;
    
    // 処理...
    
    ai_memory_free(buffer);
}

// ✓ さらに良い例: スタック割り当て（小サイズなら）
void best_function(void) {
    uint8_t buffer[1024];  // スタックに確保、自動解放
    // 処理...
}
```

---

## 4. パフォーマンス問題

### 問題4.1: "推論が遅い（>50ms）"

**診断チェックリスト**:
```c
// 各処理の時間を個別に測定

uint32_t t1 = get_time_us();
preprocess();
uint32_t t2 = get_time_us();
printf("Preprocess: %u us\n", t2 - t1);

npu_inference();
uint32_t t3 = get_time_us();
printf("NPU Inference: %u us\n", t3 - t2);

postprocess();
uint32_t t4 = get_time_us();
printf("Postprocess: %u us\n", t4 - t3);

printf("Total: %u us\n", t4 - t1);
```

**一般的なボトルネック**:

| ボトルネック | 症状 | 解決策 |
|------------|------|--------|
| FP32モデル | 推論時間250ms+ | INT8量子化 |
| CPU前処理 | 前処理20ms+ | ISPハードウェアオフロード |
| DMA転送 | 転送10ms+ | バースト転送設定 |
| 後処理 | 後処理10ms+ | Helium SIMD活用 |

**解決例**:
```c
// ISPでの前処理オフロード
void camera_hal_configure_isp(void) {
    ISP_ConfigTypeDef isp = {
        .OutputWidth = 320,
        .OutputHeight = 240,
        .PixelFormat = ISP_PIXEL_FORMAT_RGB888_NORMALIZED,
        .GammaCorrection = ISP_GAMMA_SRGB
    };
    HAL_ISP_Init(&isp);
}

// 結果: 前処理 20ms → 0.5ms
```

---

### 問題4.2: "NPU利用率が低い（<50%）"

**原因分析**:
```c
// NPUプロファイリング
npu_profile_t profile = get_npu_profile();

float compute_ratio = (float)profile.compute_cycles / profile.total_cycles;
float memory_ratio = (float)profile.memory_cycles / profile.total_cycles;

printf("Compute: %.1f%%\n", compute_ratio * 100);
printf("Memory Wait: %.1f%%\n", memory_ratio * 100);

// Memory Wait > 30% ならメモリバンド幅がボトルネック
```

**解決策**:
```c
// 1. Weight圧縮
// STM32Cube.AIで Compression: Lossless を有効化

// 2. アクティベーションメモリ配置最適化
__attribute__((section(".npu_fast_ram")))
static uint8_t activations[2 * 1024 * 1024];

// 3. DMAバースト転送
dma_config.MemBurst = DMA_MBURST_INC16;
dma_config.PeriphBurst = DMA_PBURST_INC16;
```

---

## 5. デバッグツール活用

### 5.1 SWO Traceでのプロファイリング

```c
#include "stdio.h"

// ITM経由でのログ出力（高速）
void profile_function(void) {
    DWT->CYCCNT = 0;  // サイクルカウンタリセット
    
    your_function();
    
    uint32_t cycles = DWT->CYCCNT;
    printf("Cycles: %lu (%.2f ms @ 600MHz)\n", 
           cycles, (float)cycles / 600000);
}
```

### 5.2 SEGGER RTTでのリアルタイムログ

```c
#include "SEGGER_RTT.h"

// 高速・リアルタイムログ（SWOより高速）
void fast_logging(void) {
    uint32_t start = get_time_us();
    
    // 処理...
    
    uint32_t elapsed = get_time_us() - start;
    SEGGER_RTT_printf(0, "Time: %u us\n", elapsed);
}
```

### 5.3 SystemView統合

```c
// SEGGER SystemViewでタスク可視化
#include "SEGGER_SYSVIEW.h"

void ai_task_main(void) {
    SEGGER_SYSVIEW_OnTaskStartExec(AI_TASK_ID);
    
    // AI推論処理
    
    SEGGER_SYSVIEW_OnTaskStopExec();
}
```

---

## 6. よくある質問（FAQ）

### Q1: INT8量子化で精度が大幅に低下（>5%）

**A**: キャリブレーションデータが不適切な可能性

```python
# ✗ 悪い例: ランダムデータ
def bad_dataset():
    for _ in range(100):
        yield [np.random.rand(1, 320, 240, 3)]

# ✓ 良い例: 実データ
def good_dataset():
    for img in real_training_data[:100]:
        yield [preprocess(img)]
```

### Q2: μTRON OSタスクがデッドライン違反

**A**: タスク優先度とスタックサイズの見直し

```c
// タスク優先度の調整
#define CAMERA_TASK_PRIORITY  1  // 最高優先度
#define AI_TASK_PRIORITY      2
#define AUDIO_TASK_PRIORITY   3

// スタックサイズの拡大
#define AI_TASK_STACK_SIZE    (8 * 1024)  // 8KB
```

### Q3: フラッシュ書き込みエラー

**A**: ST-LINKドライバ更新とロック解除

```bash
# Windows: ST-LINK Utilityで確認
ST-LINK_CLI -c SWD -Rst

# Read Protection解除
ST-LINK_CLI -OB RDP=0
```

---

## 7. 緊急時の対処

### システムハング時

```c
// ウォッチドッグタイマーの設定
void setup_watchdog(void) {
    IWDG_HandleTypeDef hiwdg = {
        .Instance = IWDG,
        .Init.Prescaler = IWDG_PRESCALER_64,
        .Init.Reload = 4095  // 約4秒
    };
    HAL_IWDG_Init(&hiwdg);
}

// 定期的にリフレッシュ
void ai_task_main(void) {
    while (1) {
        HAL_IWDG_Refresh(&hiwdg);
        // 処理...
    }
}
```

### ファームウェア復旧

```bash
# Mass Eraseでクリーンインストール
STM32_Programmer_CLI -c port=SWD -e all
STM32_Programmer_CLI -c port=SWD -w firmware.hex -v -rst
```

---

## 8. サポートリソース

### 公式ドキュメント
- [STM32N6 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0489.pdf)
- [X-CUBE-AI Documentation](https://www.st.com/en/embedded-software/x-cube-ai.html)
- [Neural-ART NPU User Guide](https://www.st.com/resource/en/user_manual/um3310.pdf)

### コミュニティ
- [ST Community Forum](https://community.st.com/)
- [μTRON OS Forum](https://www.tron.org/)

### 本プロジェクト
- [GitHub Issues](https://github.com/wwlapaki310/utron-edge-ai-ocr/issues)
- [技術文書](../README.md)

---

**更新日**: 2025年9月30日  
**関連Issue**: #4, #5, #8 - Neural-ART NPU統合・HAL設計・プロジェクト管理
