import os
import re
import subprocess
import sys
from datetime import datetime

# Set root directory (current directory)
root_dir = os.path.dirname(os.path.abspath(__file__))

def get_sidebar_headers():
    # Base headers that should always be at the top of the sidebar
    return [
        "* [📜 八大核心公理](README.md)",
        '* <a href="value_chain_audit_agent.html" target="_blank">🔮 价值链智能体审计</a>',
        '* <a href="ipc_scheme_audit_agent.html" target="_blank">🧠 IPC 架构技术审计</a>',
        "* [🚀 价值链物理学正文](value-chain-physics-book.md)",
        "* [🔮 全息抗熵正文](holographic-anti-entropy-book.md)",
        '* <a href="the_holographic_anti_entropy_system_science_paper.html" target="_blank">📄 全息抗熵学术论文 (HTML)</a>',
        '* <a href="the_holographic_anti_entropy_system_science_paper.pdf" target="_blank">📕 全息抗熵学术论文 (PDF)</a>',
        '* <a href="academic_papers_draft_for_cao.html" target="_blank">📝 呈送操龙兵教授的联合研究计划与学术论文草案</a>',
        '* <a href="Paper1_Decentralized_Learning_Dynamics.html" target="_blank">  ↳ 论文一：去中心化学习发散与收敛理论</a>',
        '* <a href="Paper2_Hybrid_Intelligence_Mind_OS.html" target="_blank">  ↳ 论文二：混合智能心智 OS 与边界穿透模型</a>',
        '* <a href="Paper3_Complex_Supply_Networks_Projection.html" target="_blank">  ↳ 论文三：复杂供应网络物理定律与 O(N!) 求解器</a>',
        ""
    ]


def generate_sidebar():
    sidebar_lines = get_sidebar_headers()
    
    # Find all subdirectories that match the pattern "XX_name"
    dirs = []
    for item in os.listdir(root_dir):
        item_path = os.path.join(root_dir, item)
        if os.path.isdir(item_path) and re.match(r'^\d{2}_', item):
            dirs.append(item)
            
    # Sort directories by their prefix number
    dirs.sort()
    
    for d in dirs:
        # Format folder name for sidebar header (e.g. "01_系统科学与时空定律" -> "01. 系统科学与时空定律")
        folder_display_name = d.replace('_', '. ', 1)
        sidebar_lines.append(f"* **{folder_display_name}**")
        
        # Get md files in this directory
        files = []
        dir_path = os.path.join(root_dir, d)
        for f in os.listdir(dir_path):
            if f.endswith('.md'):
                files.append(f)
                
        # Sort files (assuming they start with sequential numbers)
        files.sort()
        
        for f in files:
            # Extract display name (e.g. "01_系统第二定律...md" -> "01. 系统第二定律...")
            base_name = f[:-3] # remove .md
            # Replace the first underscore with ". "
            display_name = base_name.replace('_', '. ', 1)
            # Create a path relative to the vault root
            relative_path = f"{d}/{f}"
            sidebar_lines.append(f"  * [{display_name}]({relative_path})")
            
        sidebar_lines.append("") # add blank line between sections
        
    sidebar_content = "\n".join(sidebar_lines)
    
    # Write to _sidebar.md in UTF-8
    sidebar_path = os.path.join(root_dir, "_sidebar.md")
    with open(sidebar_path, "w", encoding="utf-8") as f:
        f.write(sidebar_content)
        
    print("[OK] Successfully regenerated _sidebar.md with directory structure!")
    return sidebar_content

def run_git_commands():
    try:
        # Check if git is available
        subprocess.run(["git", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    except Exception:
        print("Error: Git is not installed or not in PATH.")
        return False
        
    # Staging changes
    print("Staging all changes...")
    subprocess.run(["git", "add", "."], check=True)
    
    # Check if there are any changes to commit
    status_output = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True, check=True).stdout.strip()
    if not status_output:
        print("No changes to commit. Everything is up to date.")
        return True
        
    # Commit changes
    commit_msg = f"Auto-publish Obsidian notes: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
    print(f"Committing changes: '{commit_msg}'...")
    subprocess.run(["git", "commit", "-m", commit_msg], check=True)
    
    # Push changes
    print("Pushing to GitHub remote branch...")
    try:
        subprocess.run(["git", "push"], check=True)
        print("[OK] Successfully pushed changes to GitHub!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error during push: {e}. If it's a network issue, try pushing manually.")
        return False

if __name__ == "__main__":
    print("=== Obsidian to GitHub Auto-Publisher ===")
    generate_sidebar()
    print("=========================================")
    run_git_commands()
