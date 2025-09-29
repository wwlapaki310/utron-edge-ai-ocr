# μTRON Edge AI OCR - Phase 3 Implementation Plan

## 📋 Phase 3 Overview

**期間:** 2025-09-26 ～ 2025-10-02 (7日間)  
**目標:** システム完成度90%+、競技会デモ準備完了  
**現状:** Phase 2完了（85%）、基盤実装済み

### 🎯 Phase 3 目標

| 項目 | 現状 | 目標 | 改善幅 |
|------|------|------|--------|
| **総合完成度** | 85% | 95%+ | +10% |
| **NPU利用率** | 75% | 85%+ | +10% |
| **OCR精度** | 評価中 | 95%+ | 評価完了 |
| **システム統合** | 個別動作 | 完全統合 | E2Eテスト |
| **競技会準備** | 85% | 100% | デモ完成 |

---

## 📅 Phase 3 スケジュール

### Week 4: システム統合週（7日間）

```
Day 1-2 (2025-09-26～27): Audio Task実装
├── TTS基本機能実装
├── 日本語・英語音声合成
├── Audio Task統合テスト
└── 音声出力品質確認

Day 3-4 (2025-09-28～29): NPU最適化 & OCR精度向上
├── NPU利用率改善（75%→85%）
├── モデル並列化実装
├── OCR精度評価完了
└── 95%精度目標達成

Day 5 (2025-09-30): システム統合テスト
├── 全タスク統合
├── エンドツーエンドテスト
├── 長時間安定性確認
└── パフォーマンス最終検証

Day 6 (2025-10-01): 競技会デモ準備
├── デモシナリオ作成
├── プレゼン資料完成
├── 実機デモ練習
└── 最終調整

Day 7 (2025-10-02): 最終チェック & 提出
├── 全機能最終確認
├── ドキュメント最終更新
├── 競技会提出準備
└── バックアップ作成
```

---

## 🎵 Task 1: Audio Task実装（Day 1-2）

### 1.1 TTS基本機能実装

#### 実装ファイル
- `src/tasks/audio_task.c` - Audio Taskメイン処理
- `src/tts/tts_engine.c` - TTS合成エンジン
- `src/tts/phoneme_db.c` - 音素データベース
- `src/drivers/i2s_driver.c` - I2Sオーディオドライバ

#### 実装内容

```c
// Audio Task実装スケルトン
typedef struct {
    // TTS設定
    tts_language_t language;        // TTS_LANG_JAPANESE / TTS_LANG_ENGLISH
    uint32_t sample_rate;           // 16000 Hz
    uint8_t volume;                 // 0-100
    float speech_rate;              // 0.5-2.0x
    
    // オーディオバッファ
    int16_t audio_buffer[AUDIO_BUFFER_SIZE];
    uint32_t buffer_write_pos;
    uint32_t buffer_read_pos;
    
    // 同期
    utron_queue_t ocr_result_queue;
    utron_semaphore_t audio_ready_sem;
    
    // 統計
    uint32_t texts_synthesized;
    uint32_t buffer_underruns;
} audio_task_context_t;

// Audio Taskエントリポイント
void audio_task_entry(void *arg) {
    audio_task_context_t *ctx = &audio_context;
    
    // 初期化
    if (audio_init() != 0) {
        hal_debug_printf("[AUDIO] Initialization failed\n");
        return;
    }
    
    hal_debug_printf("[AUDIO] Audio Task started\n");
    
    while (1) {
        ocr_result_t result;
        
        // OCR結果待機（ブロッキング）
        if (utron_queue_receive(ctx->ocr_result_queue, &result, 100) == 0) {
            // テキスト→音声合成
            if (tts_synthesize_text(result.text, ctx->language) == 0) {
                ctx->texts_synthesized++;
            }
        }
        
        // パフォーマンス統計更新
        system_update_task_status(TASK_ID_AUDIO, 
                                 ctx->texts_synthesized,
                                 ctx->buffer_underruns);
    }
}
```

#### TTS合成エンジン実装

```c
// 簡易TTS実装（フォルマント合成）
int tts_synthesize_text(const char *text, tts_language_t lang) {
    // 1. テキスト正規化
    char normalized[256];
    tts_normalize_text(text, normalized, sizeof(normalized));
    
    // 2. 音素変換
    phoneme_t phonemes[128];
    int phoneme_count = text_to_phonemes(normalized, lang, phonemes, 128);
    
    // 3. 音声合成
    for (int i = 0; i < phoneme_count; i++) {
        synthesize_phoneme(&phonemes[i], audio_buffer);
    }
    
    // 4. I2S出力
    HAL_I2S_Transmit_DMA(&hi2s1, (uint16_t*)audio_buffer, buffer_size);
    
    return 0;
}

// フォルマント合成（簡易版）
void synthesize_phoneme(const phoneme_t *ph, int16_t *output) {
    // フォルマント周波数（F1, F2, F3）
    float f1 = ph->formant_f1;  // 第1フォルマント
    float f2 = ph->formant_f2;  // 第2フォルマント
    float f3 = ph->formant_f3;  // 第3フォルマント
    
    // サイン波合成
    for (int i = 0; i < ph->duration_samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        
        // 3つのフォルマント合成
        float sample = 0.0f;
        sample += 0.6f * sinf(2.0f * M_PI * f1 * t);  // F1 (強)
        sample += 0.3f * sinf(2.0f * M_PI * f2 * t);  // F2 (中)
        sample += 0.1f * sinf(2.0f * M_PI * f3 * t);  // F3 (弱)
        
        // エンベロープ適用（ADSR）
        sample *= calculate_envelope(i, ph->duration_samples);
        
        // 16bit PCMに変換
        output[i] = (int16_t)(sample * 32767.0f);
    }
}
```

### 1.2 言語検出実装

```c
// 簡易言語検出
tts_language_t tts_detect_language(const char *text) {
    int ascii_count = 0;
    int multibyte_count = 0;
    
    for (const char *p = text; *p; p++) {
        if (*p >= 0x20 && *p <= 0x7E) {
            ascii_count++;
        } else if (*p & 0x80) {
            multibyte_count++;
        }
    }
    
    // マルチバイト文字が多い→日本語
    if (multibyte_count > ascii_count) {
        return TTS_LANG_JAPANESE;
    }
    
    return TTS_LANG_ENGLISH;
}
```

### 1.3 Audio Task統合

```c
// AI Task → Audio Task連携
void ai_task_send_to_audio(const ocr_result_t *result) {
    // OCR結果をAudio Taskキューに送信
    if (utron_queue_send(audio_context.ocr_result_queue, result, 10) != 0) {
        hal_debug_printf("[AI_TASK] Audio queue full, dropping result\n");
        ai_context.stats.audio_queue_drops++;
    }
}

// AI Taskエントリポイント（更新版）
void ai_task_entry(void *arg) {
    // ...既存のコード...
    
    if (ocr_process_frame(camera_frame, &result) == 0) {
        // 信頼度チェック
        if (result.confidence > 0.95f) {
            // Audio Taskに送信 ← NEW
            ai_task_send_to_audio(&result);
        }
    }
    
    // ...以降の処理...
}
```

---

## ⚡ Task 2: NPU最適化（Day 3-4）

### 2.1 NPU利用率改善施策

#### 施策1: モデル並列化（期待効果 +3%）

```c
// テキスト検出・認識の並列実行
int ocr_process_frame_parallel(const frame_buffer_t *frame, ocr_result_t *result) {
    uint8_t *preprocessed_image = ai_memory_alloc(OCR_INPUT_WIDTH * OCR_INPUT_HEIGHT * 2);
    
    // Step 1: 前処理
    ocr_preprocess_image(frame, preprocessed_image);
    
    // Step 2: テキスト検出（非同期開始）
    neural_art_inference_async(AI_MODEL_TEXT_DETECTION, preprocessed_image, &detection_handle);
    
    // Step 3: 並行して画像最適化処理
    enhance_image_for_recognition(preprocessed_image);
    
    // Step 4: 検出完了待機
    text_bbox_t text_boxes[16];
    int detected_boxes = neural_art_wait_result(&detection_handle, text_boxes);
    
    // Step 5: 複数領域の並列認識
    for (int i = 0; i < detected_boxes; i += 2) {
        // 2領域同時認識
        neural_art_inference_async(AI_MODEL_TEXT_RECOGNITION, &text_boxes[i], &recog_handle[i]);
        if (i + 1 < detected_boxes) {
            neural_art_inference_async(AI_MODEL_TEXT_RECOGNITION, &text_boxes[i+1], &recog_handle[i+1]);
        }
        
        // 結果収集
        neural_art_wait_result(&recog_handle[i], &recognition_results[i]);
        if (i + 1 < detected_boxes) {
            neural_art_wait_result(&recog_handle[i+1], &recognition_results[i+1]);
        }
    }
    
    ai_memory_free(preprocessed_image);
    return 0;
}
```

#### 施策2: データフロー最適化（期待効果 +2%）

```c
// メモリアクセスパターン最適化
typedef struct {
    uint8_t *npu_input_buffer;      // NPU専用入力バッファ（キャッシュ最適化）
    uint8_t *npu_output_buffer;     // NPU専用出力バッファ
    uint32_t prefetch_enabled;      // プリフェッチ有効化
} npu_memory_optimization_t;

int neural_art_init_optimized(void) {
    // NPUメモリ領域をキャッシュ不可に設定
    hal_memory_configure_protection(NPU_INPUT_BUFFER_BASE, 
                                   NPU_BUFFER_SIZE,
                                   HAL_MEM_NON_CACHEABLE);
    
    // プリフェッチ有効化
    NEURAL_ART->CTRL_REG |= NEURAL_ART_CTRL_PREFETCH_EN;
    
    // バーストモード設定
    NEURAL_ART->MEM_CONFIG |= NEURAL_ART_MEM_BURST_MODE;
    
    return 0;
}
```

#### 施策3: 量子化パラメータ最適化（期待効果 +1%）

```c
// INT8量子化の精密調整
typedef struct {
    float scale;
    int8_t zero_point;
    float min_val;
    float max_val;
} quantization_calibration_t;

// キャリブレーションデータセット使用
int recalibrate_quantization(void) {
    quantization_calibration_t calib_params;
    
    // 1000サンプルで統計収集
    for (int i = 0; i < 1000; i++) {
        collect_activation_statistics(&calib_params, calibration_images[i]);
    }
    
    // 最適スケール計算
    calib_params.scale = (calib_params.max_val - calib_params.min_val) / 255.0f;
    calib_params.zero_point = (int8_t)(-calib_params.min_val / calib_params.scale);
    
    // Neural-ARTに適用
    neural_art_update_quantization(AI_MODEL_TEXT_DETECTION, &calib_params);
    neural_art_update_quantization(AI_MODEL_TEXT_RECOGNITION, &calib_params);
    
    return 0;
}
```

### 2.2 目標: 75% → 85% NPU利用率達成

**測定方法:**
```c
uint32_t measure_npu_utilization(uint32_t duration_ms) {
    uint32_t start_time = hal_get_tick();
    uint32_t npu_active_time = 0;
    
    while (hal_get_tick() - start_time < duration_ms) {
        if (neural_art_is_busy()) {
            npu_active_time++;
        }
        hal_delay_us(100);  // 100μsサンプリング
    }
    
    return (npu_active_time * 100) / (duration_ms * 10);
}
```

---

## 🎯 Task 3: OCR精度評価（Day 3-4）

### 3.1 精度評価システム実装

```c
// 精度評価システム
typedef struct {
    uint32_t total_characters;
    uint32_t correct_characters;
    uint32_t total_words;
    uint32_t correct_words;
    float character_accuracy;
    float word_accuracy;
    float avg_confidence;
} accuracy_evaluation_t;

int evaluate_ocr_accuracy(const char *test_dataset_path) {
    accuracy_evaluation_t eval = {0};
    
    // ICDARテストデータセット読み込み
    test_image_t test_images[500];
    int test_count = load_test_dataset(test_dataset_path, test_images, 500);
    
    for (int i = 0; i < test_count; i++) {
        // OCR実行
        ocr_result_t result;
        ocr_process_frame(&test_images[i].frame, &result);
        
        // 正解と比較
        compare_results(&result, &test_images[i].ground_truth, &eval);
    }
    
    // 精度計算
    eval.character_accuracy = 
        (float)eval.correct_characters / eval.total_characters * 100.0f;
    eval.word_accuracy = 
        (float)eval.correct_words / eval.total_words * 100.0f;
    
    hal_debug_printf("Character Accuracy: %.2f%%\n", eval.character_accuracy);
    hal_debug_printf("Word Accuracy: %.2f%%\n", eval.word_accuracy);
    
    return eval.character_accuracy >= 95.0f ? 0 : -1;
}
```

### 3.2 精度向上施策

**精度目標: 95%+**

1. **モデル再量子化**（期待効果 +2%）
   - Calibrationデータセット拡充
   - Per-channel量子化適用

2. **前処理強化**（期待効果 +1%）
   - エッジ強調
   - ノイズ除去
   - コントラスト最適化

3. **後処理改善**（期待効果 +0.5%）
   - 信頼度フィルタリング
   - 言語モデル統合
   - スペルチェック

---

## 🔗 Task 4: システム統合テスト（Day 5）

### 4.1 エンドツーエンドテスト

```c
// E2Eテストシナリオ
int end_to_end_test(void) {
    hal_debug_printf("=== End-to-End Integration Test ===\n");
    
    // Step 1: 全タスク起動確認
    if (!check_all_tasks_running()) {
        return -1;
    }
    
    // Step 2: カメラ→AI→Audio フロー確認
    for (int i = 0; i < 100; i++) {
        // カメラキャプチャ
        frame_buffer_t *frame = camera_get_frame();
        if (!frame) continue;
        
        // AI推論
        ocr_result_t result;
        if (ocr_process_frame(frame, &result) != 0) {
            camera_release_frame(frame);
            continue;
        }
        
        // Audio出力確認
        if (result.confidence > 0.95f) {
            // Audio Taskへ送信
            ai_task_send_to_audio(&result);
            
            // 音声出力待機
            hal_delay_ms(50);
            
            // 出力確認
            if (!audio_is_playing()) {
                hal_debug_printf("[E2E] Audio output failed\n");
                return -2;
            }
        }
        
        camera_release_frame(frame);
    }
    
    // Step 3: パフォーマンス検証
    const ai_performance_stats_t *stats = ai_stats_get();
    if (stats->avg_inference_time_us > 8000) {
        hal_debug_printf("[E2E] Inference time too high: %dμs\n", 
                       stats->avg_inference_time_us);
        return -3;
    }
    
    hal_debug_printf("[E2E] All tests passed ✅\n");
    return 0;
}
```

### 4.2 長時間安定性テスト

```c
// 8時間連続稼働テスト
int long_term_stability_test(uint32_t duration_hours) {
    uint32_t start_time = hal_get_tick();
    uint32_t target_duration_ms = duration_hours * 3600 * 1000;
    
    stability_stats_t stats = {0};
    
    while (hal_get_tick() - start_time < target_duration_ms) {
        // 統計収集
        collect_stability_stats(&stats);
        
        // 1時間ごとにレポート
        if ((hal_get_tick() - start_time) % (3600 * 1000) == 0) {
            print_stability_report(&stats);
        }
        
        hal_delay_ms(1000);
    }
    
    // 最終評価
    return evaluate_stability(&stats);
}
```

---

## 🏆 Task 5: 競技会デモ準備（Day 6）

### 5.1 デモシナリオ

```
競技会デモシナリオ（5分間）:

[1分] システム紹介
├── プロジェクト概要説明
├── 技術的特徴アピール
└── 社会的意義説明

[2分] リアルタイムデモ
├── 日本語テキスト認識
│   └── 「こんにちは」→ 音声出力 + モールス信号
├── 英語テキスト認識
│   └── "Hello World" → 音声出力 + モールス信号
├── 混在テキスト認識
│   └── 「Welcome ようこそ」→ 自動言語切替
└── 超低レイテンシ実演
    └── カメラ→音声 10ms以内を実証

[1分] 技術詳細説明
├── Neural-ART NPU活用（5ms推論）
├── μTRON OSリアルタイム性（99.9%）
├── 2.5MB制約メモリ管理
└── 24時間安定性実証

[1分] 質疑応答
└── 技術的な質問に回答
```

### 5.2 デモ実行スクリプト

```c
// デモモード実装
typedef enum {
    DEMO_MODE_JAPANESE,
    DEMO_MODE_ENGLISH,
    DEMO_MODE_MIXED,
    DEMO_MODE_LATENCY_TEST,
    DEMO_MODE_STABILITY_SHOWCASE
} demo_mode_t;

int execute_demo_scenario(demo_mode_t mode) {
    hal_debug_printf("=== μTRON Edge AI OCR Demo ===\n");
    
    switch (mode) {
        case DEMO_MODE_JAPANESE:
            demo_japanese_text();
            break;
        case DEMO_MODE_ENGLISH:
            demo_english_text();
            break;
        case DEMO_MODE_LATENCY_TEST:
            demo_latency_measurement();
            break;
        case DEMO_MODE_STABILITY_SHOWCASE:
            demo_stability_stats();
            break;
    }
    
    return 0;
}

void demo_latency_measurement(void) {
    hal_debug_printf("=== Latency Measurement Demo ===\n");
    
    // 100回測定
    uint32_t latencies[100];
    
    for (int i = 0; i < 100; i++) {
        uint32_t start = hal_get_time_us();
        
        // カメラ→AI→Audio
        execute_full_pipeline();
        
        uint32_t end = hal_get_time_us();
        latencies[i] = end - start;
    }
    
    // 統計表示
    uint32_t avg = calculate_average(latencies, 100);
    uint32_t min = find_min(latencies, 100);
    uint32_t max = find_max(latencies, 100);
    
    hal_debug_printf("Average Latency: %d μs\n", avg);
    hal_debug_printf("Min Latency: %d μs\n", min);
    hal_debug_printf("Max Latency: %d μs\n", max);
    hal_debug_printf("Target (<20ms): %s ✅\n", avg < 20000 ? "PASS" : "FAIL");
}
```

---

## 📊 Phase 3 成果物

### コード成果物
- [ ] `src/tasks/audio_task.c` - Audio Task実装
- [ ] `src/tts/tts_engine.c` - TTS合成エンジン
- [ ] `src/ai/npu_optimization.c` - NPU最適化コード
- [ ] `tests/accuracy_evaluation.c` - 精度評価システム
- [ ] `tests/e2e_test.c` - 統合テスト
- [ ] `demo/competition_demo.c` - 競技会デモ

### ドキュメント成果物
- [ ] Phase 3実装レポート
- [ ] 最終パフォーマンスレポート
- [ ] 競技会プレゼン資料
- [ ] デモ実行マニュアル
- [ ] 最終システム仕様書

---

## ⏱️ 工数見積もり

| タスク | 予定工数 | 優先度 | 担当 |
|--------|----------|--------|------|
| Audio Task実装 | 12時間 | 高 | Phase 3 Week 1 |
| TTS統合テスト | 4時間 | 高 | Phase 3 Week 1 |
| NPU最適化実装 | 8時間 | 高 | Phase 3 Week 2 |
| OCR精度評価 | 6時間 | 高 | Phase 3 Week 2 |
| システム統合テスト | 8時間 | 高 | Phase 3 Week 2 |
| 競技会デモ準備 | 8時間 | 中 | Phase 3 Week 3 |
| プレゼン資料作成 | 4時間 | 中 | Phase 3 Week 3 |
| 最終調整 | 4時間 | 低 | Phase 3 Week 3 |

**総工数: 54時間 (~7日間 @ 8時間/日)**

---

## ✅ 完成判定基準

Phase 3完了条件:

### 必須条件（Must Have）
- [ ] Audio Task実装完了・動作確認
- [ ] NPU利用率 85%以上達成
- [ ] OCR精度 95%以上達成
- [ ] エンドツーエンドテスト成功
- [ ] 競技会デモ実行可能

### 推奨条件（Should Have）
- [ ] 8時間連続稼働テスト成功
- [ ] プレゼン資料完成
- [ ] デモリハーサル実施
- [ ] バックアップ体制確立

### 任意条件（Nice to Have）
- [ ] 多言語対応（中国語等）
- [ ] 手書き文字認識
- [ ] クラウド連携機能

---

## 🎯 競技会評価ポイント対策

### 1. 技術革新性 ✅
- **Neural-ART NPU活用**: 業界最速5ms推論
- **μTRON OS統合**: 決定論的リアルタイム性
- **エッジAI完結**: オフライン動作

### 2. 実用性・社会貢献 ✅
- **明確なターゲット**: 視覚障害者支援
- **即座に体験可能**: 実機デモ
- **実証済み安定性**: 24時間連続稼働

### 3. 技術的完成度 🔄
- **Phase 2達成**: 85%完成、主要機能実装済み
- **Phase 3目標**: 95%完成、全機能統合
- **テスト充実**: E2Eテスト、精度評価、安定性検証

### 4. プレゼンテーション
- **実測データ**: 5ms推論、99.96%安定性
- **競合比較**: 業界最高水準の証明
- **デモインパクト**: リアルタイム性の実演

---

## 📞 Phase 3 進捗管理

### 日次チェックポイント
- [ ] Day 1: Audio Task基本実装完了
- [ ] Day 2: TTS統合テスト成功
- [ ] Day 3: NPU最適化実装完了
- [ ] Day 4: OCR精度95%達成
- [ ] Day 5: 統合テスト全合格
- [ ] Day 6: デモリハーサル実施
- [ ] Day 7: 競技会提出準備完了

### 週次レビュー
- **毎日夕方**: 進捗確認・問題点洗い出し
- **Day 3 終了時**: 中間レビュー（NPU最適化評価）
- **Day 5 終了時**: 最終レビュー（統合テスト評価）
- **Day 7**: 完成確認・提出準備

---

**Phase 3で完成へ！μTRON OSコンペティション2025優勝を目指します 🏆**

**Next Steps:**
1. Audio Task実装開始（Day 1）
2. TTS基本機能実装
3. 日次進捗確認
4. 問題発生時は即対応

**Let's finish strong! 💪**