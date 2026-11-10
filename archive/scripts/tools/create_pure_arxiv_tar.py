import os, tarfile

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
tex_file = os.path.join(work_dir, "main.tex")
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

# Ensures clean creation of tar.gz with ONLY main.tex and bib file
if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    # Add main.tex
    tar.add(tex_file, arcname="main.tex")
    # Add bib if exists
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")

print(f"Clean package generated at: {clean_tar}")
# Verify files inside tar
with tarfile.open(clean_tar, "r:gz") as tar:
    print("Files inside tar package:", tar.getnames())
