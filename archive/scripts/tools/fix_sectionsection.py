import os, tarfile

work_dir = os.path.dirname(os.path.abspath(__file__))
target_main_tex = os.path.join(work_dir, "main.tex")
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
images_dir = os.path.join(work_dir, "images")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

with open(target_main_tex, "r", encoding="utf-8") as f:
    content = f.read()

# Fixes the double section typo: \sectionsection -> \section
fixed_content = content.replace(r"\sectionsection", r"\section")
fixed_content = fixed_content.replace(r"\subsectionsection", r"\subsection")

with open(target_main_tex, "w", encoding="utf-8") as f:
    f.write(fixed_content)

print(f"Fixed typo \\sectionsection -> \\section in main.tex!")

# Re-package full clean tar.gz with all images
if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(target_main_tex, arcname="main.tex")
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")
    if os.path.exists(images_dir):
        for root, dirs, files in os.walk(images_dir):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, work_dir)
                tar.add(full_path, arcname=rel_path)

print(f"Ultra-clean arXiv package repacked at: {clean_tar}")
