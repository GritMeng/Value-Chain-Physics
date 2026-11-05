import os, glob

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
main_tex = os.path.join(work_dir, "main.tex")
backup_dir = os.path.join(work_dir, "old_draft_backups")

if not os.path.exists(backup_dir):
    os.makedirs(backup_dir)

# Move all other obsolete .tex files into old_draft_backups so they NEVER interfere again
all_tex = glob.glob(os.path.join(work_dir, "*.tex"))
moved_count = 0
for fpath in all_tex:
    filename = os.path.basename(fpath)
    if filename != "main.tex":
        dest = os.path.join(backup_dir, filename)
        os.rename(fpath, dest)
        moved_count += 1

print(f"Isolated {moved_count} obsolete .tex draft files into 'old_draft_backups'.")
print("Now h:\\系统科学\\the-holographic-anti-entropy-paper has ONLY ONE SINGLE main.tex file!")
