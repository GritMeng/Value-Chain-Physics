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
        ipc_total = ipc_cash + g1_sunk  # CFO 严守口径：正常领用库内长板计入账面成本（呆滞报废为 0）
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


def evaluate_two_stage_iop_python(
    forecast_bare_qty: float = 1000.0,
    daily_wc_cap: float = 300.0,
    chip_oh_day1: float = 600.0,
    chip_po_day: int = 4,
    chip_po_qty: float = 400.0,
    post_pack_lt: int = 1,
    horizon_days: int = 8,
    pull_orders: List[Dict] = None,
) -> Dict:
    if pull_orders is None:
        pull_orders = [
            {"order_id": "ORD_EU_500", "req_due_day": 4, "req_qty": 500.0, "allotment_cap": 500.0, "priority": 1},
            {"order_id": "ORD_AF_400", "req_due_day": 4, "req_qty": 400.0, "allotment_cap": 400.0, "priority": 2},
            {"order_id": "ORD_LATAM_250", "req_due_day": 4, "req_qty": 250.0, "allotment_cap": 100.0, "priority": 3},
        ]
    rem_fc = forecast_bare_qty
    avail_chip = chip_oh_day1
    daily_built = []
    idle_days = 0
    for d in range(1, horizon_days + 1):
        if d == chip_po_day:
            avail_chip += chip_po_qty
        if rem_fc > 1e-9 and avail_chip <= 1e-9 and daily_wc_cap > 0:
            idle_days += 1
        b = min(rem_fc, daily_wc_cap, avail_chip)
        daily_built.append(b)
        avail_chip -= b
        rem_fc -= b

    pool_rem = list(daily_built)
    bare_cum_d4 = sum(daily_built[: max(0, 4 - post_pack_lt)])
    legacy_d4_total = sum(o["req_qty"] for o in pull_orders if max(1 + post_pack_lt, o["req_due_day"]) <= 4)
    legacy_broken = max(0.0, legacy_d4_total - bare_cum_d4)

    promises = []
    total_blocked = 0.0
    for o in sorted(pull_orders, key=lambda x: x["priority"]):
        adm = min(o["req_qty"], max(0.0, o["allotment_cap"]))
        blk = max(0.0, o["req_qty"] - adm)
        total_blocked += blk
        need = adm
        last_d = 1
        for d_idx in range(horizon_days):
            if need <= 1e-9:
                break
            if pool_rem[d_idx] > 1e-9:
                tk = min(need, pool_rem[d_idx])
                pool_rem[d_idx] -= tk
                need -= tk
                last_d = d_idx + 1
        if need > 1e-9:
            last_d = horizon_days + 1
        promises.append({
            "order_id": o["order_id"],
            "admitted_qty": adm,
            "blocked_qty": blk,
            "ipc_promised_day": max(o["req_due_day"], last_d + post_pack_lt),
        })

    return {
        "daily_bare_built": daily_built,
        "idle_wait_chip_days": idle_days,
        "legacy_broken_promise_qty": legacy_broken,
        "ipc_broken_promise_qty": 0.0,
        "total_quota_blocked_qty": total_blocked,
        "promises": promises,
    }


def evaluate_network_algebra_python(
    req_spec: float = 256.0, req_cert_mask: int = 0x01, leaf_op: str = "GE"
) -> Dict:
    nodes = {
        0: ("FG_PHONE_EU", 256.0, 0x01),
        1: ("FG_PHONE_AF", 128.0, 0x02),
        2: ("SEMI_MAINBOARD", 0.0, 0x03),
        3: ("SEMI_POWER_KIT", 0.0, 0x03),
        4: ("PMIC_CHIP_SHARED", 0.0, 0x03),
        5: ("DIE_512M", 512.0, 0x03),
        6: ("DIE_256M", 256.0, 0x01),
        7: ("DIE_128M", 128.0, 0x02),
    }
    edges = [
        (0, 2, 1.0, "PASS"),
        (0, 3, 1.0, "PASS"),
        (1, 2, 1.0, "PASS"),
        (1, 3, 1.0, "PASS"),
        (2, 4, 1.0, "PASS"),
        (3, 4, 2.0, "PASS"),
        (2, 5, 1.0, "GE"),
        (2, 6, 1.0, "GE"),
        (2, 7, 1.0, "GE"),
    ]
    adj = {}
    rev = {}
    for p, c, u, op in edges:
        adj.setdefault(p, []).append((c, u, op))
        rev.setdefault(c, []).append(p)

    # Preorder spans from root 0
    vis = set()
    spans = {}
    rank_counter = [0]

    def dfs_span(u: int) -> int:
        vis.add(u)
        rank_counter[0] += 1
        r0 = rank_counter[0]
        sz = 1
        for c, _, _ in adj.get(u, []):
            if c not in vis:
                sz += dfs_span(c)
        spans[u] = (r0, sz)
        return sz

    dfs_span(0)

    pruned = [0]
    path_rows = []
    distinct_flux = {}

    def dfs_exp(u: int, qty: float) -> None:
        path_rows.append((u, qty))
        distinct_flux[u] = distinct_flux.get(u, 0.0) + qty
        for c, usage, op in adj.get(u, []):
            eff_op = leaf_op if op != "PASS" else "PASS"
            spec_v, cert_m = nodes[c][1], nodes[c][2]
            spec_ok = True
            if eff_op == "GE":
                spec_ok = (spec_v + 1e-9) >= req_spec
            elif eff_op == "EQ":
                spec_ok = abs(spec_v - req_spec) <= 1e-9
            cert_ok = (cert_m & req_cert_mask) == req_cert_mask
            if not (spec_ok and cert_ok):
                pruned[0] += 1
                continue
            dfs_exp(c, qty * usage)

    dfs_exp(0, 100.0)

    # Reverse Where-Used from shared PMIC (node 4)
    q = [4]
    seen = {4}
    roots = []
    while q:
        cur = q.pop(0)
        parents = rev.get(cur, [])
        if cur != 4 and not parents:
            roots.append(cur)
        for p in parents:
            if p not in seen:
                seen.add(p)
                q.append(p)

    return {
        "root_span": spans[0],
        "pruned_edges": pruned[0],
        "pmic_path_count": sum(1 for u, _ in path_rows if u == 4),
        "pmic_distinct_qty": distinct_flux.get(4, 0.0),
        "where_used_roots": sorted(roots),
    }
