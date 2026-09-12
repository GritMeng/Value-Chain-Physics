import docx
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

def set_font_eastasia(run, font_name):
    rPr = run._element.get_or_add_rPr()
    rFonts = rPr.get_or_add_rFonts()
    rFonts.set(qn('w:eastAsia'), font_name)

def build_title_page():
    doc = docx.Document()
    
    # Page setup
    for sec in doc.sections:
        sec.top_margin = Inches(1.0)
        sec.bottom_margin = Inches(1.0)
        sec.left_margin = Inches(1.0)
        sec.right_margin = Inches(1.0)
        
    normal = doc.styles['Normal']
    normal.font.name = 'Times New Roman'
    normal.font.size = Pt(11)
    
    # Title
    p_title = doc.add_paragraph()
    p_title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_title.paragraph_format.space_before = Pt(24)
    p_title.paragraph_format.space_after = Pt(12)
    r_t = p_title.add_run("The Productivity Paradox of Capital Returns in Enterprise Value Networks: Formal Proofs and Universal Computability of Open Complex Giant Systems via Non-IID State-Space Modeling")
    r_t.font.size = Pt(16)
    r_t.font.bold = True
    r_t.font.name = 'Times New Roman'
    
    # Running Header
    p_rh = doc.add_paragraph()
    p_rh.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_rh.paragraph_format.space_after = Pt(18)
    r_rh = p_rh.add_run("Running Head: VALUE CHAIN PHYSICS & OCGS UNIVERSAL COMPUTABILITY")
    r_rh.font.size = Pt(9.5)
    r_rh.font.italic = True
    r_rh.font.color.rgb = RGBColor(0x55, 0x55, 0x55)
    
    # Author List
    p_auth = doc.add_paragraph()
    p_auth.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_auth.paragraph_format.space_after = Pt(4)
    r_a = p_auth.add_run("Fanchun Meng (Grit Meng)")
    r_a.font.size = Pt(12)
    r_a.font.bold = True
    
    # Affiliation
    p_aff = doc.add_paragraph()
    p_aff.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_aff.paragraph_format.space_after = Pt(18)
    r_aff = p_aff.add_run("Department of Intelligent Planning & Control (IPC) / Independent Researcher, Beijing, China")
    r_aff.font.size = Pt(10.5)
    r_aff.font.italic = True
    
    # Target Journal
    p_j = doc.add_paragraph()
    p_j.paragraph_format.space_before = Pt(12)
    p_j.paragraph_format.space_after = Pt(6)
    r_jh = p_j.add_run("Target Journal: ")
    r_jh.font.bold = True
    p_j.add_run("IEEE Transactions on Systems, Man, and Cybernetics: Systems (T-SMC)")
    
    # Corresponding Author Info Block
    p_cor = doc.add_paragraph()
    p_cor.paragraph_format.space_before = Pt(6)
    p_cor.paragraph_format.space_after = Pt(6)
    r_cor_h = p_cor.add_run("Corresponding Author:\n")
    r_cor_h.font.bold = True
    r_cor_b = p_cor.add_run(
        "Fanchun Meng (Grit Meng)\n"
        "Chief Architect, Department of Intelligent Planning & Control (IPC)\n"
        "Beijing, China\n"
        "Email: gritmeng@outlook.com\n"
        "ORCID / Author Page: Abstract ID 7251098 (SSRN) | DOI: 10.5281/zenodo.22033928 (Zenodo)"
    )
    
    # Abstract Summary on Title Page
    p_abs_h = doc.add_paragraph()
    p_abs_h.paragraph_format.space_before = Pt(12)
    p_abs_h.paragraph_format.space_after = Pt(4)
    r_abs_h = p_abs_h.add_run("Word Count & Article Metadata:")
    r_abs_h.font.bold = True
    
    p_meta = doc.add_paragraph()
    p_meta.paragraph_format.space_after = Pt(12)
    p_meta.add_run("• Article Type: Regular Research Paper\n")
    p_meta.add_run("• Total Word Count: ~8,500 words\n")
    p_meta.add_run("• Number of Figures & Tables: 2 Tables, Formal Proof Equations\n")
    p_meta.add_run("• Disclosures & Conflicts of Interest: The author declares no conflicts of interest. The longitudinal field data is anonymized and audited by PwC / WEF Lighthouse Factory logs.")

    doc.save("Title_Page_IEEE_TSMC.docx")
    
    # Save txt version
    txt_content = f"""TITLE PAGE

Title: The Productivity Paradox of Capital Returns in Enterprise Value Networks: Formal Proofs and Universal Computability of Open Complex Giant Systems via Non-IID State-Space Modeling

Running Head: VALUE CHAIN PHYSICS & OCGS UNIVERSAL COMPUTABILITY

Target Journal: IEEE Transactions on Systems, Man, and Cybernetics: Systems (IEEE T-SMC)

Author Name: Fanchun Meng (Grit Meng)
Affiliation: Department of Intelligent Planning & Control (IPC) / Independent Researcher, Beijing, China

Corresponding Author Contact Information:
Fanchun Meng (Grit Meng)
Chief Architect, Department of Intelligent Planning & Control (IPC)
Beijing, China
Email: gritmeng@outlook.com
Academic Identifiers: SSRN Abstract ID 7251098 | Zenodo DOI: 10.5281/zenodo.22033928

Article Metadata:
- Article Type: Regular Research Paper
- Word Count: ~8,500 words
- Disclosures & Conflicts of Interest: The author declares no conflict of interest. Field empirical data is sourced from PwC-audited financial reports and WEF Lighthouse Factory logs.
"""
    with open("Title_Page_IEEE_TSMC.txt", "w", encoding="utf-8") as f:
        f.write(txt_content)
        
    print("Successfully generated Title_Page_IEEE_TSMC.docx and .txt!")

if __name__ == "__main__":
    build_title_page()
