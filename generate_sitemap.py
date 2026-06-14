import os
import urllib.parse
from datetime import datetime

# 配置
BASE_URL = "https://gritmeng.github.io/Value-Chain-Physics"
DOCS_PAGE = "docs.html"  # Docsify 路由页面
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))

# 需要排除的目录和文件
EXCLUDE_DIRS = {'.git', '.github', '.obsidian', '__pycache__', 'scratch', 'audio', 'audio_manifesto', 'audio_short', 'wechat_images', 'global_candidates', 'extracted_images', '商业演说与PPT大纲'}
EXCLUDE_FILES = {'.gitignore', '.nojekyll', '.markdownlint.json', 'publish_tool.py', 'check_js.py', 'inspect_cases.py'}

def get_md_files(root_dir):
    md_files = []
    for dirpath, dirnames, filenames in os.walk(root_dir):
        # 排除不需要的目录
        dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS]
        
        for file in filenames:
            if file.endswith('.md') and file not in EXCLUDE_FILES:
                full_path = os.path.join(dirpath, file)
                rel_path = os.path.relpath(full_path, root_dir)
                md_files.append(rel_path)
    return md_files

def generate_sitemap(md_files):
    sitemap_content = []
    sitemap_content.append('<?xml version="1.0" encoding="UTF-8"?>')
    sitemap_content.append('<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">')
    
    # 1. 添加主演示主页 (index.html)
    sitemap_content.append('  <url>')
    sitemap_content.append(f'    <loc>{BASE_URL}/</loc>')
    sitemap_content.append(f'    <lastmod>{datetime.now().strftime("%Y-%m-%d")}</lastmod>')
    sitemap_content.append('    <changefreq>weekly</changefreq>')
    sitemap_content.append('    <priority>1.0</priority>')
    sitemap_content.append('  </url>')
    
    # 2. 添加其他的静态 HTML 页面
    extra_htmls = ['docs.html', 'IPC_Video_Lectures.html', 'value_chain_audit_agent.html', 'ipc_scheme_audit_agent.html']
    for html in extra_htmls:
        if os.path.exists(os.path.join(ROOT_DIR, html)):
            sitemap_content.append('  <url>')
            sitemap_content.append(f'    <loc>{BASE_URL}/{html}</loc>')
            sitemap_content.append(f'    <lastmod>{datetime.now().strftime("%Y-%m-%d")}</lastmod>')
            sitemap_content.append('    <changefreq>weekly</changefreq>')
            sitemap_content.append('    <priority>0.8</priority>')
            sitemap_content.append('  </url>')

    # 3. 添加 Docsify 渲染的 Markdown 路由页
    for file in md_files:
        # Docsify 路由规则: 
        # README.md -> /
        # subdir/README.md -> /subdir/
        # subdir/file.md -> /subdir/file
        
        clean_path = file.replace('\\', '/') # Windows 路径转换
        if clean_path.lower() == 'readme.md':
            route = ""
        elif clean_path.lower().endswith('readme.md'):
            route = clean_path[:-9]  # 去除 'README.md'
        elif clean_path.lower().endswith('.md'):
            route = clean_path[:-3]  # 去除 '.md'
        else:
            route = clean_path

        # URL 编码，防止中文路径报错
        encoded_route = urllib.parse.quote(route)
        
        # 组装 Docsify Hash 路由
        if encoded_route == "":
            full_url = f"{BASE_URL}/{DOCS_PAGE}#/"
        else:
            full_url = f"{BASE_URL}/{DOCS_PAGE}#/{encoded_route}"
            
        sitemap_content.append('  <url>')
        sitemap_content.append(f'    <loc>{full_url}</loc>')
        sitemap_content.append(f'    <lastmod>{datetime.now().strftime("%Y-%m-%d")}</lastmod>')
        sitemap_content.append('    <changefreq>weekly</changefreq>')
        sitemap_content.append('    <priority>0.7</priority>')
        sitemap_content.append('  </url>')
        
    sitemap_content.append('</urlset>')
    return '\n'.join(sitemap_content)

def generate_robots():
    robots_content = [
        "User-agent: *",
        "Allow: /",
        f"Sitemap: {BASE_URL}/sitemap.xml"
    ]
    return '\n'.join(robots_content)

if __name__ == "__main__":
    print("正在扫描 Markdown 文件...")
    md_files = get_md_files(ROOT_DIR)
    
    # 写入 sitemap.xml
    sitemap_path = os.path.join(ROOT_DIR, "sitemap.xml")
    sitemap_data = generate_sitemap(md_files)
    with open(sitemap_path, "w", encoding="utf-8") as f:
        f.write(sitemap_data)
    print(f"成功生成: {sitemap_path} (共 {len(md_files)} 个文档路由)")
    
    # 写入 robots.txt
    robots_path = os.path.join(ROOT_DIR, "robots.txt")
    robots_data = generate_robots()
    with open(robots_path, "w", encoding="utf-8") as f:
        f.write(robots_data)
    print(f"成功生成: {robots_path}")
