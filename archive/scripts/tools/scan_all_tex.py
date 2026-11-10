import os

work_dir = os.path.dirname(os.path.abspath(__file__))
tex_files = [f for f in os.listdir(work_dir) if f.endswith(".tex")]

print("Scanning all .tex files in system science directory:")

for fname in tex_files:
    fpath = os.path.join(work_dir, fname)
    size = os.path.getsize(fpath)
    with open(fpath, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()[:15]
    print(f"\n==========================================")
    print(f"File: {fname} (Size: {size} bytes)")
    print("First 10 lines:")
    for line in lines[:10]:
        print("  |", line.strip())
