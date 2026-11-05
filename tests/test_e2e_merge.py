import os
import sys
import shutil
import duckdb
import datetime
import time
import subprocess
import asyncio

# Ensure the root workspace is in python module search path
sys.path.append(os.path.abspath("."))

DB_FILE = "ipc.db"
BACKUP_FILE = "ipc.db.test_bak"
SANDBOX_FILE = "sandbox_merge_test.db"

# 1. Database Safety: Backup ipc.db BEFORE importing server
if os.path.exists(DB_FILE):
    print(f"[*] Backing up production database {DB_FILE} to {BACKUP_FILE}...")
    shutil.copyfile(DB_FILE, BACKUP_FILE)
else:
    print(f"[!] No existing {DB_FILE} found. Creating dummy backup path marker.")
    with open(BACKUP_FILE, "w") as f:
        f.write("no_baseline")

try:
    # 2. Now import FastAPI server endpoints and models safely
    import server
    from server import (
        api_create_scenario, ScenarioCreateRequest,
        api_switch_scenario, ScenarioSwitchRequest,
        api_insert_demand, DemandInsertRequest,
        api_merge_scenario, ScenarioMergeRequest,
        recalculate_all_wbs_projects,
        api_pending_changes, api_update_from_parent, ScenarioUpdateRequest,
        api_resolve_conflict, ConflictResolveRequest
    )
except Exception as e:
    print(f"[!] Failed to import server modules: {e}")
    # Restore immediately
    if os.path.exists(BACKUP_FILE):
        with open(BACKUP_FILE, "rb") as f:
            marker = f.read(20)
        if marker == b"no_baseline":
            if os.path.exists(DB_FILE):
                os.remove(DB_FILE)
        else:
            shutil.copyfile(BACKUP_FILE, DB_FILE)
        os.remove(BACKUP_FILE)
    sys.exit(1)

def run_solver_on_db(db_path, mode="iop", step="all"):
    print(f"[*] Running C++ IPC Orchestration Engine (main_mem3.exe) on {db_path} (mode={mode}, step={step})...")
    conn = duckdb.connect(db_path)
    conn.execute("UPDATE ipc_solver_config SET param_value = ? WHERE param_name = 'solver_mode';", (mode,))
    conn.execute("UPDATE ipc_solver_config SET param_value = ? WHERE param_name = 'solver_step';", (step,))
    conn.close()
    
    res = subprocess.run(["main_mem3.exe", "--db", db_path], stdin=subprocess.DEVNULL, capture_output=True, text=True, encoding='utf-8', errors='ignore')
    if res.returncode != 0:
        print("----- ENGINE STDOUT -----")
        print(res.stdout)
        print("----- ENGINE STDERR -----")
        print(res.stderr)
        raise RuntimeError(f"Engine exited with code {res.returncode}")
    print("[*] Solver ran successfully.")
    return res.stdout

def init_clean_baseline():
    print("[*] Initializing clean baseline database tables...")
    conn = duckdb.connect(DB_FILE)
    
    # Clear tables to start fresh
    tables_to_clear = [
        "ipc_consensus_forecast", "ipc_independent_demand", "ipc_material_node",
        "ipc_onhand", "ipc_scheduled_receipt", "ipc_bom_route", "ipc_bom_item",
        "ipc_planned_order_ledger", "ipc_alternate_allocation", "ipc_bom_explosion_network",
        "ipc_dispatch_ledger", "ipc_part_status", "ipc_swap_result",
        "ipc_allotment_constraint", "ipc_allotment_ledger",
        "ipc_planned_supply_assignment", "ipc_supply_assignment",
        "ipc_project", "ipc_project_wbs",
        "ipc_coproduct_dimension", "ipc_coproduct_grouping", "ipc_coproduct_recipe", "ipc_coproduct_demand",
        "ipc_coproduct_allocation", "ipc_coproduct_schedule",
        "ipc_hierarchy_product_family", "ipc_hierarchy_customer", "ipc_customer", "ipc_sop_calendar_date"
    ]
    for t in tables_to_clear:
        try:
            conn.execute(f"DELETE FROM {t};")
        except Exception as e:
            print(f"Warning: could not clear table {t}: {e}")
            
    # Ingest materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer, safety_stock)
        VALUES
        ('FG_A', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true, 0.0),
        ('FG_B', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true, 0.0),
        ('SEMI_A', 'SEMI', 'MRP', 'SITE_001', false, 0.0, 1.0, true, 0.0),
        ('RAW_X', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, false, 0.0),
        ('RAW_Y', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, false, 0.0);
    """)
    # BOM Route
    conn.execute("""
        INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type)
        VALUES
        ('SITE_001', 'FG_A', 'BOM_FGA', 1, 'MPS'),
        ('SITE_001', 'FG_B', 'BOM_FGB', 1, 'MPS'),
        ('SITE_001', 'SEMI_A', 'BOM_SEMIA', 1, 'MRP');
    """)
    # BOM Items (SEMI_A requires RAW_X or RAW_Y alternate with target/ratios)
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, lot_size, eff_start_day, eff_end_day, ltb_limit, mix_group_id, relationship_type)
        VALUES
        ('BOM_FGA', 'SITE_001', 'SEMI_A', 1.0, 0.0, '', 1, 1.0, 0.0, 0.0, -1, -1, -1.0, -1, 'std'),
        ('BOM_FGB', 'SITE_001', 'SEMI_A', 1.0, 0.0, '', 1, 1.0, 0.0, 0.0, -1, -1, -1.0, -1, 'std'),
        ('BOM_SEMIA', 'SITE_001', 'RAW_X', 2.0, 0.0, 'ALT_GRP_01', 1, 0.5, 0.0, 1.0, -1, -1, -1.0, -1, 'alt'),
        ('BOM_SEMIA', 'SITE_001', 'RAW_Y', 2.0, 0.0, 'ALT_GRP_01', 2, 0.5, 0.0, 1.0, -1, -1, -1.0, -1, 'alt');
    """)
    # On-Hand (RAW_X has 1000, RAW_Y has 500)
    conn.execute("""
        INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type)
        VALUES
        ('WH_MAIN', 'RAW_X', 'SITE_001', '2026-05-29', 1000.0, 'Standard'),
        ('WH_MAIN', 'RAW_Y', 'SITE_001', '2026-05-29', 500.0, 'Standard');
    """)
    
    # Calendar dates
    start_date = datetime.date(2026, 5, 29)
    for i in range(100):
        d = start_date + datetime.timedelta(days=i)
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'DEFAULT');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_001');", (d,))
        
    # Customer and family hierarchies
    conn.execute("INSERT INTO ipc_hierarchy_product_family (family_num, description, material) VALUES ('FAM_A', 'Family A', 'FG_A'), ('FAM_B', 'Family B', 'FG_B');")
    conn.execute("INSERT INTO ipc_hierarchy_customer (customer, parent_customer) VALUES ('CUST_VVIP', 'CG_VVIP'), ('CUST_NORMAL', 'CG_NORMAL');")
    conn.execute("INSERT INTO ipc_customer (customer, region, site, name) VALUES ('CUST_VVIP', 'US', 'SITE_001', 'VVIP Customer'), ('CUST_NORMAL', 'US', 'SITE_001', 'Normal Customer');")
    
    # Project & WBS CPM definitions
    conn.execute("INSERT INTO ipc_project (project, delivery_lead_time) VALUES ('PROJ_001', 30);")
    conn.execute("""
        INSERT INTO ipc_project_wbs (wbs_code, project_code, parent_wbs_code, wbs_level, wbs_status, description, duration)
        VALUES
        ('WBS_ROOT', 'PROJ_001', '', 1, 'ACTIVE', 'Root Project Task', 0.0),
        ('WBS_SUB1', 'PROJ_001', 'WBS_ROOT', 2, 'ACTIVE', 'Engineering Design', 10.0),
        ('WBS_SUB2', 'PROJ_001', 'WBS_ROOT', 2, 'ACTIVE', 'Procurement', 5.0);
    """)

    conn.close()

async def test_main():
    print("\n=================================================================")
    print("  IPC END-TO-END PLANNING & SCENARIO MERGE INTEGRATION TEST")
    print("=================================================================")

    # Ensure no leftover sandbox databases exist
    for f in [SANDBOX_FILE, SANDBOX_FILE + ".wal", "sandbox_parent_scen.db", "sandbox_parent_scen.db.wal"]:
        if os.path.exists(f):
            try:
                os.remove(f)
            except Exception:
                pass

    # Step 1. Initialize Baseline Master Data
    init_clean_baseline()

    # Step 2. Setup IBP Consensus Forecast & History for Holt-Winters Disaggregation
    print("[*] Setting up IBP Consensus Forecast...")
    conn = duckdb.connect(DB_FILE)
    # Seeding daily historic demand sequence for Holt-Winters
    start_date = datetime.date(2026, 5, 29)
    for d_idx in range(35):
        due_date = start_date + datetime.timedelta(days=d_idx)
        due_date_str = due_date.strftime("%Y-%m-%d")
        qty = 100.0 + (d_idx % 7) * 10.0
        conn.execute("""
            INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue)
            VALUES (?, 1.0, 'FG_A', 'SITE_001', 'CUST_VVIP', ?, ?, ?, ?, 'OPEN', 1, 'SITE_001', 'N', 1, 1000.0);
        """, (f"HIST_DEM_{d_idx}", due_date_str, due_date_str, qty, qty))
        
    conn.execute("""
        INSERT INTO ipc_consensus_forecast (part, customer, date, qty, unit_price, sales_qty, marketing_qty, statistical_qty, consensus_forecast, override_qty)
        VALUES ('FG_A@SITE_001', 'CUST_VVIP', '2026-05-29', 500.0, 10.0, 4800.0, 5200.0, 0.0, '5000.0', 500.0);
    """)
    conn.close()

    # Step 3. Run solver on baseline to generate baseline ITP allotment constraints
    print("[*] Running solver in baseline to calculate allotments constraints...")
    run_solver_on_db(DB_FILE, mode="itp", step="all")
    
    # Establish custom allotment constraint guidelines (ITP outputs)
    conn = duckdb.connect(DB_FILE)
    conn.execute("""
        INSERT INTO ipc_allotment_constraint (scenario_id, part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked)
        VALUES
        ('baseline', 'FG_B', 'SITE_001', '*', '*', 'FAM_B', 10, 50.0, 50.0, true);
    """)
    # Set the solver mode to iop so that subsequent sandbox updates and solver runs
    # enforce the constraints rather than recalculating them in itp mode.
    conn.execute("UPDATE ipc_solver_config SET param_value = 'iop' WHERE param_name = 'solver_mode';")
    conn.close()

    # Verify IBP outputs in baseline db (forecast disaggregated, safety stock computed)
    conn = duckdb.connect(DB_FILE)
    fg_a_demands = conn.execute("SELECT SUM(request_qty) FROM ipc_independent_demand WHERE part = 'FG_A'").fetchone()[0]
    fg_a_safety_stock = conn.execute("SELECT safety_stock FROM ipc_material_node WHERE part = 'FG_A'").fetchone()[0]
    conn.close()
    
    print(f"    [IBP Check] FG_A Disaggregated Demand Sum: {fg_a_demands} (expected: 500.0)")
    print(f"    [IBP Check] FG_A safety_stock calculated by Holt-Winters: {fg_a_safety_stock}")
    assert fg_a_demands is not None and abs(fg_a_demands - 500.0) < 1e-3, "IBP disaggregation failed!"
    assert fg_a_safety_stock is not None and fg_a_safety_stock > 0.0, "Holt-Winters safety stock failed!"

    PARENT_SCEN_FILE = "sandbox_parent_scen.db"

    # Step 4. Create parent scenario to test multi-level branching and conflict detection
    print("[*] Creating parent scenario 'parent_scen' via FastAPI endpoint...")
    req_create_p = ScenarioCreateRequest(scenario_code="parent_scen", scenario_name="Parent Scenario", parent_code="baseline")
    resp_create_p = await api_create_scenario(req_create_p)
    assert resp_create_p.status_code == 200, f"Failed to create parent scenario: {resp_create_p.body}"
    assert os.path.exists(PARENT_SCEN_FILE), "Parent scenario database file was not created!"

    # Step 4.1. Create scenario 'merge_test' branched from 'parent_scen'
    print("[*] Creating scenario 'merge_test' branched from 'parent_scen' via FastAPI endpoint...")
    req_create = ScenarioCreateRequest(scenario_code="merge_test", scenario_name="IOP Sandbox Merge Test Scenario", parent_code="parent_scen")
    resp_create = await api_create_scenario(req_create)
    assert resp_create.status_code == 200, f"Failed to create scenario: {resp_create.body}"
    assert os.path.exists(SANDBOX_FILE), "Sandbox database file was not created!"

    # Step 5. Call api_switch_scenario to activate sandbox
    print("[*] Switching to scenario 'merge_test' via FastAPI endpoint...")
    req_switch = ScenarioSwitchRequest(scenario_code="merge_test")
    resp_switch = await api_switch_scenario(req_switch)
    assert resp_switch.status_code == 200, f"Failed to switch scenario: {resp_switch.body}"
    assert server.DB_PATH == SANDBOX_FILE, "server.DB_PATH was not updated to sandbox!"

    # Step 6. Ingest demand in sandbox (exceeds allotment constraints to test breakwater)
    print("[*] Inserting new IOP demand in sandbox to trigger planning optimization...")
    req_insert = DemandInsertRequest(part_code="FG_B", qty=100.0, day=10, customer="CUST_NORMAL", priority=1)
    resp_insert = await api_insert_demand(req_insert)
    assert resp_insert.status_code == 200, f"Failed to insert demand: {resp_insert.body}"

    # Step 6.1. Test api_pending_changes to see diff between sandbox and parent (baseline)
    print("[*] Testing api_pending_changes to see diff...")
    resp_pending = await api_pending_changes(scenario_code="merge_test")
    assert resp_pending.status_code == 200, f"api_pending_changes failed: {resp_pending.body}"
    import json
    pending_data = json.loads(resp_pending.body.decode('utf-8'))
    print(f"    [Pending Changes Check] Commits: {pending_data['pending_commits']}")
    assert len(pending_data["pending_commits"]) > 0, "No pending commits found when sandboxed demand is present!"
    assert any(c["table"] == "Independent Demand" for c in pending_data["pending_commits"]), "Expected Independent Demand pending commit not found!"

    # Step 6.2. Test api_update_from_parent (should succeed)
    print("[*] Testing api_update_from_parent...")
    req_update = ScenarioUpdateRequest(scenario_code="merge_test")
    resp_update = await api_update_from_parent(req_update)
    assert resp_update.status_code == 200, f"api_update_from_parent failed: {resp_update.body}"

    # Step 6.3. Test api_resolve_conflict by introducing a conflict
    print("[*] Testing api_resolve_conflict by introducing a conflict...")
    # Update consensus forecast in parent (sandbox_parent_scen.db)
    conn_p = duckdb.connect(PARENT_SCEN_FILE)
    conn_p.execute("UPDATE ipc_consensus_forecast SET qty = 600.0, override_qty = 600.0 WHERE part = 'FG_A@SITE_001' AND customer = 'CUST_VVIP' AND date = '2026-05-29';")
    conn_p.close()

    # Update consensus forecast in sandbox (sandbox_merge_test.db) to a different value
    conn_s = duckdb.connect(SANDBOX_FILE)
    conn_s.execute("UPDATE ipc_consensus_forecast SET qty = 700.0, override_qty = 700.0 WHERE part = 'FG_A@SITE_001' AND customer = 'CUST_VVIP' AND date = '2026-05-29';")
    conn_s.close()

    # Call api_pending_changes to verify conflict is detected
    resp_pending_conflict = await api_pending_changes(scenario_code="merge_test")
    if resp_pending_conflict.status_code != 200:
        print(f"[!] api_pending_changes failed: {resp_pending_conflict.body.decode('utf-8', errors='ignore')}")
    assert resp_pending_conflict.status_code == 200
    pending_conflict_data = json.loads(resp_pending_conflict.body.decode('utf-8'))
    print(f"    [Conflict Detection Check] Conflicts: {pending_conflict_data['conflicts']}")
    assert len(pending_conflict_data["conflicts"]) > 0, "No conflict detected when same forecast is modified differently in parent and sandbox!"
    assert any(c["table"] == "Consensus Forecast" for c in pending_conflict_data["conflicts"]), "Expected Consensus Forecast conflict not found!"

    # Resolve conflict by accepting parent value
    print("[*] Resolving conflict by accepting parent value...")
    req_resolve = ConflictResolveRequest(
        scenario_code="merge_test",
        table="Consensus Forecast",
        key="FG_A@SITE_001-CUST_VVIP-0",
        resolution="accept_parent"
    )
    resp_resolve = await api_resolve_conflict(req_resolve)
    assert resp_resolve.status_code == 200, f"api_resolve_conflict failed: {resp_resolve.body}"

    # Verify sandbox has been updated to parent value (600.0)
    conn_s = duckdb.connect(SANDBOX_FILE)
    resolved_qty = conn_s.execute("SELECT qty FROM ipc_consensus_forecast WHERE part = 'FG_A@SITE_001' AND customer = 'CUST_VVIP' AND date = '2026-05-29';").fetchone()[0]
    conn_s.close()
    print(f"    [Conflict Resolution Check] Resolved sandbox qty: {resolved_qty} (expected: 600.0)")
    assert float(resolved_qty) == 600.0, f"Conflict resolution failed to update sandbox value! Got {resolved_qty}, expected 600.0"

    # Step 7. Verify sandbox table persistence and check each module output
    print("[*] Verifying all module outputs in sandbox database...")
    conn = duckdb.connect(SANDBOX_FILE)
    
    # A. IOP Allotment breakwater checking
    # FG_B order (qty=100) on day 10 should be capped at allotment limit of 50.0.
    allotment = conn.execute("SELECT allotment_limit, consumed_qty, blocked_demand_qty FROM ipc_allotment_ledger WHERE product_family = 'FAM_B' AND customer_group = '*'").fetchone()
    print(f"    [IOP Check] Allotment FAM_B: Limit={allotment[0]}, Consumed={allotment[1]}, Blocked={allotment[2]}")
    assert allotment is not None, "Allotment ledger entry not found!"
    assert allotment[0] == 50.0 and allotment[1] == 50.0 and allotment[2] >= 50.0, "Allotment breakwater constraint not enforced!"

    # B. ITP Netting & Planned Orders
    planned_orders = conn.execute("SELECT part_code, SUM(order_qty) FROM ipc_planned_order_ledger GROUP BY part_code ORDER BY part_code").fetchall()
    print(f"    [ITP Check] Planned Orders in Sandbox: {planned_orders}")
    # We should have planned orders for FG_A, FG_B, SEMI_A, and Raw components
    planned_map = dict(planned_orders)
    assert "FG_A" in planned_map and "FG_B" in planned_map, "Planned orders for FG_A or FG_B are missing!"

    # C. IOP Substitution
    # We have demand for SEMI_A. SEMI_A explodes 1:1 for FG_A (500) and FG_B (50 planned) -> 550 SEMI_A.
    # SEMI_A requires RAW_X or RAW_Y alternate with ratio 2.0 -> 1100 total raw materials.
    # RAW_X stock is 1000, shortage of 100 is filled by alternate RAW_Y.
    alternates = conn.execute("SELECT main_part, alt_part, allocated_qty FROM ipc_alternate_allocation WHERE alt_part = 'RAW_Y'").fetchall()
    print(f"    [IOP Check] Raw material alternates allocated: {alternates}")
    assert len(alternates) > 0, "No raw material substitutions found!"
    assert alternates[0][2] > 0.0, "Substitution allocated quantity is zero!"

    # D. ETO Project CPM
    # Check that CPM dates are computed and early_finish > 0
    wbs_tasks = conn.execute("SELECT wbs_code, early_finish, late_finish FROM ipc_project_wbs WHERE project_code = 'PROJ_001'").fetchall()
    print(f"    [ETO Check] WBS CPM Task schedules: {wbs_tasks}")
    assert len(wbs_tasks) > 0, "No WBS tasks found!"
    for wbs_code, ef, lf in wbs_tasks:
        assert ef >= 0 and lf >= 0, f"CPM calculations are invalid for {wbs_code}!"

    # E. Coproduct Dimension Planning
    # Check that Coproduct tables are seeded and solved
    coprod_sched = conn.execute("SELECT routing_code, batch_count, leftover_512, leftover_256, leftover_128 FROM ipc_coproduct_schedule").fetchall()
    coprod_alloc = conn.execute("SELECT order_code, allocated_512, shortage FROM ipc_coproduct_allocation").fetchall()
    print(f"    [Coproduct Check] Schedule: {coprod_sched}")
    print(f"    [Coproduct Check] Allocations: {coprod_alloc}")
    assert len(coprod_sched) > 0, "Coproduct schedule result was not saved!"
    assert len(coprod_alloc) > 0, "Coproduct allocation was not saved!"

    conn.close()

    # Step 8. Call api_merge_scenario to commit sandboxed changes back to baseline
    print("[*] Merging sandbox scenario 'merge_test' back to production baseline...")
    req_merge = ScenarioMergeRequest(scenario_code="merge_test")
    resp_merge = await api_merge_scenario(req_merge)
    assert resp_merge.status_code == 200, f"Failed to merge scenario: {resp_merge.body}"
    assert server.ACTIVE_SCENARIO == "baseline" and server.DB_PATH == DB_FILE, "Server did not switch back to baseline after merge!"

    # Step 9. Connect to baseline DB and assert merged database state is persistent
    print("[*] Connecting to baseline database to verify merged persistence...")
    conn = duckdb.connect(DB_FILE)
    
    # The new independent demand (DEM_INS_FG_B_...) should exist in baseline
    merged_demands = conn.execute("SELECT demand, part, request_qty FROM ipc_independent_demand WHERE part = 'FG_B'").fetchall()
    print(f"    [Merged Baseline Check] FG_B demands: {merged_demands}")
    assert len(merged_demands) > 0, "Urgent sandboxed demand did not merge into baseline!"
    assert any(row[2] == 100.0 for row in merged_demands), "Urgent sandboxed demand qty is wrong in baseline!"

    # The allotment limit should show consumption in baseline
    merged_allotment = conn.execute("SELECT allotment_limit, consumed_qty, blocked_demand_qty FROM ipc_allotment_ledger WHERE product_family = 'FAM_B' AND customer_group = '*'").fetchone()
    print(f"    [Merged Baseline Check] Allotment FAM_B: Limit={merged_allotment[0]}, Consumed={merged_allotment[1]}, Blocked={merged_allotment[2]}")
    assert merged_allotment is not None and merged_allotment[1] == 50.0, "Allotment ledger updates did not merge into baseline!"

    # Alternate allocation raw material substitution should exist in baseline
    merged_alts = conn.execute("SELECT main_part, alt_part, allocated_qty FROM ipc_alternate_allocation WHERE alt_part = 'RAW_Y'").fetchall()
    print(f"    [Merged Baseline Check] Alternates allocated: {merged_alts}")
    assert len(merged_alts) > 0, "Substitutions did not merge into baseline!"

    # ETO Project WBS CPM schedules should exist in baseline
    merged_wbs = conn.execute("SELECT wbs_code, early_finish, late_finish FROM ipc_project_wbs WHERE project_code = 'PROJ_001'").fetchall()
    print(f"    [Merged Baseline Check] WBS Tasks schedules: {merged_wbs}")
    assert len(merged_wbs) > 0, "ETO project CPM schedules did not merge into baseline!"

    # Coproduct schedule results should exist in baseline
    merged_coprod = conn.execute("SELECT routing_code, batch_count FROM ipc_coproduct_schedule").fetchall()
    print(f"    [Merged Baseline Check] Coproduct schedules: {merged_coprod}")
    assert len(merged_coprod) > 0, "Coproduct schedule results did not merge into baseline!"

    conn.close()
    print("\n=================================================================")
    print("  [ALL PASS] End-to-End integration and scenario merge verified successfully!")
    print("=================================================================")

if __name__ == "__main__":
    success = False
    try:
        # Run async test runner
        asyncio.run(test_main())
        success = True
    except Exception as e:
        print(f"\n[FAILURE] End-to-end integration test failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
    finally:
        if success:
            # 10. Clean up sandbox files
            for f in [SANDBOX_FILE, SANDBOX_FILE + ".wal", "sandbox_parent_scen.db", "sandbox_parent_scen.db.wal"]:
                if os.path.exists(f):
                    try:
                        os.remove(f)
                    except Exception:
                        pass
        # 11. Database Safety: Restore original ipc.db
        if os.path.exists(BACKUP_FILE):
            print(f"[*] Restoring original production database {DB_FILE} from {BACKUP_FILE}...")
            try:
                # Check dummy marker
                with open(BACKUP_FILE, "rb") as f:
                    marker = f.read(20)
                if marker == b"no_baseline":
                    if os.path.exists(DB_FILE):
                        os.remove(DB_FILE)
                else:
                    shutil.copyfile(BACKUP_FILE, DB_FILE)
            except Exception as e:
                print(f"[!] Warning: Failed to parse backup file, trying raw copy: {e}")
                try:
                    shutil.copyfile(BACKUP_FILE, DB_FILE)
                except Exception:
                    pass
            os.remove(BACKUP_FILE)
