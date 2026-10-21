import os
import sys
import win32com.client

base_dir = r"H:\系统科学\价值链物理学"
zh_docx = os.path.join(base_dir, "Paper_1_Formal_Proof_OCGS_Qian_Xuesen_ZH.docx")
en_docx = os.path.join(base_dir, "Paper_1_Formal_Proof_OCGS_Qian_Xuesen_EN.docx")

zh_pdf = os.path.join(base_dir, "Paper_1_Formal_Proof_OCGS_Qian_Xuesen_ZH.pdf")
en_pdf = os.path.join(base_dir, "Paper_1_Formal_Proof_OCGS_Qian_Xuesen_EN.pdf")

def docx_to_pdf(word_app, docx_path, pdf_path):
    print(f"Converting {docx_path} -> {pdf_path}...")
    abs_docx = os.path.abspath(docx_path)
    abs_pdf = os.path.abspath(pdf_path)
    
    doc = word_app.Documents.Open(abs_docx, ReadOnly=True)
    # FileFormat=17 is wdFormatPDF
    doc.SaveAs(abs_pdf, FileFormat=17)
    doc.Close(0) # wdDoNotSaveChanges
    print(f"Successfully converted to {pdf_path}")

def main():
    word = win32com.client.Dispatch("Word.Application")
    word.Visible = False
    word.DisplayAlerts = 0 # wdAlertsNone
    
    try:
        docx_to_pdf(word, zh_docx, zh_pdf)
        docx_to_pdf(word, en_docx, en_pdf)
    except Exception as e:
        print(f"Error during conversion: {e}")
    finally:
        word.Quit()
        print("Word application quit successfully.")

if __name__ == "__main__":
    main()
