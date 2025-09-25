# μTRON Edge AI OCR - API Reference

## 📋 概要

本文書は、μTRON Edge AI OCRシステムで実装されたAPI仕様を詳細に説明します。Phase 2で実装されたNeural-ART NPU統合、AIメモリ管理、パフォーマンス監視などの機能へのプログラムインターフェースを提供します。

**実装状況:** Phase 2完了 (85%)  
**対応ファイル:** `src/tasks/ai_task.c`, `src/ai/ai_memory.c`  
**更新日:** 2025-09-25

## 🧠 AI Task Core API

### タスク管理関数

#### `void create_ai_task(void)`
**概要:** AIタスクを作成し、μTRON OSに登録します。

**機能詳細:**
- AIタスクコンテキスト初期化
- 8KB専用スタック割り当て
- 高優先度タスクとして登録 (TASK_PRIORITY_AI)
- デフォルト設定: INT8量子化、95%信頼度閾値、8ms推論制限

**使用例:**
```c
// システム初期化時に呼び出し
create_ai_task();
```

**注意事項:**
- システム初期化時に一度だけ呼び出すこと
- HAL初期化後に呼び出すこと

---

#### `void ai_task_entry(void *arg)`
**概要:** AIタスクのメインエントリポイント。

**機能詳細:**
- AI subsystem初期化
- カメラフレーム待機・処理
- OCR推論実行 (20ms周期)
- パフォーマンス監視
- Audio Taskとの連携

**パラメータ:** なし (μTRON OS仕様)

**実行フロー:**
```
1. AI subsystem初期化
2. カメラフレーム取得待機
3. OCR推論処理
4. 結果をAudio Taskに送信
5. パフォーマンス統計更新
6. システム監視報告
7. 20ms待機後、2に戻る
```

**注意事項:**
- create_ai_task()により自動起動
- 直接呼び出し不可

---

#### `int ai_init(void)`
**概要:** AI サブシステム全体を初期化します。

**戻り値:**
- `0`: 成功
- `AI_ERROR_MEMORY_ALLOC_FAILED`: メモリ初期化失敗
- `AI_ERROR_NPU_ERROR`: Neural-ART NPU初期化失敗
- `AI_ERROR_MODEL_LOAD_FAILED`: OCRモデル読み込み失敗
- `AI_ERROR_INIT_FAILED`: その他初期化失敗

**初期化内容:**
1. AIメモリプール初期化 (2.5MB)
2. Neural-ART NPU初期化 (1GHz)
3. OCRモデル読み込み (EAST + CRNN)
4. パフォーマンス検証 (8ms制約確認)
5. 統計システム初期化

**使用例:**
```c
if (ai_init() != 0) {
    hal_debug_printf("AI初期化失敗\n");
    // エラー処理
}
```

---

### OCR処理関数

#### `int ocr_process_frame(const frame_buffer_t *frame, ocr_result_t *result)`
**概要:** フレームバッファからOCR処理を実行し、テキストを抽出します。

**パラメータ:**
- `frame`: 入力フレームバッファ (640x480 RGB565)
- `result`: OCR結果出力構造体

**戻り値:**
- `0`: 成功
- `AI_ERROR_INPUT_INVALID`: 無効な入力
- `AI_ERROR_MEMORY_ALLOC_FAILED`: メモリ不足
- `AI_ERROR_NPU_ERROR`: NPU推論失敗

**処理フロー:**
```c
// 1. 前処理 (640x480 → 320x240)
int preprocess_result = ocr_preprocess_image(frame, preprocessed_image);

// 2. テキスト検出 (EAST/CRAFT)
int detected_boxes = ocr_detect_text(preprocessed_image, text_boxes, 16);

// 3. テキスト認識 (CRNN)
for (int i = 0; i < detected_boxes; i++) {
    ocr_recognize_text(preprocessed_image, &text_boxes[i], region_text, &confidence);
}

// 4. 結果統合
result->confidence = total_confidence / detected_boxes;
result->language_detected = tts_detect_language(result->text);
```

**パフォーマンス:**
- **実測推論時間**: 5ms (目標 < 8ms) ✅
- **メモリ使用量**: ~2.1MB (制約 < 2.5MB) ✅
- **精度**: 評価システム実装済み

**使用例:**
```c
frame_buffer_t *camera_frame = camera_get_frame();
ocr_result_t result;

int process_result = ocr_process_frame(camera_frame, &result);
if (process_result == 0 && result.confidence > 0.95f) {
    // 高信頼度結果を音声出力へ
    audio_queue_ocr_result(&result);
}
camera_release_frame(camera_frame);
```

---

#### `int ocr_preprocess_image(const frame_buffer_t *input_frame, uint8_t *output_buffer)`
**概要:** カメラ画像をOCR処理用に前処理します。

**パラメータ:**
- `input_frame`: 入力フレーム (640x480 RGB565)
- `output_buffer`: 出力バッファ (320x240 RGB565)

**戻り値:**
- `0`: 成功
- `AI_ERROR_INPUT_INVALID`: 無効な入力

**処理内容:**
- 2x2ダウンサンプリング (640x480 → 320x240)
- RGB565フォーマット維持
- 平均値フィルタリング

**パフォーマンス:** ~1ms

---

#### `int ocr_detect_text(const uint8_t *image, text_bbox_t *bboxes, uint32_t max_boxes)`
**概要:** 画像内のテキスト領域を検出します。

**パラメータ:**
- `image`: 前処理済み画像 (320x240)
- `bboxes`: 検出結果出力配列
- `max_boxes`: 最大検出数 (通常16)

**戻り値:**
- `>= 0`: 検出されたテキスト領域数
- `< 0`: エラーコード

**使用モデル:** EAST/CRAFT (Neural-ART NPU)

**パフォーマンス:** ~2ms

---

#### `int ocr_recognize_text(const uint8_t *image, const text_bbox_t *bbox, char *text_output, float *confidence)`
**概要:** 指定領域のテキストを認識します。

**パラメータ:**
- `image`: 前処理済み画像
- `bbox`: テキスト領域情報
- `text_output`: 認識結果テキスト (64文字まで)
- `confidence`: 信頼度 (0.0-1.0)

**戻り値:**
- `0`: 成功  
- `AI_ERROR_NPU_ERROR`: 認識失敗

**使用モデル:** CRNN (Neural-ART NPU)

**パフォーマンス:** ~3ms

---

### システム状態取得関数

#### `ai_state_t ai_get_state(void)`
**概要:** AI システムの現在状態を取得します。

**戻り値:**
```c
typedef enum {
    AI_STATE_IDLE = 0,           // アイドル状態
    AI_STATE_READY,              // 処理可能状態
    AI_STATE_INFERENCING,        // 推論実行中
    AI_STATE_ERROR               // エラー状態
} ai_state_t;
```

**使用例:**
```c
ai_state_t state = ai_get_state();
if (state == AI_STATE_READY) {
    // OCR処理可能
}
```

---

#### `uint8_t ai_is_result_ready(void)`
**概要:** OCR結果が利用可能かチェックします。

**戻り値:**
- `1`: 結果利用可能
- `0`: 結果なし

---

#### `int ai_get_result(ocr_result_t *result)`
**概要:** 最新のOCR結果を取得します。

**パラメータ:**
- `result`: 結果出力構造体

**戻り値:**
- `0`: 成功
- `AI_ERROR_INPUT_INVALID`: 無効な出力バッファ

**結果構造体:**
```c
typedef struct {
    char text[OCR_MAX_TEXT_LENGTH];  // 認識テキスト (256文字)
    float confidence;                // 全体信頼度 (0.0-1.0)
    uint32_t char_count;             // 文字数
    uint32_t word_count;             // 単語数
    uint32_t bbox_count;             // 検出領域数
    tts_language_t language_detected; // 検出言語
    uint32_t timestamp;              // 処理時刻
} ocr_result_t;
```

---

## 🧠 Memory Management API

### メモリプール管理

#### `int ai_memory_init(void)`
**概要:** AIメモリプール (2.5MB) を初期化します。

**戻り値:**
- `HAL_OK`: 成功
- その他: 初期化失敗

**初期化内容:**
- 2.5MB専用メモリプール設定
- 8バイトアライメント設定
- リーク検出システム初期化

---

#### `void* ai_memory_alloc(uint32_t size)`
**概要:** AIメモリプールから指定サイズを割り当てます。

**パラメータ:**
- `size`: 割り当てサイズ (バイト)

**戻り値:**
- `!= NULL`: 割り当て成功 (データ領域ポインタ)
- `NULL`: 割り当て失敗 (容量不足)

**特徴:**
- 8バイトアライメント自動調整
- リーク検出用ヘッダー自動追加
- マジックナンバーによる破損検出
- 高速線形アロケータ使用

**使用例:**
```c
uint8_t *buffer = ai_memory_alloc(1024);
if (buffer != NULL) {
    // メモリ使用
    ai_memory_free(buffer); // 必須: 解放
}
```

**注意事項:**
- 割り当て後は必ず解放すること
- NULLチェック必須
- スレッドセーフ (簡易mutex使用)

---

#### `void ai_memory_free(void *ptr)`
**概要:** 割り当てたメモリを解放します。

**パラメータ:**
- `ptr`: 解放対象ポインタ

**機能:**
- マジックナンバー検証
- 割り当てリストから除去  
- セキュリティクリア実行

**注意事項:**
- 二重解放検出機能あり
- NULLポインタ渡し可能 (安全)

---

#### `void ai_memory_get_stats(uint32_t *used, uint32_t *free, uint32_t *peak)`
**概要:** メモリ使用統計を取得します。

**パラメータ:**
- `used`: 現在使用量 (バイト)
- `free`: 空き容量 (バイト) 
- `peak`: ピーク使用量 (バイト)

**使用例:**
```c
uint32_t used, free, peak;
ai_memory_get_stats(&used, &free, &peak);
hal_debug_printf("Memory: %d/%d KB (peak: %d KB)\n", 
                used/1024, (used+free)/1024, peak/1024);
```

---

#### `uint32_t ai_memory_check_leaks(void)`
**概要:** メモリリークを検出します。

**戻り値:** 検出されたリーク数

**検出基準:** 30秒以上解放されていないブロック

**使用例:**
```c
uint32_t leaks = ai_memory_check_leaks();
if (leaks > 0) {
    hal_debug_printf("Warning: %d potential memory leaks detected\n", leaks);
}
```

---

## 📊 Performance Monitoring API

### 統計管理

#### `void ai_stats_reset(void)`
**概要:** パフォーマンス統計をリセットします。

**リセット内容:**
- 推論時間統計 (最小・最大・平均)
- 成功・失敗カウント
- 品質統計 (信頼度・精度)
- メモリ使用量統計

---

#### `const ai_performance_stats_t* ai_stats_get(void)`
**概要:** 現在のパフォーマンス統計を取得します。

**戻り値:** 統計構造体への読み取り専用ポインタ

**統計構造体:**
```c
typedef struct {
    // 推論時間統計
    uint32_t total_inferences;          // 総推論回数
    uint32_t successful_inferences;     // 成功回数  
    uint32_t failed_inferences;         // 失敗回数
    uint32_t min_inference_time_us;     // 最小時間 (μs)
    uint32_t max_inference_time_us;     // 最大時間 (μs)
    uint32_t avg_inference_time_us;     // 平均時間 (μs)
    
    // 品質統計
    float avg_confidence_score;         // 平均信頼度
    uint32_t low_confidence_count;      // 低信頼度回数
    uint32_t character_accuracy;        // 文字精度 (%)
    
    // リソース統計
    uint32_t current_memory_usage;      // 現在メモリ使用量
    uint32_t peak_memory_usage;         // ピークメモリ使用量
    uint32_t memory_leaks_detected;     // リーク検出数
} ai_performance_stats_t;
```

**使用例:**
```c
const ai_performance_stats_t *stats = ai_stats_get();
hal_debug_printf("Avg inference time: %d μs\n", stats->avg_inference_time_us);
hal_debug_printf("Success rate: %.1f%%\n", 
                (float)stats->successful_inferences * 100 / stats->total_inferences);
```

---

#### `void ai_stats_update_timing(uint32_t inference_time_us)`
**概要:** 推論時間統計を更新します。

**パラメータ:**
- `inference_time_us`: 推論時間 (マイクロ秒)

**更新内容:**
- 最小・最大時間更新
- 移動平均計算
- 最新推論時間記録

**注意:** 通常は内部で自動呼び出し

---

#### `void ai_stats_update_quality(float confidence, uint32_t accuracy)`
**概要:** 品質統計を更新します。

**パラメータ:**
- `confidence`: 信頼度 (0.0-1.0)
- `accuracy`: 文字精度 (%)

**更新内容:**
- 平均信頼度計算
- 低信頼度結果カウント
- 文字認識精度更新

---

#### `uint8_t ai_stats_check_targets(void)`
**概要:** パフォーマンス目標達成状況をチェックします。

**戻り値:**
- `1`: 全目標達成 ✅
- `0`: 一部目標未達成 ⚠️

**チェック項目:**
- **推論時間**: < 8ms
- **文字精度**: > 95%
- **信頼度**: > 0.95

**使用例:**
```c
if (!ai_stats_check_targets()) {
    hal_debug_printf("Performance targets not met\n");
    // 最適化処理やアラート
}
```

---

## ⚙️ Neural-ART NPU Integration API

### NPU制御関数

#### `int neural_art_init(void)`
**概要:** Neural-ART NPUハードウェアを初期化します。

**戻り値:**
- `0`: 成功
- `< 0`: 初期化失敗

**初期化内容:**
- NPU周辺クロック有効化
- 1GHz動作設定
- 割り込み設定
- メモリ保護設定

---

#### `int neural_art_load_model(ai_model_type_t model_type, const void *model_data, uint32_t size)`
**概要:** AIモデルをNPUメモリに読み込みます。

**パラメータ:**
- `model_type`: モデル種別 (AI_MODEL_TEXT_DETECTION / AI_MODEL_TEXT_RECOGNITION)
- `model_data`: モデルデータ
- `size`: データサイズ

**戻り値:**
- `0`: 成功
- `AI_ERROR_INPUT_INVALID`: 無効なパラメータ

**メモリ配置:** 外部フラッシュに16MB/モデル確保

---

#### `int neural_art_inference(ai_model_type_t model_type, const void *input, void *output)`
**概要:** NPU上でAI推論を実行します。

**パラメータ:**
- `model_type`: 使用モデル
- `input`: 入力データ
- `output`: 出力バッファ

**戻り値:**
- `0`: 成功
- `AI_ERROR_MODEL_LOAD_FAILED`: モデル未ロード
- `AI_ERROR_NPU_ERROR`: NPU実行失敗

**パフォーマンス:** ~5ms (実測値)

---

#### `uint32_t neural_art_get_utilization(void *npu_handle)`
**概要:** NPU使用率を取得します。

**戻り値:** NPU使用率 (0-100%)

**実測値:** ~75% (目標 > 80%)

---

#### `uint8_t neural_art_is_model_ready(neural_art_model_t *model)`
**概要:** モデルの準備状況を確認します。

**戻り値:**
- `1`: モデル準備完了
- `0`: モデル未準備

---

## 🔧 Error Handling & Recovery API

### エラー処理

#### `int ai_recovery_attempt(void)`
**概要:** AIシステムの自動復旧を試行します。

**戻り値:**
- `0`: 復旧成功
- `AI_ERROR_RECOVERY_FAILED`: 復旧失敗

**復旧手順:**
1. NPU電源リセット
2. NPU再初期化
3. モデル再ロード  
4. エラーカウンターリセット
5. セルフテスト実行

**自動起動条件:** 5回連続推論エラー

---

#### `const char* ai_get_last_error(uint32_t *error_code, uint32_t *error_count)`
**概要:** 最新エラー情報を取得します。

**パラメータ:**
- `error_code`: エラーコード出力 (NULL可)
- `error_count`: 連続エラー回数出力 (NULL可)

**戻り値:** エラー説明文字列

**エラーコード一覧:**
```c
#define AI_ERROR_NONE                    0
#define AI_ERROR_INIT_FAILED            -1
#define AI_ERROR_MODEL_LOAD_FAILED      -2
#define AI_ERROR_INFERENCE_TIMEOUT      -3
#define AI_ERROR_MEMORY_ALLOC_FAILED    -4
#define AI_ERROR_INPUT_INVALID          -5
#define AI_ERROR_NPU_ERROR             -6
#define AI_ERROR_CONFIDENCE_TOO_LOW     -7
#define AI_ERROR_RECOVERY_FAILED        -8
```

---

### デバッグ・診断

#### `int ai_self_test(void)`
**概要:** AIシステムの自己診断を実行します。

**戻り値:**
- `0`: 全テストパス ✅
- `-1`: NPU接続失敗
- `-2`: モデル未ロード
- `-3`: メモリ割り当て失敗  
- `-4`: 性能目標未達成

**テスト内容:**
1. NPU接続確認
2. モデルロード確認
3. メモリ割り当てテスト
4. 性能ベンチマーク (10回平均)

---

#### `void ai_set_debug(uint8_t enable)`
**概要:** デバッグ出力を制御します。

**パラメータ:**
- `enable`: 1=有効, 0=無効

---

#### `uint32_t ai_benchmark(uint32_t iterations)`
**概要:** 推論性能ベンチマークを実行します。

**パラメータ:**
- `iterations`: 測定回数

**戻り値:** 平均推論時間 (マイクロ秒)

**テストパターン:** 320x240グレー画像

**実測結果:** ~5ms/推論

---

#### `void ai_dump_state(void)`
**概要:** AIシステムの詳細状態をデバッグ出力します。

**出力内容:**
- 現在状態・統計情報
- メモリ使用状況
- エラー情報
- NPU利用率

**出力例:**
```
[AI_TASK] === AI Task State Dump ===
State: 1 (AI_STATE_READY)
Total inferences: 1245
Successful: 1189 (95.5%)
Failed: 56 (4.5%)
Avg inference time: 5234μs
Memory usage: 2048 KB / 2560 KB (80%)
Peak memory: 2234 KB
NPU utilization: 75%
```

---

## 🔗 Integration APIs

### 他タスクとの連携

#### Audio Task連携
```c
// OCR結果をAudio Taskに送信 (ai_task.c内で自動実行)
audio_queue_ocr_result(&ocr_result);
```

#### System Task連携  
```c
// システム監視への状態報告 (ai_task.c内で自動実行)
system_update_task_status(TASK_ID_AI_TASK, cpu_usage, memory_usage);
system_log_error(ERROR_SEVERITY_ERROR, TASK_ID_AI_TASK, error_code, description, data);
```

#### Camera Task連携
```c
// カメラフレーム取得・解放 (ai_task.c内で自動実行)
frame_buffer_t *frame = camera_get_frame();
// OCR処理
camera_release_frame(frame);
```

---

## 📋 データ構造一覧

### フレームバッファ
```c
typedef struct {
    uint8_t *data;              // 画像データ (RGB565)
    uint32_t size;              // データサイズ
    uint32_t timestamp;         // キャプチャ時刻
    uint8_t ready;              // 処理可能フラグ
} frame_buffer_t;
```

### テキスト境界ボックス
```c
typedef struct {
    uint16_t x, y;              // 左上座標
    uint16_t width, height;     // サイズ
    float confidence;           // 検出信頼度
    uint8_t text_direction;     // テキスト方向 (0=水平)
} text_bbox_t;
```

### AI設定
```c
typedef struct {
    ai_precision_mode_t precision_mode;    // AI_PRECISION_INT8
    float confidence_threshold;           // 0.95 (95%)
    uint32_t max_inference_time_us;       // 8000 (8ms)
    uint8_t debug_enabled;                // デバッグ出力
} ai_config_t;
```

---

## 🚀 使用例・ベストプラクティス

### 基本的なOCR処理
```c
#include "ai_task.h"

void example_ocr_usage(void) {
    // 1. AI Task作成 (初期化時)
    create_ai_task();
    
    // 2. 状態確認
    if (ai_get_state() == AI_STATE_READY) {
        // 3. 結果確認・取得
        if (ai_is_result_ready()) {
            ocr_result_t result;
            if (ai_get_result(&result) == 0) {
                printf("認識テキスト: %s (信頼度: %.2f)\n", 
                       result.text, result.confidence);
            }
        }
    }
}
```

### パフォーマンス監視
```c
void example_performance_monitoring(void) {
    const ai_performance_stats_t *stats = ai_stats_get();
    
    // 性能チェック
    if (!ai_stats_check_targets()) {
        printf("WARNING: Performance targets not met\n");
        printf("Current avg time: %d μs (target: < 8000 μs)\n", 
               stats->avg_inference_time_us);
    }
    
    // メモリ使用状況
    uint32_t used, free, peak;
    ai_memory_get_stats(&used, &free, &peak);
    printf("Memory: %d KB used, %d KB peak\n", used/1024, peak/1024);
    
    // リーク検出
    uint32_t leaks = ai_memory_check_leaks();
    if (leaks > 0) {
        printf("WARNING: %d memory leaks detected\n", leaks);
    }
}
```

### エラーハンドリング
```c
void example_error_handling(void) {
    uint32_t error_code, error_count;
    const char *error_desc = ai_get_last_error(&error_code, &error_count);
    
    if (error_code != AI_ERROR_NONE) {
        printf("AI Error: %s (code: %d, count: %d)\n", 
               error_desc, error_code, error_count);
        
        // 連続エラーが多い場合は手動復旧試行
        if (error_count > 3) {
            printf("Attempting manual recovery...\n");
            if (ai_recovery_attempt() == 0) {
                printf("Recovery successful\n");
            } else {
                printf("Recovery failed - system restart required\n");
            }
        }
    }
}
```

### システム診断
```c
void example_system_diagnostics(void) {
    printf("Running AI system diagnostics...\n");
    
    int test_result = ai_self_test();
    if (test_result == 0) {
        printf("✅ All tests passed\n");
    } else {
        printf("❌ Test failed: %d\n", test_result);
    }
    
    // ベンチマーク実行
    uint32_t avg_time = ai_benchmark(10);
    printf("Benchmark result: %d μs average\n", avg_time);
    
    // 詳細状態出力
    ai_dump_state();
}
```

---

## 🔍 トラブルシューティング

### よくある問題と解決方法

#### 1. 推論時間が8msを超過する
**症状:** `ai_stats_check_targets()`が`0`を返す
**原因:** NPU設定・メモリアクセスパターンの問題
**解決:** 
```c
// NPU利用率確認
uint32_t utilization = neural_art_get_utilization(npu_handle);
if (utilization < 80) {
    // NPU設定見直し・モデル最適化
}
```

#### 2. メモリリーク検出
**症状:** `ai_memory_check_leaks()`が0以外を返す
**原因:** `ai_memory_free()`呼び出し漏れ
**解決:** 割り当て・解放のペアを確認

#### 3. 連続推論エラー
**症状:** 自動復旧が頻発
**原因:** NPUハードウェア・モデルデータの問題  
**解決:** セルフテスト実行・ハードウェア確認

---

## 📊 Performance Benchmarks

### 実測性能データ (Phase 2)

| 項目 | 実測値 | 目標値 | 達成状況 |
|------|--------|--------|----------|
| **推論時間** | 5ms | < 8ms | ✅ 達成 |
| **メモリ使用量** | 2.1MB | < 2.5MB | ✅ 達成 |
| **NPU利用率** | 75% | > 80% | 🔄 継続改善 |
| **文字精度** | 評価中 | > 95% | 🔄 評価システム準備済み |
| **エンドツーエンド** | ~10ms | < 20ms | ✅ 達成 |

### ベンチマーク環境
- **プラットフォーム:** STM32N6570-DK  
- **NPU周波数:** 1GHz
- **テスト画像:** 320x240 グレーパターン
- **測定回数:** 10回平均

---

## 📚 関連資料

- [技術スタック文書](./technical-stack.md) - 実装詳細とアーキテクチャ
- [実装ガイド](./implementation-guide.md) - 開発手順
- [プロジェクト概要](./project-overview.md) - システム全体概要
- [セットアップガイド](./setup.md) - 開発環境構築

---

## 📝 更新履歴

| 日付 | バージョン | 更新内容 |
|------|------------|----------|
| 2025-09-25 | 1.0.0 | 初版作成 (Phase 2実装完了版) |

---

**Copyright © 2025 μTRON Competition Team. All rights reserved.**