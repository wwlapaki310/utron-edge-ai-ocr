# μTRON Edge AI OCR - 技術スタック

## 🔧 技術選択と設計根拠

### プラットフォーム選択理由

| 技術要素 | 選択 | 理由 |
|---------|------|------|
| **マイコン** | STM32N6570-DK | Neural-ART NPU搭載、リアルタイム性、μTRON OS対応 |
| **OS** | μTRON OS | 決定論的スケジューリング、日本発の組込みOS、競技会要件 |
| **AI加速** | Neural-ART NPU | 600 GOPS、低消費電力、INT8量子化対応 |
| **開発環境** | STM32CubeIDE | 統合開発環境、デバッグ支援、HAL自動生成 |

## 🧠 AI・機械学習スタック

### OCRモデルアーキテクチャ

```
OCR Pipeline Architecture:

Input Image (640×480 RGB565)
    ▼
┌─────────────────────────────────────┐
│         Preprocessing               │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │Format Conv  │ │ Normalization   │ │
│ │RGB565→RGB888│ │ [0,255]→[0,1]   │ │
│ └─────────────┘ └─────────────────┘ │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │   Resize    │ │ Noise Reduction │ │
│ │ →224×224    │ │  (Optional)     │ │
│ └─────────────┘ └─────────────────┘ │
└─────────────────────────────────────┘
    ▼
┌─────────────────────────────────────┐
│      Neural-ART NPU Inference      │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │ Text Det.   │ │ Text Recog.     │ │
│ │ (EAST/CRAFT)│ │ (CRNN/TrOCR)    │ │
│ │ INT8 Quant  │ │ INT8 Quant      │ │
│ └─────────────┘ └─────────────────┘ │
│ Performance: < 8ms @ 1GHz           │
│ Memory: 2.5MB Activation            │
└─────────────────────────────────────┘
    ▼
┌─────────────────────────────────────┐
│         Postprocessing              │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │Text Extract │ │ Confidence      │ │
│ │& Formatting │ │ Calculation     │ │
│ └─────────────┘ └─────────────────┘ │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │Language Det │ │ Error Correction│ │
│ │(JP/EN Auto) │ │ (Dictionary)    │ │
│ └─────────────┘ └─────────────────┘ │
└─────────────────────────────────────┘
```

### モデル量子化戦略

```c
// Neural-ART NPU 最適化設定
typedef struct {
    // 量子化パラメータ
    quantization_type_t quant_type;  // INT8
    float scale_factor;              // 量子化スケール
    int8_t zero_point;              // ゼロポイント
    
    // NPU設定
    npu_precision_t precision;       // INT8_PRECISION
    npu_memory_mode_t memory_mode;   // OPTIMIZED_FOR_SPEED
    
    // バッチサイズ（リアルタイム用）
    uint32_t batch_size;            // 1 (single image)
    
    // メモリ制約
    uint32_t max_activation_size;   // 2.5MB
    uint32_t max_weight_size;       // 8MB
} npu_config_t;
```

## 🎵 音声合成技術スタック

### Text-to-Speech実装

```
TTS Architecture:

OCR Text Input
    ▼
┌─────────────────────────────────────┐
│        Text Processing             │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │ Language    │ │ Text Normalize  │ │
│ │ Detection   │ │ (Numbers, etc.) │ │
│ └─────────────┘ └─────────────────┘ │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │ Phonetic    │ │ Prosody         │ │
│ │ Conversion  │ │ Estimation      │ │
│ └─────────────┘ └─────────────────┘ │
└─────────────────────────────────────┘
    ▼
┌─────────────────────────────────────┐
│       Audio Synthesis              │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │ Concatenate │ │   Formant       │ │
│ │ Synthesis   │ │  Synthesis      │ │
│ │ (Pre-rec)   │ │  (Real-time)    │ │
│ └─────────────┘ └─────────────────┘ │
│ Quality: 16kHz, 16bit               │
│ Latency: < 5ms                      │
└─────────────────────────────────────┘
    ▼
┌─────────────────────────────────────┐
│       Audio Output                 │
│ ┌─────────────┐ ┌─────────────────┐ │
│ │ I2S/SAI     │ │ Volume Control  │ │
│ │ Interface   │ │ & EQ            │ │
│ └─────────────┘ └─────────────────┘ │
└─────────────────────────────────────┘
```

### オーディオコーデック設定

```c
// I2S/SAI オーディオ設定
typedef struct {
    uint32_t sample_rate;        // 16kHz
    uint32_t bit_depth;          // 16bit
    uint32_t channels;           // Mono (1ch)
    audio_format_t format;       // PCM
    
    // バッファ設定
    uint32_t buffer_size;        // 512 samples
    uint32_t buffer_count;       // 4 buffers (ping-pong)
    
    // 品質設定
    uint8_t enable_noise_gate;   // ノイズゲート
    uint8_t enable_compressor;   // 動的圧縮
    float output_gain;           // 出力ゲイン
} audio_config_t;
```

## 🔌 ハードウェアインターフェース

### MIPI CSI-2 カメラインターフェース

```c
// MIPI CSI-2 設定
typedef struct {
    // Physical Layer
    uint32_t lane_count;         // 2-lane
    uint32_t bit_rate_mbps;      // 800 Mbps/lane
    
    // Protocol Configuration
    mipi_data_type_t data_type;  // RGB565
    uint32_t virtual_channel;    // VC0
    
    // Timing Parameters
    uint32_t blanking_h;         // Horizontal blanking
    uint32_t blanking_v;         // Vertical blanking
    uint32_t sync_mode;          // Master mode
    
    // Error Detection
    uint8_t enable_ecc;          // Error Correction Code
    uint8_t enable_crc;          // Cyclic Redundancy Check
} mipi_csi2_config_t;

// ISP (Image Signal Processor) パイプライン
typedef struct {
    // Auto Functions
    uint8_t auto_exposure;       // 自動露出
    uint8_t auto_white_balance;  // 自動ホワイトバランス
    uint8_t auto_focus;          // 自動フォーカス
    
    // Image Enhancement
    uint8_t denoise_level;       // ノイズ除去レベル (0-10)
    uint8_t sharpness;           // シャープネス (0-10)
    uint8_t contrast;            // コントラスト (0-10)
    
    // OCR最適化
    uint8_t text_enhancement;    // テキスト強調
    uint8_t edge_enhancement;    // エッジ強調
} isp_config_t;
```

### ソレノイド制御インターフェース

```c
// ソレノイド制御システム
typedef struct {
    // Hardware Control
    gpio_pin_t enable_pin;       // イネーブルピン
    gpio_pin_t direction_pin;    // 方向制御ピン
    tim_channel_t pwm_channel;   // PWM制御チャンネル
    
    // Safety Parameters
    uint32_t max_pulse_ms;       // 最大パルス幅 (2000ms)
    uint32_t min_interval_ms;    // 最小間隔 (50ms)
    uint32_t thermal_limit;      // 温度制限 (70°C)
    
    // Performance Tuning
    uint32_t rise_time_us;       // 立ち上がり時間
    uint32_t fall_time_us;       // 立ち下がり時間
    uint8_t duty_cycle;          // PWMデューティ比
} solenoid_hw_config_t;
```

## ⚡ リアルタイムシステム設計

### μTRON OSタスク構成

```c
// タスク定義構造体
typedef struct {
    char name[16];               // タスク名
    void (*entry_func)(void*);   // エントリポイント
    uint32_t stack_size;         // スタックサイズ
    uint8_t priority;            // 優先度 (1=最高)
    uint32_t period_ms;          // 実行周期
    uint32_t deadline_ms;        // デッドライン
    uint8_t preemptible;         // プリエンプション可否
} task_definition_t;

// システムタスク一覧
static const task_definition_t system_tasks[] = {
    // Emergency Handler (最高優先度)
    {
        .name = "emergency",
        .entry_func = emergency_task_entry,
        .stack_size = 1024,
        .priority = TASK_PRIORITY_EMERGENCY,
        .period_ms = 0,          // イベント駆動
        .deadline_ms = 1,        // 1ms以内
        .preemptible = 0         // プリエンプション不可
    },
    
    // Camera Task (高優先度)
    {
        .name = "camera",
        .entry_func = camera_task_entry,
        .stack_size = CAMERA_TASK_STACK_SIZE,
        .priority = TASK_PRIORITY_CAMERA,
        .period_ms = 20,         // 50 FPS
        .deadline_ms = 20,       // 厳密なデッドライン
        .preemptible = 1
    },
    
    // AI Task (中優先度)
    {
        .name = "ai_inference",
        .entry_func = ai_task_entry,
        .stack_size = AI_TASK_STACK_SIZE,
        .priority = TASK_PRIORITY_AI,
        .period_ms = 0,          // イベント駆動
        .deadline_ms = 10,       // 10ms以内
        .preemptible = 1
    },
    
    // Audio Task (中低優先度)
    {
        .name = "audio_output",
        .entry_func = audio_task_entry,
        .stack_size = AUDIO_TASK_STACK_SIZE,
        .priority = TASK_PRIORITY_OUTPUT,
        .period_ms = 0,          // イベント駆動
        .deadline_ms = 5,        // 5ms以内
        .preemptible = 1
    },
    
    // Solenoid Task (中低優先度)
    {
        .name = "solenoid",
        .entry_func = solenoid_task_entry,
        .stack_size = SOLENOID_TASK_STACK_SIZE,
        .priority = TASK_PRIORITY_OUTPUT,
        .period_ms = 1,          // 1ms精度
        .deadline_ms = 1,        // 1ms以内
        .preemptible = 1
    },
    
    // System Monitor (最低優先度)
    {
        .name = "system",
        .entry_func = system_task_entry,
        .stack_size = SYSTEM_TASK_STACK_SIZE,
        .priority = TASK_PRIORITY_SYSTEM,
        .period_ms = 100,        // 100ms周期
        .deadline_ms = 100,      // ソフトデッドライン
        .preemptible = 1
    }
};
```

### スケジューリング戦略

```
Rate Monotonic Scheduling with Priority Inheritance:

┌─────────────────────────────────────────────────────────┐
│ Time Slot Allocation (20ms period)                     │
├─────────────────────────────────────────────────────────┤
│ 0-1ms   : Emergency Handler (if triggered)             │
│ 1-6ms   : Camera Task (capture + preprocessing)        │
│ 6-16ms  : AI Task (Neural-ART inference)              │
│ 16-18ms : Audio Task (TTS synthesis)                   │
│ 18-19ms : Solenoid Task (Morse output)                │
│ 19-20ms : System Task (monitoring)                     │
└─────────────────────────────────────────────────────────┘

Worst-Case Execution Time Analysis:
├── Camera Task: 5ms (DMA + ISP processing)
├── AI Task: 10ms (Neural-ART @ 1GHz)
├── Audio Task: 3ms (16kHz synthesis)
├── Solenoid Task: 1ms (PWM control)
└── System Task: 1ms (monitoring)
Total: 20ms (100% CPU utilization)
```

## 🛠️ 開発ツールチェイン

### 開発環境構成

```
Development Environment:

┌─────────────────────────────────────────────────────────┐
│                  Host Development PC                    │
│ OS: Windows 10/11 or Ubuntu 20.04 LTS                 │
├─────────────────────────────────────────────────────────┤
│ Primary IDE:                                           │
│ ├── STM32CubeIDE 1.15.0+                             │
│ ├── Eclipse CDT based                                 │
│ └── Integrated debugger (ST-Link)                     │
├─────────────────────────────────────────────────────────┤
│ Code Generation:                                       │
│ ├── STM32CubeMX (HAL generation)                      │
│ ├── STM32CubeN6 (Platform package)                    │
│ └── μTRON OS SDK                                       │
├─────────────────────────────────────────────────────────┤
│ AI Tools:                                             │
│ ├── STM32Cube.AI 10.0+                               │
│ ├── Neural Network Tools                              │
│ ├── Model Quantization Tools                          │
│ └── Performance Analyzer                              │
├─────────────────────────────────────────────────────────┤
│ Debugging & Analysis:                                 │
│ ├── ST-Link Debugger                                  │
│ ├── SWV (System Workbench Viewer)                     │
│ ├── FreeRTOS Trace Tools                              │
│ └── Power Consumption Analyzer                        │
└─────────────────────────────────────────────────────────┘
```

### ビルドシステム

```makefile
# Makefile configuration
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Compiler flags
CFLAGS = -mcpu=cortex-m55 \
         -mthumb \
         -mfpu=fpv5-sp-d16 \
         -mfloat-abi=hard \
         -Os \
         -Wall \
         -fdata-sections \
         -ffunction-sections

# Linker flags
LDFLAGS = -T$(LDSCRIPT) \
          -Wl,--gc-sections \
          -Wl,--print-memory-usage \
          --specs=nano.specs

# Neural-ART library
NEURAL_ART_LIB = -lneural_art_n6
AI_INCLUDES = -Iai/include

# μTRON OS library
UTRON_LIB = -lutron_n6
UTRON_INCLUDES = -Iutron/include

# Build targets
all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

# Size analysis
size: $(TARGET).elf
    @$(SIZE) --format=berkeley $<
    @$(SIZE) --format=sysv $<
```

### デバッグ設定

```json
// launch.json (VS Code/STM32CubeIDE)
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "STM32 Debug",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "stlink",
            "cwd": "${workspaceRoot}",
            "executable": "./build/utron-edge-ai-ocr.elf",
            "device": "STM32N657X0H3Q",
            "interface": "swd",
            "serialNumber": "",
            "runToMain": true,
            "svdFile": "./STM32N657.svd",
            
            // Real-time tracing
            "swoConfig": {
                "enabled": true,
                "cpuFrequency": 800000000,
                "swoFrequency": 4000000,
                "source": "probe",
                "decoders": [
                    { "port": 0, "type": "console" },
                    { "port": 1, "type": "binary" }
                ]
            },
            
            // Performance monitoring
            "rttConfig": {
                "enabled": true,
                "address": "auto",
                "decoders": [
                    {
                        "port": 0,
                        "type": "console",
                        "label": "RTT Console"
                    }
                ]
            }
        }
    ]
}
```

## 📊 性能測定・評価

### ベンチマーク設定

```c
// Performance measurement framework
typedef struct {
    uint32_t start_time;
    uint32_t end_time;
    uint32_t min_time;
    uint32_t max_time;
    uint32_t avg_time;
    uint32_t sample_count;
} perf_counter_t;

// システム全体のパフォーマンス指標
typedef struct {
    // レイテンシ測定
    perf_counter_t camera_capture;
    perf_counter_t ai_inference;
    perf_counter_t audio_synthesis;
    perf_counter_t end_to_end;
    
    // リソース使用率
    uint32_t cpu_utilization;
    uint32_t memory_usage;
    uint32_t npu_utilization;
    
    // エラー統計
    uint32_t frame_drops;
    uint32_t inference_errors;
    uint32_t audio_underruns;
} system_performance_t;
```

この技術スタックにより、20ms以下のエンドツーエンドレイテンシを実現し、リアルタイムOCRシステムを構築できます。
