# -*- coding: utf-8 -*-
"""
IPC Core 测试数据生成器 (Deterministic Test Data Generator)

生成 IPC Core 引擎基准测试所需的全部 CSV 数据。
所有生成逻辑为纯函数推导，固定 seed，无随机性，保证完全可复现。

用法:
    python3 data/generate_data.py            # 生成全量数据
    python3 data/generate_data.py --sample   # 仅生成 sample/ 迷你集
"""
import os
import csv
import argparse
import random

HERE = os.path.dirname(os.path.abspath(__file__))

NUM_SKUS = 2_000_000
NUM_DEMANDS = 500_000


def write_csv(path, header, rows):
    """流式写入 CSV，避免大文件占用内存。"""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)
    size_mb = os.path.getsize(path) / 1e6
    print(f"  [OK] {os.path.relpath(path, HERE)}  ({size_mb:.1f} MB)")


# ---------------------------------------------------------------- 全量数据

def gen_parts():
    header = ["part_id", "part_code", "site", "safety_stock",
              "initial_on_hand", "lot_size", "lead_time"]
    rows = ((i, f"SKU_{i}", "PLANT_MAIN", 10.0, 100.0, 1.0, 1)
            for i in range(NUM_SKUS))
    write_csv(os.path.join(HERE, "parts.csv"), header, rows)


def gen_demands_master():
    header = ["demand_id", "part_id", "due_day", "qty",
              "priority", "customer_group", "region"]
    rows = ((i + 1, i % (NUM_SKUS // 2), i % 30 + 1, 50.0,
             i % 5 + 1, f"CUST_GRP_{i % 20}", f"REGION_{i % 5}")
            for i in range(NUM_DEMANDS))
    write_csv(os.path.join(HERE, "demands_master.csv"), header, rows)


def gen_demands_execution():
    """
    执行计划需求：刻意与主计划制造差异以触发 IOP 刚性阻断。
    每 5 条中有 2 条按 1.3 倍放大，使配额被突破 -> blocked_orders > 0
    """
    header = ["demand_id", "part_id", "due_day", "qty",
              "priority", "customer_group", "region"]
    rows = []
    for i in range(NUM_DEMANDS):
        qty = 50.0
        if i % 5 in (0, 1):
            qty = round(50.0 * 1.3, 2)   # 65.0 -> 超出 50*1.15=57.5 的防波堤配额
        rows.append((i + 1, i % (NUM_SKUS // 2), i % 30 + 1, qty,
                     i % 5 + 1, f"CUST_GRP_{i % 20}", f"REGION_{i % 5}"))
    write_csv(os.path.join(HERE, "demands_execution.csv"), header, rows)


def gen_small_tables():
    """BOM / ATP 供给 / 产能 / 替代料组 —— 小表，手工构造。"""

    # BOM：含主料与四类替代料示例（Class 1/2/3）
    # part 0 = 成品, 1..3 = 组件, 10..15 = 芯片替代料组
    write_csv(
        os.path.join(HERE, "bom.csv"),
        ["parent_id", "child_id", "usage_qty", "alt_class", "alt_group",
         "target_ratio", "historical_qty", "lot_size"],
        [
            (0, 1, 1.0, 0, 0, 1.0, 0.0, 1.0),    # 成品 -> 组件1
            (0, 2, 2.0, 0, 0, 1.0, 0.0, 1.0),    # 成品 -> 组件2
            (1, 10, 1.0, 1, 1, 0.6, 120.0, 1.0),  # Class 1 替代料 A (目标 60%)
            (1, 11, 1.0, 1, 1, 0.4, 100.0, 1.0),  # Class 1 替代料 B (目标 40%)
            (2, 12, 1.0, 2, 2, 0.7, 0.0, 1.0),    # Class 2 高优先
            (2, 13, 1.0, 2, 2, 0.3, 0.0, 1.0),    # Class 2 低优先
            (0, 14, 1.0, 3, 3, 0.5, 0.0, 5.0),    # Class 3 (Lot=5)
            (0, 15, 1.0, 3, 3, 0.5, 0.0, 1.0),    # Class 3 同组
        ])

    # ATP 供给：在手 / 在途 / 计划产出
    write_csv(
        os.path.join(HERE, "atp_supply.csv"),
        ["supply_code", "supply_type", "part_id", "available_day", "qty", "priority"],
        [
            ("OH_B",  "On-Hand",       1, 0,  50.0, 100),
            ("OH_C",  "On-Hand",       2, 0,  20.0, 100),
            ("SR_B1", "SR",            1, 2, 100.0, 200),
            ("SR_C1", "SR",            2, 3,  80.0, 200),
            ("PO_A1", "Planned-Order", 0, 5, 200.0, 300),
            ("OH_10", "On-Hand",      10, 0, 100.0, 100),
            ("OH_11", "On-Hand",      11, 0,  80.0, 100),
            ("OH_12", "On-Hand",      12, 0, 100.0, 100),
            ("OH_13", "On-Hand",      13, 0, 100.0, 100),
            ("OH_14", "On-Hand",      14, 0, 100.0, 100),
            ("OH_15", "On-Hand",      15, 0, 100.0, 100),
            ("OH_0",  "On-Hand",       0, 0,  10.0, 100),
        ])

    # 产能：2 个工作中心 × 30 天
    cap_rows = []
    for wc in ("WC_01", "WC_02"):
        for day in range(30):
            cap_rows.append((wc, day, 100.0))
    write_csv(os.path.join(HERE, "capacity.csv"),
              ["work_center", "day", "capacity_hours"], cap_rows)

    # 替代料组
    write_csv(
        os.path.join(HERE, "substitution_group.csv"),
        ["alt_group", "alt_class", "member_part_id", "target_ratio", "historical_qty"],
        [
            (1, 1, 10, 0.6, 120.0),
            (1, 1, 11, 0.4, 100.0),
            (2, 2, 12, 0.7,   0.0),
            (2, 2, 13, 0.3,   0.0),
            (3, 3, 14, 0.5,   0.0),
            (3, 3, 15, 0.5,   0.0),
        ])


# ---------------------------------------------------------------- 迷你集

def gen_sample():
    """手工可读的迷你数据集，用于单步调试与格式示例。"""
    sample_dir = os.path.join(HERE, "sample")

    write_csv(os.path.join(sample_dir, "parts.csv"),
              ["part_id", "part_code", "site", "safety_stock",
               "initial_on_hand", "lot_size", "lead_time"],
              [
                  (0, "FINISHED_PRODUCT_A", "PLANT_01", 0.0, 0.0, 1.0, 2),
                  (1, "SUB_ASSEMBLY_B",     "PLANT_01", 0.0, 50.0, 1.0, 1),
                  (2, "RAW_MATERIAL_C",     "PLANT_01", 0.0, 20.0, 1.0, 0),
                  (3, "ALT_CHIP_CLASS1",    "PLANT_01", 0.0, 100.0, 1.0, 0),
                  (4, "ALT_CHIP_CLASS2",    "PLANT_01", 0.0, 100.0, 1.0, 0),
                  (5, "ALT_CHIP_CLASS3",    "PLANT_01", 10.0, 100.0, 5.0, 0),
              ])

    write_csv(os.path.join(sample_dir, "demands_master.csv"),
              ["demand_id", "part_id", "due_day", "qty",
               "priority", "customer_group", "region"],
              [
                  (1, 10, 5, 1000.0, 1, "VIP_CLIENT_A", "NORTH_AMERICA"),
                  (2, 20, 5,  500.0, 2, "RETAIL_CLIENT_B", "EAST_ASIA"),
              ])

    write_csv(os.path.join(sample_dir, "demands_execution.csv"),
              ["demand_id", "part_id", "due_day", "qty",
               "priority", "customer_group", "region"],
              [
                  (101, 10, 5, 600.0, 1, "VIP_CLIENT_A", "NORTH_AMERICA"),
                  (102, 10, 5, 450.0, 2, "VIP_CLIENT_A", "NORTH_AMERICA"),
                  (103, 10, 5, 300.0, 3, "VIP_CLIENT_A", "NORTH_AMERICA"),
                  (104, 20, 5, 500.0, 1, "RETAIL_CLIENT_B", "EAST_ASIA"),
              ])

    write_csv(os.path.join(sample_dir, "bom.csv"),
              ["parent_id", "child_id", "usage_qty", "alt_class", "alt_group",
               "target_ratio", "historical_qty", "lot_size"],
              [
                  (0, 1, 1.0, 0, 0, 1.0, 0.0, 1.0),
                  (1, 2, 2.0, 0, 0, 1.0, 0.0, 1.0),
                  (0, 3, 1.0, 1, 1, 0.6, 120.0, 1.0),
                  (0, 4, 1.0, 1, 1, 0.4, 100.0, 1.0),
                  (0, 5, 1.0, 3, 3, 0.5, 0.0, 5.0),
              ])

    write_csv(os.path.join(sample_dir, "atp_supply.csv"),
              ["supply_code", "supply_type", "part_id", "available_day", "qty", "priority"],
              [
                  ("OH_B", "On-Hand", 1, 0, 50.0, 100),
                  ("OH_C", "On-Hand", 2, 0, 20.0, 100),
                  ("PO_A1", "Planned-Order", 0, 5, 200.0, 300),
              ])

    write_csv(os.path.join(sample_dir, "capacity.csv"),
              ["work_center", "day", "capacity_hours"],
              [("WC_01", d, 100.0) for d in range(30)])

    write_csv(os.path.join(sample_dir, "substitution_group.csv"),
              ["alt_group", "alt_class", "member_part_id", "target_ratio", "historical_qty"],
              [
                  (1, 1, 3, 0.6, 120.0),
                  (1, 1, 4, 0.4, 100.0),
                  (3, 3, 5, 0.5, 0.0),
              ])

    print(f"  [OK] sample/ 迷你数据集生成完成")


def main():
    ap = argparse.ArgumentParser(description="IPC Core deterministic data generator")
    ap.add_argument("--sample", action="store_true", help="仅生成 sample/ 迷你集")
    args = ap.parse_args()

    print("=" * 56)
    print("  IPC Core 测试数据生成器")
    print("=" * 56)

    if args.sample:
        gen_sample()
    else:
        print(f"\n[1/6] parts.csv  ({NUM_SKUS:,} 行)...")
        gen_parts()
        print(f"\n[2/6] demands_master.csv  ({NUM_DEMANDS:,} 行)...")
        gen_demands_master()
        print(f"\n[3/6] demands_execution.csv  ({NUM_DEMANDS:,} 行, 含 1.3x 超量插单)...")
        gen_demands_execution()
        print("\n[4/6] bom.csv / atp_supply.csv / capacity.csv / substitution_group.csv ...")
        gen_small_tables()
        print("\n[5/6] sample/ 迷你数据集 ...")
        gen_sample()
        print("\n[6/6] 完成。")

    print("\n数据生成完毕。\n")


if __name__ == "__main__":
    main()
