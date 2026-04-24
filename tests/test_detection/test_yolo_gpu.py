#!/usr/bin/env python3
"""
Task 4.1: Verify YOLOv8 + CUDA in WSL2
- Check CUDA availability
- Load YOLOv8n model
- Run inference on a sample image
"""

import sys

def test_cuda_availability():
    """Test if CUDA is available"""
    import torch
    assert torch.cuda.is_available(), "CUDA not available"
    print(f"✓ CUDA available: {torch.cuda.get_device_name(0)}")
    return True

def test_yolo_inference():
    """Test YOLOv8n inference on GPU"""
    from ultralytics import YOLO

    # Load YOLOv8n model
    model = YOLO("yolov8n.pt")
    print("✓ YOLOv8n model loaded")

    # Run inference on a sample image from ultralytics
    results = model.predict("https://ultralytics.com/images/bus.jpg", device=0, verbose=False)

    # Check inference device
    infer_device = str(results[0].boxes.data.device)
    assert "cuda" in infer_device, f"Inference not on CUDA: {infer_device}"
    print(f"✓ Inference device: {infer_device}")

    # Check detection results
    num_detections = len(results[0].boxes)
    print(f"✓ Detected {num_detections} objects in image")

    return True

if __name__ == "__main__":
    try:
        print("Test 4.1: YOLOv8 + CUDA Verification")
        print("=" * 50)

        test_cuda_availability()
        test_yolo_inference()

        print("=" * 50)
        print("✓ All tests PASSED")
        sys.exit(0)
    except Exception as e:
        print(f"✗ Test FAILED: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)
