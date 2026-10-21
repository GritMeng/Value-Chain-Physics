import os, re, tarfile

work_dir = os.path.dirname(os.path.abspath(__file__))
full_source_tex = os.path.join(work_dir, "the_holographic_anti_entropy_system_science_paper_en.tex")
target_main_tex = os.path.join(work_dir, "main.tex")
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

print(f"Reading full 70KB LaTeX manuscript from {full_source_tex}...")

with open(full_source_tex, "r", encoding="utf-8") as f:
    full_tex_content = f.read()

# Escapes unescaped & in text
unescaped_amp = re.compile(r'(?<!\\)&')
fixed_tex = unescaped_amp.sub(r'\\&', full_tex_content)

# Overwrite main.tex with the 70KB unabridged full manuscript
with open(target_main_tex, "w", encoding="utf-8") as f:
    f.write(fixed_tex)

print(f"Successfully overwritten main.tex with the full unabridged monograph! New main.tex size: {len(fixed_tex)} bytes")

# Re-create ultra-clean arXiv tar.gz package
if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(target_main_tex, arcname="main.tex")
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")

print(f"Re-packaged clean tar.gz at: {clean_tar}")
with tarfile.open(clean_tar, "r:gz") as tar:
    print("Files inside tar package:", tar.getnames())
