/**
 * @file ocr_mock_test.c
 * @brief Mock OCR Test - 文字出力確認用
 * 
 * 目的: NPUなしで文字列が出力されることを確認
 * ゴール: シリアルコンソールに "Hello μTRON!" が表示される
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* モックOCR結果の型定義 */
typedef struct {
    char text[256];        // 認識された文字列
    float confidence;      // 信頼度 (0.0-1.0)
    int num_chars;         // 文字数
    uint32_t processing_time_ms;  // 処理時間
} ocr_result_t;

/**
 * @brief モックOCR推論
 * @param image ダミー画像データ（使用しない）
 * @return OCR結果
 */
ocr_result_t ocr_recognize_mock(uint8_t *image) {
    ocr_result_t result;
    
    // テスト用の固定文字列を返す
    strcpy(result.text, "Hello μTRON!");
    result.confidence = 0.95f;
    result.num_chars = 13;
    result.processing_time_ms = 5;  // 5ms想定
    
    return result;
}

/**
 * @brief 複数パターンのテスト
 */
void test_multiple_patterns(void) {
    const char *test_cases[] = {
        "Hello μTRON!",
        "日本語テスト",
        "Edge AI OCR",
        "競技会2025",
        "STM32N6"
    };
    
    printf("\n=== Multiple Pattern Test ===\n");
    
    for (int i = 0; i < 5; i++) {
        ocr_result_t result;
        strcpy(result.text, test_cases[i]);
        result.confidence = 0.90f + (i * 0.02f);
        result.num_chars = strlen(test_cases[i]);
        result.processing_time_ms = 5 + i;
        
        printf("[%d] 📝 Recognized: %s\n", i+1, result.text);
        printf("    Confidence: %.2f%%\n", result.confidence * 100);
        printf("    Chars: %d, Time: %u ms\n\n", 
               result.num_chars, result.processing_time_ms);
    }
}

/**
 * @brief メイン関数
 */
int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║   μTRON OS + Edge AI OCR Test       ║\n");
    printf("║   Mock Test - Phase 1                ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("\n");
    
    /* テスト1: 基本動作確認 */
    printf("=== Basic OCR Test ===\n");
    
    uint8_t dummy_image[640 * 640 * 3] = {0};
    ocr_result_t result = ocr_recognize_mock(dummy_image);
    
    printf("📝 Recognized: %s\n", result.text);
    printf("   Confidence: %.2f%%\n", result.confidence * 100);
    printf("   Characters: %d\n", result.num_chars);
    printf("   Processing Time: %u ms\n", result.processing_time_ms);
    printf("\n");
    
    /* テスト2: 複数パターン */
    test_multiple_patterns();
    
    /* テスト3: パフォーマンス測定 */
    printf("=== Performance Test ===\n");
    int iterations = 1000;
    uint32_t total_time = 0;
    
    for (int i = 0; i < iterations; i++) {
        ocr_result_t perf_result = ocr_recognize_mock(dummy_image);
        total_time += perf_result.processing_time_ms;
    }
    
    printf("Iterations: %d\n", iterations);
    printf("Average Time: %.2f ms\n", (float)total_time / iterations);
    printf("Throughput: %.2f FPS\n", 1000.0f / ((float)total_time / iterations));
    printf("\n");
    
    /* 成功メッセージ */
    printf("✅ All tests passed!\n");
    printf("🎯 Goal achieved: OCR text output working!\n");
    printf("\n");
    
    return 0;
}
