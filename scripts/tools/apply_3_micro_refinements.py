import os, re, tarfile

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
target_main_tex = os.path.join(work_dir, "main.tex")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

print("Applying 3 micro-refinements from user to main.tex...")

with open(target_main_tex, "r", encoding="utf-8") as f:
    tex_content = f.read()

# 1. Update Definition 2
old_def2 = r"Awareness ($\text{Zhi}$, 能审之明) is the clarity to discern, the source of partition; Construction ($\text{Shi}$, 能建之功) is the power to build, the boundary of order."
new_def2 = r"Awareness ($\text{Zhi}$, 能审之明) is the capacity for discernment---the source of partition; Construction ($\text{Shi}$, 能建之功) is the capacity for building---the boundary of order."

if old_def2 in tex_content:
    tex_content = tex_content.replace(old_def2, new_def2)

# Alternative fallback for definition 2
old_def2_alt = "is the clarity to discern, the source of partition; Construction"
new_def2_alt = "is the capacity for discernment---the source of partition; Construction (Shi, 能建之功) is the capacity for building---the boundary of order."

# 2. Update Writing Constitution Rule 1
tex_content = tex_content.replace("sudden mutation is strictly prohibited.", "sudden mutations are strictly prohibited.")

# 3. Update Theorem 1 Pareto-optimal
tex_content = tex_content.replace("unique Pareto optimal state", "Pareto-optimal state")
tex_content = tex_content.replace("Pareto optimal state", "Pareto-optimal state")

with open(target_main_tex, "w", encoding="utf-8") as f:
    f.write(tex_content)

print("Successfully applied micro-refinements to main.tex!")

# Package clean tar
if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(target_main_tex, arcname="main.tex")

print(f"Ultra-pure single-file tar.gz updated at: {clean_tar}")
