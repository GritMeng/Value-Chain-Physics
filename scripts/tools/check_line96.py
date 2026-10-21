import os

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
target_main_tex = os.path.join(work_dir, "main.tex")

with open(target_main_tex, "r", encoding="utf-8") as f:
    lines = f.readlines()

print("Line 80 to 110 of main.tex:")
for i in range(79, min(110, len(lines))):
    print(f"Line {i+1}: {repr(lines[i])}")
