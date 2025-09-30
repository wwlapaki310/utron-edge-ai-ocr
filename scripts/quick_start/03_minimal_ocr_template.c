/**
 * @file minimal_ocr.c
 * @brief 最小限の OCR 実装テンプレート
 * 
 * このファイルを src/minimal_ocr.c にコピーして使用してください
 * 
 * 使い方:
 *   1. STM32Cube.AI で生成された ocr_net.h と ocr_net.c をプロジェクトに追加
 *   2. このファイルを src/minimal_ocr.c にコピー
 *   3. ビルドして実行
 * 
 * Related: Issue #13
 */

#include "stm32n6xx_hal.h"
#include "ocr_net.h"  // STM32Cube.AI 生成ヘッダー
#include "ocr_net_data.h"
#include <stdio.h>
#include <string.h>

/* ==================== 設定 ==================== */

// デバッグ出力有効化
#define OCR_DEBUG_ENABLED   1

// 推論統計の収集
#define OCR_STATS_ENABLED   1

/* ==================== グローバル変数 ==================== */

// AI ネットワークハンドル
static ai_handle g_ocr_network = AI_HANDLE_NULL;

// 入出力バッファ
static ai_buffer g_ai_input[AI_OCR_NET_IN_NUM];
static ai_buffer g_ai_output[AI_OCR_NET_OUT_NUM];

// アクティベーションメモリ（スタックオーバーフロー防止のため static）
static AI_ALIGNED(4) 
uint8_t g_activation_buffer[AI_OCR_NET_DATA_ACTIVATIONS_SIZE];

#if OCR_STATS_ENABLED
// 統計情報
typedef struct {
    uint32_t total_inferences;
    uint32_t successful_inferences;
    uint32_t failed_inferences;
    uint32_t total_time_ms;
    uint32_t min_time_ms;
    uint32_t max_time_ms;
} ocr_stats_t;

static ocr_stats_t g_stats = {0};
#endif

/* ==================== デバッグマクロ ==================== */

#if OCR_DEBUG_ENABLED
#define OCR_LOG(fmt, ...) printf("[OCR] " fmt "\n", ##__VA_ARGS__)
#define OCR_ERROR(fmt, ...) printf("[OCR ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define OCR_LOG(fmt, ...)
#define OCR_ERROR(fmt, ...)
#endif

/* ==================== 関数プロトタイプ ==================== */

int minimal_ocr_init(void);
int minimal_ocr_inference(const uint8_t *input_data, uint32_t input_size,
                         float *output_data, uint32_t output_size);
void minimal_ocr_print_stats(void);

/* ==================== 実装 ==================== */

/**
 * @brief OCR ネットワークを初期化
 * @return 0: 成功, 負の値: エラー
 */
int minimal_ocr_init(void)
{
    ai_error err;
    
    OCR_LOG("Initializing OCR network...");
    
    // 1. ネットワーク作成
    err = ai_ocr_net_create(&g_ocr_network, AI_OCR_NET_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) {
        OCR_ERROR("Network creation failed: type=%d, code=%d", 
                  err.type, err.code);
        return -1;
    }
    OCR_LOG("✓ Network created");
    
    // 2. 初期化パラメータ設定
    ai_network_params params = {
        .activations.data = g_activation_buffer,
        .activations.size = AI_OCR_NET_DATA_ACTIVATIONS_SIZE
    };
    
    // 3. ネットワーク初期化
    if (!ai_ocr_net_init(g_ocr_network, &params)) {
        OCR_ERROR("Network initialization failed");
        ai_ocr_net_destroy(g_ocr_network);
        g_ocr_network = AI_HANDLE_NULL;
        return -2;
    }
    OCR_LOG("✓ Network initialized");
    
    // 4. 入出力バッファ取得
    ai_ocr_net_get_input(g_ocr_network, &g_ai_input[0], 0);
    ai_ocr_net_get_output(g_ocr_network, &g_ai_output[0], 0);
    
    // 5. ネットワーク情報表示
    ai_network_report report;
    if (ai_ocr_net_get_report(g_ocr_network, &report)) {
        OCR_LOG("Network Info:");
        OCR_LOG("  Model name: %s", report.model_name);
        OCR_LOG("  Model signature: %s", report.model_signature);
        OCR_LOG("  Input size: %u bytes", g_ai_input[0].size);
        OCR_LOG("  Output size: %u bytes", g_ai_output[0].size);
        OCR_LOG("  Activation size: %u KB", 
                AI_OCR_NET_DATA_ACTIVATIONS_SIZE / 1024);
        OCR_LOG("  Weights size: %u KB", report.weights_size / 1024);
    }
    
    OCR_LOG("✓ OCR ready!");
    
#if OCR_STATS_ENABLED
    // 統計情報リセット
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.min_time_ms = UINT32_MAX;
#endif
    
    return 0;
}

/**
 * @brief OCR 推論を実行
 * 
 * @param input_data 入力データ（例: カメラ画像）
 * @param input_size 入力データサイズ
 * @param output_data 出力データバッファ
 * @param output_size 出力データバッファサイズ
 * @return 0: 成功, 負の値: エラー
 */
int minimal_ocr_inference(const uint8_t *input_data, uint32_t input_size,
                         float *output_data, uint32_t output_size)
{
    if (!g_ocr_network) {
        OCR_ERROR("Network not initialized");
        return -1;
    }
    
    if (!input_data || !output_data) {
        OCR_ERROR("Invalid parameters");
        return -2;
    }
    
    // 入力サイズチェック
    if (input_size != g_ai_input[0].size) {
        OCR_ERROR("Input size mismatch: expected %u, got %u",
                  g_ai_input[0].size, input_size);
        return -3;
    }
    
    // 出力サイズチェック
    if (output_size < g_ai_output[0].size) {
        OCR_ERROR("Output buffer too small: expected %u, got %u",
                  g_ai_output[0].size, output_size);
        return -4;
    }
    
    // 入力データコピー
    memcpy(g_ai_input[0].data, input_data, input_size);
    
    // 推論実行
    uint32_t start_tick = HAL_GetTick();
    ai_i32 batch = ai_ocr_net_run(g_ocr_network, g_ai_input, g_ai_output);
    uint32_t end_tick = HAL_GetTick();
    uint32_t inference_time_ms = end_tick - start_tick;
    
    if (batch != 1) {
        OCR_ERROR("Inference failed: batch=%d", batch);
#if OCR_STATS_ENABLED
        g_stats.failed_inferences++;
#endif
        return -5;
    }
    
    // 出力データコピー
    memcpy(output_data, g_ai_output[0].data, g_ai_output[0].size);
    
#if OCR_STATS_ENABLED
    // 統計更新
    g_stats.total_inferences++;
    g_stats.successful_inferences++;
    g_stats.total_time_ms += inference_time_ms;
    
    if (inference_time_ms < g_stats.min_time_ms) {
        g_stats.min_time_ms = inference_time_ms;
    }
    if (inference_time_ms > g_stats.max_time_ms) {
        g_stats.max_time_ms = inference_time_ms;
    }
#endif
    
    OCR_LOG("Inference completed in %u ms", inference_time_ms);
    
    return 0;
}

/**
 * @brief 統計情報を表示
 */
void minimal_ocr_print_stats(void)
{
#if OCR_STATS_ENABLED
    printf("\n=== OCR Statistics ===\n");
    printf("Total inferences: %u\n", g_stats.total_inferences);
    printf("Successful: %u\n", g_stats.successful_inferences);
    printf("Failed: %u\n", g_stats.failed_inferences);
    
    if (g_stats.successful_inferences > 0) {
        uint32_t avg_time = g_stats.total_time_ms / g_stats.successful_inferences;
        printf("Inference time:\n");
        printf("  Min: %u ms\n", g_stats.min_time_ms);
        printf("  Max: %u ms\n", g_stats.max_time_ms);
        printf("  Avg: %u ms\n", avg_time);
        printf("  Success rate: %.2f%%\n",
               (float)g_stats.successful_inferences / g_stats.total_inferences * 100.0f);
    }
    printf("======================\n\n");
#endif
}

/* ==================== テスト用メイン関数 ==================== */

#ifdef MINIMAL_OCR_TEST

/**
 * @brief テスト用のダミー入力データ生成
 */
static void generate_test_input(uint8_t *buffer, uint32_t size)
{
    // 簡単なテストパターン
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)(i % 256);
    }
}

/**
 * @brief メイン関数（テスト用）
 */
int main(void)
{
    // HAL 初期化
    HAL_Init();
    SystemClock_Config();
    
    // UART 初期化（ログ出力用）
    MX_USART1_UART_Init();
    
    printf("\n");
    printf("====================================\n");
    printf("  STM32N6 Minimal OCR Test\n");
    printf("====================================\n\n");
    
    // OCR 初期化
    int ret = minimal_ocr_init();
    if (ret != 0) {
        printf("❌ OCR initialization failed: %d\n", ret);
        while (1) {
            HAL_Delay(1000);
        }
    }
    
    // テスト用バッファ確保
    uint8_t *test_input = malloc(g_ai_input[0].size);
    float *test_output = malloc(g_ai_output[0].size);
    
    if (!test_input || !test_output) {
        printf("❌ Memory allocation failed\n");
        while (1) {
            HAL_Delay(1000);
        }
    }
    
    // テストループ
    printf("\n🚀 Starting inference test loop...\n\n");
    
    for (int i = 0; i < 10; i++) {
        printf("Test %d: ", i + 1);
        
        // テスト入力生成
        generate_test_input(test_input, g_ai_input[0].size);
        
        // 推論実行
        ret = minimal_ocr_inference(test_input, g_ai_input[0].size,
                                   test_output, g_ai_output[0].size);
        
        if (ret == 0) {
            printf("✓ Success\n");
            
            // 出力の一部を表示
            printf("  Output (first 5): ");
            for (int j = 0; j < 5 && j < g_ai_output[0].size / sizeof(float); j++) {
                printf("%.2f ", test_output[j]);
            }
            printf("\n");
        } else {
            printf("✗ Failed (error: %d)\n", ret);
        }
        
        HAL_Delay(100);  // 短い待機
    }
    
    // 統計表示
    minimal_ocr_print_stats();
    
    // クリーンアップ
    free(test_input);
    free(test_output);
    
    printf("\n✅ Test completed!\n");
    printf("====================================\n\n");
    
    // 無限ループ
    while (1) {
        HAL_Delay(1000);
    }
    
    return 0;
}

#endif /* MINIMAL_OCR_TEST */
