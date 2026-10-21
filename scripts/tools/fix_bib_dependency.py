import os, tarfile

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
target_main_tex = os.path.join(work_dir, "main.tex")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

with open(target_main_tex, "r", encoding="utf-8") as f:
    content = f.read()

# Remove \bibliography and \bibliographystyle commands completely, replace with inline bibliography
fixed_content = content.replace(r"\bibliographystyle{plain}", "")
fixed_content = fixed_content.replace(r"\bibliography{GritMeng_Research_Outputs}", r"""\begin{thebibliography}{99}
\bibitem{Meng2026} F. Meng, \textit{Physics of Value Chain Management and Holographic Anti-Entropy}, Monograph Manuscript, 2026.
\end{thebibliography}""")

with open(target_main_tex, "w", encoding="utf-8") as f:
    f.write(fixed_content)

print("Fixed main.tex! Completely removed external .bib requirement.")

# Package ultra-clean tar containing ONLY main.tex and images (NO .bib file needed!)
if os.path.exists(clean_tar):
    os.remove(clean_tar)

images_dir = os.path.join(work_dir, "images")
with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(target_main_tex, arcname="main.tex")
    if os.path.exists(images_dir):
        for root, dirs, files in os.walk(images_dir):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, work_dir)
                tar.add(full_path, arcname=rel_path)

print(f"Ultra-clean tar.gz created at: {clean_tar}")
with tarfile.open(clean_tar, "r:gz") as tar:
    print("Files inside clean package:", tar.getnames())
