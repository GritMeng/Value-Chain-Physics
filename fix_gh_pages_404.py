import os
import shutil

docs_index = r'h:\IPC\docs\index.html'
root_index = r'h:\IPC\index.html'
docs_404 = r'h:\IPC\docs\404.html'
root_404 = r'h:\IPC\404.html'
docs_nojekyll = r'h:\IPC\docs\.nojekyll'
root_nojekyll = r'h:\IPC\.nojekyll'

# 1. Ensure .nojekyll in both root and docs
with open(root_nojekyll, 'w', encoding='utf-8') as f:
    f.write('\n')
with open(docs_nojekyll, 'w', encoding='utf-8') as f:
    f.write('\n')

# 2. Read docs/index.html
with open(docs_index, 'r', encoding='utf-8') as f:
    docs_content = f.read()

# Create root index.html: it sets basePath to './docs/' so Docsify loads from ./docs/ seamlessly!
if 'basePath:' not in docs_content:
    root_content = docs_content.replace(
        'window.$docsify = {',
        "window.$docsify = {\n      basePath: './docs/',"
    )
else:
    root_content = docs_content

with open(root_index, 'w', encoding='utf-8') as f:
    f.write(root_content)

# 3. Ensure 404.html in root
if os.path.exists(docs_404):
    shutil.copyfile(docs_404, root_404)
elif os.path.exists(root_index):
    shutil.copyfile(root_index, root_404)

print('SUCCESS: Created h:\\IPC\\index.html, h:\\IPC\\404.html, and .nojekyll in both root and docs!')
