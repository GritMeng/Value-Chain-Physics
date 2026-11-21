"""
IPC Core Independent Reference Engine (Python 独立数学审计参照实现)
用于与 C++17 开源微内核逐项双发交叉对账（零第三方库依赖）。
"""

import math
from typing import List, Dict, Tuple


def evaluate_sequencing_coupling_python(
    d1_req=4, d2_req=2, m1_avail=4, m2_oh=1, m2_po=3, m3_avail=2
) -> Dict[str, int]:
    trad_d1_m2 = min(m2_oh, m2_po)
    trad_d2_m2 = max(m2_oh, m2_po)
    trad_d1_fin = max(m1_avail, trad_d1_m2)
    trad_d2_fin = max(m3_avail, trad_d2_m2)

    d1_earliest = max(m1_avail, trad_d1_m2)
    d2_earliest = max(m3_avail, trad_d1_m2)
    if d2_earliest < d1_earliest and trad_d2_m2 <= d1_earliest:
        ipc_d2_fin = max(m3_avail, trad_d1_m2)
        ipc_d1_fin = max(m1_avail, trad_d2_m2)
        ipc_idle = max(0, ipc_d2_fin - trad_d1_m2)
    else:
        ipc_d1_fin, ipc_d2_fin = trad_d1_fin, trad_d2_fin
        ipc_idle = max(0, trad_d1_fin - trad_d1_m2)

    return {
        "trad_d1_finish": trad_d1_fin,
        "trad_d2_finish": trad_d2_fin,
        "trad_d2_delay": max(0, trad_d2_fin - d2_req),
        "trad_idle_lock": max(0, trad_d1_fin - trad_d1_m2),
        "ipc_d1_finish": ipc_d1_fin,
        "ipc_d2_finish": ipc_d2_fin,
        "ipc_d2_delay": max(0, ipc_d2_fin - d2_req),
        "ipc_idle_lock": ipc_idle,
    }


def evaluate_group_sub_python(
    g1_sunk=11000.0, g1_expedite=1000.0, g2_new=12000.0
) -> Dict[str, float]:
    trad_total = g2_new + g1_sunk
    if g1_expedite < g2_new:
        ipc_cash, ipc_scrap = g1_expedite, 0.0
    else:
        ipc_cash, ipc_scrap = g2_new, g1_sunk
    ipc_total = ipc_cash + ipc_scrap
    return {
        "trad_total": trad_total,
        "ipc_cash": ipc_cash,
        "ipc_scrap": ipc_scrap,
        "ipc_total": ipc_total,
        "net_savings": trad_total - ipc_total,
    }


def evaluate_incomplete_sub_python(
    gross_demand=1000.0,
    main_kitted=500.0,
    sub_avail=500.0,
    max_sub_ratio=0.30,
    dim_op="GE",
    candidate_dim=5000.0,
    required_dim=4500.0,
) -> Dict:
    dim_passed = (
        (candidate_dim + 1e-9 >= required_dim)
        if dim_op == "GE"
        else (abs(candidate_dim - required_dim) <= 1e-9)
    )
    theta = max(0.0, min(1.0, max_sub_ratio))
    gap = max(0.0, gross_demand - main_kitted)
    cap_gross = math.floor(theta * gross_demand + 1e-9)
    legacy_used = min(gap, sub_avail, cap_gross) if (dim_passed and theta > 0) else 0.0
    legacy_out = main_kitted + legacy_used
    legacy_ratio = (legacy_used / legacy_out) if legacy_out > 0 else 0.0

    if not dim_passed or theta <= 0:
        ipc_used = 0.0
    else:
        cap_anchor = (
            math.floor((theta / (1.0 - theta)) * main_kitted + 1e-9)
            if theta < 1.0 - 1e-9
            else 1e18
        )
        ipc_used = min(gap, sub_avail, cap_gross, cap_anchor)
    ipc_out = main_kitted + ipc_used
    ipc_ratio = (ipc_used / ipc_out) if ipc_out > 0 else 0.0
    return {
        "dim_passed": dim_passed,
        "legacy_sub_used": legacy_used,
        "legacy_ratio": legacy_ratio,
        "legacy_breached": legacy_ratio > theta + 1e-9,
        "ipc_sub_used": ipc_used,
        "ipc_ratio": ipc_ratio,
        "ipc_within_cap": ipc_ratio <= theta + 1e-9,
    }


def allocate_class1_python(net_demand: float, group_items: List[Dict]) -> int:
    total_hist = sum(item["cum_dispatched_qty"] for item in group_items)
    current_total = total_hist + net_demand
    best_alt = None
    max_gap = -1.0
    for item in group_items:
        due_qty = current_total * item["target_ratio"]
        gap = abs(item["cum_dispatched_qty"] - due_qty)
        if gap > max_gap or (
            abs(gap - max_gap) < 1e-9
            and (best_alt is None or item["target_ratio"] > best_alt["target_ratio"])
        ):
            max_gap = gap
            best_alt = item
    return best_alt["child_id"] if best_alt else -1


def allocate_class3_renorm_python(
    net_demand: float, candidates: List[Dict]
) -> List[Tuple[int, float]]:
    active = [dict(c, current_ratio=c["target_ratio"]) for c in candidates]
    rem = net_demand
    out = []
    while rem > 0 and active:
        for c in active:
            c["due_qty"] = c["current_ratio"] * rem
        active.sort(key=lambda x: (-x["due_qty"], -x["target_ratio"]))
        chosen = active.pop(0)
        lot = chosen["lot_size"]
        actual = math.ceil(chosen["due_qty"] / lot) * lot
        take = min(actual, chosen["avail"], rem)
        out.append((chosen["child_id"], take))
        rem -= take
        if rem <= 0 or not active:
            break
        s_due = sum(c["due_qty"] for c in active)
        for c in active:
            c["current_ratio"] = c["due_qty"] / s_due if s_due > 0 else 1.0 / len(active)
    return out


def evaluate_codp_python(
    bare_mfg_lt=4, pack_lt=1, dtol=3, sla=1, countries=4, sigma=40.0, z=2.0,
    bare_cost=600.0, fg_cost=800.0, legacy_early_engraving=False
) -> Dict:
    chip_gate1 = (bare_mfg_lt + pack_lt) <= dtol
    bare_gate1 = pack_lt <= dtol
    bare_gate2 = bare_gate1 and (not legacy_early_engraving)

    tau_a = max(0, bare_mfg_lt + pack_lt - sla)
    plan_a_units = countries * (z * sigma * math.sqrt(tau_a))
    plan_a_cap = plan_a_units * fg_cost

    tau_b = max(0, bare_mfg_lt)
    pooled_sigma = math.sqrt(countries) * sigma
    plan_b_units = z * pooled_sigma * math.sqrt(tau_b)
    plan_b_cap = plan_b_units * bare_cost

    return {
        "chip_gate1": chip_gate1,
        "bare_gate1": bare_gate1,
        "bare_gate2": bare_gate2,
        "plan_a_units": plan_a_units,
        "plan_a_cap": plan_a_cap,
        "plan_b_units": plan_b_units,
        "plan_b_cap": plan_b_cap,
        "unit_red_pct": (plan_a_units - plan_b_units) / plan_a_units * 100.0,
        "cap_red_pct": (plan_a_cap - plan_b_cap) / plan_a_cap * 100.0,
        "phi_shadow_cap": max(0.0, plan_a_cap - plan_b_cap),
    }


def evaluate_push_pull_python(
    dem_a=600.0, dem_b=400.0, sup_cap=800.0, daily_caps=(140.0, 140.0, 140.0, 140.0, 140.0, 100.0)
) -> Dict:
    tot = dem_a + dem_b
    eff = min(tot, sup_cap)
    alloc_a = eff * (dem_a / tot)
    alloc_b = eff * (dem_b / tot)
    tot_c = sum(daily_caps)
    da = [alloc_a * (c / tot_c) for c in daily_caps]
    db = [alloc_b * (c / tot_c) for c in daily_caps]
    return {
        "alloc_a": alloc_a,
        "alloc_b": alloc_b,
        "roll_a": dem_a - alloc_a,
        "roll_b": dem_b - alloc_b,
        "daily_a": da,
        "daily_b": db,
    }


def route_pull_overflow_python(
    order_qty, bucket_qty, month_cum, month_cap, days_to_due, procure_lt
) -> Tuple[str, float, float]:
    ov = max(0.0, order_qty - bucket_qty)
    if ov <= 1e-9:
        return ("IN_BUCKET_SILENT_CONSUME", 0.0, 0.0)
    if month_cum <= month_cap + 1e-9:
        return ("MODE_A_CONSUME_NEXT_BUCKET_ONLY", ov, 0.0)
    if days_to_due >= procure_lt:
        return ("MODE_B_TRIGGER_MRP_DIRECT", 0.0, ov)
    return ("MODE_C_BORROW_W2_AND_MRP_BACKFILL", ov, ov)


def itp_iop_alignment_python(
    master_demands: List[Dict], execution_demands: List[Dict], buffer_factor: float = 1.1
) -> Dict:
    firewalls = {}
    for d in master_demands:
        key = (d["due_day"], d["part_id"] // 10, d["customer_group"], d["region"])
        firewalls.setdefault(key, {"total_quota": 0.0, "consumed_qty": 0.0})
        firewalls[key]["total_quota"] += d["qty"] * buffer_factor

    scheduled, blocked = 0, 0
    for d in sorted(execution_demands, key=lambda x: x["priority"]):
        key = (d["due_day"], d["part_id"] // 10, d["customer_group"], d["region"])
        fw = firewalls.get(key)
        if fw and (fw["total_quota"] - fw["consumed_qty"]) >= d["qty"]:
            fw["consumed_qty"] += d["qty"]
            scheduled += 1
        else:
            blocked += 1
    return {"scheduled_orders": scheduled, "blocked_orders": blocked, "total_orders": len(execution_demands)}
