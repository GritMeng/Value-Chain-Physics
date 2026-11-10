import duckdb
import time

db_path = "ipc.db"
print("Connecting to database...")
start_time = time.time()
conn = duckdb.connect(db_path)
print(f"Connected in {time.time() - start_time:.4f} seconds.")

queries = [
    "CREATE INDEX IF NOT EXISTS idx_po_part ON ipc_planned_order_ledger (part_code);",
    "CREATE INDEX IF NOT EXISTS idx_po_finish ON ipc_planned_order_ledger (finish_day);",
    "CREATE INDEX IF NOT EXISTS idx_id_demand ON ipc_independent_demand (demand);",
    "CREATE INDEX IF NOT EXISTS idx_psa_demand ON ipc_planned_supply_assignment (demand);",
    "CREATE INDEX IF NOT EXISTS idx_parts_part ON ipc_part_status (part_code);",
]

for q in queries:
    print(f"Running: {q}")
    s = time.time()
    try:
        conn.execute(q)
        print(f"  Success in {time.time() - s:.4f} seconds.")
    except Exception as e:
        print(f"  Error: {e}")

conn.close()
print("Done.")
