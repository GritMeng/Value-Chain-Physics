import os
import urllib.parse
from datetime import datetime

# 配置
BASE_URL = "https://gritmeng.github.io/Value-Chain-Physics"
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))

def get_valid_html_pages(root_dir):
    """
    搜集所有根目录下真实可直接 HTTP 200 访问的 HTML 静态页面
    排除 404.html、.venv、git 等内部页面
    """
    valid_pages = []
    
    # 核心根目录 HTML 页面
    for file in os.listdir(root_dir):
        if file.endswith('.html') and file != '404.html' and file != 'google41f8bfc02cf1af0b.html':
            valid_pages.append(file)
            
    return sorted(valid_pages)

def generate_sitemap(html_files):
    sitemap_content = []
    sitemap_content.append('<?xml version="1.0" encoding="UTF-8"?>')
    sitemap_content.append('<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">')
    
    today = datetime.now().strftime("%Y-%m-%d")
    
    # 1. 首页 (Highest priority)
    sitemap_content.append('  <url>')
    sitemap_content.append(f'    <loc>{BASE_URL}/</loc>')
    sitemap_content.append(f'    <lastmod>{today}</lastmod>')
    sitemap_content.append('    <changefreq>daily</changefreq>')
    sitemap_content.append('    <priority>1.0</priority>')
    sitemap_content.append('  </url>')
    
    # 2. 真实可访问的 HTML 静态页面
    for html in html_files:
        if html == 'index.html':
            continue  # 已由 BASE_URL/ 覆盖
        encoded_html = urllib.parse.quote(html)
        sitemap_content.append('  <url>')
        sitemap_content.append(f'    <loc>{BASE_URL}/{encoded_html}</loc>')
        sitemap_content.append(f'    <lastmod>{today}</lastmod>')
        sitemap_content.append('    <changefreq>weekly</changefreq>')
        sitemap_content.append('    <priority>0.8</priority>')
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
    print("正在扫描真实 HTML 页面...")
    html_files = get_valid_html_pages(ROOT_DIR)
    
    # 写入 sitemap.xml
    sitemap_path = os.path.join(ROOT_DIR, "sitemap.xml")
    sitemap_data = generate_sitemap(html_files)
    with open(sitemap_path, "w", encoding="utf-8") as f:
        f.write(sitemap_data)
    print(f"成功生成纯净版 Sitemap: {sitemap_path}")
    
    # 写入 robots.txt
    robots_path = os.path.join(ROOT_DIR, "robots.txt")
    robots_data = generate_robots()
    with open(robots_path, "w", encoding="utf-8") as f:
        f.write(robots_data)
    print(f"成功生成 Robots.txt: {robots_path}")
