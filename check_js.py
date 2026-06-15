import re
import subprocess
import os

html_files = [
    r"h:\系统科学\价值链物理学\IPC_Video_Lectures.html",
    r"h:\系统科学\价值链物理学\index.html",
    r"h:\系统科学\价值链物理学\value_chain_audit_agent.html",
    r"h:\系统科学\价值链物理学\ipc_scheme_audit_agent.html",
    r"h:\系统科学\价值链物理学\value_chain_orchestration.html",
    r"h:\系统科学\价值链物理学\value_chain_orchestration_local.html"
]

for html_path in html_files:
    print(f"\n========================================")
    print(f"Checking file: {os.path.basename(html_path)}")
    print(f"========================================")
    
    with open(html_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Find all script blocks
    scripts = re.findall(r"<script[^>]*>(.*?)</script>", content, re.DOTALL)

    for i, script in enumerate(scripts):
        # Skip CDN script imports or empty tags
        if not script.strip():
            continue
        
        js_path = f"temp_script_{os.path.basename(html_path)}_{i}.js"
        with open(js_path, "w", encoding="utf-8") as f_js:
            f_js.write(script)
            
        print(f"Checking script block {i}...")
        res = subprocess.run(["node", "--check", js_path], capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Error in script block {i}:")
            print(res.stderr)
            exit_code = 1
        else:
            print(f"Script block {i} is syntactically correct.")
            
        os.remove(js_path)
