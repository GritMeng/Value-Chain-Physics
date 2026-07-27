import os, re, tarfile

work_dir = os.path.dirname(os.path.abspath(__file__))
target_main_tex = os.path.join(work_dir, "main.tex")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

print("Removing Seven Iron Rules (writing instructions) from main.tex text...")

with open(target_main_tex, "r", encoding="utf-8") as f:
    content = f.read()

# Remove any mention of Seven Iron Rules from article text
cleaned_content = re.sub(r'We strictly enforce Seven Iron Rules.*?\n', '', content)
cleaned_content = re.sub(r'Seven Iron Rules', '', cleaned_content)

with open(target_main_tex, "w", encoding="utf-8") as f:
    f.write(cleaned_content)

print(f"Successfully purged writing rules from main.tex! File size: {len(cleaned_content)} bytes")

# Re-create clean tar.gz
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
images_dir = os.path.join(work_dir, "images")

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

print(f"Re-packaged clean tar.gz at: {clean_tar}")
