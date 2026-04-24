#!/usr/bin/env python3
"""
Task 4.2: Export YOLOv8n to ONNX format
"""

import os
import sys
from pathlib import Path

def export_yolo_onnx():
    """Export YOLOv8n model to ONNX format"""
    try:
        from ultralytics import YOLO

        # Load YOLOv8n model
        print("Loading YOLOv8n model...")
        model = YOLO("yolov8n.pt")

        # Export to ONNX
        print("Exporting to ONNX...")
        export_path = model.export(
            format="onnx",
            opset=12,
            simplify=True,
            dynamic=False,
            imgsz=640
        )
        print(f"✓ Model exported to: {export_path}")

        # Move to models directory
        models_dir = Path(__file__).parent.parent / "models"
        models_dir.mkdir(exist_ok=True)

        onnx_file = models_dir / "yolov8n.onnx"
        os.rename(export_path, str(onnx_file))
        print(f"✓ Model moved to: {onnx_file}")

        # Verify file size
        size_mb = onnx_file.stat().st_size / (1024 * 1024)
        print(f"✓ File size: {size_mb:.1f} MB")

        return True
    except Exception as e:
        print(f"✗ Export failed: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = export_yolo_onnx()
    sys.exit(0 if success else 1)
