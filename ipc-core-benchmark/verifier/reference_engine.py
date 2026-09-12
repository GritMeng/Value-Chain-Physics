"""
IPC Core Reference Engine (Python 审计引擎参考实现)
用于验证替代料配额、ATP/CTP 计算与 ITP/IOP 协同配额逻辑的数学正确性。
"""

import math
from typing import List, Dict, Tuple

def allocate_class1_python(net_demand: float, group_items: List[Dict], current_on_hand: List[float]) -> int:
    """一类替代料配额平摊算法 Python 审计参照实现"""
    total_hist = sum(item["historical_qty"] for item in group_items)
    current_total_demand = total_hist + net_demand
    
    best_alt = None
    max_gap = -1.0
    
    for item in group_items:
        due_qty = current_total_demand * item["target_ratio"]
        gap = abs(item["historical_qty"] - due_qty)
        
        if gap > max_gap:
            max_gap = gap;
            best_alt = item
        elif abs(gap - max_gap) < 1e-9:
            if best_alt is None or item["target_ratio"] > best_alt["target_ratio"]:
                best_alt = item
                
    return best_alt["child_id"] if best_alt else -1


def itp_iop_alignment_python(master_demands: List[Dict], execution_demands: List[Dict], buffer_factor: float = 1.1) -> Dict:
    """ITP / IOP 配额拦截协同 Python 审计参照实现"""
    allotments = {}
    
    # 1. 生成 ITP 主计划防波堤
    for demand in master_demands:
        key = (demand["due_day"], demand["part_id"] // 10, hash(demand["customer_group"]) % 100, hash(demand["region"]) % 10)
        if key not in allotments:
            allotments[key] = {"total_quota": 0.0, "consumed_qty": 0.0}
        allotments[key]["total_quota"] += demand["qty"] * buffer_factor
        
    # 2. IOP 车间执行校验
    scheduled_orders = 0
    blocked_orders = 0
    sorted_demands = sorted(execution_demands, key=lambda x: x["priority"])
    
    for demand in sorted_demands:
        key = (demand["due_day"], demand["part_id"] // 10, hash(demand["customer_group"]) % 100, hash(demand["region"]) % 10)
        if key in allotments:
            remaining = allotments[key]["total_quota"] - allotments[key]["consumed_qty"]
            if remaining >= demand["qty"]:
                allotments[key]["consumed_qty"] += demand["qty"]
                scheduled_orders += 1
            else:
                blocked_orders += 1
        else:
            blocked_orders += 1
            
    return {
        "scheduled_orders": scheduled_orders,
        "blocked_orders": blocked_orders,
        "total_orders": len(execution_demands)
    }

if __name__ == "__main__":
    print("IPC Core Reference Engine Loaded Successfully.")
