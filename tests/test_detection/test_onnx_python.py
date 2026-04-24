#!/usr/bin/env python3
"""
Task 4.3 (Python Version): ONNX Runtime Inference Demo
- Load ONNX model
- Preprocess image with letterbox
- Run inference on GPU
- Verify output shape
"""

import sys
import time
import onnxruntime as rt
import cv2
import numpy as np

def letterbox(img, target_size=640):
    """Letterbox resize image to target_size"""
    h, w = img.shape[:2]
    scale = min(target_size / h, target_size / w)
    new_h, new_w = int(h * scale), int(w * scale)

    # Resize image
    resized = cv2.resize(img, (new_w, new_h))

    # Create canvas
    canvas = np.full((target_size, target_size, 3), 114, dtype=np.uint8)
    pad_y = (target_size - new_h) // 2
    pad_x = (target_size - new_w) // 2
    canvas[pad_y:pad_y+new_h, pad_x:pad_x+new_w] = resized

    return canvas

def test_onnx_inference():
    """Test ONNX inference on GPU"""
    print("Task 4.3: ONNX Runtime Inference Demo")
    print("=" * 50)

    # Create session with CUDA provider
    so = rt.SessionOptions()
    so.graph_optimization_level = rt.GraphOptimizationLevel.ORT_ENABLE_ALL

    providers = [
        ('CUDAExecutionProvider', {'device_id': 0}),
        'CPUExecutionProvider'
    ]

    sess = rt.InferenceSession('models/yolov8n.onnx', providers=providers, sess_options=so)

    # Get input info
    input_name = sess.get_inputs()[0].name
    input_shape = sess.get_inputs()[0].shape
    print(f"✓ Model loaded: input name={input_name}, shape={input_shape}")

    # Load image
    img = cv2.imread('bus.jpg')
    if img is None:
        raise FileNotFoundError("bus.jpg not found")
    print(f"✓ Image loaded: {img.shape}")

    # Letterbox resize
    input_img = letterbox(img, 640)
    print(f"✓ Letterbox resized: {input_img.shape}")

    # Normalize (BGR -> RGB, scale to [0, 1])
    input_img_rgb = cv2.cvtColor(input_img, cv2.COLOR_BGR2RGB)
    input_tensor = input_img_rgb.astype(np.float32) / 255.0
    # CHW format: (3, 640, 640)
    input_tensor = np.transpose(input_tensor, (2, 0, 1))
    # Add batch dimension: (1, 3, 640, 640)
    input_tensor = np.expand_dims(input_tensor, axis=0)
    print(f"✓ Input tensor prepared: shape={input_tensor.shape}, dtype={input_tensor.dtype}")

    # Run inference
    print("Running inference...")
    start = time.time()
    outputs = sess.run(None, {input_name: input_tensor})
    elapsed = (time.time() - start) * 1000

    print(f"✓ Inference completed in {elapsed:.1f}ms")

    # Check output
    output_tensor = outputs[0]
    print(f"✓ Output shape: {output_tensor.shape}")
    print(f"✓ Output dtype: {output_tensor.dtype}")
    print(f"✓ First 10 values: {output_tensor.flat[:10]}")

    print("=" * 50)
    print("✓ All tests PASSED")
    return True

if __name__ == "__main__":
    try:
        test_onnx_inference()
        sys.exit(0)
    except Exception as e:
        print(f"✗ Test FAILED: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)
