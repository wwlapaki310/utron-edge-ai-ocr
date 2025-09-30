#!/usr/bin/env python3
"""\nPaddleOCR モデルを ONNX に変換するスクリプト\n\n使い方:\n  python 01_convert_paddle_to_onnx.py\n\n出力:\n  ocr_model.onnx - STM32Cube.AI で使用可能な ONNX モデル\n"""

import os
import sys
import urllib.request
import tarfile

try:
    import paddle
    import paddle2onnx
except ImportError:
    print("❌ 必要なパッケージがインストールされていません")
    print("\n以下を実行してください:")
    print("  pip install paddlepaddle paddle2onnx onnx")
    sys.exit(1)

def download_paddle_model():
    """PaddleOCR 英語モデルをダウンロード"""
    model_url = "https://paddleocr.bj.bcebos.com/PP-OCRv3/english/en_PP-OCRv3_rec_infer.tar"
    model_tar = "en_PP-OCRv3_rec_infer.tar"
    model_dir = "en_PP-OCRv3_rec_infer"
    
    if os.path.exists(model_dir):
        print(f"✅ モデルディレクトリが既に存在します: {model_dir}")
        return model_dir
    
    print(f"📥 モデルをダウンロード中: {model_url}")
    
    try:
        urllib.request.urlretrieve(model_url, model_tar)
        print(f"✅ ダウンロード完了: {model_tar}")
        
        # 解凍
        print(f"📦 解凍中...")
        with tarfile.open(model_tar) as tar:
            tar.extractall()
        
        print(f"✅ 解凍完了: {model_dir}")
        
        # ダウンロードファイル削除
        os.remove(model_tar)
        
        return model_dir
        
    except Exception as e:
        print(f"❌ ダウンロードエラー: {e}")
        sys.exit(1)

def convert_to_onnx(model_dir, output_file="ocr_model.onnx"):
    """Paddle モデルを ONNX に変換"""
    model_file = os.path.join(model_dir, "inference.pdmodel")
    params_file = os.path.join(model_dir, "inference.pdiparams")
    
    if not os.path.exists(model_file):
        print(f"❌ モデルファイルが見つかりません: {model_file}")
        sys.exit(1)
    
    if not os.path.exists(params_file):
        print(f"❌ パラメータファイルが見つかりません: {params_file}")
        sys.exit(1)
    
    print(f"\n🔄 ONNX 変換中...")
    print(f"  入力モデル: {model_file}")
    print(f"  出力ファイル: {output_file}")
    
    try:
        # ONNX 変換
        onnx_model = paddle2onnx.command.c_paddle_to_onnx(
            model_file=model_file,
            params_file=params_file,
            save_file=output_file,
            opset_version=11,
            enable_onnx_checker=True
        )
        
        if onnx_model:
            size_mb = os.path.getsize(output_file) / (1024 * 1024)
            print(f"\n✅ ONNX 変換完了!")
            print(f"  ファイル: {output_file}")
            print(f"  サイズ: {size_mb:.2f} MB")
            
            if size_mb > 2.5:
                print(f"\n⚠️  警告: モデルサイズが 2.5MB を超えています")
                print(f"  STM32N6 で動作させるには量子化が必要です")
                print(f"  次のステップ (02_convert_to_stm32.sh) で自動的に量子化されます")
            
            return True
        else:
            print(f"❌ ONNX 変換に失敗しました")
            return False
            
    except Exception as e:
        print(f"❌ 変換エラー: {e}")
        return False

def verify_onnx_model(onnx_file):
    """ONNX モデルを検証"""
    try:
        import onnx
        
        print(f"\n🔍 ONNX モデル検証中...")
        model = onnx.load(onnx_file)
        onnx.checker.check_model(model)
        
        print(f"✅ モデル検証成功")
        
        # モデル情報表示
        print(f"\n📊 モデル情報:")
        print(f"  IR バージョン: {model.ir_version}")
        print(f"  プロデューサー: {model.producer_name}")
        print(f"  Opset バージョン: {model.opset_import[0].version}")
        
        # 入出力情報
        print(f"\n  入力:")
        for input in model.graph.input:
            shape = [dim.dim_value if dim.dim_value > 0 else '?' 
                    for dim in input.type.tensor_type.shape.dim]
            print(f"    {input.name}: {shape}")
        
        print(f"\n  出力:")
        for output in model.graph.output:
            shape = [dim.dim_value if dim.dim_value > 0 else '?' 
                    for dim in output.type.tensor_type.shape.dim]
            print(f"    {output.name}: {shape}")
        
        return True
        
    except Exception as e:
        print(f"⚠️  モデル検証エラー: {e}")
        print(f"  (変換は成功していますが、検証でエラーが発生しました)")
        return False

def main():
    print("="*60)
    print("  PaddleOCR → ONNX 変換スクリプト")
    print("="*60)
    print()
    
    # モデルダウンロード
    model_dir = download_paddle_model()
    
    # ONNX 変換
    output_file = "ocr_model.onnx"
    if convert_to_onnx(model_dir, output_file):
        # 検証
        verify_onnx_model(output_file)
        
        print("\n" + "="*60)
        print("✅ すべて完了!")
        print("="*60)
        print(f"\n次のステップ:")
        print(f"  bash 02_convert_to_stm32.sh")
        print()
    else:
        print("\n❌ 変換に失敗しました")
        sys.exit(1)

if __name__ == "__main__":
    main()
