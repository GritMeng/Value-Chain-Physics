import os
import shutil
import subprocess
import duckdb
import datetime
import sys

DB_PATH = "sandbox_modes_test.db"
SOLVER_EXE = "main_mem3.exe"

def clean_database():
    print("[*] Initializing sandbox database...")
    for path in [DB_PATH, DB_PATH + ".wal"]:
        if os.path.exists(path):
            try:
                os.remove(path)
            except Exception:
                pass
    # Copy baseline database structure
    shutil.copyfile("data/ipc.db", DB_PATH)
    
    conn = duckdb.connect(DB_PATH)
    # Clear tables to start fresh
    tables_to_clear = [
        "ipc_consensus_forecast", "ipc_independent_demand", "ipc_material_node",
        "ipc_onhand", "ipc_scheduled_receipt", "ipc_bom_route", "ipc_bom_item",
        "ipc_planned_order_ledger", "ipc_alternate_allocation", "ipc_bom_explosion_network",
        "ipc_dispatch_ledger", "ipc_part_status", "ipc_swap_result",
        "ipc_allotment_constraint", "ipc_allotment_ledger",
        "ipc_planned_supply_assignment", "ipc_supply_assignment",
        "ipc_hierarchy_product_family", "ipc_hierarchy_customer", "ipc_customer", "ipc_sop_calendar_date"
    ]
    for t in tables_to_clear:
        try:
            conn.execute(f"DELETE FROM {t};")
        except Exception as e:
            print(f"Warning: could not clear table {t}: {e}")
    conn.execute("CREATE TABLE IF NOT EXISTS ipc_solver_config (param_name VARCHAR PRIMARY KEY, param_value VARCHAR);")
    conn.execute("INSERT OR IGNORE INTO ipc_solver_config VALUES ('solver_mode', 'iop');")
    conn.execute("INSERT OR IGNORE INTO ipc_solver_config VALUES ('solver_step', 'all');")
    conn.close()

def ingest_master_data():
    print("[*] Ingesting master data...")
    conn = duckdb.connect(DB_PATH)
    # Ingest materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer)
        VALUES
        ('FG_A', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true),
        ('FG_B', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true),
        ('SEMI_A', 'SEMI', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
        ('RAW_X', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true);
    """)
    # Ingest BOM Route
    conn.execute("""
        INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type)
        VALUES
        ('SITE_001', 'FG_A', 'BOM_FGA', 1, 'MPS'),
        ('SITE_001', 'FG_B', 'BOM_FGB', 1, 'MPS'),
        ('SITE_001', 'SEMI_A', 'BOM_SEMIA', 1, 'MRP');
    """)
    # Ingest BOM Items
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, relationship_type)
        VALUES
        ('BOM_FGA', 'SITE_001', 'SEMI_A', 1.0, 0.0, '', 1, 'std'),
        ('BOM_FGB', 'SITE_001', 'SEMI_A', 1.0, 0.0, '', 1, 'std'),
        ('BOM_SEMIA', 'SITE_001', 'RAW_X', 1.0, 0.0, '', 1, 'std');
    """)
    # Ingest On-Hand
    conn.execute("""
        INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type)
        VALUES
        ('WH_MAIN', 'RAW_X', 'SITE_001', '2026-05-29', 1000000.0, 'Standard');
    """)
    
    # Ingest hierarchies (needed for allotment groupings)
    conn.execute("INSERT INTO ipc_hierarchy_product_family (family_num, description, material) VALUES ('FAM_A', 'Family A', 'FG_A'), ('FAM_B', 'Family B', 'FG_B');")
    conn.execute("INSERT INTO ipc_hierarchy_customer (customer, parent_customer) VALUES ('CUST_VVIP', 'CG_VVIP'), ('CUST_NORMAL', 'CG_NORMAL');")
    conn.execute("INSERT INTO ipc_customer (customer, region, site, name) VALUES ('CUST_VVIP', 'US', 'SITE_001', 'VVIP Customer'), ('CUST_NORMAL', 'US', 'SITE_001', 'Normal Customer');")
    
    # Set Calendar dates
    start_date = datetime.date(2026, 5, 29)
    for i in range(100):
        d = start_date + datetime.timedelta(days=i)
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'DEFAULT');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_001');", (d,))
        
    conn.close()

def run_solver(mode, step):
    print(f"\n>>> Running solver in Mode: {mode}, Step: {step}...")
    conn = duckdb.connect(DB_PATH)
    conn.execute("UPDATE ipc_solver_config SET param_value = ? WHERE param_name = 'solver_mode';", (mode,))
    conn.execute("UPDATE ipc_solver_config SET param_value = ? WHERE param_name = 'solver_step';", (step,))
    conn.close()
    
    res = subprocess.run([SOLVER_EXE, "--db", DB_PATH], stdin=subprocess.DEVNULL, capture_output=True, text=True, encoding='utf-8', errors='ignore')
    print("----- Solver Stdout -----")
    try:
        print(res.stdout)
    except Exception:
        try:
            print(res.stdout.encode('gbk', errors='replace').decode('gbk'))
        except Exception:
            print("[Could not print stdout due to encoding issues]")
    print("-------------------------")
    if res.returncode != 0:
        print("Solver execution failed! Output:")
        print(res.stderr)
        raise RuntimeError(f"Solver exited with code {res.returncode}")
    print("Solver ran successfully.")
    return res.stdout

def test_itp_mode():
    print("\n=================================================================")
    print("  TEST 1: ITP Mode (Tactical Balancing & Bottleneck Gating)")
    print("=================================================================")
    clean_database()
    ingest_master_data()
    
    # Ingest Demands: VVIP (Tier 1) FG_A, Normal (Tier 3) FG_B on Day 10
    # Qty = 80000 each, total = 160000. Finished goods capacity = 91000 total.
    conn = duckdb.connect(DB_PATH)
    conn.execute("""
        INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
        VALUES
        ('DEMAND_VVIP', 1.0, 'FG_A', 'SITE_001', 'CUST_VVIP', '2026-06-08', '2026-06-08', 80000.0, 80000.0, 'OPEN', 1, 'SITE_001', 'N', 1, 800000.0),
        ('DEMAND_NORMAL', 1.0, 'FG_B', 'SITE_001', 'CUST_NORMAL', '2026-06-08', '2026-06-08', 80000.0, 80000.0, 'OPEN', 5, 'SITE_001', 'N', 3, 400000.0);
    """)
    conn.close()
    
    # Phase 1: Run unconstrained MRP (solver_step = "lbl")
    stdout_mrp = run_solver(mode="itp", step="lbl")
    
    # Assertions for unconstrained MRP:
    assert "Starting LBL MRP Netting Engine" in stdout_mrp or "Starting LBL MRP" in stdout_mrp, "LBL MRP engine was not started!"
    assert "发现 Gating 瓶颈" not in stdout_mrp, "Gating bottleneck should not be detected in unconstrained MRP!"
    
    # Check that planned orders are fully created in DuckDB (no constraint limit scaling yet)
    conn = duckdb.connect(DB_PATH)
    orders = conn.execute("SELECT part_code, SUM(order_qty) FROM ipc_planned_order_ledger GROUP BY part_code ORDER BY part_code;").fetchall()
    conn.close()
    
    print("Planned orders generated during ITP MRP (LBL):", orders)
    orders_map = dict(orders)
    assert orders_map.get("FG_A", 0.0) == 80000.0, f"Expected FG_A planned order qty to be 80000.0, got {orders_map.get('FG_A', 0.0)}"
    assert orders_map.get("FG_B", 0.0) == 80000.0, f"Expected FG_B planned order qty to be 80000.0, got {orders_map.get('FG_B', 0.0)}"
    print("[PASS] ITP Phase 1: Unconstrained MRP verified successfully!")

    # Phase 2: Run constrained plan (solver_step = "all" -> LBL + DBD)
    # We clean the database and re-ingest fresh master data and demands to ensure no state pollution from Phase 1
    clean_database()
    ingest_master_data()
    
    conn = duckdb.connect(DB_PATH)
    conn.execute("""
        INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
        VALUES
        ('DEMAND_VVIP', 1.0, 'FG_A', 'SITE_001', 'CUST_VVIP', '2026-06-08', '2026-06-08', 80000.0, 80000.0, 'OPEN', 1, 'SITE_001', 'N', 1, 800000.0),
        ('DEMAND_NORMAL', 1.0, 'FG_B', 'SITE_001', 'CUST_NORMAL', '2026-06-08', '2026-06-08', 80000.0, 80000.0, 'OPEN', 5, 'SITE_001', 'N', 3, 400000.0);
    """)
    conn.close()

    stdout_const = run_solver(mode="itp", step="all")
    
    # Verification: Check stdout for capacity bottleneck detection and correct scaling output
    assert "发现 Gating 瓶颈" in stdout_const, "Gating bottleneck was not detected in ITP constrained plan!"
    assert "LINE_FINISHED" in stdout_const, "LINE_FINISHED constraint was not gated!"
    assert "满足: 11000.00" in stdout_const or "满足: 11000" in stdout_const, "Normal demand was not correctly scaled down to 11000.00!"
    print("[PASS] ITP Phase 2: Constrained Plan (LBL+DBD) bottleneck scaling and VVIP priority verified via solver logs!")

def test_iop_mode():
    print("\n=================================================================")
    print("  TEST 2: IOP Mode (Detailed Allotment Breakwater Enforcement)")
    print("=================================================================")
    clean_database()
    ingest_master_data()
    
    # Ingest allotment constraints directly (simulating locking after ITP scaling)
    # FG_A allotment limit = 80, FG_B allotment limit = 20
    # Must use 'baseline' as the scenario_id because the solver execution defaults to 'baseline'
    conn = duckdb.connect(DB_PATH)
    conn.execute("""
        INSERT INTO ipc_allotment_constraint (scenario_id, part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked)
        VALUES
        ('baseline', 'FG_A', 'SITE_001', '*', '*', 'FAM_A', 10, 80.0, 80.0, true),
        ('baseline', 'FG_B', 'SITE_001', '*', '*', 'FAM_B', 10, 20.0, 20.0, true);
    """)
    
    # Ingest new demands with swapped priorities: Normal runs first (Tier 1, Priority 1), VVIP runs second (Tier 3, Priority 5)
    # Each wants 80.0 on Day 10
    conn.execute("""
        INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
        VALUES
        ('DEMAND_VVIP', 1.0, 'FG_A', 'SITE_001', 'CUST_VVIP', '2026-06-08', '2026-06-08', 80.0, 80.0, 'OPEN', 5, 'SITE_001', 'N', 3, 800.0),
        ('DEMAND_NORMAL', 1.0, 'FG_B', 'SITE_001', 'CUST_NORMAL', '2026-06-08', '2026-06-08', 80.0, 80.0, 'OPEN', 1, 'SITE_001', 'N', 1, 400.0);
    """)
    conn.close()
    
    stdout = run_solver(mode="iop", step="all")
    
    conn = duckdb.connect(DB_PATH)
    ledger = conn.execute("SELECT product_family, allotment_limit, consumed_qty, blocked_demand_qty FROM ipc_allotment_ledger ORDER BY product_family;").fetchall()
    print("Allotment Ledger in IOP:")
    for row in ledger:
        print(f"  Family: {row[0]}, Limit: {row[1]}, Consumed: {row[2]}, Blocked: {row[3]}")
    conn.close()
    
    # Assertions
    # FAM_A (FG_A): Limit = 80, Consumed = 80, Blocked = 0
    # FAM_B (FG_B): Limit = 20, Consumed = 20, Blocked = 60 (capped at limit 20, demand is 80)
    ledger_map = {r[0]: (r[1], r[2], r[3]) for r in ledger}
    assert ledger_map['FAM_A'][0] == 80.0, f"Expected FAM_A limit to be 80.0, got {ledger_map['FAM_A'][0]}"
    assert ledger_map['FAM_B'][0] == 20.0, f"Expected FAM_B limit to be 20.0, got {ledger_map['FAM_B'][0]}"
    assert ledger_map['FAM_B'][1] == 20.0, f"Expected FAM_B consumed to be 20.0, got {ledger_map['FAM_B'][1]}"
    assert ledger_map['FAM_B'][2] == 60.0, f"Expected FAM_B blocked to be 60.0, got {ledger_map['FAM_B'][2]}"
    print("[PASS] IOP Mode allotment breakwater enforcement verified successfully!")

def test_ibp_mode():
    print("\n=================================================================")
    print("  TEST 3: IBP Mode (Consensus Disaggregation & Holt-Winters)")
    print("=================================================================")
    clean_database()
    ingest_master_data()
    
    conn = duckdb.connect(DB_PATH)
    # 1. Populate historic daily demands to run Holt-Winters on FG_A
    # We will generate a simple history of 35 days
    start_date = datetime.date(2026, 5, 29)
    for d_idx in range(35):
        due_date = start_date + datetime.timedelta(days=d_idx)
        due_date_str = due_date.strftime("%Y-%m-%d")
        qty = 100.0 + (d_idx % 7) * 10.0 # basic seasonal wave
        conn.execute("""
            INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
            VALUES
            (?, 1.0, 'FG_A', 'SITE_001', 'CUST_VVIP', ?, ?, ?, ?, 'OPEN', 1, 'SITE_001', 'N', 1, 1000.0);
        """, (f"HIST_DEMAND_{d_idx}", due_date_str, due_date_str, qty, qty))
        
    # 2. Ingest Consensus Forecast for FG_A@SITE_001 with total quantity = 5000.0
    # Note: Use 'FG_A@SITE_001' to align with in-memory demand part vocab mappings!
    # Note: Also insert '5000.0' to consensus_forecast column because it is non-nullable!
    conn.execute("""
        INSERT INTO ipc_consensus_forecast (part, customer, qty, unit_price, sales_qty, marketing_qty, statistical_qty, consensus_forecast, override_qty)
        VALUES
        ('FG_A@SITE_001', 'CUST_VVIP', 5000.0, 10.0, 4800.0, 5200.0, 0.0, '5000.0', 5000.0);
    """)
    conn.close()
    
    # Run solver in IBP mode. This corresponds to solver_step != "dbd" (e.g. solver_step = "lbl" or "all")
    # We set mode = "iop" and step = "lbl" to trigger IBP consensus disaggregation + MRP netting
    stdout = run_solver(mode="iop", step="lbl")
    
    conn = duckdb.connect(DB_PATH)
    # Check disaggregated independent demands in DuckDB
    # Total sum of demands for FG_A should now equal the consensus forecast quantity (5000.0)
    tot_qty = conn.execute("SELECT SUM(request_qty) FROM ipc_independent_demand WHERE part = 'FG_A'").fetchone()[0]
    print(f"Total FG_A independent demand quantity after disaggregation: {tot_qty}")
    
    # Check safety stock populated in material status
    ss_row = conn.execute("SELECT part, safety_stock FROM ipc_material_node WHERE part = 'FG_A'").fetchone()
    print(f"Safety stock calculated by Holt-Winters for FG_A: {ss_row}")
    
    conn.close()
    
    # Assertions
    # Sum of demands should be 5000.0 (consensus forecast quantity)
    assert tot_qty is not None and abs(tot_qty - 5000.0) < 1e-3, f"Expected disaggregated demand sum to be 5000.0, got {tot_qty}"
    # Safety stock should be calculated and non-zero
    assert ss_row is not None and ss_row[1] > 0.0, f"Expected calculated safety stock to be positive, got {ss_row}"
    
    print("[PASS] IBP Mode disaggregation and Holt-Winters safety stock verified successfully!")

def test_advanced_features_e2e():
    print("\n=================================================================")
    print("  TEST 4: Advanced Features (Safety Buffer, ECN Cutovers, Cross-Site, Planning BOM)")
    print("=================================================================")
    clean_database()
    
    conn = duckdb.connect(DB_PATH)
    # 1. Ingest parts
    # COMP_ALT is at SITE_002, with safety_stock = 100.0, transshipment_lead_time = 3, selling_ave_price (cost) = 1.5
    # FG_A and FG_B have selling_ave_price = 10.0
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, selling_ave_price, transshipment_lead_time, safety_stock)
        VALUES
        ('FG_A', 'FINISHED', 'MPS', 'SITE_001', false, 10.0, 0, 0.0),
        ('FG_B', 'FINISHED', 'MPS', 'SITE_001', false, 10.0, 0, 0.0),
        ('COMP_1', 'RAW', 'MRP', 'SITE_001', false, 2.0, 0, 0.0),
        ('COMP_ALT', 'RAW', 'MRP', 'SITE_002', false, 1.5, 3, 100.0),
        ('COMP_NEW', 'RAW', 'MRP', 'SITE_001', false, 2.0, 0, 0.0);
    """)

    # 2. Ingest BOM Route
    conn.execute("""
        INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type)
        VALUES
        ('SITE_001', 'FG_A', 'BOM_FGA', 1, 'MPS'),
        ('SITE_001', 'FG_B', 'BOM_FGB', 1, 'MPS');
    """)

    # 3. Ingest BOM Items:
    # alt_group_id = 1 (we map alt_grp as 'ALT_GRP_1' which translates to alt_group_id = 1 in the solver)
    # eff_start_day, eff_end_day, relationship_type
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, eff_start_day, eff_end_day, relationship_type)
        VALUES
        ('BOM_FGA', 'SITE_001', 'COMP_1', 1.0, 0.0, 'ALT_GRP_1', 1, 1.0, 0, 10, 'hard'),
        ('BOM_FGA', 'SITE_002', 'COMP_ALT', 1.0, 0.0, 'ALT_GRP_1', 2, 1.0, 0, 10, 'hard'),
        ('BOM_FGA', 'SITE_001', 'COMP_NEW', 1.0, 0.0, '', 1, 1.0, 11, -1, 'std'),
        
        ('BOM_FGB', 'SITE_001', 'COMP_1', 1.0, 0.0, 'ALT_GRP_1', 1, 1.0, 0, 10, 'soft'),
        ('BOM_FGB', 'SITE_002', 'COMP_ALT', 1.0, 0.0, 'ALT_GRP_1', 2, 1.0, 0, 10, 'soft'),
        ('BOM_FGB', 'SITE_001', 'COMP_NEW', 1.0, 0.0, '', 1, 1.0, 11, -1, 'std');
    """)

    # 4. Ingest On-Hand:
    # COMP_ALT has 150.0 on-hand (with 100.0 safety stock, so 50.0 is available for substitution)
    # COMP_1 has 30.0 on-hand at SITE_001
    conn.execute("""
        INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type)
        VALUES
        ('WH_ALT', 'COMP_ALT', 'SITE_002', '2026-05-29', 150.0, 'Standard'),
        ('WH_MAIN', 'COMP_1', 'SITE_001', '2026-05-29', 30.0, 'Standard');
    """)

    # 5. Ingest Hierarchy (Planning BOM product family)
    # FG_A and FG_B belong to family FAM_A
    conn.execute("INSERT INTO ipc_hierarchy_product_family (family_num, description, material) VALUES ('FAM_A', 'Family A', 'FG_A'), ('FAM_A', 'Family A', 'FG_B');")
    conn.execute("INSERT INTO ipc_hierarchy_customer (customer, parent_customer) VALUES ('CUST_VVIP', 'CG_VVIP');")
    conn.execute("INSERT INTO ipc_customer (customer, region, site, name) VALUES ('CUST_VVIP', 'US', 'SITE_001', 'VVIP Customer');")

    # Ingest Consensus Forecast for FAM_A with qty = 1000.0
    conn.execute("""
        INSERT INTO ipc_consensus_forecast (part, customer, qty, unit_price, sales_qty, marketing_qty, statistical_qty, consensus_forecast, override_qty)
        VALUES
        ('FAM_A', 'CUST_VVIP', 1000.0, 10.0, 1000.0, 1000.0, 0.0, '1000.0', 1000.0);
    """)

    # Ingest daily demands for FG_A and FG_B to establish historical disaggregation shares (3:2)
    # FG_A demand sum = 3.0, FG_B demand sum = 2.0
    # Note: demands are on Day 15 (2026-06-13, which is 15 days offset from 2026-05-29)
    conn.execute("""
        INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
        VALUES
        ('D1_A', 1.0, 'FG_A', 'SITE_001', 'CUST_VVIP', '2026-06-13', '2026-06-13', 3.0, 3.0, 'OPEN', 1, 'SITE_001', 'N', 1, 100.0),
        ('D1_B', 1.0, 'FG_B', 'SITE_001', 'CUST_VVIP', '2026-06-13', '2026-06-13', 2.0, 2.0, 'OPEN', 1, 'SITE_001', 'N', 1, 100.0);
    """)

    # Set Calendar dates
    start_date = datetime.date(2026, 5, 29)
    for i in range(100):
        d = start_date + datetime.timedelta(days=i)
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'DEFAULT');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_001');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_002');", (d,))
    conn.close()

    # Run solver in IOP mode to disaggregate forecast & run MRP netting
    run_solver(mode="iop", step="all")

    # Connect to check results
    conn = duckdb.connect(DB_PATH)
    
    # Check disaggregated demands (Planning BOM):
    # Sum of demands should be split 3:2 -> FG_A = 600.0, FG_B = 400.0
    fg_a_sum = conn.execute("SELECT SUM(request_qty) FROM ipc_independent_demand WHERE part = 'FG_A'").fetchone()[0]
    fg_b_sum = conn.execute("SELECT SUM(request_qty) FROM ipc_independent_demand WHERE part = 'FG_B'").fetchone()[0]
    print(f"Planning BOM Disaggregation check: FG_A = {fg_a_sum}, FG_B = {fg_b_sum}")
    assert abs(fg_a_sum - 600.0) < 1e-3, f"Expected FG_A disaggregated demand to be 600.0, got {fg_a_sum}"
    assert abs(fg_b_sum - 400.0) < 1e-3, f"Expected FG_B disaggregated demand to be 400.0, got {fg_b_sum}"

    # Check Alternate Safety Stock Buffer Protection:
    # COMP_ALT has OnHand = 150.0, SS = 100.0. The total consumption from COMP_ALT (which is logged in `ipc_alternate_allocation`)
    # must NOT exceed 50.0. The remaining 100.0 safety stock must be protected!
    alt_alloc_sum = conn.execute("SELECT SUM(allocated_qty) FROM ipc_alternate_allocation WHERE alt_part = 'COMP_ALT'").fetchone()[0] or 0.0
    print(f"Alternate Safety Stock Buffer Protection: COMP_ALT consumed quantity = {alt_alloc_sum}")
    assert alt_alloc_sum <= 50.01, f"Expected alternate consumption to be <= 50.0, got {alt_alloc_sum}"

    # Check ECN Hard Cutover:
    # FG_A has COMP_1 / COMP_ALT expiring on Day 10. The planned order starting on Day 15 (due to demand on Day 15)
    # must NOT explode to COMP_1 or COMP_ALT (since they are expired and it is a hard cutover).
    # Instead, it must explode to COMP_NEW on Day 15 (start_day = 13).
    # COMP_NEW lead time = 1.0, so COMP_NEW start_day = 12, finish_day = 13.
    # Therefore, we should see COMP_NEW planned orders of qty 600.0.
    comp_new_order_qty = conn.execute("SELECT SUM(order_qty) FROM ipc_planned_order_ledger WHERE part_code='COMP_NEW'").fetchone()[0] or 0.0
    print(f"COMP_NEW planned order qty: {comp_new_order_qty}")
    assert comp_new_order_qty >= 600.0, f"Expected COMP_NEW planned order qty to cover hard cutover, got {comp_new_order_qty}"

    # Check ECN Soft Cutover:
    # FG_B has soft cutover on COMP_1/COMP_ALT.
    # FG_B demand on Day 15 -> start day Day 13.
    # COMP_1 has 30.0 stock.
    # Since it is a soft cutover, it should consume 30.0 COMP_1 stock first (since Day 13 > eff_end_day 10).
    # And the remaining 370.0 (400.0 FG_B demand - 30.0 COMP_1 stock) should explode to COMP_NEW!
    # Let's check COMP_1 inventory. It should be 0.0 now because the 30.0 stock was consumed!
    comp_1_alloc = conn.execute("SELECT SUM(allocated_qty) FROM ipc_alternate_allocation WHERE alt_part = 'COMP_1'").fetchone()[0] or 0.0
    print(f"ECN Soft Cutover check: COMP_1 consumed from stock = {comp_1_alloc}")
    assert abs(comp_1_alloc - 30.0) < 1e-3, f"Expected COMP_1 consumed stock to be 30.0, got {comp_1_alloc}"

    # Check Cross-Site Transshipment Lead Time Offset:
    # COMP_ALT is at SITE_002. Transshipment lead time = 3 days.
    # FG_B demand starts on Day 13.
    # COMP_ALT is soft cutover and has 150.0 - 100.0 = 50.0 available.
    # Since Day 13 > 10 (expired), FG_B consumes 30.0 of COMP_1 stock first, then consumes 50.0 of COMP_ALT stock.
    # The consumption of COMP_ALT stock should happen on Day 13 - 3 = 10!
    # Let's check the alternate allocation ledger for COMP_ALT allocations:
    all_allocs = conn.execute("SELECT day, allocated_qty FROM ipc_alternate_allocation WHERE alt_part = 'COMP_ALT'").fetchall()
    print(f"COMP_ALT allocation records: {all_allocs}")
    comp_alt_alloc_days = conn.execute("""
        SELECT DISTINCT day FROM ipc_alternate_allocation 
        WHERE alt_part = 'COMP_ALT'
        AND day = 10;
    """).fetchall()
    print(f"Cross-Site Transshipment Lead Time check: COMP_ALT allocation days (expecting Day 10): {comp_alt_alloc_days}")
    assert len(comp_alt_alloc_days) > 0, f"Expected COMP_ALT allocation on Day 10 due to 3-day transshipment offset, got {comp_alt_alloc_days}"
    
    conn.close()
    print("[PASS] Advanced Features (Safety Buffer, ECN Cutovers, Cross-Site, Planning BOM) verified successfully!")

if __name__ == "__main__":
    try:
        test_itp_mode()
        test_iop_mode()
        test_ibp_mode()
        test_advanced_features_e2e()
        print("\n=================================================================")
        print("  [ALL PASS] E2E IBP, ITP, and IOP Planning modes successfully tested!")
        print("=================================================================")
        # Clean up temp db
        if os.path.exists(DB_PATH):
            os.remove(DB_PATH)
    except Exception as e:
        print(f"\n[FAILURE] Planning modes E2E verification failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
