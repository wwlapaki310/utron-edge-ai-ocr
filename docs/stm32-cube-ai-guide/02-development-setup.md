# STM32Cube.AI開発環境セットアップ

## 概要

STM32N6でNeural-ART NPUを活用したAI開発を始めるための完全なセットアップガイドです。

---

## 1. ハードウェア要件

### 1.1 必須ハードウェア

| 項目 | 仕様 | 備考 |
|------|------|------|
| **開発ボード** | STM32N6570-DK | Neural-ART NPU搭載 |
| **デバッガ** | ST-LINK/V3 | オンボード搭載 |
| **カメラ** | MIPI CSI-2対応 | 解像度640x480以上推奨 |
| **PC** | Windows 10/11 | RAM 8GB以上推奨 |

### 1.2 推奨ハードウェア

- USBケーブル: Type-C (データ転送対応)
- 外部電源: 5V/2A以上
- microSDカード: 32GB以上 (モデル保存用)

---

## 2. ソフトウェアインストール

### 2.1 STM32CubeIDE

**バージョン**: 1.16.0以降

**ダウンロード**: 
https://www.st.com/ja/development-tools/stm32cubeide.html

**インストール手順**:
```bash
1. インストーラーをダウンロード
2. 実行してデフォルト設定でインストール
3. 初回起動時にワークスペースを指定
   推奨: C:\Users\<YourName>\STM32CubeIDE\workspace
```

### 2.2 STM32CubeMX

**バージョン**: 6.12.0以降

**注意**: STM32CubeIDEに統合されているため、個別インストール不要

### 2.3 X-CUBE-AI (STM32Cube.AI)

**バージョン**: 10.0.1以降 (ST Edge AI Core v2.0)

**インストール方法**:

#### 方法1: STM32CubeMX GUI経由
```
1. STM32CubeMXを起動
2. Help → Manage Embedded Software Packages
3. STMicroelectronics → X-CUBE-AI を選択
4. バージョン10.0.1以降をインストール
```

#### 方法2: コマンドライン
```bash
# ST Edge AI CLI ツールのインストール
pip install stedgeai-cli

# バージョン確認
stedgeai --version
# 出力例: stedgeai 10.0.1
```

### 2.4 Python環境 (モデル変換用)

**Python**: 3.8 - 3.11 推奨

```bash
# 仮想環境作成
python -m venv stm32ai_env

# 仮想環境アクティベート
# Windows:
stm32ai_env\Scripts\activate
# Linux/Mac:
source stm32ai_env/bin/activate

# 必要なパッケージインストール
pip install tensorflow==2.15.0
pip install torch==2.1.0
pip install onnx==1.15.0
pip install onnx-tf==1.10.0
pip install stedgeai-cli
```

---

## 3. プロジェクト作成

### 3.1 新規プロジェクト作成

**STM32CubeIDEでの手順**:

```
1. File → New → STM32 Project
2. Board Selector → STM32N6570-DK を選択
3. Project Name: utron-edge-ai-ocr
4. Targeted Language: C
5. Finish
```

### 3.2 X-CUBE-AIの有効化

**STM32CubeMXでの設定**:

```
1. Software Packs → Select Components
2. STMicroelectronics.X-CUBE-AI を展開
3. Core にチェック
4. OK
```

**設定画面**:
```
Middleware → X-CUBE-AI → Core Settings:
├── Model: (モデルファイルパス)
├── Target Device: STM32N6570
├── Optimization: Time (または Balanced)
└── Compression: None
```

### 3.3 クロック設定

**System Core → RCC**:
```
HSE (High Speed External): Crystal/Ceramic Resonator
PLL Configuration:
├── Source: HSE
├── PLLM: 4
├── PLLN: 160  # 25MHz * 160 / 4 = 1000MHz
└── PLLP: 1

System Clock Mux: PLLCLK
Cortex-M55 Clock: 600MHz
Neural-ART NPU Clock: 1000MHz
```

---

## 4. AIモデルの追加

### 4.1 モデルファイルの配置

```bash
# プロジェクトルートにAIフォルダ作成
mkdir AI
cd AI

# モデルファイルをコピー
cp /path/to/your/model_int8.tflite .
```

### 4.2 X-CUBE-AIでのモデル設定

**Model Settings**:
```
Model Path: AI/model_int8.tflite
Model Name: ocr_model
Compression: None (すでにINT8量子化済み)
Optimization: Time
```

**Analyze実行**:
```
1. Analyze ボタンをクリック
2. 結果を確認:
   - Model RAM: ~2.1MB
   - Model Flash: ~800KB
   - Inference Time: ~5ms @ 1GHz
   - NPU Utilization: ~75%
```

**コード生成**:
```
1. Generate Code ボタンをクリック
2. 生成されるファイル:
   ├── X-CUBE-AI/App/ocr_model.c
   ├── X-CUBE-AI/App/ocr_model.h
   ├── X-CUBE-AI/App/ocr_model_data.c
   └── X-CUBE-AI/App/network_runtime.c
```

---

## 5. デバッグ環境設定

### 5.1 ST-LINK設定

**Run → Debug Configurations**:
```
Debugger タブ:
├── Debug probe: ST-LINK (ST-LINK GDB server)
├── Interface: SWD
├── Reset Mode: Software system reset
└── Frequency: 4000 kHz (最大速度)
```

### 5.2 SWO Trace設定 (パフォーマンス測定用)

```
Debugger → Serial Wire Viewer (SWV):
├── Enable: チェック
├── Core Clock: 600 MHz
└── SWO Clock: 2000 kHz
```

**使用例**:
```c
#include "stdio.h"

// SWO経由でprintf出力
printf("Inference time: %u ms\n", inference_time);
```

### 5.3 RTT (Real-Time Transfer) 設定

**SEGGER RTT のインストール**:
```bash
# SEGGER J-Link Software をダウンロード・インストール
# https://www.segger.com/downloads/jlink/
```

**プロジェクトへの統合**:
```c
// RTTを使った高速ログ出力
#include "SEGGER_RTT.h"

SEGGER_RTT_printf(0, "NPU inference: %d ms\n", time_ms);
```

---

## 6. ビルドとデプロイ

### 6.1 ビルド設定

**最適化レベル**:
```
Project → Properties → C/C++ Build → Settings:
├── MCU GCC Compiler → Optimization:
│   └── Optimization level: -O3 (最高速度)
├── MCU GCC Linker → General:
│   └── Script files: 自動生成されたリンカスクリプト
```

**メモリ配置の確認**:
```
Linker Script (.ld file):
RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 2560K
FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
NPU_RAM (xrw) : ORIGIN = 0x30000000, LENGTH = 2560K
```

### 6.2 ビルド実行

```bash
# コマンドラインビルド
cd /path/to/project
make clean
make all

# または IDEから
# Project → Build Project (Ctrl+B)
```

### 6.3 フラッシュ書き込み

**STM32CubeIDEから**:
```
1. Run → Debug (F11) または Run (Ctrl+F11)
2. 自動的にビルド→書き込み→実行
```

**コマンドラインから**:
```bash
# STM32_Programmer_CLI を使用
STM32_Programmer_CLI -c port=SWD -w build/utron-edge-ai-ocr.hex -v -rst
```

---

## 7. 動作確認

### 7.1 シリアルモニター設定

**シリアルポート設定**:
```
Port: COM3 (Windows) または /dev/ttyUSB0 (Linux)
Baud Rate: 115200
Data Bits: 8
Stop Bits: 1
Parity: None
```

**Tera Term / PuTTY での接続**:
```
1. シリアルポートを選択
2. ボーレート 115200 に設定
3. 接続
4. STM32N6をリセット
5. 起動ログを確認
```

### 7.2 初回起動確認

**期待される出力**:
```
=== μTRON OS Boot ===
STM32N6570-DK Initializing...
Neural-ART NPU: OK (1GHz)
Camera: OK (MIPI CSI-2)
Memory: 2.5MB allocated
AI Model: ocr_model loaded (800KB)

System Ready.
AI Task: Running (Priority 2)
Inference Time: 5.2ms ✓
```

---

## 8. トラブルシューティング

### 問題1: ビルドエラー "undefined reference to..."

**原因**: X-CUBE-AIライブラリがリンクされていない

**解決策**:
```bash
Project → Properties → C/C++ Build → Settings:
├── MCU GCC Linker → Libraries:
│   ├── Library search path: ${workspace_loc:/${ProjName}/X-CUBE-AI/Lib}
│   └── Libraries: NetworkRuntime800_CM55_GCC
```

### 問題2: NPUが動作しない

**原因**: クロック設定が不正

**確認方法**:
```c
// デバッガでレジスタ値を確認
printf("NPU Clock: %lu Hz\n", HAL_RCC_GetNPUFreq());
// 期待値: 1000000000 (1GHz)
```

### 問題3: メモリ不足エラー

**原因**: スタックまたはヒープサイズが不足

**解決策**:
```c
// Linker Scriptで調整
_Min_Heap_Size = 0x10000;  /* 64KB */
_Min_Stack_Size = 0x8000;  /* 32KB */
```

---

## 9. 次のステップ

- **次章**: [03-model-conversion.md](03-model-conversion.md) - モデル変換実践
- **関連**: [04-quantization.md](04-quantization.md) - INT8量子化
- **実装**: [08-ocr-case-study.md](08-ocr-case-study.md) - 完全な実装例

---

**更新日**: 2025年9月30日  
**関連Issue**: #4, #5 - Neural-ART NPU統合・HAL設計
