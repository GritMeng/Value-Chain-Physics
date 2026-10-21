import os
import subprocess
import datetime
import duckdb

DB_PATH = "ipc.db"
SOLVER_EXE = "main_mem3.exe"

def main():
    print("[*] Connecting to DuckDB...")
    conn = duckdb.connect(DB_PATH)
    
    # 1. Clear existing database rows to start fresh
    print("[*] Clearing database tables...")
    tables_to_clear = [
        "ipc_consensus_forecast", "ipc_independent_demand", "ipc_material_node",
        "ipc_onhand", "ipc_scheduled_receipt", "ipc_bom_route", "ipc_bom_item",
        "ipc_planned_order_ledger", "ipc_alternate_allocation", "ipc_bom_explosion_network",
        "ipc_dispatch_ledger", "ipc_part_status", "ipc_swap_result",
        "ipc_allotment_constraint", "ipc_allotment_ledger",
        "ipc_planned_supply_assignment", "ipc_supply_assignment",
        "ipc_hierarchy_product_family", "ipc_hierarchy_customer", "ipc_customer", "ipc_sop_calendar_date",
        "ipc_operation", "ipc_work_center_capacity"
    ]
    for t in tables_to_clear:
        try:
            conn.execute(f"DELETE FROM {t};")
        except Exception as e:
            print(f"Warning: could not clear table {t}: {e}")

    # 2. Seed Baseline Parts (needed as standard dictionary anchors)
    print("[*] Seeding baseline parts...")
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer)
        VALUES
        ('FG_A', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true),
        ('FG_B', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 2.0, true),
        ('SEMI_A', 'SEMI', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
        ('RAW_X', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
        ('RAW_Y', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true);
    """)

    # 3. Seed 200 Dummy Parts to Pad Vocab IDs
    # This guarantees SCEN4_FG_MAIN receives ID >= 200, and child P1/P2/P3 receive multiples of 5, 3, and standard IDs.
    print("[*] Seeding 200 dummy parts for vocab alignment...")
    for i in range(200):
        part_name = f"A_DUMMY_{i:04d}"
        conn.execute(f"""
            INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer)
            VALUES ('{part_name}', 'RAW', 'MRP', 'SITE_001', false, 1.0, true);
        """)

    # 4. Ingest Scenario Material Nodes
    print("[*] Seeding scenario material nodes...")
    
    # Scenario 1 Materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer) VALUES
        ('SCEN1_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true),
        ('SCEN1_PART_P1', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
        ('SCEN1_PART_P2', 'RAW', 'MRP', 'SITE_001', false, 1.0, true);
    """)
    
    # Scenario 2 Materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer) VALUES
        ('SCEN2_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true),
        ('SCEN2_PART_P1', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
        ('SCEN2_PART_P2', 'RAW', 'MRP', 'SITE_001', false, 1.0, true);
    """)
    
    # Scenario 3 Materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer) VALUES
        ('SCEN3_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true),
        ('SCEN3_PART_P1', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
        ('SCEN3_PART_P2', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
        ('SCEN3_PART_P3', 'RAW', 'MRP', 'SITE_001', false, 1.0, true);
    """)
    
    # Scenario 4 Materials
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer) VALUES
        ('SCEN4_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true),
        ('SCEN4_PART_P1', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
        ('SCEN4_PART_P2', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
        ('SCEN4_PART_P3', 'RAW', 'MRP', 'SITE_001', false, 1.0, true);
    """)
    
    # Scenario 5 Materials (MCDM Group Sourcing, selling_ave_price is cost)
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer, selling_ave_price) VALUES
        ('SCEN5_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true, 10.0),
        ('SCEN5_PART_P1_A', 'RAW', 'MRP', 'SITE_001', false, 1.0, true, 10.0),
        ('SCEN5_PART_P1_B', 'RAW', 'MRP', 'SITE_001', false, 1.0, true, 10.0),
        ('SCEN5_PART_P2_A', 'RAW', 'MRP', 'SITE_001', false, 1.0, true, 50.0),
        ('SCEN5_PART_P2_B', 'RAW', 'MRP', 'SITE_001', false, 1.0, true, 50.0);
    """)
    
    # Scenario 6 Materials (Swap Engine and Safety Buffer Protection)
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer, safety_stock) VALUES
        ('SCEN6_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true, 0.0),
        ('SCEN6_COMP_1', 'RAW', 'MRP', 'SITE_001', false, 1.0, true, 0.0),
        ('SCEN6_COMP_ALT', 'RAW', 'MRP', 'SITE_002', false, 1.0, true, 100.0);
    """)
    
    # Scenario 8 Materials (DBD scheduling)
    conn.execute("""
        INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer) VALUES
        ('SCEN8_FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true);
    """)

    # 5. Ingest BOM Routes & Items (Isolated using distinct ALT_GRP_x)
    print("[*] Seeding scenario BOM networks...")
    
    # Scenario 1 BOM
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN1_FG_MAIN', 'BOM_SCEN1', 1, 'MPS');")
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, relationship_type, lot_size)
        VALUES
        ('BOM_SCEN1', 'SITE_001', 'SCEN1_PART_P1', 1.0, 0.0, 'ALT_GRP_1', 1, 0.6, 0.0, -1, -1, 'alt', 0.0),
        ('BOM_SCEN1', 'SITE_001', 'SCEN1_PART_P2', 1.0, 0.0, 'ALT_GRP_1', 1, 0.4, 0.0, -1, -1, 'alt', 0.0);
    """)
    
    # Scenario 2 BOM (Uses ALT_GRP_2 to isolate from ALT_GRP_1)
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN2_FG_MAIN', 'BOM_SCEN2', 1, 'MPS');")
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, relationship_type, lot_size)
        VALUES
        ('BOM_SCEN2', 'SITE_001', 'SCEN2_PART_P1', 1.0, 0.0, 'ALT_GRP_2', 2, 0.6, 0.0, -1, -1, 'alt', 0.0),
        ('BOM_SCEN2', 'SITE_001', 'SCEN2_PART_P2', 1.0, 0.0, 'ALT_GRP_2', 2, 0.4, 0.0, -1, -1, 'alt', 0.0);
    """)
    
    # Scenario 3 BOM (Uses ALT_GRP_3 to isolate)
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN3_FG_MAIN', 'BOM_SCEN3', 1, 'MPS');")
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, relationship_type, lot_size)
        VALUES
        ('BOM_SCEN3', 'SITE_001', 'SCEN3_PART_P1', 1.0, 0.0, 'ALT_GRP_3', 3, 0.5, 0.0, -1, -1, 'alt', 20.0),
        ('BOM_SCEN3', 'SITE_001', 'SCEN3_PART_P2', 1.0, 0.0, 'ALT_GRP_3', 3, 0.3, 0.0, -1, -1, 'alt', 15.0),
        ('BOM_SCEN3', 'SITE_001', 'SCEN3_PART_P3', 1.0, 0.0, 'ALT_GRP_3', 3, 0.2, 0.0, -1, -1, 'alt', 10.0);
    """)
    
    # Scenario 4 BOM
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN4_FG_MAIN', 'BOM_SCEN4', 1, 'MPS');")
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, priority, relationship_type)
        VALUES
        ('BOM_SCEN4', 'SITE_001', 'SCEN4_PART_P1', 1.0, 0.0, 1, 'std'),
        ('BOM_SCEN4', 'SITE_001', 'SCEN4_PART_P2', 1.0, 0.0, 1, 'std'),
        ('BOM_SCEN4', 'SITE_001', 'SCEN4_PART_P3', 1.0, 0.0, 1, 'std');
    """)
    
    # Scenario 5 BOM (MCDM groups ALT_GRP_51 and ALT_GRP_52)
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN5_FG_MAIN', 'BOM_SCEN5', 1, 'MPS');")
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, relationship_type)
        VALUES
        ('BOM_SCEN5', 'SITE_001', 'SCEN5_PART_P1_A', 1.0, 0.0, 'ALT_GRP_51', 1, 1.0, 'alt'),
        ('BOM_SCEN5', 'SITE_001', 'SCEN5_PART_P1_B', 1.0, 0.0, 'ALT_GRP_51', 1, 1.0, 'alt'),
        ('BOM_SCEN5', 'SITE_001', 'SCEN5_PART_P2_A', 1.0, 0.0, 'ALT_GRP_52', 1, 1.0, 'alt'),
        ('BOM_SCEN5', 'SITE_001', 'SCEN5_PART_P2_B', 1.0, 0.0, 'ALT_GRP_52', 1, 1.0, 'alt');
    """)
    
    # Scenario 6 BOM (Uses ALT_GRP_6 to isolate)
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN6_FG_MAIN', 'BOM_SCEN6', 1, 'MPS');")
    conn.execute("""
        INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, eff_start_day, eff_end_day, relationship_type)
        VALUES
        ('BOM_SCEN6', 'SITE_001', 'SCEN6_COMP_1', 1.0, 0.0, 'ALT_GRP_6', 1, 1.0, 0, 10, 'soft'),
        ('BOM_SCEN6', 'SITE_002', 'SCEN6_COMP_ALT', 1.0, 0.0, 'ALT_GRP_6', 2, 1.0, 0, 10, 'soft');
    """)
    
    # Scenario 8 BOM
    conn.execute("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES ('SITE_001', 'SCEN8_FG_MAIN', 'BOM_SCEN8', 1, 'MPS');")
    conn.execute("INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, priority, relationship_type) VALUES ('BOM_SCEN8', 'SITE_001', 'SCEN8_FG_MAIN', 1.0, 0.0, 1, 'std');")

    # 6. Ingest On-Hand Stocks
    print("[*] Seeding scenario on-hand stocks...")
    
    # Scenario 3 Stock (satisfying lot-sizing constraints)
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN3_P1', 'SCEN3_PART_P1', 'SITE_001', '2026-05-29', 100.0, 'Standard');")
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN3_P2', 'SCEN3_PART_P2', 'SITE_001', '2026-05-29', 100.0, 'Standard');")
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN3_P3', 'SCEN3_PART_P3', 'SITE_001', '2026-05-29', 100.0, 'Standard');")
    
    # Scenario 5 Stock (MCDM Sourcing)
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN5_P2A', 'SCEN5_PART_P2_A', 'SITE_001', '2026-05-29', 5.0, 'Standard');")
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN5_P2B', 'SCEN5_PART_P2_B', 'SITE_001', '2026-05-29', 5.0, 'Standard');")
    
    # Scenario 6 Stock (COMP_ALT has 150 total stock, with 100 units safety stock. 50 units available for swap)
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN6_ALT', 'SCEN6_COMP_ALT', 'SITE_002', '2026-05-29', 150.0, 'Standard');")
    conn.execute("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES ('WH_SCEN6_COMP1', 'SCEN6_COMP_1', 'SITE_001', '2026-05-29', 30.0, 'Standard');")

    # 7. Ingest Work Centers, Operations, & Capacities (Scenario 8)
    print("[*] Seeding operations and work center capacities...")
    conn.execute("INSERT INTO ipc_operation (operation, description, sequence, work_center, operation_type, setup_time, run_time, routing, site) VALUES ('OP_SCEN8', 'Assembly', 10, 'WC_MAIN', 'Assembly', 1.0, 0.05, 'SCEN8_FG_MAIN', 'SITE_001');")

    # 8. Ingest Calendar Dates
    print("[*] Seeding calendar dates (100 days)...")
    start_date = datetime.date(2026, 5, 29)
    for i in range(100):
        d = start_date + datetime.timedelta(days=i)
        d_str = d.strftime("%Y-%m-%d")
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'DEFAULT');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_001');", (d,))
        conn.execute("INSERT INTO ipc_sop_calendar_date (date, display, calendar) VALUES (?, 'work', 'SITE_002');", (d,))
        # Capacity (Scenario 8)
        conn.execute("INSERT INTO ipc_work_center_capacity (work_center, date, working_hour, efficiency, number_of_resources, utilization, capacity, capacity_override, source) VALUES ('WC_MAIN', ?, 8.0, 1.0, 1.0, 1.0, '8.0', '8.0', 'CAL');", (d_str,))

    # 9. Ingest Independent Demands
    print("[*] Seeding demands...")
    
    # Scenario 1
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN1_1', 1.0, 'SCEN1_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 10.0, 10.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);")
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN1_2', 1.0, 'SCEN1_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-18', '2026-06-18', 10.0, 10.0, 'OPEN', 2, 'SITE_001', 3, 1000.0);")
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN1_3', 1.0, 'SCEN1_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-28', '2026-06-28', 20.0, 20.0, 'OPEN', 3, 'SITE_001', 3, 2000.0);")
    
    # Scenario 2
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN2_1', 1.0, 'SCEN2_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 10.0, 10.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);")
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN2_2', 1.0, 'SCEN2_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-18', '2026-06-18', 20.0, 20.0, 'OPEN', 2, 'SITE_001', 3, 2000.0);")
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN2_3', 1.0, 'SCEN2_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-28', '2026-06-28', 30.0, 30.0, 'OPEN', 3, 'SITE_001', 3, 3000.0);")
    
    # Scenario 3
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN3_1', 1.0, 'SCEN3_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 100.0, 100.0, 'OPEN', 1, 'SITE_001', 3, 10000.0);")
    
    # Scenario 4
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, dimension_grp, customer_tier, revenue) VALUES ('D_SCEN4_1', 1.0, 'SCEN4_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 10.0, 10.0, 'OPEN', 1, 'SITE_001', 'DIM_102.0', 3, 1000.0);")
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, dimension_grp, customer_tier, revenue) VALUES ('D_SCEN4_2', 1.0, 'SCEN4_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-18', '2026-06-18', 10.0, 10.0, 'OPEN', 2, 'SITE_001', 'DIM_101.0', 3, 1000.0);")
    
    # Scenario 5
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN5_1', 1.0, 'SCEN5_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 5.0, 5.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);")
    
    # Scenario 6 (Due on Day 15)
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN6_1', 1.0, 'SCEN6_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-13', '2026-06-13', 100.0, 100.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);")
    
    # Scenario 8 (Due on Day 10. Load required = 1 + 100 * 0.05 = 6 hours)
    conn.execute("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES ('D_SCEN8_1', 1.0, 'SCEN8_FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 100.0, 100.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);")

    # 10. Configure Solver
    print("[*] Setting solver configuration parameters...")
    conn.execute("DELETE FROM ipc_solver_config;")
    conn.execute("INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'iop');")
    conn.execute("INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_step', 'all');")
    
    conn.close()
    print("[*] Seeding completed. Running solver engine...")

    # 11. Run Solver Engine to Pre-calculate and Save all Ledger Records
    res = subprocess.run([SOLVER_EXE, "--db", DB_PATH], stdin=subprocess.DEVNULL, capture_output=True, text=True, encoding='utf-8', errors='ignore')
    if res.returncode != 0:
        print("[!] Solver failed:")
        print(res.stderr)
        return
        
    print("[*] Solver ran successfully. Pre-calculated database state is locked.")
    print("[*] Verifying pre-calculated records...")
    
    conn = duckdb.connect(DB_PATH)
    
    print("\n--- Alternate Allocation Records (Scenarios 1, 2, 3, 5, 6) ---")
    alts = conn.execute("SELECT main_part, alt_part, allocated_qty, alt_class FROM ipc_alternate_allocation ORDER BY main_part, alt_part;").fetchall()
    for row in alts:
         print(f"  Replaced: {row[0]}, Alternate: {row[1]}, Qty: {row[2]}, Class: {row[3]}")
         
    print("\n--- Planned Order Ledger Records (Scenario 4) ---")
    po = conn.execute("SELECT part_code, order_qty, dimension_val FROM ipc_planned_order_ledger WHERE part_code LIKE 'SCEN4_%' ORDER BY part_code;").fetchall()
    for row in po:
         print(f"  Part: {row[0]}, Qty: {row[1]}, Grade: {row[2]}")

    print("\n--- Dispatch Ledger Records (Scenario 8) ---")
    dispatches = conn.execute("SELECT part_code, order_qty, scheduled_day, allocated_capacity FROM ipc_dispatch_ledger WHERE part_code = 'SCEN8_FG_MAIN' ORDER BY scheduled_day;").fetchall()
    for row in dispatches:
         print(f"  Part: {row[0]}, Qty: {row[1]}, Day: {row[2]}, Cap: {row[3]}")

    print("\n--- Swap safety valve records (Scenario 6) ---")
    swaps = conn.execute("SELECT * FROM ipc_swap_result;").fetchall()
    for row in swaps:
         print(f"  Demand: {row[0]}, From: {row[1]}, To: {row[2]}, Qty: {row[3]}, Day: {row[4]}")
         
    conn.close()
    print("[*] Done! Database is primed and pre-calculated for the interactive demo.")

if __name__ == "__main__":
    main()
