import os, tarfile

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
tex_file = os.path.join(work_dir, "main.tex")
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

with open(tex_file, "r", encoding="utf-8") as f:
    tex_content = f.read()

# Fixes misplaced alignment tab character & -> \& in main.tex
fixed_tex = tex_content.replace(r"Preface & Introduction", r"Preface \& Introduction")
fixed_tex = fixed_tex.replace(r"Banach Contraction & $O(N \log N)$", r"Banach Contraction \& $O(N \log N)$")
fixed_tex = fixed_tex.replace(r"Physical Laws, Isomorphism, and Empirical Validation", r"Physical Laws, Isomorphism, \& Empirical Validation")

# Ensure no other unescaped & exists in section titles
with open(tex_file, "w", encoding="utf-8") as f:
    f.write(fixed_tex)

print("Fixed unescaped & in main.tex!")

# Re-create ultra-clean tar.gz
if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(tex_file, arcname="main.tex")
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")

print(f"Re-packaged clean tar.gz at: {clean_tar}")
with tarfile.open(clean_tar, "r:gz") as tar:
    print("Files in tar:", tar.getnames())
