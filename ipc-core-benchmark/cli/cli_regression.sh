#!/usr/bin/env bash
# IPC Engine CLI 契约回归脚本
#
# 用法: ./cli/cli_regression.sh [CLI 可执行文件]
#   默认使用 bin/ipc_engine_cli（需先 ./build_and_run.sh 构建）
#
# 覆盖四个引擎，逐一校验输出 CSV 的表头、行数与关键数值。
# 任一断言失败即以非零退出码结束。
set -euo pipefail

cd "$(dirname "$0")/.."
CLI="${1:-bin/ipc_engine_cli}"
DEMO="data/demo"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

PASS=0
FAIL=0

pass() { echo "  [PASS] $1"; PASS=$((PASS+1)); }
fail() { echo "  [FAIL] $1"; FAIL=$((FAIL+1)); }

# 断言某个 CSV 的表头等于期望值
assert_header() {
    local file="$1"; shift
    local expected="$1"; shift
    local actual
    actual="$(head -n 1 "$file")"
    if [ "$actual" = "$expected" ]; then
        pass "$file 表头"
    else
        fail "$file 表头: 期望 [$expected] 实际 [$actual]"
    fi
}

# 断言文件的数据行数（不含表头）等于期望值
assert_rows() {
    local file="$1"; local expected="$2"
    local actual
    actual="$(( $(wc -l < "$file") - 1 ))"
    if [ "$actual" = "$expected" ]; then
        pass "$file 行数=$expected"
    else
        fail "$file 行数: 期望 $expected 实际 $actual"
    fi
}

# 断言 CSV 中某个字段的值（字段名, 期望值, 行号从 0 起）
assert_field() {
    local file="$1"; local col="$2"; local expected="$3"; local row="${4:-0}"
    local actual
    actual="$(python3 - "$file" "$col" "$((row+2))" << 'PY'
import csv, sys
path, col, lineno = sys.argv[1], sys.argv[2], int(sys.argv[3])
rows = list(csv.reader(open(path, newline='')))
with open(path, newline='') as f:
    r = list(csv.DictReader(f))
print(r[lineno-2][col])
PY
)"
    if [ "$actual" = "$expected" ]; then
        pass "$file[$col]=$expected"
    else
        fail "$file[$col]: 期望 $expected 实际 $actual"
    fi
}

# 断言 CSV 中某列存在某个值
assert_contains() {
    local file="$1"; local needle="$2"
    if grep -qF "$needle" "$file"; then
        pass "$file 含 [$needle]"
    else
        fail "$file 缺少 [$needle]"
    fi
}

echo "========================================================"
echo "  IPC Engine CLI 契约回归 (cli=$CLI)"
echo "========================================================"

if [ ! -x "$CLI" ]; then
    echo "[ERROR] CLI 不存在或不可执行: $CLI"
    echo "        请先运行 ./build_and_run.sh"
    exit 1
fi

# ---------------------------------------------------------------------------
echo
echo "[1/4] delivery —— 交付承诺 (承诺 Day 3 / 预留 3 工时)"
DEL="$WORK/delivery"; mkdir -p "$DEL"
"$CLI" --engine delivery --in "$DEMO/delivery-benchmark" --out "$DEL" --part-id 0 --due-day 3 --qty 30 >/dev/null
assert_header "$DEL/delivery_result.csv" "part_id,due_day,qty,priority,is_fulfillable,promised_day,promised_qty,total_capacity_used,rollback_steps_count"
assert_rows   "$DEL/delivery_result.csv" 1
assert_field  "$DEL/delivery_result.csv" is_fulfillable true
assert_field  "$DEL/delivery_result.csv" promised_day 3
assert_field  "$DEL/delivery_result.csv" total_capacity_used 3
assert_header "$DEL/delivery_steps.csv" "attempt_day,success,capacity_used,rollback_steps,note"
assert_rows   "$DEL/delivery_steps.csv" 1
assert_header "$DEL/delivery_bom.csv" "level_parent_id,level_child_id,usage_qty,child_required_qty,alt_class,alt_group"
assert_rows   "$DEL/delivery_bom.csv" 2

# ---------------------------------------------------------------------------
echo
echo "[2/4] ITP —— 主计划防波堤配额 (配额 = Σ需求 × 缓冲系数)"
ITP="$WORK/itp"; mkdir -p "$ITP"
"$CLI" --engine itp --in "$DEMO/itp-iop-benchmark" --out "$ITP" --buffer-factor 1.1 >/dev/null
assert_header "$ITP/itp_allotments.csv" "day,family_id,cust_group_id,region_id,total_quota,consumed_qty"
assert_rows   "$ITP/itp_allotments.csv" 2
assert_contains "$ITP/itp_allotments.csv" "5,1,95,3,1100,0"
assert_contains "$ITP/itp_allotments.csv" "5,2,17,9,550,0"

# ---------------------------------------------------------------------------
echo
echo "[3/4] IOP —— 执行计划刚性阻断 (下派 3 / 阻断 1)"
IOP="$WORK/iop"; mkdir -p "$IOP"
"$CLI" --engine iop --in "$DEMO/itp-iop-benchmark" --out "$IOP" >/dev/null
assert_header "$IOP/iop_orders.csv" "demand_id,part_id,due_day,qty,priority,family_id,cust_group_id,region_id,status,reason"
assert_rows   "$IOP/iop_orders.csv" 4
assert_header "$IOP/iop_summary.csv" "total_orders,scheduled_orders,blocked_orders,total_fulfilled_qty,quota_utilization"
assert_field  "$IOP/iop_summary.csv" total_orders 4
assert_field  "$IOP/iop_summary.csv" scheduled_orders 3
assert_field  "$IOP/iop_summary.csv" blocked_orders 1
assert_field  "$IOP/iop_summary.csv" total_fulfilled_qty 1550
assert_contains "$IOP/iop_orders.csv" "BLOCKED"
assert_contains "$IOP/iop_orders.csv" "突破 ITP 配额上限"

# 逐单明细计数必须与算子聚合结果完全相等
python3 - "$IOP/iop_orders.csv" "$IOP/iop_summary.csv" << 'PY'
import csv, sys
orders = list(csv.DictReader(open(sys.argv[1], newline='')))
summary = list(csv.DictReader(open(sys.argv[2], newline='')))[0]
total = len(orders)
sched = sum(1 for r in orders if r['status'] == 'SCHEDULED')
blocked = sum(1 for r in orders if r['status'] == 'BLOCKED')
fulfilled = sum(float(r['qty']) for r in orders if r['status'] == 'SCHEDULED')
ok = (str(total) == summary['total_orders'] and
      str(sched) == summary['scheduled_orders'] and
      str(blocked) == summary['blocked_orders'] and
      abs(fulfilled - float(summary['total_fulfilled_qty'])) < 1e-9)
if not ok:
    raise SystemExit("[FAIL] IOP 逐单明细与聚合结果不一致: "
                     f"detail({total},{sched},{blocked},{fulfilled}) vs summary({summary})")
print("  [PASS] IOP 逐单明细计数 == 算子聚合结果")
PY
PASS=$((PASS+1))

# ---------------------------------------------------------------------------
echo
echo "[4/4] substitution —— 替代料三级决策 (一类选 ID 1 / 三类分配 10)"
SUB1="$WORK/sub1"; mkdir -p "$SUB1"
"$CLI" --engine substitution --in "$DEMO/substitution-benchmark" --out "$SUB1" --alt-class 1 --alt-group 1 --net-demand 50 >/dev/null
assert_header "$SUB1/substitution_decisions.csv" "category,chosen_part_id,basis,net_demand,remaining_on_hand_after"
assert_field  "$SUB1/substitution_decisions.csv" chosen_part_id 1

SUB2="$WORK/sub2"; mkdir -p "$SUB2"
"$CLI" --engine substitution --in "$DEMO/substitution-benchmark" --out "$SUB2" --alt-class 2 --alt-group 2 --net-demand 20 >/dev/null
assert_field  "$SUB2/substitution_decisions.csv" chosen_part_id 4

SUB3="$WORK/sub3"; mkdir -p "$SUB3"
"$CLI" --engine substitution --in "$DEMO/substitution-benchmark" --out "$SUB3" --alt-class 3 --alt-group 3 --net-demand 12 --day 1 >/dev/null
assert_header "$SUB3/substitution_allocations.csv" "day,parent_part_id,alt_part_id,allocated_qty,day_allocated,alt_class"
assert_rows   "$SUB3/substitution_allocations.csv" 1
assert_field  "$SUB3/substitution_allocations.csv" allocated_qty 10
assert_field  "$SUB3/substitution_allocations.csv" alt_part_id 3
assert_header "$SUB3/substitution_water.csv" "member_part_id,target_ratio,historical_qty,on_hand_before,safety_stock,allocatable_before,on_hand_after"
assert_field  "$SUB3/substitution_water.csv" safety_stock 10
assert_field  "$SUB3/substitution_water.csv" on_hand_after 90

echo
echo "========================================================"
echo "  通过: $PASS  失败: $FAIL"
echo "========================================================"
[ "$FAIL" -eq 0 ]
