#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "================================================================"
echo "  IPC Core Open Benchmarks v2.2 - POSIX Build & Verify (7 Suites)"
echo "================================================================"

CXX_BIN="${CXX:-}"
if [[ -z "$CXX_BIN" ]]; then
  if command -v g++ >/dev/null 2>&1; then
    CXX_BIN="g++"
  elif command -v clang++ >/dev/null 2>&1; then
    CXX_BIN="clang++"
  else
    echo "[ERROR] No C++17 compiler (g++ or clang++) found." >&2
    exit 1
  fi
fi

mkdir -p build bin

echo "[1/4] Compiling clean-room C++17 modules via ${CXX_BIN}..."
for s in substitution_engine atp_ctp_engine itp_iop_alignment sandbox_operators; do
  "$CXX_BIN" -std=c++17 -O3 -I include -c "src/${s}.cpp" -o "build/${s}.o"
done

OBJS="build/substitution_engine.o build/atp_ctp_engine.o build/itp_iop_alignment.o build/sandbox_operators.o"

echo "[2/4] Linking 7 benchmark executables..."
SUITES=(
  test_delivery_precision
  test_substitution_rules
  test_codp_positioning
  test_push_pull_router
  test_itp_iop_alignment
  test_two_stage_iop_network
  stress_benchmark_2m
)

for t in "${SUITES[@]}"; do
  "$CXX_BIN" -std=c++17 -O3 -I include "benchmarks/${t}.cpp" $OBJS -o "bin/${t}"
done

echo "[3/4] Running all 7 C++17 benchmarks..."
for t in "${SUITES[@]}"; do
  echo ""
  "./bin/${t}"
done

echo ""
echo "[4/4] Running Python Independent Mathematical Cross-Validator..."
PYTHON_BIN="python3"
command -v python3 >/dev/null 2>&1 || PYTHON_BIN="python"
"$PYTHON_BIN" verifier/cross_validator.py

echo "================================================================"
echo "  [ALL PASSED] 7 C++17 Suites + Python Cross-Validator Verified."
echo "================================================================"

