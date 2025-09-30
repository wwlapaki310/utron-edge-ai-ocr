# OCRテスト実装

## 📁 ディレクトリ構成

```
test/
├── ocr_mock_test.c          # モックテスト（今ここ！）
├── ocr_integration_test.c   # 統合テスト（Phase 2）
└── README.md                # このファイル
```

---

## 🚀 Phase 1: モックテスト

### 目的
**NPUなしで文字列が出力されることを確認**

### 実行方法

#### Windows (PC上で動作確認)
```bash
# コンパイル
gcc -o ocr_mock_test ocr_mock_test.c

# 実行
.\ocr_mock_test.exe
```

#### Linux/Mac (PC上で動作確認)
```bash
# コンパイル
gcc -o ocr_mock_test ocr_mock_test.c

# 実行
./ocr_mock_test
```

### 期待される出力

```
╔═══════════════════════════════════════╗
║   μTRON OS + Edge AI OCR Test       ║
║   Mock Test - Phase 1                ║
╚═══════════════════════════════════════╝

=== Basic OCR Test ===
📝 Recognized: Hello μTRON!
   Confidence: 95.00%
   Characters: 13
   Processing Time: 5 ms

=== Multiple Pattern Test ===
[1] 📝 Recognized: Hello μTRON!
    Confidence: 90.00%
    Chars: 13, Time: 5 ms

[2] 📝 Recognized: 日本語テスト
    Confidence: 92.00%
    Chars: 7, Time: 6 ms

[3] 📝 Recognized: Edge AI OCR
    Confidence: 94.00%
    Chars: 11, Time: 7 ms

[4] 📝 Recognized: 競技会2025
    Confidence: 96.00%
    Chars: 7, Time: 8 ms

[5] 📝 Recognized: STM32N6
    Confidence: 98.00%
    Chars: 7, Time: 9 ms

=== Performance Test ===
Iterations: 1000
Average Time: 5.00 ms
Throughput: 200.00 FPS

✅ All tests passed!
🎯 Goal achieved: OCR text output working!
```

---

## 🎯 STM32N6実機での実行

### ステップ1: STM32CubeIDEへの統合

1. **プロジェクトにファイル追加**
   ```
   右クリック > New > Folder > "test"
   test/ocr_mock_test.c をドラッグ&ドロップ
   ```

2. **main.cを編集**
   ```c
   // Core/Src/main.c
   
   extern ocr_result_t ocr_recognize_mock(uint8_t *image);
   
   int main(void) {
       HAL_Init();
       SystemClock_Config();
       
       // UART初期化（シリアル出力用）
       MX_USART1_UART_Init();
       
       printf("\n=== OCR Mock Test on STM32N6 ===\n\n");
       
       uint8_t dummy[640*640*3] = {0};
       ocr_result_t result = ocr_recognize_mock(dummy);
       
       printf("📝 Recognized: %s\n", result.text);
       printf("   Confidence: %.2f%%\n", result.confidence * 100);
       printf("\n✅ OCR Mock Test Success!\n");
       
       while (1) {
           HAL_Delay(1000);
       }
   }
   ```

3. **printfのリダイレクト設定**
   ```c
   // Core/Src/syscalls.c または main.c に追加
   
   #include <stdio.h>
   
   // UART経由でprintf出力
   int _write(int file, char *ptr, int len) {
       HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
       return len;
   }
   ```

4. **ビルド＆実行**
   ```
   Project > Build Project (Ctrl+B)
   Run > Debug (F11)
   ```

5. **シリアルコンソールで確認**
   - Tera Term / PuTTY で COM3を開く
   - ボーレート: 115200
   - 「Hello μTRON!」が表示されればOK！ ✅

---

## 📝 チェックリスト

### Phase 1 完了条件

- [ ] PC上で ocr_mock_test.c がコンパイル成功
- [ ] PC上で実行して文字列が出力される
- [ ] STM32N6実機でビルド成功
- [ ] STM32N6実機でシリアル出力確認
- [ ] 「Hello μTRON!」が表示される 🎯

### 確認できたら次へ

✅ **Phase 1完了！**  
→ Phase 2: 実際のOCRモデル統合へ進む

---

## 🐛 トラブルシューティング

### Q1: printfが出力されない

**A**: UARTリダイレクト設定を確認
```c
// syscalls.c に追加必要
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

### Q2: 文字化けする

**A**: ボーレート確認
```
Tera Term設定:
- Speed: 115200
- Data: 8bit
- Parity: none
- Stop: 1bit
- Flow control: none
```

### Q3: ビルドエラー

**A**: インクルードパス確認
```c
#include <stdio.h>   // printf用
#include <string.h>  // strcpy用
#include <stdint.h>  // uint8_t用
```

---

## 📚 次のステップ

1. ✅ **Phase 1**: モックテスト（今ここ）
2. ⏭️ **Phase 2**: PaddleOCRモデル統合
3. ⏭️ **Phase 3**: カメラ入力統合
4. ⏭️ **Phase 4**: リアルタイムOCR

---

**更新日**: 2025年9月30日  
**ステータス**: Phase 1 実装完了
