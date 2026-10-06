import os, docx

doc_path = r"h:\系统科学\秩序的生成、存续和进化 - 1.0.docx"
txt_output = r"h:\系统科学\the-holographic-anti-entropy-paper\canonical_chinese_text.txt"

print(f"Extracting text from user's canonical document: {doc_path}")

doc = docx.Document(doc_path)
full_text = []
for p in doc.paragraphs:
    if p.text.strip():
        full_text.append(p.text.strip())

for table in doc.tables:
    for row in table.rows:
        row_text = [cell.text.strip() for cell in row.cells if cell.text.strip()]
        if row_text:
            full_text.append(" | ".join(row_text))

extracted = "\n\n".join(full_text)

with open(txt_output, "w", encoding="utf-8") as f:
    f.write(extracted)

print(f"Successfully extracted {len(extracted)} characters from canonical docx! Saved to {txt_output}")
