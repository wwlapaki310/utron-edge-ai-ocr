# μTRON Edge AI OCR - システムアーキテクチャ

## 🏗️ 全体システム構成

### システム概要図

```
┌─────────────────────────────────────────────────────────────────┐
│                    μTRON Edge AI OCR System                     │
├─────────────────────────────────────────────────────────────────┤
│  Input Layer                                                    │
│  ┌─────────────┐    ┌─────────────┐                           │
│  │ MIPI CSI-2  │    │   Physical  │                           │
│  │   Camera    │───▶│  Interface  │                           │
│  │ (640x480)   │    │    (ISP)    │                           │
│  └─────────────┘    └─────────────┘                           │
├─────────────────────────────────────────────────────────────────┤
│  Processing Layer (μTRON OS)                                   │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│  │ Camera Task │───▶│  AI Task    │───▶│ Audio Task  │        │
│  │ (Priority 2)│    │(Priority 3) │    │(Priority 4) │        │
│  │   20ms      │    │Neural-ART   │    │Voice Synth  │        │
│  └─────────────┘    └─────────────┘    └─────────────┘        │
│         │                  │                  │               │
│         ▼                  ▼                  ▼               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│  │System Task  │    │OCR Results  │    │Solenoid Task│        │
│  │(Priority 5) │    │   Queue     │    │(Priority 4) │        │
│  │Monitor/Log  │    │             │    │Morse Output │        │
│  └─────────────┘    └─────────────┘    └─────────────┘        │
├─────────────────────────────────────────────────────────────────┤
│  Output Layer                                                  │
│  ┌─────────────┐    ┌─────────────┐                           │
│  │Audio Output │    │Tactile Out  │                           │
│  │ (Speaker/   │    │ Solenoid×2  │                           │
│  │ Headphone)  │    │(Morse Code) │                           │
│  └─────────────┘    └─────────────┘                           │
└─────────────────────────────────────────────────────────────────┘
```

## 🧩 コンポーネント詳細設計

### 1. ハードウェア層

```
┌────────────────────────────────────────────────────────────┐
│                 STM32N6570-DK Hardware Platform            │
├────────────────────────────────────────────────────────────┤
│ CPU: Cortex-M55 @ 800MHz                                  │
│ NPU: Neural-ART @ 1GHz (600 GOPS)                        │
│ RAM: 4.2MB Internal + External PSRAM                      │
│ Flash: External Octo-SPI Flash                           │
├────────────────────────────────────────────────────────────┤
│ Peripherals:                                              │
│ • MIPI CSI-2 Camera Interface                            │
│ • Image Signal Processor (ISP)                           │
│ • Audio Codec (I2S/SAI)                                  │
│ • GPIO for Solenoid Control                              │
│ • DMA Controllers                                         │
│ • Timers for Precise Timing                              │
└────────────────────────────────────────────────────────────┘
```

### 2. ソフトウェア層構成

```
┌─────────────────────────────────────────────────────────────┐
│                   Application Layer                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │  OCR App    │ │Audio Synth  │ │Tactile FB   │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
├─────────────────────────────────────────────────────────────┤
│                    μTRON OS Layer                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │Task Manager │ │   Memory    │ │ Scheduler   │           │
│  │             │ │ Management  │ │             │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │Semaphores   │ │Message Queue│ │   Timers    │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
├─────────────────────────────────────────────────────────────┤
│                Hardware Abstraction Layer                   │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │Camera HAL   │ │   AI HAL    │ │ Audio HAL   │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
├─────────────────────────────────────────────────────────────┤
│                     STM32 HAL/Drivers                       │
│                    (STM32CubeN6)                           │
└─────────────────────────────────────────────────────────────┘
```

## ⚡ リアルタイム性能要件

### タスク優先度とタイミング

```
Priority 1 (Emergency): 緊急停止・エラーハンドリング
    ├── 応答時間: < 1ms
    └── 用途: ハードウェア保護

Priority 2 (Camera): カメラキャプチャタスク
    ├── 周期: 20ms (50 FPS)
    ├── 実行時間: < 5ms
    └── デッドライン: 厳密

Priority 3 (AI): AI推論タスク
    ├── 実行時間: < 10ms (Neural-ART使用)
    ├── メモリ使用: 2.5MB アクティベーション
    └── 省電力モード対応

Priority 4 (Output): 音声・触覚出力
    ├── 音声合成: < 5ms レイテンシ
    ├── モールス出力: 精密タイミング制御
    └── バッファ管理

Priority 5 (System): システム監視
    ├── 周期: 100ms
    ├── CPU使用率監視
    └── メモリリーク検出
```

## 🔄 データフロー設計

### メインデータパイプライン

```
[Camera] ──(Image Buffer)──▶ [AI Task] ──(OCR Result)──▶ [Output Tasks]
    │                           │                            │
    ▼                           ▼                            ▼
[System Monitor]        [Confidence Check]              [Audio + Tactile]
    │                           │                            │
    ▼                           ▼                            ▼
[Performance Log]       [Error Handling]               [User Feedback]
```

### メモリマップ設計

```
Memory Layout (4.2MB Internal + External):

0x0000_0000 - 0x0010_0000: Code Section (1MB)
0x2000_0000 - 0x2040_0000: Data + BSS (256KB)
0x2040_0000 - 0x2080_0000: Camera Buffers (256KB)
    ├── Frame Buffer 1: 640×480×2 = 614KB
    └── Frame Buffer 2: 640×480×2 = 614KB

External PSRAM:
0x7000_0000 - 0x7280_0000: AI Activation Memory (2.5MB)
0x7280_0000 - 0x7300_0000: Audio Buffers (512KB)
0x7300_0000 - 0x7380_0000: System Buffers (512KB)
```

## 🔧 同期・通信メカニズム

### セマフォ設計

```c
// 画像処理パイプライン用
sem_image_ready      : Binary  (カメラ→AI)
sem_inference_done   : Binary  (AI→出力)
sem_audio_complete   : Binary  (音声出力完了)

// リソース保護用
mutex_frame_buffer   : Mutex   (フレームバッファ保護)
mutex_ocr_result     : Mutex   (OCR結果保護)
```

### メッセージキュー設計

```c
// OCR結果伝達 (AI Task → Output Tasks)
mq_ocr_results: Queue<ocr_result_t>[8]
    ├── 要素サイズ: 272 bytes (text[256] + metadata)
    └── 容量: 8メッセージ

// システム状態監視 (All Tasks → System Task)
mq_system_status: Queue<system_status_t>[4]
    ├── CPU使用率、メモリ使用率
    └── パフォーマンス統計

// エラー情報 (All Tasks → Error Handler)
mq_error_handling: Queue<error_info_t>[8]
    ├── エラーコード、タイムスタンプ
    └── 復旧アクション指示
```

## 🧠 AI推論アーキテクチャ

### Neural-ART NPU活用設計

```
Input Image (640×480 RGB565)
    ▼
Preprocessing (CPU)
    ├── Format Conversion: RGB565 → RGB888
    ├── Normalization: [0,255] → [0,1]
    └── Resize: 640×480 → 224×224 (OCRモデル入力)
    ▼
Neural-ART NPU Inference
    ├── OCR Model: Quantized INT8
    ├── Inference Time: < 8ms
    └── Memory: 2.5MB Activation
    ▼
Postprocessing (CPU)
    ├── Text Extraction
    ├── Confidence Calculation
    └── Result Formatting
```

## 📊 性能目標と制約

### レイテンシ目標

```
End-to-End Latency: < 20ms
├── Camera Capture: 0-1ms (DMA)
├── Image Processing: 2-3ms
├── AI Inference: 8-10ms (Neural-ART)
├── Postprocessing: 1-2ms
└── Output Preparation: 2-3ms

リアルタイム性保証:
├── Hard Real-time: Camera Task
├── Soft Real-time: AI Task
└── Best Effort: System Task
```

### 消費電力設計

```
Target Power Consumption: < 300mW

Power Budget:
├── CPU (Cortex-M55): ~100mW
├── NPU (Neural-ART): ~150mW (推論時)
├── Camera & ISP: ~30mW
├── Audio Output: ~10mW
└── Solenoid: ~10mW (アクティブ時)

省電力戦略:
├── Dynamic Frequency Scaling
├── NPU Power Gating
└── Sleep Mode最適化
```
