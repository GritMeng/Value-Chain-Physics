import os, re
import docx

def anonymize_text(text):
    text = re.sub(r'Fanchun Meng \(Grit Meng\)', '[Anonymized Author]', text)
    text = re.sub(r'Fanchun Meng', '[Anonymized Author]', text)
    text = re.sub(r'Grit Meng', '[Anonymized Author]', text)
    text = re.sub(r'Department of Intelligent Planning & Control \(IPC\) / Independent Researcher, Beijing, China', '[Anonymized Institution & Department]', text)
    text = re.sub(r'Lenovo Group', '[Anonymized Global Tech Enterprise]', text)
    text = re.sub(r'Lenovo', '[Anonymized Enterprise]', text)
    text = re.sub(r'Hefei Lighthouse Factory', '[Anonymized Lighthouse Factory]', text)
    text = re.sub(r'SSRN Working Paper Version:.*?\n', '[Anonymized Preprint Link]\n', text)
    text = re.sub(r'DOI: 10\.5281/zenodo\.\d+', '[Anonymized DOI]', text)
    return text

def create_anonymized_version():
    with open("value_chain_physics_scm_paper_draft_en.md", "r", encoding="utf-8") as f:
        content = f.read()
        
    anon_content = anonymize_text(content)
    
    with open("value_chain_physics_scm_paper_draft_en_anonymized.md", "w", encoding="utf-8") as f:
        f.write(anon_content)
        
    # Run pandoc and style
    os.system("pandoc value_chain_physics_scm_paper_draft_en_anonymized.md -o value_chain_physics_scm_paper_draft_en_anonymized_native.docx")
    os.system("python -c \"import style_native_docx; style_native_docx.process_document('value_chain_physics_scm_paper_draft_en_anonymized_native.docx', 'value_chain_physics_scm_paper_draft_en_anonymized.docx', is_english=True)\"")
    print("Successfully created anonymized docx and md files!")

if __name__ == "__main__":
    create_anonymized_version()
