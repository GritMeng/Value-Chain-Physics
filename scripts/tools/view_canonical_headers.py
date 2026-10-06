import os

txt_file = r"h:\系统科学\the-holographic-anti-entropy-paper\canonical_chinese_text.txt"
with open(txt_file, "r", encoding="utf-8") as f:
    text = f.read()

lines = text.split("\n\n")
print("Total paragraphs extracted:", len(lines))
print("\nFirst 15 paragraphs of the canonical text:")
for i in range(min(15, len(lines))):
    print(f"[{i+1}]", lines[i])
