import subprocess
import sys

sys.stdout.reconfigure(encoding='utf-8')
repo_dir = r"h:\系统科学\价值链物理学"

print("=== Executing Git Add, Commit, and Push ===")

# 1. Git add
try:
    res_add = subprocess.run(["git", "add", "."], cwd=repo_dir, check=True, capture_output=True, text=True, encoding='utf-8', errors='replace')
    print("Git add completed successfully!")
except Exception as e:
    print(f"Git add error: {e}")

# 2. Git status
res_status = subprocess.run(["git", "status", "-s"], cwd=repo_dir, capture_output=True, text=True, encoding='utf-8', errors='replace')
print("Git status:\n", res_status.stdout.strip() if res_status.stdout else "Empty")

# 3. Git commit
commit_msg = "feat: sync 5D Mind Model and Qian Xuesen Formal Proof (ZH & EN) bilingual papers and master monograph"
try:
    res_commit = subprocess.run(["git", "commit", "-m", commit_msg], cwd=repo_dir, capture_output=True, text=True, encoding='utf-8', errors='replace')
    print("Git commit output:\n", res_commit.stdout.strip() if res_commit.stdout else res_commit.stderr.strip())
except Exception as e:
    print(f"Git commit info/warning: {e}")

# 4. Git push to origin (GitHub)
print("\n--- Pushing to GitHub (origin main) ---")
try:
    res_push_gh = subprocess.run(["git", "push", "origin", "main"], cwd=repo_dir, capture_output=True, text=True, encoding='utf-8', errors='replace')
    print("GitHub push output:\n", res_push_gh.stdout.strip() if res_push_gh.stdout else res_push_gh.stderr.strip())
except Exception as e:
    print(f"GitHub push output/error: {e}")

# 5. Git push to gitee (Gitee)
print("\n--- Pushing to Gitee (gitee main) ---")
try:
    res_push_gt = subprocess.run(["git", "push", "gitee", "main"], cwd=repo_dir, capture_output=True, text=True, encoding='utf-8', errors='replace')
    print("Gitee push output:\n", res_push_gt.stdout.strip() if res_push_gt.stdout else res_push_gt.stderr.strip())
except Exception as e:
    print(f"Gitee push output/error: {e}")
