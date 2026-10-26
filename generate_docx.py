#!/usr/bin/env python3
"""
generate_docx.py
Converts report.html into a beautifully formatted Microsoft Word (.docx) document
following the VIT SCOPE B.Tech Course Project report template guidelines.
"""

import os
import re
import html
import lxml.html
import docx
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml import parse_xml, OxmlElement
from docx.oxml.ns import nsdecls, qn

def set_cell_shading(cell, color_hex):
    """Sets background fill color of a table cell."""
    tcPr = cell._tc.get_or_add_tcPr()
    shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}"/>')
    tcPr.append(shd)

def set_cell_margins(cell, top=100, bottom=100, left=150, right=150):
    """Sets cell padding (in twips, 20 twips = 1 pt)."""
    tcPr = cell._tc.get_or_add_tcPr()
    tcMar = parse_xml(
        f'<w:tcMar {nsdecls("w")}>'
        f'<w:top w:w="{top}" w:type="dxa"/>'
        f'<w:bottom w:w="{bottom}" w:type="dxa"/>'
        f'<w:left w:w="{left}" w:type="dxa"/>'
        f'<w:right w:w="{right}" w:type="dxa"/>'
        f'</w:tcMar>'
    )
    tcPr.append(tcMar)

def set_table_borders(table, color="D3D3D3", sz="4", val="single"):
    """Applies neat borders to a table."""
    tblPr = table._tbl.tblPr
    borders = parse_xml(
        f'<w:tblBorders {nsdecls("w")}>'
        f'<w:top w:val="{val}" w:sz="{sz}" w:space="0" w:color="{color}"/>'
        f'<w:bottom w:val="{val}" w:sz="{sz}" w:space="0" w:color="{color}"/>'
        f'<w:left w:val="none"/>'
        f'<w:right w:val="none"/>'
        f'<w:insideH w:val="{val}" w:sz="{sz}" w:space="0" w:color="{color}"/>'
        f'<w:insideV w:val="none"/>'
        f'</w:tblBorders>'
    )
    tblPr.append(borders)

def add_runs_from_element(p, elem, is_bold=False, is_italic=False, is_code=False, font_name="Times New Roman", font_size=12, text_color=None):
    """Recursively processes HTML inline elements and appends formatted runs to paragraph."""
    
    def process_node(node, b, i, c):
        # 1. Node text
        if node.text:
            text = node.text
            # Replace non-breaking spaces or tabs
            text = text.replace('\xa0', ' ')
            r = p.add_run(text)
            r.font.name = "Consolas" if c else font_name
            r.font.size = Pt(font_size - 1.5 if c else font_size)
            r.font.bold = b
            r.font.italic = i
            if c:
                r.font.color.rgb = RGBColor(199, 37, 78)  # standard code highlight color
            elif text_color:
                r.font.color.rgb = text_color

        # 2. Children
        for child in node:
            cb = b or child.tag in ['strong', 'b']
            ci = i or child.tag in ['em', 'i']
            cc = c or child.tag in ['code', 'kbd', 'tt']
            if child.tag == 'br':
                p.add_run('\n')
            else:
                process_node(child, cb, ci, cc)
            
            # Child tail
            if child.tail:
                tail = child.tail.replace('\xa0', ' ')
                r = p.add_run(tail)
                r.font.name = "Consolas" if c else font_name
                r.font.size = Pt(font_size - 1.5 if c else font_size)
                r.font.bold = b
                r.font.italic = i
                if c:
                    r.font.color.rgb = RGBColor(199, 37, 78)
                elif text_color:
                    r.font.color.rgb = text_color

    process_node(elem, is_bold, is_italic, is_code)

def main():
    html_path = '/Users/anushkagattani/compiler-project/report.html'
    logo_path = '/Users/anushkagattani/compiler-project/vit_logo.jpeg'
    output_docx = '/Users/anushkagattani/compiler-project/resumec_project_report.docx'
    output_docx_alt = '/Users/anushkagattani/compiler-project/Compiler_Report_Anushka_Madhur.docx'

    tree = lxml.html.parse(html_path)
    root = tree.getroot()
    pages = root.xpath('//body/div[contains(@class, "page")]')

    doc = docx.Document()

    # Configure Default Styles
    style_normal = doc.styles['Normal']
    style_normal.font.name = 'Times New Roman'
    style_normal.font.size = Pt(12)
    style_normal.font.color.rgb = RGBColor(30, 30, 30)

    # Page Margins (A4)
    for section in doc.sections:
        section.page_width = Inches(8.27)
        section.page_height = Inches(11.69)
        section.top_margin = Inches(1.0)
        section.bottom_margin = Inches(1.0)
        section.left_margin = Inches(1.25)
        section.right_margin = Inches(1.0)
        section.different_first_page_header_footer = True

        # Header for subsequent pages
        header = section.header
        hp = header.paragraphs[0]
        hp.alignment = WD_ALIGN_PARAGRAPH.RIGHT
        hrun = hp.add_run('resumec: Domain-Specific Compiler | VIT SCOPE Project Report')
        hrun.font.name = 'Times New Roman'
        hrun.font.size = Pt(9)
        hrun.font.italic = True
        hrun.font.color.rgb = RGBColor(128, 128, 128)

        # Footer
        footer = section.footer
        fp = footer.paragraphs[0]
        fp.alignment = WD_ALIGN_PARAGRAPH.RIGHT
        frun1 = fp.add_run('Page ')
        frun1.font.name = 'Times New Roman'
        frun1.font.size = Pt(10)
        frun1.font.color.rgb = RGBColor(100, 100, 100)
        
        # Add PAGE field
        fldSimple = parse_xml(r'<w:fldSimple %s w:instr="PAGE"/>' % nsdecls('w'))
        frun1._r.append(fldSimple)

    print(f"Loaded {len(pages)} pages from HTML.")

    # -------------------------------------------------------------
    # 1. PAGE 0: TITLE / COVER PAGE
    # -------------------------------------------------------------
    print("Generating Title Page...")
    
    # Course Badge
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(10)
    p.paragraph_format.space_after = Pt(20)
    r = p.add_run("B.Tech. BCSE307P – Course Project")
    r.font.name = "Times New Roman"
    r.font.size = Pt(14)
    r.font.bold = True
    r.font.color.rgb = RGBColor(40, 40, 40)

    # Project Title
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(10)
    p.paragraph_format.space_after = Pt(20)
    p.paragraph_format.line_spacing = 1.3
    r = p.add_run("RESUMEC: A DOMAIN-SPECIFIC COMPILER AND TYPESETTING ENGINE FOR RESUME GENERATION")
    r.font.name = "Times New Roman"
    r.font.size = Pt(18)
    r.font.bold = True
    r.font.color.rgb = RGBColor(16, 44, 87)

    # Submitted text
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(10)
    p.paragraph_format.space_after = Pt(15)
    p.paragraph_format.line_spacing = 1.2
    r = p.add_run("Submitted in partial fulfillment of the requirements for the degree of\n")
    r.font.name = "Times New Roman"
    r.font.size = Pt(12)
    r = p.add_run("Bachelor of Technology\n")
    r.font.name = "Times New Roman"
    r.font.size = Pt(16)
    r.font.bold = True
    r = p.add_run("in\n")
    r.font.name = "Times New Roman"
    r.font.size = Pt(12)
    r = p.add_run("Computer Science and Engineering")
    r.font.name = "Times New Roman"
    r.font.size = Pt(15)
    r.font.bold = True

    # "by" and Author table
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(10)
    p.paragraph_format.space_after = Pt(10)
    r = p.add_run("by")
    r.font.name = "Times New Roman"
    r.font.size = Pt(12)

    # Candidate details (2 columns table, neat and centered)
    table = doc.add_table(rows=2, cols=2)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    candidates = [
        ("Reg. No. 24BCE2283", "ANUSHKA GATTANI"),
        ("Reg. No. 24BCE2343", "MADHUR MISHRA")
    ]
    for row_idx, (reg, name) in enumerate(candidates):
        row = table.rows[row_idx]
        cell_reg = row.cells[0]
        cell_reg.width = Inches(2.2)
        cp1 = cell_reg.paragraphs[0]
        cp1.alignment = WD_ALIGN_PARAGRAPH.RIGHT
        cr1 = cp1.add_run(reg)
        cr1.font.name = "Times New Roman"
        cr1.font.size = Pt(12)

        cell_name = row.cells[1]
        cell_name.width = Inches(2.8)
        cp2 = cell_name.paragraphs[0]
        cp2.alignment = WD_ALIGN_PARAGRAPH.LEFT
        cr2 = cp2.add_run("  " + name)
        cr2.font.name = "Times New Roman"
        cr2.font.size = Pt(12)
        cr2.font.bold = True

    # VIT Logo
    if os.path.exists(logo_path):
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.space_before = Pt(20)
        p.paragraph_format.space_after = Pt(15)
        run = p.add_run()
        run.add_picture(logo_path, width=Inches(1.8))

    # Guide block
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(10)
    p.paragraph_format.space_after = Pt(5)
    r = p.add_run("Under the Supervision of\n")
    r.font.name = "Times New Roman"
    r.font.size = Pt(12)
    r = p.add_run("Dr. Islabudeen M\n")
    r.font.name = "Times New Roman"
    r.font.size = Pt(14)
    r.font.bold = True
    r = p.add_run("Associate Professor Grade 1\nSchool of Computer Science and Engineering (SCOPE)\nVellore Institute of Technology, Vellore")
    r.font.name = "Times New Roman"
    r.font.size = Pt(12)

    # Date
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(15)
    p.paragraph_format.space_after = Pt(0)
    r = p.add_run("October 2026")
    r.font.name = "Times New Roman"
    r.font.size = Pt(12)
    r.font.bold = True

    # Page Break after Cover Page
    doc.add_page_break()

    # -------------------------------------------------------------
    # 2. PROCESS REMAINING PAGES (Pages 1 to 16)
    # -------------------------------------------------------------
    for page_idx, page in enumerate(pages[1:], 1):
        print(f"Processing Page {page_idx}...")

        # Process children of this page div
        for child in page.iterchildren():
            tag = child.tag
            cls = child.get("class", "")

            # A. Centered Heading (Declaration, Acknowledgements, Table of Contents, Lists)
            if "centered-heading" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                p.paragraph_format.space_before = Pt(18)
                p.paragraph_format.space_after = Pt(14)
                text = "".join(child.itertext()).strip().upper()
                r = p.add_run(text)
                r.font.name = "Times New Roman"
                r.font.size = Pt(14)
                r.font.bold = True
                r.font.color.rgb = RGBColor(16, 44, 87)

            # B. Abstract Heading
            elif "abstract-heading" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                p.paragraph_format.space_before = Pt(18)
                p.paragraph_format.space_after = Pt(16)
                r = p.add_run("ABSTRACT")
                r.font.name = "Times New Roman"
                r.font.size = Pt(16)
                r.font.bold = True
                r.font.color.rgb = RGBColor(16, 44, 87)

            # C. Chapter Number + Section Heading
            elif "chapter-number" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                p.paragraph_format.space_before = Pt(20)
                p.paragraph_format.space_after = Pt(4)
                text = "".join(child.itertext()).strip().upper()
                r = p.add_run(text)
                r.font.name = "Times New Roman"
                r.font.size = Pt(16)
                r.font.bold = True
                r.font.color.rgb = RGBColor(16, 44, 87)

            elif "section-heading" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                p.paragraph_format.space_before = Pt(0)
                p.paragraph_format.space_after = Pt(14)
                text = "".join(child.itertext()).strip().upper()
                r = p.add_run(text)
                r.font.name = "Times New Roman"
                r.font.size = Pt(16)
                r.font.bold = True
                r.font.color.rgb = RGBColor(16, 44, 87)

            # D. Sub-headings (1.1, 1.2, etc.)
            elif "sub-heading" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.LEFT
                p.paragraph_format.space_before = Pt(14)
                p.paragraph_format.space_after = Pt(5)
                text = "".join(child.itertext()).strip()
                r = p.add_run(text)
                r.font.name = "Times New Roman"
                r.font.size = Pt(13.5)
                r.font.bold = True
                r.font.color.rgb = RGBColor(20, 50, 90)

            # E. Sub-sub-headings (3.1.1, etc.)
            elif "sub-sub-heading" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.LEFT
                p.paragraph_format.space_before = Pt(10)
                p.paragraph_format.space_after = Pt(3)
                text = "".join(child.itertext()).strip()
                r = p.add_run(text)
                r.font.name = "Times New Roman"
                r.font.size = Pt(12)
                r.font.bold = True
                r.font.color.rgb = RGBColor(30, 30, 30)

            # F. Table Captions (above table)
            elif "table-caption" in cls:
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.LEFT
                p.paragraph_format.space_before = Pt(12)
                p.paragraph_format.space_after = Pt(4)
                text = "".join(child.itertext()).strip()
                r = p.add_run(text)
                r.font.name = "Times New Roman"
                r.font.size = Pt(10)
                r.font.bold = True
                r.font.color.rgb = RGBColor(50, 50, 50)

            # G. Standard Paragraphs
            elif tag == "p":
                p = doc.add_paragraph()
                p.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
                p.paragraph_format.space_before = Pt(2)
                p.paragraph_format.space_after = Pt(6)
                p.paragraph_format.line_spacing = 1.15
                add_runs_from_element(p, child, font_name="Times New Roman", font_size=12)

            # H. Figures with Diagram Boxes
            elif tag == "figure":
                # Find pre / diagram inside
                pre_nodes = child.xpath('.//pre')
                for pre in pre_nodes:
                    text = "".join(pre.itertext())
                    tbl = doc.add_table(rows=1, cols=1)
                    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
                    cell = tbl.cell(0, 0)
                    cell.width = Inches(5.8)
                    set_cell_shading(cell, "F6F8FA")
                    set_cell_margins(cell, top=140, bottom=140, left=180, right=180)
                    cp = cell.paragraphs[0]
                    cp.paragraph_format.space_before = Pt(0)
                    cp.paragraph_format.space_after = Pt(0)
                    cp.paragraph_format.line_spacing = 1.05
                    r = cp.add_run(text.strip('\n'))
                    r.font.name = "Consolas"
                    r.font.size = Pt(8.5)
                    r.font.color.rgb = RGBColor(30, 30, 30)
                    set_table_borders(tbl, color="D0D7DE", sz="6", val="single")

                # Caption
                cap_nodes = child.xpath('.//figcaption')
                for cap in cap_nodes:
                    p = doc.add_paragraph()
                    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                    p.paragraph_format.space_before = Pt(5)
                    p.paragraph_format.space_after = Pt(12)
                    text = "".join(cap.itertext()).strip()
                    r = p.add_run(text)
                    r.font.name = "Times New Roman"
                    r.font.size = Pt(10)
                    r.font.bold = True
                    r.font.color.rgb = RGBColor(60, 60, 60)

            # I. Preformatted Code Blocks
            elif tag == "pre":
                text = "".join(child.itertext())
                tbl = doc.add_table(rows=1, cols=1)
                tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
                cell = tbl.cell(0, 0)
                cell.width = Inches(5.8)
                set_cell_shading(cell, "F8F9FA")
                set_cell_margins(cell, top=140, bottom=140, left=180, right=180)
                cp = cell.paragraphs[0]
                cp.paragraph_format.space_before = Pt(0)
                cp.paragraph_format.space_after = Pt(0)
                cp.paragraph_format.line_spacing = 1.05
                r = cp.add_run(text.strip('\n'))
                r.font.name = "Consolas"
                r.font.size = Pt(8.5)
                r.font.color.rgb = RGBColor(36, 41, 47)
                set_table_borders(tbl, color="E1E4E8", sz="6", val="single")
                # Add spacing after code block
                sp = doc.add_paragraph()
                sp.paragraph_format.space_before = Pt(0)
                sp.paragraph_format.space_after = Pt(4)

            # J. Signature Blocks
            elif "sig-block" in cls:
                for sp_node in child.xpath('.//p[contains(@class, "sig-line")]'):
                    p = doc.add_paragraph()
                    p.paragraph_format.space_before = Pt(10)
                    p.paragraph_format.space_after = Pt(4)
                    add_runs_from_element(p, sp_node, font_name="Times New Roman", font_size=12)

                # Signature tables
                sig_tables = child.xpath('.//table')
                for st in sig_tables:
                    rows = st.xpath('.//tr')
                    tbl = doc.add_table(rows=len(rows), cols=2)
                    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
                    for r_idx, row in enumerate(rows):
                        cells = row.xpath('./td | ./th')
                        for c_idx, c_node in enumerate(cells[:2]):
                            cell = tbl.cell(r_idx, c_idx)
                            cell.width = Inches(2.9)
                            cp = cell.paragraphs[0]
                            cp.paragraph_format.space_before = Pt(15)
                            cp.paragraph_format.space_after = Pt(6)
                            add_runs_from_element(cp, c_node, font_name="Times New Roman", font_size=12)

            # K. Tables (TOC, Figures, Tables, Abbreviations, Data Tables)
            elif tag == "table":
                rows = child.xpath('.//tr')
                if not rows:
                    continue

                # Determine column count
                first_row_cells = rows[0].xpath('./td | ./th')
                num_cols = len(first_row_cells)
                if num_cols == 0:
                    continue

                tbl = doc.add_table(rows=len(rows), cols=num_cols)
                tbl.alignment = WD_TABLE_ALIGNMENT.CENTER

                is_toc_or_list = any(c in cls for c in ["toc-table", "fig-table", "abbrev-table"])
                header_bg = "EBF1F5" if not is_toc_or_list else "F0F4F8"

                for r_idx, row in enumerate(rows):
                    is_head = bool(r_idx == 0 and (len(row.xpath('./th')) > 0 or is_toc_or_list))
                    cells = row.xpath('./td | ./th')

                    for c_idx in range(min(num_cols, len(cells))):
                        c_node = cells[c_idx]
                        cell = tbl.cell(r_idx, c_idx)
                        set_cell_margins(cell, top=80, bottom=80, left=100, right=100)

                        if is_head:
                            set_cell_shading(cell, header_bg)

                        cp = cell.paragraphs[0]
                        cp.paragraph_format.space_before = Pt(1)
                        cp.paragraph_format.space_after = Pt(1)
                        cp.paragraph_format.line_spacing = 1.1

                        # Column alignments
                        if num_cols == 3 and c_idx == 0:
                            cp.alignment = WD_ALIGN_PARAGRAPH.CENTER
                        elif num_cols == 3 and c_idx == 2:
                            cp.alignment = WD_ALIGN_PARAGRAPH.CENTER
                        elif is_head:
                            cp.alignment = WD_ALIGN_PARAGRAPH.LEFT
                        else:
                            cp.alignment = WD_ALIGN_PARAGRAPH.LEFT

                        add_runs_from_element(
                            cp,
                            c_node,
                            is_bold=is_head,
                            font_name="Times New Roman",
                            font_size=10.5 if not is_head else 11
                        )

                set_table_borders(tbl, color="CCCCCC", sz="4", val="single")

                # Spacing after table
                sp = doc.add_paragraph()
                sp.paragraph_format.space_before = Pt(0)
                sp.paragraph_format.space_after = Pt(6)

            # L. Unordered Lists (ul)
            elif tag == "ul":
                for li in child.xpath('./li'):
                    p = doc.add_paragraph(style='List Bullet')
                    p.paragraph_format.space_before = Pt(1)
                    p.paragraph_format.space_after = Pt(3)
                    p.paragraph_format.line_spacing = 1.15
                    add_runs_from_element(p, li, font_name="Times New Roman", font_size=12)

            # M. Ordered Lists (ol) / References
            elif tag == "ol":
                for li_idx, li in enumerate(child.xpath('./li'), 1):
                    p = doc.add_paragraph()
                    p.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
                    p.paragraph_format.space_before = Pt(3)
                    p.paragraph_format.space_after = Pt(6)
                    p.paragraph_format.line_spacing = 1.15
                    p.paragraph_format.left_indent = Inches(0.3)
                    
                    r_num = p.add_run(f"[{li_idx}]  ")
                    r_num.font.name = "Times New Roman"
                    r_num.font.size = Pt(11)
                    r_num.font.bold = True
                    
                    add_runs_from_element(p, li, font_name="Times New Roman", font_size=11)

            # N. Horizontal Rule
            elif tag == "hr":
                # We skip or add subtle break
                pass

        # Add page break between pages (except last page)
        if page_idx < len(pages) - 1:
            doc.add_page_break()

    # Save to both file locations
    print(f"Saving to {output_docx}...")
    doc.save(output_docx)
    print(f"Saving to {output_docx_alt}...")
    doc.save(output_docx_alt)
    print("Done! Word documents successfully generated.")

if __name__ == "__main__":
    main()
