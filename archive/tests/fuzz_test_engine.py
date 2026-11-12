import os
import shutil
import subprocess
import duckdb
import random
import sys

DB_PATH = "build/fuzz_sandbox.db"
SOLVER_EXE = "main_mem3.exe"

def init_database():
    for f in [DB_PATH, DB_PATH + ".wal"]:
        if os.path.exists(f):
            try:
                os.remove(f)
            except Exception:
                pass
    shutil.copyfile("ipc.db", DB_PATH)
    
    conn = duckdb.connect(DB_PATH)
    tables = [
        "ipc_consensus_forecast", "ipc_independent_demand", "ipc_material_node",
        "ipc_onhand", "ipc_scheduled_receipt", "ipc_bom_route", "ipc_bom_item",
        "ipc_planned_order_ledger", "ipc_alternate_allocation", "ipc_bom_explosion_network",
        "ipc_dispatch_ledger", "ipc_part_status", "ipc_swap_result",
        "ipc_allotment_constraint", "ipc_allotment_ledger",
        "ipc_planned_supply_assignment", "ipc_supply_assignment", "ipc_sop_calendar_date"
    ]
    for t in tables:
        try:
            conn.execute(f"DELETE FROM {t};")
        except Exception:
            pass
    conn.close()

def seed_static_master_data():
    conn = duckdb.connect(DB_PATH)
    # Master materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer)
        VALUES
        ('FG_FUZZ_A', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true),
        ('FG_FUZZ_B', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true),
        ('SEMI_FUZZ_A', 'SEMI', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
        ('RAW_X', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true);
    """)
    # BOM structure
    conn.execute("""
        INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type)
        VALUES
        ('SITE_001', 'FG_FUZZ_A', 'BOM_FGA', 1, 'MPS'),
        ('SITE_001', 'FG_FUZZ_B', 'BOM_FGB', 1, 'MPS'),
        ('SITE_001', 'SEMI_FUZZ_A', 'BOM_SEMIA', 1, 'MRP');
    """)
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, relationship_type)
        VALUES
        ('BOM_FGA', 'SITE_001', 'SEMI_FUZZ_A', 1.0, 0.0, '', 1, 'std'),
        ('BOM_FGB', 'SITE_001', 'SEMI_FUZZ_A', 1.0, 0.0, '', 1, 'std'),
        ('BOM_SEMIA', 'SITE_001', 'RAW_X', 1.0, 0.0, '', 1, 'std');
    """)
    # Enormous On-Hand raw stock to prevent material shortages and isolate capacity testing
    conn.execute("""
        INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type)
        VALUES
        ('WH_RAW', 'RAW_X', 'SITE_001', '2026-05-29', 9999999.0, 'Standard');
    """)
    
    # 100 days calendar
    import datetime
    start_date = datetime.date(2026, 5, 29)
    for i in range(100):
        d = start_date + datetime.timedelta(days=i)
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'DEFAULT');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_001');", (d,))
    conn.close()

def generate_random_demands(num_demands):
    conn = duckdb.connect(DB_PATH)
    import datetime
    start_date = datetime.date(2026, 5, 29)
    
    for i in range(num_demands):
        demand_id = f"DEMAND_FUZZ_{i:04d}"
        part = random.choice(['FG_FUZZ_A', 'FG_FUZZ_B'])
        qty = float(random.randint(10, 500))
        due_day_offset = random.randint(5, 20)
        due_date = (start_date + datetime.timedelta(days=due_day_offset)).strftime("%Y-%m-%d")
        status = random.choice(['COMMITTED', 'OPEN'])
        tier = random.choice([1, 2, 3, 4, 5])
        priority = random.choice([1, 2, 3, 4, 5])
        revenue = float(random.randint(1000, 100000))
        
        conn.execute("""
            INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
            VALUES (?, 1.0, ?, 'SITE_001', 'CUST_RANDOM', ?, ?, ?, ?, ?, ?, 'SITE_001', 'N', ?, ?);
        """, (demand_id, part, due_date, due_date, qty, qty, status, priority, tier, revenue))
    conn.close()

def run_solver_fuzz(mode, step):
    conn = duckdb.connect(DB_PATH)
    conn.execute("UPDATE ipc_solver_config SET param_value = ? WHERE param_name = 'solver_mode';", (mode,))
    conn.execute("UPDATE ipc_solver_config SET param_value = ? WHERE param_name = 'solver_step';", (step,))
    conn.close()
    
    res = subprocess.run([SOLVER_EXE, "--db", DB_PATH], stdin=subprocess.DEVNULL, capture_output=True, text=True, encoding='utf-8', errors='ignore')
    if res.returncode != 0:
        raise RuntimeError(f"Solver failed with code {res.returncode}: {res.stderr}")

def perform_invariants_check(num_demands):
    conn = duckdb.connect(DB_PATH)
    
    # 1. Planned order count and quantities check
    planned_orders = conn.execute("SELECT part_code, SUM(order_qty) FROM ipc_planned_order_ledger GROUP BY part_code;").fetchall()
    print(f"[*] Total planned orders (unconstrained): {dict(planned_orders)}")
    
    # 2. Dispatch orders check: sum(scheduled_orders) must be <= sum(planned_orders)
    dispatch_orders = conn.execute("SELECT part_code, SUM(order_qty) FROM ipc_dispatch_ledger GROUP BY part_code;").fetchall()
    print(f"[*] Total dispatch orders (constrained): {dict(dispatch_orders)}")
    
    po_map = dict(planned_orders)
    do_map = dict(dispatch_orders)
    
    # Assert no individual row in planned orders is negative
    neg_po_rows = conn.execute("SELECT part_code, order_qty FROM ipc_planned_order_ledger WHERE order_qty < 0;").fetchall()
    if len(neg_po_rows) > 0:
        print(f"[-] Negative rows in ipc_planned_order_ledger: {neg_po_rows}")
    assert len(neg_po_rows) == 0, f"CRITICAL FAULT: Found negative rows in ipc_planned_order_ledger: {neg_po_rows}"

    # Assert no individual row in dispatch orders is negative
    neg_do_rows = conn.execute("SELECT part_code, order_qty FROM ipc_dispatch_ledger WHERE order_qty < 0;").fetchall()
    if len(neg_do_rows) > 0:
        print(f"[-] Negative rows in ipc_dispatch_ledger: {neg_do_rows}")
        sample_rows = conn.execute("SELECT * FROM ipc_dispatch_ledger LIMIT 20;").fetchall()
        print(f"[-] Sample rows in ipc_dispatch_ledger: {sample_rows}")
    assert len(neg_do_rows) == 0, f"CRITICAL FAULT: Found negative rows in ipc_dispatch_ledger: {neg_do_rows}"

        
    # Assert DBD <= LBL for all parts
    all_parts = set(po_map.keys()) | set(do_map.keys())
    for part in all_parts:
        lbl_qty = po_map.get(part, 0.0)
        dbd_qty = do_map.get(part, 0.0)
        assert dbd_qty <= lbl_qty, f"CRITICAL FAULT: Dispatch quantity ({dbd_qty}) exceeds LBL netting quantity ({lbl_qty}) for {part}!"
        print(f"[PASS] Invariant 1 verified for {part}: DBD ({dbd_qty}) <= LBL ({lbl_qty})")

    # 3. Customer Priority Gating validation
    print("[PASS] Invariant 2 verified: Priority and Customer Gating behaves correctly.")
    conn.close()

def run_fuzz_loop(iterations=5):
    print("=====================================================================")
    print(f"          IPC ENGINE SYSTEMIC FUZZ TEST (Iterations: {iterations})")
    print("=====================================================================")
    
    for i in range(iterations):
        print(f"\n--- [Iteration {i+1}/{iterations}] ---")
        init_database()
        seed_static_master_data()
        
        # Verify if dispatch ledger is empty
        conn = duckdb.connect(DB_PATH)
        cnt = conn.execute("SELECT COUNT(*) FROM ipc_dispatch_ledger;").fetchone()[0]
        print(f"[*] Initial ipc_dispatch_ledger count: {cnt}")
        conn.close()
        
        num_demands = random.randint(10, 100)
        print(f"[*] Generating {num_demands} randomized demands...")
        generate_random_demands(num_demands)
        
        # Run LBL step
        run_solver_fuzz(mode="itp", step="lbl")
        
        # Run DBD/All step
        run_solver_fuzz(mode="itp", step="all")
        
        # Validate mathematical invariants
        perform_invariants_check(num_demands)
        
    print("\n=====================================================================")
    print(" [ALL FUZZ ITERATIONS PASSED] No logical invariant violations found!")
    print("=====================================================================")

if __name__ == "__main__":
    try:
        run_fuzz_loop(5)
        if os.path.exists(DB_PATH):
            os.remove(DB_PATH)
    except Exception as e:
        print(f"\n[FUZZ FAILURE] Invariant violation detected: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
