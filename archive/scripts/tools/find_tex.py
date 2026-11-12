import os, glob

work_dir = os.path.dirname(os.path.abspath(__file__))
tex_files = [f for f in os.listdir(work_dir) if f.endswith(".tex")]
print("All .tex files found:", tex_files)
