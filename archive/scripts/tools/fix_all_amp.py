import os, re, tarfile

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
tex_file = os.path.join(work_dir, "main.tex")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

with open(tex_file, "r", encoding="utf-8") as f:
    content = f.read()

# Comprehensive regex replacement for unescaped & (not preceded by a backslash \)
# Matches & that is NOT preceded by \
unescaped_amp = re.compile(r'(?<!\\)&')

# Replace all unescaped & with \&
new_content = unescaped_amp.sub(r'\\&', content)

with open(tex_file, "w", encoding="utf-8") as f:
    f.write(new_content)

print("Comprehensive scan: All unescaped & replaced with \\&!")

# Re-pack clean tar.gz
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(tex_file, arcname="main.tex")
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")

print(f"Updated tar.gz created at: {clean_tar}")
