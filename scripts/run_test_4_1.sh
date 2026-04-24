#!/bin/bash
# Task 4.1: Run YOLOv8 + CUDA verification test

set -e

echo "Task 4.1: WSL2 Python 验证 YOLOv8 + CUDA"
echo "=========================================="

# Activate conda environment
source $(conda info --base)/etc/profile.d/conda.sh
conda activate ai_dt

# Run test
cd /home/peter/dog/dog_dev
python tests/test_detection/test_yolo_gpu.py

echo "=========================================="
echo "✓ Task 4.1 completed successfully"
