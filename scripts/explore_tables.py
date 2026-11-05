import duckdb

conn = duckdb.connect("ipc.db", read_only=True)
tables = conn.execute("SELECT table_name FROM information_schema.tables WHERE table_schema='main'").fetchall()
print("Tables in ipc.db:")
for t in tables:
    tname = t[0]
    print(f"\n=========================================\nTable: {tname}")
    try:
        columns = conn.execute(f"DESCRIBE {tname}").fetchall()
        print("Columns:")
        for col in columns:
            print(f"  - {col[0]}: {col[1]}")
        
        row_count = conn.execute(f"SELECT count(*) FROM {tname}").fetchone()[0]
        print(f"Row count: {row_count}")
        
        if row_count > 0:
            print("First 3 rows:")
            rows = conn.execute(f"SELECT * FROM {tname} LIMIT 3").fetchall()
            for r in rows:
                print(f"  {r}")
    except Exception as e:
        print(f"Error querying {tname}: {e}")
conn.close()
