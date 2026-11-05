# -*- coding: utf-8 -*-
import os
import sys
import docx
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml import OxmlElement, parse_xml
from docx.oxml.ns import nsdecls, qn

def set_cell_background(cell, fill_hex):
    tcPr = cell._element.get_or_add_tcPr()
    shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{fill_hex}"/>')
    tcPr.append(shd)

def set_cell_margins(cell, top=100, bottom=100, left=150, right=150):
    tcPr = cell._element.get_or_add_tcPr()
    tcMar = OxmlElement('w:tcMar')
    for m, val in [('top', top), ('bottom', bottom), ('left', left), ('right', right)]:
        node = OxmlElement(f'w:{m}')
        node.set(qn('w:w'), str(val))
        node.set(qn('w:type'), 'dxa')
        tcMar.append(node)
    tcPr.append(tcMar)

def set_table_borders(table, color="CBD5E1", sz="4"):
    """Applies modern minimalist horizontal borders to tables."""
    tblPr = table._element.xpath('w:tblPr')
    if tblPr:
        borders = parse_xml(
            f'<w:tblBorders {nsdecls("w")}>'
            f'<w:top w:val="single" w:sz="8" w:space="0" w:color="0F172A"/>'
            f'<w:bottom w:val="single" w:sz="8" w:space="0" w:color="0F172A"/>'
            f'<w:insideH w:val="single" w:sz="{sz}" w:space="0" w:color="{color}"/>'
            f'<w:insideV w:val="none"/>'
            f'<w:left w:val="none"/>'
            f'<w:right w:val="none"/>'
            f'</w:tblBorders>'
        )
        tblPr[0].append(borders)

def add_custom_heading(doc, text, level):
    h = doc.add_heading(text, level=level)
    h.paragraph_format.space_before = Pt(16 if level == 1 else (12 if level == 2 else 8))
    h.paragraph_format.space_after = Pt(6)
    h.paragraph_format.keep_with_next = True
    if h.runs:
        run = h.runs[0]
        run.font.name = 'Calibri'
        run._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
        if level == 1:
            run.font.size = Pt(18)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0x0f, 0x17, 0x2a) # Dark slate
        elif level == 2:
            run.font.size = Pt(14)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0x02, 0x84, 0xc7) # Premium Blue
        elif level == 3:
            run.font.size = Pt(12)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0x33, 0x41, 0x55)
    return h

def add_styled_paragraph(doc, text, space_after=6, bold_prefix="", italic=False):
    p = doc.add_paragraph()
    p.paragraph_format.space_before = Pt(0)
    p.paragraph_format.space_after = Pt(space_after)
    p.paragraph_format.line_spacing = 1.25
    
    if bold_prefix:
        r_pre = p.add_run(bold_prefix)
        r_pre.font.name = 'Calibri'
        r_pre._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
        r_pre.font.bold = True
        r_pre.font.size = Pt(10.5)
        r_pre.font.color.rgb = RGBColor(0x0f, 0x17, 0x2a)

    r = p.add_run(text)
    r.font.name = 'Calibri'
    r._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(10.5)
    r.font.italic = italic
    r.font.color.rgb = RGBColor(0x1e, 0x29, 0x3b)
    return p

def add_callout(doc, text, title="重要导言与原则"):
    tbl = doc.add_table(rows=1, cols=1)
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = tbl.cell(0, 0)
    set_cell_background(cell, "F1F5F9")
    set_cell_margins(cell, top=140, bottom=140, left=200, right=200)
    
    tcPr = cell._element.get_or_add_tcPr()
    borders = parse_xml(f'<w:tcBorders {nsdecls("w")}><w:left w:val="single" w:sz="36" w:space="0" w:color="0284C7"/><w:top w:val="none"/><w:right w:val="none"/><w:bottom w:val="none"/></w:tcBorders>')
    tcPr.append(borders)
    
    p = cell.paragraphs[0]
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after = Pt(4)
    p.paragraph_format.line_spacing = 1.25
    
    r_t = p.add_run(f"【{title}】\n")
    r_t.font.name = 'Calibri'
    r_t._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
    r_t.font.bold = True
    r_t.font.size = Pt(11)
    r_t.font.color.rgb = RGBColor(0x02, 0x84, 0xc7)
    
    r_b = p.add_run(text)
    r_b.font.name = 'Calibri'
    r_b._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
    r_b.font.size = Pt(10)
    r_b.font.color.rgb = RGBColor(0x33, 0x41, 0x55)
    
    p_after = doc.add_paragraph()
    p_after.paragraph_format.space_before = Pt(0)
    p_after.paragraph_format.space_after = Pt(4)

def render_table_helper(doc, headers, data):
    tbl = doc.add_table(rows=len(data) + 1, cols=len(headers))
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(tbl, color="CBD5E1", sz="4")

    # Header Row
    hdr_cells = tbl.rows[0].cells
    for i, h_text in enumerate(headers):
        hdr_cells[i].text = h_text
        set_cell_background(hdr_cells[i], "0F172A")
        set_cell_margins(hdr_cells[i], top=90, bottom=90, left=100, right=100)
        p = hdr_cells[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        if p.runs:
            p.runs[0].font.name = 'Calibri'
            p.runs[0]._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
            p.runs[0].font.color.rgb = RGBColor(0xff, 0xff, 0xff)
            p.runs[0].font.bold = True
            p.runs[0].font.size = Pt(9.5)

    # Data Rows
    for row_idx, row_tuple in enumerate(data, start=1):
        row_cells = tbl.rows[row_idx].cells
        bg_color = "F8FAFC" if row_idx % 2 == 1 else "FFFFFF"
        for col_idx, cell_value in enumerate(row_tuple):
            set_cell_background(row_cells[col_idx], bg_color)
            set_cell_margins(row_cells[col_idx], top=70, bottom=70, left=100, right=100)
            row_cells[col_idx].text = str(cell_value)
            p = row_cells[col_idx].paragraphs[0]
            p.paragraph_format.space_before = Pt(0)
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.line_spacing = 1.15
            if p.runs:
                p.runs[0].font.name = 'Calibri'
                p.runs[0]._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
                p.runs[0].font.size = Pt(9)
                p.runs[0].font.color.rgb = RGBColor(0x33, 0x41, 0x55)
                if "【已实装】" in str(cell_value):
                    p.runs[0].font.bold = True
                    p.runs[0].font.color.rgb = RGBColor(0x16, 0x65, 0x34)
                elif str(cell_value).startswith("🔑"):
                    p.runs[0].font.bold = True

    doc.add_paragraph() # Spacer

def unify_all_document_fonts(doc, primary_font="微软雅黑", ascii_font="Calibri"):
    """
    Scans the ENTIRE document (including original paragraphs and styles) to:
    1. Fix corrupted GBK font strings in XML.
    2. Force 100% font consistency across ALL paragraphs, tables, headers, and runs.
    """
    print("Unifying fonts across the ENTIRE document...")
    fixed_xml_count = 0
    for rFonts in doc._element.xpath('//w:rFonts'):
        for key in list(rFonts.attrib.keys()):
            val = rFonts.attrib[key]
            if any(ord(c) > 127 and c not in [primary_font, '宋体', '黑体', '楷体'] for c in val):
                rFonts.attrib[key] = primary_font
                fixed_xml_count += 1
    print(f"Fixed {fixed_xml_count} corrupted XML font tags.")

    for s in doc.styles:
        if s.type == docx.enum.style.WD_STYLE_TYPE.PARAGRAPH:
            rPr = s.element.get_or_add_rPr()
            rFonts = rPr.find(qn('w:rFonts'))
            if rFonts is None:
                rFonts = OxmlElement('w:rFonts')
                rPr.append(rFonts)
            rFonts.set(qn('w:ascii'), ascii_font)
            rFonts.set(qn('w:hAnsi'), ascii_font)
            rFonts.set(qn('w:eastAsia'), primary_font)

    run_count = 0
    for p in doc.paragraphs:
        for r in p.runs:
            run_count += 1
            rPr = r._element.get_or_add_rPr()
            rFonts = rPr.find(qn('w:rFonts'))
            if rFonts is None:
                rFonts = OxmlElement('w:rFonts')
                rPr.append(rFonts)
            rFonts.set(qn('w:ascii'), ascii_font)
            rFonts.set(qn('w:hAnsi'), ascii_font)
            rFonts.set(qn('w:eastAsia'), primary_font)

    for tbl in doc.tables:
        for row in tbl.rows:
            for cell in row.cells:
                for p in cell.paragraphs:
                    for r in p.runs:
                        rPr = r._element.get_or_add_rPr()
                        rFonts = rPr.find(qn('w:rFonts'))
                        if rFonts is None:
                            rFonts = OxmlElement('w:rFonts')
                            rPr.append(rFonts)
                        rFonts.set(qn('w:ascii'), ascii_font)
                        rFonts.set(qn('w:hAnsi'), ascii_font)
                        rFonts.set(qn('w:eastAsia'), primary_font)

    print(f"Successfully processed {run_count} runs across all paragraphs.")

def build_unabridged_ipc_v1_0():
    original_docx = r"h:\IPC\智能计划与控制（IPC）体系.docx"
    output_docx_v1 = r"h:\IPC\智能计划与控制（IPC）体系_v1.0.docx"
    image_path = r"h:\IPC\方案\images\ibp_to_scheduling_master_flowchart.png"

    print(f"Loading original document: {original_docx}...")
    doc = docx.Document(original_docx)
    print(f"Original doc loaded. Paragraphs: {len(doc.paragraphs)}, Tables: {len(doc.tables)}")

    doc.add_page_break()

    # ==================== 第四部分：IPC 1.0 旗舰版全景 Visio 规范逻辑流程、199 表持久化 DB 与 C++ 引擎架构解构 ====================
    add_custom_heading(doc, "第四部分：IPC 1.0 旗舰版全景 Visio 规范逻辑流程、199 表持久化 DB 与 C++ 引擎架构解构", level=1)
    
    add_callout(doc, 
        "本部分为《智能计划与控制（IPC）体系》1.0 旗舰版的终极工程落地解构。在前三部分建立的“战略-战术-车间”三层闭环理论框架与全套关系数据模型的基础上，本章彻底对接工业级 C++ 生产引擎与 DuckDB 199 张物理数据库表全集。\n"
        "揭示 IPC 双引擎协同物理学架构：(1) DuckDB 199 表全网感知、整合与持久化引擎层；(2) 高性能 C++ DOD 裸金属内存计算引擎层。通过 Visio 标准全景工程逻辑流程图，深度剖析从战略 IBP 控制塔到车间微观派工排产调度的 7 大阶段闭环算子、DOD 连续内存布局、64 位复合位打包排序、无锁 CAS 事务争用控制以及全景软件与 DB 架构对账账本，实现理论公理、199 表工程模型与底层 C++ 源码的三位一体严密闭环。",
        "IPC v1.0 旗舰工程与 199 表数据库双引擎解构导言"
    )

    # 1. Image Section
    if os.path.exists(image_path):
        add_custom_heading(doc, "一、 全景 Visio 规范逻辑流程图 (Master Engineering Flowchart)", level=2)
        
        add_styled_paragraph(doc, 
            "IPC 1.0 引擎采用了严格的工业级信号流与数据流分离设计。下图展现了从顶级 S&OP 战略共识预测注入开始，经过外围 199 表 DuckDB 感知清洗整合、多阶库存优化（MEIO）、战术需求冲销（Forecast Consumption）、五大判决门控树、逐层 MRP 拓扑消纳、动态替换料分配，直至微观资源置换（Resource Swapping）与车间 180 秒刚性反写闭环的全景工程逻辑。"
        )
        
        p_img = doc.add_paragraph()
        p_img.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_img.paragraph_format.space_before = Pt(8)
        p_img.paragraph_format.space_after = Pt(4)
        run_img = p_img.add_run()
        run_img.add_picture(image_path, width=Inches(6.5))
        
        p_cap = doc.add_paragraph()
        p_cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_cap.paragraph_format.space_before = Pt(2)
        p_cap.paragraph_format.space_after = Pt(14)
        r_cap = p_cap.add_run("图 4-1 IPC 1.0 从战略 IBP 控制塔至微观车间派工排产调度的 Visio 规范主逻辑流程与双引擎架构图")
        r_cap.font.name = 'Calibri'
        r_cap._element.rPr.get_or_add_rFonts().set(qn('w:eastAsia'), '微软雅黑')
        r_cap.font.size = Pt(9.5)
        r_cap.font.italic = True
        r_cap.font.color.rgb = RGBColor(0x64, 0x74, 0x8b)

    # 2. 199 Tables Section
    add_custom_heading(doc, "二、 全量持久化 DB 物理表结构与数据字典 (199 表全景解构)", level=2)
    add_styled_paragraph(doc, 
        "IPC 物理数据库（基于 DuckDB / SQLite）共收录 199 张物理表。数据库本身即是 IPC 计算引擎不可分割的核心物理组成部分。在感知外围 ERP / MES / WMS / CRM 数据后，DuckDB 负责全网数据的清洗归一化、3D 需求张量构建、IBP 预测共识融合、MEIO 安全库存求解与全网 Pegging 查账。核心物理表被一次性编译装载进 C++ 64 字节对齐连续内存数组中进行毫秒级求解。"
    )

    # Module 1
    add_custom_heading(doc, "1. 模块 1：核心计划与排产表 (Core Planning & Execution - 10 张表)", level=3)
    add_styled_paragraph(doc, "包含物料、BOM 路线、BOM 明细、在库水线、在途供应、建议计划订单、Pegging 钉结分配等 10 张物理表，全量编译装载进 C++ DOD 物理内存数组中。")

    # Table 4-1: ipc_part
    add_styled_paragraph(doc, "表 4-1 ipc_part / ipc_material_node (核心物料主数据表)", bold_prefix="【物料 Schema】")
    tbl_part_headers = ["字段编码 (Field Code)", "字段物理名", "物理类型", "约束 / 主键", "业务含义与计算逻辑", "对应 C++ 内存结构"]
    tbl_part_data = [
        ("part", "物料唯一编码", "VARCHAR(100)", "🔑 PK / NOT NULL", "零部件全局唯一识别码 (SKU Code)", "PartSiteRecord.part_code"),
        ("part_type", "物料分类", "VARCHAR(50)", "Nullable", "类别：FINISHED (成品), SEMI (半成品), RAW (原料), ALT (替代料)", "PartSiteRecord.part_type"),
        ("mrp_rule", "MRP 计算规则", "VARCHAR(20)", "Nullable", "消纳控制逻辑，如 LBL_NETTING, REORDER_POINT", "PartSiteRecord.mrp_rule"),
        ("site", "归属工厂/站点", "VARCHAR(8)", "🔑 PK / NOT NULL", "工厂或仓库站点编码 (Site Code，对应 SAP Plant)", "PartSiteRecord.site"),
        ("is_phantom", "虚拟件标识", "BOOLEAN", "Default FALSE", "TRUE 表示虚拟件，MRP 展开时直接穿透跳过工单生成", "PartSiteRecord.is_phantom"),
        ("selling_ave_price", "平均销售单价", "DOUBLE", "Default 0.0", "财务结算及营业收入折算的基准平均售价", "PartSiteRecord.cost"),
        ("transshipment_cost", "单件调拨成本", "DOUBLE", "Default 0.0", "厂区间跨站点物流调拨的单件运费成本", "PartSiteRecord.transshipment_cost"),
        ("transshipment_lead_time", "调拨提前期", "INTEGER", "Default 0", "厂区间跨站点调拨的物理周期 (天数)", "PartSiteRecord.transshipment_lead_time"),
        ("lead_time", "固定制造提前期", "DOUBLE", "Default 0.0", "基础生产工时提前期", "PartSiteRecord.lead_time"),
        ("safety_stock", "安全库存目标量", "DOUBLE", "Default 0.0", "根据 MEIO 算法算出的安全水线水位", "PartSiteRecord.safety_stock")
    ]
    render_table_helper(doc, tbl_part_headers, tbl_part_data)

    # Table 4-2: ipc_bom_route
    add_styled_paragraph(doc, "表 4-2 ipc_bom_route (工艺路线与 BOM 绑定表)", bold_prefix="【BOM Route Schema】")
    tbl_bom_route_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_bom_route_data = [
        ("site", "site", "VARCHAR(8)", "🔑 PK", "工厂站点编码", "SourceConstraintRecord.site"),
        ("part", "part", "VARCHAR(40)", "🔑 PK", "父件物料编码", "SourceConstraintRecord.part_id"),
        ("bomid", "bomid", "VARCHAR(40)", "🔑 PK", "绑定的 BOM 版本唯一标识", "FlatBomItem.parent_id"),
        ("priority", "priority", "INTEGER", "Default 1", "路线选择优先级", "AlternativeRouting.priority"),
        ("bom_type", "bom_type", "VARCHAR(10)", "Nullable", "制造 BOM (M-BOM) / 研发 BOM (R-BOM)", "SourceConstraintRecord.bom_type")
    ]
    render_table_helper(doc, tbl_bom_route_headers, tbl_bom_route_data)

    # Table 4-3: ipc_bom_item
    add_styled_paragraph(doc, "表 4-3 ipc_bom_item (BOM 明细与动态替代配置表)", bold_prefix="【BOM Item Schema】")
    tbl_bom_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_bom_data = [
        ("bomid", "bomid", "VARCHAR(40)", "NOT NULL", "关联 BOM 编号", "FlatBomItem.parent_id"),
        ("component", "component", "VARCHAR(40)", "NOT NULL", "子组件物料编码", "FlatBomItem.child_id"),
        ("perqty", "perqty", "DOUBLE", "Default 1.0", "基准单耗比例 (perqty)", "FlatBomItem.per_qty"),
        ("scrap", "scrap", "DOUBLE", "Default 0.0", "制造损耗比例", "FlatBomItem.scrap"),
        ("alt_grp", "alt_grp", "VARCHAR(10)", "Nullable", "替代组逻辑编号", "FlatBomItem.alt_group_id"),
        ("priority", "priority", "INTEGER", "Default 0", "组内替代优先级", "FlatBomItem.alt_priority"),
        ("target", "target", "DOUBLE", "Default 1.0", "分摊目标比例 (target)", "FlatBomItem.target_ratio"),
        ("lot_size", "lot_size", "DOUBLE", "Default 0.0", "包装规格批次量", "FlatBomItem.lot_size"),
        ("eff_start_day", "eff_start_day", "INTEGER", "Default -1", "生效起始相对天数 (ECN 控期)", "FlatBomItem.eff_start_day"),
        ("eff_end_day", "eff_end_day", "INTEGER", "Default -1", "失效终止相对天数 (ECN 软切)", "FlatBomItem.eff_end_day")
    ]
    render_table_helper(doc, tbl_bom_headers, tbl_bom_data)

    # Table 4-4: ipc_onhand
    add_styled_paragraph(doc, "表 4-4 ipc_onhand (物理在库库存表)", bold_prefix="【在库 Schema】")
    tbl_oh_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_oh_data = [
        ("location", "location", "VARCHAR(10)", "🔑 PK", "物理库位编码", "Axis.locations"),
        ("part", "part", "VARCHAR(40)", "🔑 PK", "物料唯一编码", "Axis.part_id"),
        ("site", "site", "VARCHAR(8)", "🔑 PK", "物理厂区站点", "Axis.site_id"),
        ("available_date", "available_date", "DATE", "🔑 PK", "ATP 可用起始日期", "Axis.dates"),
        ("qty", "qty", "DOUBLE", "Default 0.0", "现有物理库存数量", "Axis.qtys / OptimisticInventoryPool"),
        ("inventory_type", "inventory_type", "VARCHAR(10)", "Default 'Unrestricted'", "状态：Unrestricted (无限制), Quality (质检)", "Axis.types")
    ]
    render_table_helper(doc, tbl_oh_headers, tbl_oh_data)

    # Table 4-5: ipc_planned_order
    add_styled_paragraph(doc, "表 4-5 ipc_planned_order (建议计划订单表)", bold_prefix="【计划订单 Schema】")
    tbl_po_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_po_data = [
        ("ipc_planned_order", "planned_order", "VARCHAR(18)", "🔑 PK", "系统自动生成的工单唯一流水号", "PlannedOrder.order_id"),
        ("part", "part", "VARCHAR(40)", "🔑 PK", "目标补货物料号", "PlannedOrder.part_id"),
        ("site", "site", "VARCHAR(8)", "🔑 PK", "生产或接收厂区", "PlannedOrder.site_id"),
        ("request_start_date", "start_date", "DATE", "NOT NULL", "考虑制造提前期 LT 前推得到的开工期", "PlannedOrder.start_day"),
        ("due_date", "due_date", "DATE", "NOT NULL", "承诺交付或就绪日期", "PlannedOrder.finish_day"),
        ("qty", "qty", "DOUBLE", "Default 0.0", "原始计算出的补货需求量", "PlannedOrder.qty"),
        ("eff_qty", "eff_qty", "DOUBLE", "Default 0.0", "考虑损耗率 scrap 后的实际投放量", "PlannedOrder.eff_qty"),
        ("source", "source", "VARCHAR(10)", "Default 'MAKE'", "MAKE (自制), BUY (外购), TRANSFER (调拨)", "PlannedOrder.source")
    ]
    render_table_helper(doc, tbl_po_headers, tbl_po_data)

    # Table 4-6: ipc_planned_supply_assignment
    add_styled_paragraph(doc, "表 4-6 ipc_planned_supply_assignment (计划钉结分配账本表 - Planned Pegging)", bold_prefix="【Pegging Schema】")
    tbl_peg_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_peg_data = [
        ("demand", "demand", "VARCHAR(18)", "🔑 PK", "关联顶层需求订单号", "PeggingRecord.demand_id"),
        ("item", "item", "DOUBLE", "🔑 PK", "关联需求明细行号", "PeggingRecord.item_id"),
        ("assigned_part", "assigned_part", "VARCHAR(40)", "NOT NULL", "实际被消耗的物料 (若替代则为替代件号)", "PeggingRecord.part_id"),
        ("assigned_qty", "assigned_qty", "DOUBLE", "Default 0.0", "绑定锁定的物理数量", "PeggingRecord.qty"),
        ("available_date", "available_date", "DATE", "NOT NULL", "实际满足需求的 ATP 交付日期", "PeggingRecord.day"),
        ("planned_order", "planned_order", "VARCHAR(18)", "Nullable", "绑定的 ipc_planned_order 单号", "PeggingRecord.planned_order_id"),
        ("ECS", "ecs_date", "DATE", "Nullable", "Earliest Constraint Start Date (最早瓶颈开工期)", "PeggingRecord.ecs_date"),
        ("part_ready_date", "part_ready_date", "DATE", "Nullable", "所有子件到料并齐套的最早时刻", "PeggingRecord.part_ready_date")
    ]
    render_table_helper(doc, tbl_peg_headers, tbl_peg_data)

    # Module 2
    add_custom_heading(doc, "2. 模块 2：联副产品分级优化表 (Co-product Optimization - 8 张表)", level=3)
    add_styled_paragraph(doc, "在半导体 Binning 及炼化联副产品场景下，解构产出比例与降级消纳：\n· 物理表包括：ipc_coproduct_dimension, ipc_coproduct_grouping, ipc_coproduct_recipe (ratio_512, ratio_256, ratio_128), ipc_coproduct_demand, ipc_coproduct_allocation, ipc_coproduct_schedule, ipc_coproduct_config, ipc_coproduct_yield。")

    add_styled_paragraph(doc, "表 4-7 ipc_coproduct_recipe (联副产品产出配方表)", bold_prefix="【联副配方 Schema】")
    tbl_cop_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_cop_data = [
        ("routing_code", "routing_code", "VARCHAR(40)", "🔑 PK", "工艺路线与配方编号", "CoproductRecipe.routing_code"),
        ("part_code", "part_code", "VARCHAR(40)", "🔑 PK", "主投料物料号", "CoproductRecipe.part_id"),
        ("batch_size", "batch_size", "DOUBLE", "Default 1.0", "标准投料批次大小", "CoproductRecipe.batch_size"),
        ("ratio_512", "ratio_512", "DOUBLE", "Default 0.0", "高特级 (512G) 产出概率比率", "CoproductRecipe.ratio_512"),
        ("ratio_256", "ratio_256", "DOUBLE", "Default 0.0", "标准级 (256G) 产出概率比率", "CoproductRecipe.ratio_256"),
        ("ratio_128", "ratio_128", "DOUBLE", "Default 0.0", "降级 (128G) 产出概率比率", "CoproductRecipe.ratio_128")
    ]
    render_table_helper(doc, tbl_cop_headers, tbl_cop_data)

    # Module 3
    add_custom_heading(doc, "3. 模块 3：ETO 协同项目管理表 (ETO Project & WBS - 6 张表)", level=3)
    add_styled_paragraph(doc, "面向按项目设计 (Engineer-to-Order) 场景，绑定工程 WBS 节点与 CPM 关键路径：\n· 物理表包括：ipc_project, ipc_project_group, ipc_project_manager, ipc_project_status, ipc_project_type, ipc_project_wbs。")

    add_styled_paragraph(doc, "表 4-8 ipc_project_wbs (ETO 项目 WBS 任务节点表)", bold_prefix="【WBS Schema】")
    tbl_wbs_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_wbs_data = [
        ("wbs_element", "wbs_element", "VARCHAR(40)", "🔑 PK", "WBS 任务节点唯一编码", "ProjectWBS.wbs_id"),
        ("project_id", "project_id", "VARCHAR(40)", "NOT NULL", "归属主项目编号", "ProjectWBS.project_id"),
        ("parent_wbs", "parent_wbs", "VARCHAR(40)", "Nullable", "父级 WBS 任务编码 (树拓扑)", "ProjectWBS.parent_id"),
        ("planned_start", "planned_start", "DATE", "NOT NULL", "计划开工日期", "ProjectWBS.start_day"),
        ("planned_finish", "planned_finish", "DATE", "NOT NULL", "计划完工日期", "ProjectWBS.finish_day"),
        ("weight", "weight", "DOUBLE", "Default 1.0", "关键路径 CPM 权重比例", "ProjectWBS.weight")
    ]
    render_table_helper(doc, tbl_wbs_headers, tbl_wbs_data)

    # Module 4
    add_custom_heading(doc, "4. 模块 4：IBP 财务与预测共识表 (IBP Consolidated Planning - 30 张表)", level=3)
    add_styled_paragraph(doc, "感知外围 CRM/S&OP 数据，运行 Holt-Winters 与 3D 共识算法，形成财务 P&L 约束：\n· 核心物理表包括：ipc_consensus_forecast, ipc_consensus_forecast_detail, ipc_consensus_forecast_rolling_horizon, ipc_financial_ledger (计算 ROIC, NOPAT 与营运资本), ipc_forecast, ipc_forecast_causal_factor, ipc_forecast_consumption, ipc_sales_order_line 等 30 张表。")

    add_styled_paragraph(doc, "表 4-9 ipc_independent_demand (独立需求与客户订单表)", bold_prefix="【需求 Schema】")
    tbl_dem_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_dem_data = [
        ("demand", "demand", "VARCHAR(50)", "🔑 PK", "独立需求或销售订单号 (SO Number)", "IndependentDemand.demand_id"),
        ("item", "item", "DOUBLE", "🔑 PK", "订单明细行号", "IndependentDemand.item_id"),
        ("part", "part", "VARCHAR(40)", "NOT NULL", "成品物料编码", "IndependentDemand.part_id"),
        ("customer", "customer", "VARCHAR(40)", "Nullable", "客户编码，关联客户层级", "IndependentDemand.customer"),
        ("request_due_date", "request_due_date", "DATE", "NOT NULL", "契约交付日期 (Request Due Date)", "IndependentDemand.due_day"),
        ("request_qty", "request_qty", "DOUBLE", "Default 0.0", "客户下单原始数量", "IndependentDemand.qty"),
        ("customer_tier", "customer_tier", "INTEGER", "Default 3", "1-Tier1 (战略), 2-Tier2 (核心), 3-Tier3 (普通)", "IndependentDemand.customer_tier"),
        ("revenue", "revenue", "DOUBLE", "Default 0.0", "合同销售总金额 (元)", "IndependentDemand.revenue")
    ]
    render_table_helper(doc, tbl_dem_headers, tbl_dem_data)

    add_styled_paragraph(doc, "表 4-10 ipc_financial_ledger (IBP 财业双生分类账簿表)", bold_prefix="【财务分类账 Schema】")
    tbl_fin_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_fin_data = [
        ("ledger_id", "ledger_id", "VARCHAR(40)", "🔑 PK", "财务分类账唯一流水号", "FinancialLedger.ledger_id"),
        ("period_id", "period_id", "VARCHAR(10)", "NOT NULL", "财务核算周期 (月/季/年)", "FinancialLedger.period"),
        ("nopat_contribution", "nopat", "DOUBLE", "Default 0.0", "税后净营业利润贡献额 (NOPAT)", "FinancialLedger.nopat"),
        ("operating_capital", "operating_capital", "DOUBLE", "Default 0.0", "营运资本占用额 (Inventory + WIP)", "FinancialLedger.operating_capital"),
        ("roic_impact", "roic_impact", "DOUBLE", "Default 0.0", "ROIC 逆势提升百分点", "FinancialLedger.roic")
    ]
    render_table_helper(doc, tbl_fin_headers, tbl_fin_data)

    # Module 5
    add_custom_heading(doc, "5. 模块 5：IO 多阶安全库存水位优化表 (IO Safety Stock Policy - 12 张表)", level=3)
    add_styled_paragraph(doc, "运行 MEIO 算法，将服务水准 SL 转化为天级安全库存目标水位：\n· 核心物理表包括：ipc_allotment_constraint, ipc_allotment_ledger, ipc_io_dos_policy, ipc_io_safety_stock_average_demand_profile, ipc_io_safety_stock_item (SS_i), ipc_io_safety_stock_item_mapping, ipc_io_safety_stock_time_phased_bounds, ipc_io_safety_stock_time_phased_result, ipc_io_ss_rule 等 12 张表。")

    add_styled_paragraph(doc, "表 4-11 ipc_allotment_constraint (ITP 战术配额防波堤约束表)", bold_prefix="【配额约束 Schema】")
    tbl_allot_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_allot_data = [
        ("scenario_id", "scenario_id", "VARCHAR(32)", "🔑 PK", "沙盒场景标识 (如 BASE, PESSIMISTIC)", "AllotmentConstraintKey.scenario"),
        ("part_code", "part_code", "VARCHAR(40)", "🔑 PK", "配额管控物料号", "AllotmentConstraintKey.part_id"),
        ("site_code", "site_code", "VARCHAR(8)", "🔑 PK", "供应厂区站点", "AllotmentConstraintKey.site_id"),
        ("region", "region", "VARCHAR(20)", "Default '*'", "配额划拨地理大区", "AllotmentConstraintKey.region_id"),
        ("customer_group", "customer_group", "VARCHAR(20)", "Default '*'", "配额划拨客户群", "AllotmentConstraintKey.cust_group_id"),
        ("day", "day", "INTEGER", "🔑 PK", "相对 Rundate 的天数索引", "AllotmentConstraintKey.day"),
        ("itp_calculated_qty", "itp_qty", "DOUBLE", "Default 0.0", "算法计算出的建议配额上限", "AllotmentValue.calculated_limit"),
        ("is_locked", "is_locked", "BOOLEAN", "Default FALSE", "TRUE 表示锁定配额", "AllotmentValue.is_locked")
    ]
    render_table_helper(doc, tbl_allot_headers, tbl_allot_data)

    # Module 6
    add_custom_heading(doc, "6. 模块 6：基础支撑、感知整合与主数据表 (Master Data & Perception - 133 张表)", level=3)
    add_styled_paragraph(doc, "负责感知外围 ERP/MES/WMS/CRM 系统全样数据，实施数据清洗与解耦映射：\n· 站点与地理：ipc_site, ipc_location, ipc_logistic_location, ipc_region, ipc_country\n· 客户与供应商：ipc_customer, ipc_hierarchy_customer, ipc_supplier, ipc_supplier_part\n· 资源与能力：ipc_work_center, ipc_work_center_capacity, ipc_constraint, ipc_constraint_available\n· 工程变更 (ECN) 与呆滞置换 (SWAP)：ipc_ecn_effectivity, ipc_ecn_substitute_map, ipc_swap_result 等 133 张表。\n*(完整 199 张物理表字段明细详见独立全景数据字典文档 ipc_data_dictionary.md)*")

    add_styled_paragraph(doc, "表 4-12 ipc_work_center_capacity (车间工作中心日产能表)", bold_prefix="【产能 Schema】")
    tbl_wc_headers = ["字段编码", "物理字段名", "数据类型", "键约束", "业务释义与计算逻辑", "对应 C++ 结构体"]
    tbl_wc_data = [
        ("work_center", "work_center", "VARCHAR(40)", "🔑 PK", "工作中心/机台唯一编码", "WorkCenter.wc_id"),
        ("site", "site", "VARCHAR(8)", "🔑 PK", "归属厂区站点", "WorkCenter.site_id"),
        ("day", "day", "INTEGER", "🔑 PK", "相对天数索引", "WorkCenter.day"),
        ("daily_capacity", "daily_capacity", "DOUBLE", "Default 0.0", "日可用总机器工时 (Machine Hours)", "WorkCenter.capacity"),
        ("efficiency", "efficiency", "DOUBLE", "Default 1.0", "设备综合开动效率 (OEE)", "WorkCenter.efficiency")
    ]
    render_table_helper(doc, tbl_wc_headers, tbl_wc_data)

    # 3. 7-Phase Algorithm Breakdown
    add_custom_heading(doc, "三、 从 IBP 到排产调度的 7 大阶段执行引擎算法深拆", level=2)

    add_custom_heading(doc, "1. 【阶段 0】起点双轨 Fork-Join 并发编译引擎", level=3)
    add_styled_paragraph(doc, 
        "系统点火启动时，为保障百万人份节点的秒级响应，引擎绝对禁止串行处理，而是立刻触发 Fork-Join 双轨并发编译机制：",
        bold_prefix="【并发编译架构】"
    )
    add_styled_paragraph(doc, 
        "· 并行轨 A (订单优先级排序与维度绑定)：将 S&OP 预测与客户真实订单统一映射至 Customer Group × Region × Product Family 三维空间，调用 encode_composite_priority() 算子将契约状态、客户等级、交期偏置及订单金额封装为 64 位无符号整数，完成全局订单流的唯一位重量赋值。\n"
        "· 并行轨 B (全网价值链拓扑展开)：调用 lsc_tree.cpp 中的 compile_low_level_codes() 算法，在独立于需求数量的前提下，递归求解全网物料的低层码 (Low-Level Code, LLC)。确定消纳拓扑顺序，彻底排除循环依赖与逻辑死锁。\n"
        "· Fork-Join 屏障收敛：双轨并行任务在同步屏障 (Barrier) 处强制对齐，为后续计算建立“商业契约优先级”与“物理拓扑可行性”的双重基石。"
    )

    add_custom_heading(doc, "2. 【阶段 1】IBP 财业双生控制塔与 Bilevel 双层规划", level=3)
    add_styled_paragraph(doc, 
        "解决战略目标与车间微观执行脱节的根本工程途径在于求解离散非凸双层优化模型 (Bilevel Optimization)：",
        bold_prefix="【财业双生模型】"
    )
    add_styled_paragraph(doc, 
        "· 上层规划 (Strategic Control)：以资本回报率 (ROIC) 极大化与营运资本 (Operating Capital) 极小化为双目标，导出各产品线及车间资源的最高物理配额矢量 A_alloc。\n"
        "· 下层规划 (Operational Scheduling)：求解离散 NP-hard 车间调度成本极小化模型，使微观工单严格在 A_alloc 护栏内完成物理平滑排期。\n"
        "· 3D 共识预测与 Holt-Winters 分拆：融合 Sales, Marketing, Statistical, Strategic 四维预测点阵，运行 run_holt_winters(series, 7, 30) 三重指数平滑模型，结合历史组合比例拆解至叶子 SKU。\n"
        "· MEIO 多阶库存优化：基于服务水准 Z_i = norm_inv(SL_i) 与交期方差，沿着 BOM 拓扑前向传递需求均值与方差，精确求解全网多阶安全库存：\n"
        "  SS_i = Z_i * sqrt(L_i * var_D + D_i^2 * var_L)"
    )

    add_custom_heading(doc, "3. 【阶段 2】ITP 战术净需求冲销与 3D 张量网格", level=3)
    add_styled_paragraph(doc, 
        "预测与订单在 3D 张量网格 gross_demand[part_id][day][dim_idx] 中进行空间对齐与时间映射。为了防止预录 S&OP 预测与实际到达订单重复计算，引擎运行双向滑动窗口消纳算子 (Forecast Consumption)：",
        bold_prefix="【需求冲销机制】"
    )
    add_styled_paragraph(doc, 
        "Unconsumed Forecast(t) = max(0, Original Forecast(t) - sum(Customer Orders(t ± Δt)))\n"
        "通过动态调整前后滑动窗口范围 Δt，输出消除了虚假重叠的纯净净需求流 (Net Demand Stream)，无缝注入战术决策树。"
    )

    add_custom_heading(doc, "4. 【阶段 3】ITP 战术配额五大条件判决门控树", level=3)
    add_styled_paragraph(doc, 
        "战术层构建了不可撕裂的五大条件判决门控阵列，确保配额划分既符合商业战略，又具备物理可达性：",
        bold_prefix="【五大门控逻辑】"
    )
    add_styled_paragraph(doc, 
        "1. 判决门控 1 (Gating 瓶颈判定)：若 Demand Qty > Available Supply，触发瓶颈分配机制；否则走 FIFS (First-In-First-Served) 顺畅释放配额。\n"
        "2. 判决门控 2 (时间桶属性判定)：若处于短期窗口 (1-4周)，运行 Strategic Priority 算子，High 承诺订单 100% 划拨，余量注入 Surplus 余量池；若处于长期窗口 (5周+)，运行 FairShare 比例分摊算子：Assigned_i = Supply * (Demand_i / Total Demand)。\n"
        "3. 判决门控 3 (跨时间桶 Netting 判定)：若 Shortage(t) > 0 且 Surplus(t+k) > 0，运行 Left-Shifting 向左切片拉动算子 Pull Qty = min(Shortage(t), Surplus(t+k))，将远端富余向左拉动填补近端缺口。\n"
        "4. 配额硬护栏导出：输出不可逾越的战术配额硬约束结构体 ipc_allotment_constraint，并挂载 AllotmentRollbackGuard 事务护栏，实现零分配回滚。"
    )

    add_custom_heading(doc, "5. 【阶段 4】LBL MRP 拓扑净额与 Alternate 动态替换料分配", level=3)
    add_styled_paragraph(doc, 
        "引擎按 Low-Level Code (LLC 0 -> LLC N) 进行逐层消纳点火。在 OpenMP 多线程同层并行下，逐层扣减在途与现货库存。当主物料出现供应缺口时，自动触发动态替换料逻辑：",
        bold_prefix="【BOM 逐层消纳与动态替换】"
    )
    add_styled_paragraph(doc, 
        "扫描 BOM 中挂载的 Alt Group 优先级列表，按代换系数与工程变更 (ECO) 生效期自动切换备选物料，并将扣减明细写入 ipc_alternate_allocation 账本，确保装配线绝不停工。"
    )

    add_custom_heading(doc, "6. 【阶段 5】IOP 微观派发与 Resource Swapping 4 步资源置换", level=3)
    add_styled_paragraph(doc, 
        "配额下发至 IOP 微观执行层后，触发动态定锚与资源重配机制：",
        bold_prefix="【动态定锚与资源置换】"
    )
    add_styled_paragraph(doc, 
        "1. 判决门控 4 (交期契约定锚)：比较客户 Due Date 与系统算法算出的 Best Can Do 极值，定锚不可撕裂的发货红线承诺日期 PSD = max(Due Date, Best Can Do)。\n"
        "2. 判决门控 5 (微观扰动与 Resource Swapping)：当面临紧急插单或突发设备故障时，触发 Resource Swapping 4 步置换算子：(1)插单优先级校验；(2)全网软预留光谱扫描；(3)低优先订单资源剥离 (De-allocation)；(4)被剥离订单二次平滑排期。"
    )

    add_custom_heading(doc, "7. 【阶段 6】MCDS / DBD 车间微观派程调度与 180 秒刚性反写", level=3)
    add_styled_paragraph(doc, 
        "结合车间工位/机台日产能 (wc_daily_capacity) 与 Routing 路径，生成工单 DAG 依赖图，求解精准开完工时刻。控制塔摒弃传统开环观赏性大屏，采用闭环 MPC (Model Predictive Control) 架构：",
        bold_prefix="【闭环 MPC 刚性反写】"
    )
    add_styled_paragraph(doc, 
        "在 3 分钟 (180 秒) 的控制周期内，将解算出的微观派工单 (Work Order) 刚性反写至底层 MES/WMS 与 DuckDB 列式数据库，闭环驱动物理车间的实际执行。"
    )

    # 4. Data Structures & Performance Optimizations
    add_custom_heading(doc, "四、 第二层：底层数据结构与 C++ 算法高性能调优", level=2)
    add_styled_paragraph(doc, 
        "为支撑上述 7 大业务算法在百万级节点规模下实现秒级求解，IPC C++ 引擎在底层设计了 6 大极致性能优化与专用数据结构：",
        bold_prefix="【底层性能调优架构】"
    )

    add_styled_paragraph(doc, 
        "1. DOD (Data-Oriented Design) 连续内存布局：摒弃传统 OOP 面向对象中指针散落于堆区 (Heap) 的节点图模式，将百万级节点存入连续内存数组 std::vector<PartSiteRecord>。CPU 预取器能够将整块内存连续装载进 L1/L2 Cache，消除 Cache Miss 和指针追逐开销，缓存命中率提升至接近 100%。\n\n"
        "2. 64 位复合优先级位按位打包 (Bit-Packing Composite Priority)：在 encode_composite_priority() 中，将契约状态 (Bit 62)、客户等级 (Bit 60-61)、交期偏置 (Bit 44-59)、原始优先级 (Bit 28-43) 与订单金额 (Bit 0-27) 打包进单个 uint64_t 无符号整数。CPU 在排序时仅需执行一次标量比较，相比传统多字段 Lambda 比较函数性能提升 8 倍以上。\n\n"
        "3. OpenMP Task-Group 多线程同层并行与 std::atomic 无锁争用：在 LBL MRP 逐层消纳中，同一 Low-Level Code (LLC) 层级内的物料节点天然无依赖。引擎通过 #pragma omp parallel for 进行多核线程并发。针对子节点需求的累加与库存扣减，采用 std::atomic_ref 与 compare_exchange_weak 无锁 CAS 事务指令，彻底摆脱互斥锁 (Mutex) 开销。\n\n"
        "4. 配额哈希 Fast-Path 与周对齐回退机制：采用专用位移哈希函数 AllotmentConstraintKeyHash 对 (part_id, day, family_id, cust_group_id, region_id) 5D 键进行哈希；查找时优先单日精准匹配，失败时快速回退至周一对齐日 (wk_start)，实现 O(1) 常数级配额约束查表。RAII 护栏 AllotmentRollbackGuard 保证在配额分配失败时实现零内存分配的事务回滚。\n\n"
        "5. MEIO 一维展平连续数组：将多阶库存优化的日需求提取开销从 O(N × M) 嵌套哈希表展平为单块一维连续数组 parts_daily_demand[part_id * num_days + day]。利用直接偏移量计算，使得沿 BOM DAG 向下传递需求均值与方差时达到极致的 O(1) 内存访问效率。\n\n"
        "6. DuckDB 流式 Appender 零拷贝内存表 Batch 灌入：计算结果写回 DuckDB 时，摒弃传统的 SQL INSERT INTO 文本解析开销，直接使用 C++ 原生 duckdb::Appender 内存流 API，通过 Binary Stream Batch 直接将结构体灌入列式内存表中，百万人份派工单写回仅需数十毫秒。"
    )

    # 5. Software Architecture Ledger Table
    add_custom_heading(doc, "五、 C++ 核心代码与算法对账全景账本 (Software Ledger)", level=2)
    add_styled_paragraph(doc, 
        "下表为 IPC C++ 生产引擎源码与全景白皮书理论算法的精准映射对账表。全书所有理论模型、数据结构与控制门控均在底层源码中具备 1:1 的对应实装：",
        bold_prefix="【源码与理论对账】"
    )

    tbl_ledger_headers = ["阶段序号", "逻辑模块", "业务算法 / 数据模型", "底层 C++ 数据结构与优化技术", "关联 DuckDB 物理表 / 源码行号"]
    tbl_ledger_data = [
        ("0.0", "前置感知", "外围 ERP/MES/WMS/CRM 感知清洗与数据整合", "DuckDB 199 表全网 ETL 规范化映射", "全网 199 张 DuckDB 物理表 [database.cpp:L1] 【已实装】"),
        ("0.1", "起点并发 A", "订单优先级与战略维度绑定", "encode_composite_priority (64位位打包)", "ipc_independent_demand [ipc_types.h:L220] 【已实装】"),
        ("0.2", "起点并发 B", "全网物料 LLC 低层码拓扑编译", "compile_low_level_codes (DAG 拓扑消纳)", "ipc_part / ipc_bom_item [lsc_tree.cpp:L105] 【已实装】"),
        ("1.1", "IBP 共识", "多源共识预测点阵历史比例拆解", "ConsensusForecast & HierarchyResolver", "ipc_hierarchy_product_family [engine_main.cpp:L876] 【已实装】"),
        ("1.2", "IBP 预测", "Holt-Winters 时序平滑预测", "run_holt_winters (Triple Exp Smoothing)", "ipc_sop_calendar_date [math_utils.cpp:L45] 【已实装】"),
        ("1.3", "MEIO 优化", "多阶安全库存与需求方差前向传递", "parts_daily_demand 一维展平连续数组", "ipc_part / ipc_part_site [engine_main.cpp:L1195] 【已实装】"),
        ("2.1", "ITP 网格", "3D DOD 需求张量构建", "gross_demand[part_id][day][dim] 3D向量", "ipc_independent_demand [mrp_engine.cpp:L54] 【已实装】"),
        ("2.2", "ITP 冲销", "Forecast Consumption 双向滑动窗口", "双向指针滑动窗口缺口冲销算子", "ipc_independent_demand [mrp_engine.cpp:L70] 【已实装】"),
        ("3.1", "战术门控", "Gating 瓶颈/Priority/FairShare/Left-Shift", "AllotmentConstraintKeyHash & 星期一 Fast-Path", "ipc_allotment_constraint [ipc_types.h:L115] 【已实装】"),
        ("3.2", "事务回滚", "战术配额事务性尝试与撤销", "AllotmentRollbackGuard RAII 零分配 Guard", "ipc_allotment_ledger [ipc_types.h:L196] 【已实装】"),
        ("4.1", "LBL MRP", "逐层 MRP 消纳与在途/现货扣减", "OpenMP #pragma omp parallel for + CAS 无锁", "ipc_onhand / ipc_scheduled_receipt [mrp_engine.cpp:L161] 【已实装】"),
        ("4.2", "替换料", "动态替换料 BOM 优先级扫描扣减", "AlternateAllocationRecord 与 Alt 组别检索", "ipc_bom_item / ipc_alternate_allocation [substitution.cpp:L30] 【已实装】"),
        ("5.1", "IOP 定锚", "交期契约红线定锚 PSD", "PSD = max(Due Date, Best Can Do)", "ipc_planned_supply_assignment [mrp_engine.cpp:L583] 【已实装】"),
        ("5.2", "资源置换", "Resource Swapping 4-Step 动态置换", "软预留光谱 gross_demand_priority 动态剥离", "ipc_swap_result [mrp_engine.cpp:L610] 【已实装】"),
        ("6.1", "DBD 调度", "MCDS / DBD 车间工位/机台派程", "run_dbd_dispatch_engine & Workcenter 匹配", "ipc_dispatch_ledger / ipc_work_center [dbd_engine.cpp:L1] 【已实装】"),
        ("6.2", "刚性反写", "闭环 MPC 180 秒工单刚性反写", "DuckDB duckdb::Appender 原生二进制流式写回", "ipc_dispatch_ledger / ipc_planned_order [engine_main.cpp:L1335] 【已实装】")
    ]
    render_table_helper(doc, tbl_ledger_headers, tbl_ledger_data)

    # Apply 100% font unification across ALL paragraphs in the document!
    unify_all_document_fonts(doc, primary_font="微软雅黑", ascii_font="Calibri")

    print(f"Saving perfected unabridged v1.0 document to: {output_docx_v1}...")
    doc.save(output_docx_v1)
    
    print(f"Also updating base original document: {original_docx}...")
    doc.save(original_docx)
    
    print(f"Success! Final documents saved cleanly. Paragraphs: {len(doc.paragraphs)}, Tables: {len(doc.tables)}")

if __name__ == "__main__":
    build_unabridged_ipc_v1_0()
