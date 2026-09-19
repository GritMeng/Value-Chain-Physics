#!/usr/bin/env bash
# IPC Core Engine —— 一键编译与测试 (Linux / macOS)
# 用法: ./build_and_run.sh [data_dir]
set -euo pipefail

cd "$(dirname "$0")"
DATA_DIR="${1:-data}"

echo "========================================================"
echo "  IPC Core Engine - Build and Test"
echo "========================================================"

CXX="${CXX:-clang++}"
CXXFLAGS="-std=c++17 -O2 -I include -Wall"

mkdir -p build bin

echo "[1/4] 编译核心引擎模块..."
for src in data_loader atp_ctp_engine itp_iop_alignment substitution_engine; do
    echo "      - $src.cpp"
    $CXX $CXXFLAGS -c "src/$src.cpp" -o "build/$src.o"
done

echo "[2/4] 链接基准测试可执行文件..."
OBJS="build/data_loader.o build/atp_ctp_engine.o build/itp_iop_alignment.o build/substitution_engine.o"
for t in test_delivery_precision test_itp_iop_alignment test_substitution_rules stress_benchmark_2m; do
    $CXX $CXXFLAGS "benchmarks/$t.cpp" $OBJS -o "bin/$t"
    echo "      - bin/$t"
done

echo "[3/4] 运行 C++ 引擎基准..."
echo
./bin/test_delivery_precision
echo
./bin/test_itp_iop_alignment
echo
./bin/test_substitution_rules
echo
echo "  注: 极限压测需先准备数据 -> python3 data/generate_data.py"
if [ -d "$DATA_DIR" ]; then
    ./bin/stress_benchmark_2m "$DATA_DIR"
else
    echo "  [SKIP] 数据目录 '$DATA_DIR' 不存在，跳过压测。"
fi
echo

echo "[4/4] 运行 Python 数学交叉校验..."
( cd verifier && python3 cross_validator.py )
echo
echo "========================================================"
echo "  全部基准与交叉校验完成。"
echo "========================================================"
