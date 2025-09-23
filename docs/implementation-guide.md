# μTRON Edge AI OCR - 実装ポイント & ベストプラクティス

## 🎯 重要な実装ポイント

### 1. リアルタイム性の確保

#### タスク間同期の最適化

```c
// ✅ 良い例: 非ブロッキング同期
int camera_task_entry(void *arg)
{
    while(1) {
        // フレームキャプチャ (DMA使用)
        if (camera_start_dma_capture() == 0) {
            
            // セマフォで通知 (ノンブロッキング)
            utron_signal_semaphore(sem_image_ready);
            
            // 次のフレームまで待機 (正確な20ms周期)
            utron_delay_until(next_wake_time);
            next_wake_time += 20; // ms
        }
    }
}

// ❌ 悪い例: ブロッキング同期
int bad_camera_task(void *arg)
{
    while(1) {
        // 同期待ちでブロック (デッドライン違反リスク)
        utron_wait_semaphore(sem_ai_complete, INFINITE);
        
        // 不定期なキャプチャ (ジッター発生)
        camera_capture_blocking();
        utron_delay(20); // 固定遅延 (ドリフト発生)
    }
}
```

#### メモリアクセス最適化

```c
// DMAと競合しないメモリアクセスパターン
typedef struct {
    // キャッシュライン境界に配置 (64 bytes aligned)
    uint8_t frame_data[CAMERA_FRAME_SIZE] __attribute__((aligned(64)));
    
    // DMA使用中フラグ (volatile必須)
    volatile uint8_t dma_in_progress;
    
    // メタデータ (別キャッシュライン)
    uint32_t timestamp;
    uint32_t sequence_number;
} optimized_frame_buffer_t;

// キャッシュ管理
void camera_dma_complete_handler(void)
{
    // DMA完了後、CPUキャッシュを無効化
    SCB_InvalidateDCache_by_Addr(
        (uint32_t*)current_frame->frame_data, 
        CAMERA_FRAME_SIZE);
    
    current_frame->dma_in_progress = 0;
    utron_signal_semaphore(sem_image_ready);
}
```

### 2. Neural-ART NPU活用

#### モデル量子化の実装

```c
// Neural-ART向け量子化設定
typedef struct {
    // INT8量子化パラメータ
    float input_scale;      // 入力スケール
    int8_t input_zero_point; // 入力ゼロポイント
    float output_scale;     // 出力スケール
    int8_t output_zero_point; // 出力ゼロポイント
    
    // レイヤー別量子化
    struct {
        float weight_scale[MAX_LAYERS];
        float bias_scale[MAX_LAYERS];
    } per_layer;
} quantization_params_t;

// Neural-ART NPUセットアップ
int neural_art_setup(const quantization_params_t *params)
{
    neural_art_config_t config = {
        .precision = NEURAL_ART_INT8,
        .memory_mode = NEURAL_ART_OPTIMIZED_SPEED,
        .power_mode = NEURAL_ART_HIGH_PERFORMANCE,
        
        // メモリマッピング
        .weights_addr = (uint32_t)&model_weights_flash,
        .activation_addr = (uint32_t)&activation_memory_psram,
        .activation_size = AI_ACTIVATION_SIZE,
        
        // パフォーマンス設定
        .prefetch_enabled = 1,
        .cache_policy = NEURAL_ART_CACHE_WRITE_THROUGH,
    };
    
    return neural_art_init(&config);
}
```

#### 推論パイプライン最適化

```c
// 効率的な推論パイプライン
int ai_inference_optimized(const uint8_t *image_data)
{
    static neural_art_session_t session;
    static uint8_t preprocessing_buffer[224*224*3] __attribute__((aligned(32)));
    
    // 1. 前処理 (CPU並列処理)
    preprocess_image_parallel(image_data, preprocessing_buffer);
    
    // 2. NPUへのデータ転送 (DMA使用)
    neural_art_load_input_dma(&session, preprocessing_buffer);
    
    // 3. 推論実行 (NPU)
    neural_art_run_inference(&session);
    
    // 4. 結果取得 (DMA使用)
    neural_art_get_output_dma(&session, inference_results);
    
    // 5. 後処理 (CPU)
    return postprocess_ocr_results(inference_results);
}

// 前処理の並列化
void preprocess_image_parallel(const uint8_t *input, uint8_t *output)
{
    // SIMD命令活用 (Cortex-M55 Helium)
    for (int i = 0; i < IMAGE_SIZE; i += 16) {
        // 16ピクセル同時処理
        uint8x16_t pixels = vld1q_u8(&input[i]);
        
        // 正規化: [0,255] → [0,1]
        float32x4_t normalized = vcvtq_f32_u32(
            vmovl_u16(vget_low_u16(vmovl_u8(vget_low_u8(pixels)))));
        normalized = vmulq_n_f32(normalized, 1.0f/255.0f);
        
        // 量子化: float32 → int8
        int8x16_t quantized = vqmovn_s16(
            vcombine_s16(
                vqmovn_s32(vcvtq_s32_f32(vmulq_n_f32(normalized, 127.0f))),
                vqmovn_s32(vcvtq_s32_f32(vmulq_n_f32(normalized, 127.0f)))
            ));
        
        vst1q_s8((int8_t*)&output[i], quantized);
    }
}
```

### 3. 音声合成の低レイテンシ実装

#### ストリーミング音声合成

```c
// リアルタイム音声合成
typedef struct {
    // 音素データベース (Flash ROM)
    const phoneme_data_t *phoneme_db;
    
    // ストリーミングバッファ (SRAM)
    int16_t audio_buffer[AUDIO_BUFFER_SIZE];
    volatile uint32_t write_pos;
    volatile uint32_t read_pos;
    
    // 状態管理
    tts_state_t state;
} streaming_tts_t;

int tts_synthesize_streaming(const char *text)
{
    // 1. 音素変換 (リアルタイム)
    phoneme_sequence_t phonemes;
    text_to_phonemes(text, &phonemes);
    
    // 2. 並列合成
    for (int i = 0; i < phonemes.count; i++) {
        // 音素データ取得 (Flash読み込み、キャッシュ最適化)
        const phoneme_data_t *ph = get_phoneme_data(phonemes.data[i]);
        
        // バッファにストリーミング出力
        stream_phoneme_to_buffer(ph, &tts_context);
        
        // バッファフル時は出力開始
        if (get_buffer_fill_level() > STREAM_THRESHOLD) {
            start_audio_output_dma();
        }
    }
    
    return 0;
}

// 音素ストリーミング (低レイテンシ)
void stream_phoneme_to_buffer(const phoneme_data_t *phoneme, streaming_tts_t *ctx)
{
    // 音素データの補間合成
    for (int sample = 0; sample < phoneme->length; sample++) {
        // フォルマント合成
        int16_t audio_sample = synthesize_formant(
            phoneme->formants, 
            phoneme->pitch_curve[sample],
            sample);
        
        // バッファに書き込み (リングバッファ)
        ctx->audio_buffer[ctx->write_pos] = audio_sample;
        ctx->write_pos = (ctx->write_pos + 1) % AUDIO_BUFFER_SIZE;
        
        // オーバーラン検出
        if (ctx->write_pos == ctx->read_pos) {
            // Buffer overrun - drop oldest samples
            ctx->read_pos = (ctx->read_pos + 1) % AUDIO_BUFFER_SIZE;
        }
    }
}
```

### 4. モールス信号の精密制御

#### 高精度タイミング制御

```c
// モールス信号の精密タイミング制御
typedef struct {
    // ハードウェアタイマー設定
    TIM_HandleTypeDef *precision_timer;  // 1μs精度
    
    // モールスパターンキュー
    morse_element_t pattern_queue[MAX_QUEUE_SIZE];
    volatile uint32_t queue_head;
    volatile uint32_t queue_tail;
    
    // 現在の状態
    morse_state_t current_state;
    uint32_t current_duration_us;
    uint32_t remaining_time_us;
} precision_morse_t;

// タイマー割り込みハンドラ (1μs精度)
void TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == PRECISION_TIMER) {
        morse_context.remaining_time_us--;
        
        if (morse_context.remaining_time_us == 0) {
            // 現在の要素完了
            morse_complete_current_element();
            
            // 次の要素開始
            if (!morse_queue_empty()) {
                morse_start_next_element();
            }
        }
    }
}

// ソレノイド制御 (PWM + 安全機能)
void solenoid_control_safe(solenoid_id_t id, uint32_t duration_us)
{
    solenoid_control_t *sol = &solenoids[id];
    
    // 安全チェック
    if (duration_us > MAX_PULSE_DURATION_US) {
        error_handler(ERROR_SOLENOID_OVERLOAD);
        return;
    }
    
    // サーマルプロテクション
    uint32_t elapsed = get_system_time_us() - sol->last_activation;
    if (elapsed < MIN_COOLDOWN_TIME_US) {
        // Thermal protection triggered
        return;
    }
    
    // PWM制御開始
    HAL_TIM_PWM_Start(&htim_solenoid, sol->pwm_channel);
    
    // 安全タイマー設定 (ハードウェア保護)
    HAL_TIM_Base_Start_IT(&htim_safety);
    htim_safety.Init.Period = duration_us;
    
    sol->last_activation = get_system_time_us();
    sol->state = SOLENOID_STATE_ACTIVE;
}
```

## 🚀 パフォーマンス最適化テクニック

### CPU使用率最適化

```c
// CPU使用率監視
typedef struct {
    uint32_t idle_count;
    uint32_t total_count;
    uint32_t last_measurement;
    float cpu_utilization;
} cpu_monitor_t;

void system_idle_hook(void)
{
    // アイドル時のカウンタ増加
    cpu_monitor.idle_count++;
}

void calculate_cpu_utilization(void)
{
    uint32_t current_time = get_system_time_ms();
    uint32_t elapsed = current_time - cpu_monitor.last_measurement;
    
    if (elapsed >= 1000) { // 1秒ごと
        cpu_monitor.cpu_utilization = 
            100.0f * (1.0f - (float)cpu_monitor.idle_count / cpu_monitor.total_count);
        
        // カウンタリセット
        cpu_monitor.idle_count = 0;
        cpu_monitor.total_count = 0;
        cpu_monitor.last_measurement = current_time;
    }
}

// CPU使用率が高い場合の対策
void cpu_overload_mitigation(void)
{
    if (cpu_monitor.cpu_utilization > 90.0f) {
        // 1. AI推論頻度を下げる
        ai_inference_skip_count++;
        
        // 2. 音声合成品質を下げる
        reduce_audio_quality();
        
        // 3. システム監視頻度を下げる
        system_monitor_period_ms *= 2;
    }
}
```

### メモリ使用量最適化

```c
// メモリプール管理
typedef struct {
    uint8_t *pool_base;
    uint32_t pool_size;
    uint32_t used_size;
    uint32_t peak_usage;
    memory_block_t *free_list;
} memory_pool_t;

// 高速メモリ割り当て (O(1))
void* fast_malloc(memory_pool_t *pool, size_t size)
{
    // アライメント調整
    size = (size + 7) & ~7;  // 8バイト境界
    
    memory_block_t *block = pool->free_list;
    if (block && block->size >= size) {
        // 最初のフィットブロック使用
        pool->free_list = block->next;
        pool->used_size += block->size;
        
        if (pool->used_size > pool->peak_usage) {
            pool->peak_usage = pool->used_size;
        }
        
        return block->data;
    }
    
    return NULL; // メモリ不足
}

// メモリ断片化防止
void memory_defragmentation(memory_pool_t *pool)
{
    // フリーブロックを結合
    memory_block_t *current = pool->free_list;
    while (current && current->next) {
        if ((uint8_t*)current + current->size == (uint8_t*)current->next) {
            // 隣接ブロックを結合
            current->size += current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
```

## 🔍 デバッグ・テスト戦略

### リアルタイムデバッグ

```c
// SWO (Single Wire Output) トレース
#define SWO_PORT_CAMERA     0
#define SWO_PORT_AI         1  
#define SWO_PORT_AUDIO      2
#define SWO_PORT_SYSTEM     3

// 高速トレース出力 (レイテンシ測定)
static inline void trace_task_start(uint8_t task_id)
{
    uint32_t timestamp = DWT->CYCCNT; // サイクルカウンタ
    ITM_SendChar(SWO_PORT_SYSTEM, 0x01); // Start marker
    ITM_SendChar(SWO_PORT_SYSTEM, task_id);
    ITM_SendData(SWO_PORT_SYSTEM, timestamp);
}

static inline void trace_task_end(uint8_t task_id)
{
    uint32_t timestamp = DWT->CYCCNT;
    ITM_SendChar(SWO_PORT_SYSTEM, 0x02); // End marker
    ITM_SendChar(SWO_PORT_SYSTEM, task_id);
    ITM_SendData(SWO_PORT_SYSTEM, timestamp);
}

// パフォーマンスプロファイリング
void camera_task_entry(void *arg)
{
    while(1) {
        trace_task_start(TASK_ID_CAMERA);
        
        // カメラ処理
        camera_capture_frame();
        
        trace_task_end(TASK_ID_CAMERA);
        
        utron_delay_until(next_wake_time);
    }
}
```

### 自動テストフレームワーク

```c
// ユニットテスト構造
typedef struct {
    const char *test_name;
    int (*test_func)(void);
    uint32_t timeout_ms;
    uint8_t critical;  // 失敗時システム停止
} unit_test_t;

// テストケース定義
static const unit_test_t system_tests[] = {
    {
        .test_name = "Camera DMA Test",
        .test_func = test_camera_dma,
        .timeout_ms = 100,
        .critical = 1
    },
    {
        .test_name = "AI Inference Test", 
        .test_func = test_ai_inference,
        .timeout_ms = 50,
        .critical = 1
    },
    {
        .test_name = "Solenoid Safety Test",
        .test_func = test_solenoid_safety,
        .timeout_ms = 1000,
        .critical = 1
    }
};

// 自動テスト実行
int run_system_tests(void)
{
    int passed = 0;
    int failed = 0;
    
    for (int i = 0; i < ARRAY_SIZE(system_tests); i++) {
        const unit_test_t *test = &system_tests[i];
        
        printf("Running: %s... ", test->test_name);
        
        uint32_t start_time = get_system_time_ms();
        int result = test->test_func();
        uint32_t elapsed = get_system_time_ms() - start_time;
        
        if (result == 0 && elapsed <= test->timeout_ms) {
            printf("PASS (%lums)\n", elapsed);
            passed++;
        } else {
            printf("FAIL (%lums)\n", elapsed);
            failed++;
            
            if (test->critical) {
                printf("Critical test failed! System halt.\n");
                return -1;
            }
        }
    }
    
    printf("Test Results: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : -1;
}
```

## ⚠️ よくある落とし穴と対策

### 1. タスク優先度逆転

```c
// ❌ 問題のあるコード
void low_priority_task(void)
{
    utron_lock_mutex(shared_resource_mutex);
    
    // 長時間の処理 (高優先度タスクをブロック)
    heavy_computation();
    
    utron_unlock_mutex(shared_resource_mutex);
}

// ✅ 優先度継承プロトコル使用
void corrected_low_priority_task(void)
{
    mutex_config_t config = {
        .priority_inheritance = ENABLED,
        .timeout_ms = 10
    };
    
    if (utron_lock_mutex_ex(shared_resource_mutex, &config) == 0) {
        // クリティカルセクションを最小限に
        quick_critical_operation();
        utron_unlock_mutex(shared_resource_mutex);
    }
}
```

### 2. メモリリーク

```c
// ❌ メモリリークの例
void process_image_bad(void)
{
    uint8_t *temp_buffer = malloc(IMAGE_SIZE);
    
    if (preprocess_image(temp_buffer) != 0) {
        return; // メモリリーク!
    }
    
    ai_inference(temp_buffer);
    free(temp_buffer);
}

// ✅ RAIIパターン (C言語版)
typedef struct {
    uint8_t *buffer;
    size_t size;
} auto_buffer_t;

static void auto_buffer_cleanup(auto_buffer_t *buf)
{
    if (buf && buf->buffer) {
        free(buf->buffer);
        buf->buffer = NULL;
    }
}

void process_image_good(void)
{
    auto_buffer_t temp_buffer __attribute__((cleanup(auto_buffer_cleanup))) = {
        .buffer = malloc(IMAGE_SIZE),
        .size = IMAGE_SIZE
    };
    
    if (!temp_buffer.buffer) return;
    
    if (preprocess_image(temp_buffer.buffer) != 0) {
        return; // 自動的にクリーンアップ
    }
    
    ai_inference(temp_buffer.buffer);
    // 自動的にfree()が呼ばれる
}
```

### 3. リアルタイム性の破綻

```c
// ✅ デッドライン監視
typedef struct {
    uint32_t deadline_ms;
    uint32_t start_time;
    uint8_t deadline_missed;
} deadline_monitor_t;

void camera_task_with_monitoring(void *arg)
{
    deadline_monitor_t monitor = {
        .deadline_ms = 20,
        .deadline_missed = 0
    };
    
    while(1) {
        monitor.start_time = get_system_time_ms();
        
        // カメラ処理
        camera_capture_frame();
        
        // デッドラインチェック
        uint32_t elapsed = get_system_time_ms() - monitor.start_time;
        if (elapsed > monitor.deadline_ms) {
            monitor.deadline_missed++;
            
            // 緊急対策
            if (monitor.deadline_missed > 3) {
                // システム負荷軽減
                reduce_system_load();
            }
            
            // ログ出力
            log_deadline_miss("Camera Task", elapsed, monitor.deadline_ms);
        }
        
        utron_delay_until(next_wake_time);
    }
}
```

これらの実装ポイントを遵守することで、安定したリアルタイムOCRシステムを構築できます。
