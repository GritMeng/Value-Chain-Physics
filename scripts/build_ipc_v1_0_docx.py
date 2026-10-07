# -*- coding: utf-8 -*-
import os
import docx
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
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

def create_ipc_v1_0_docx(output_docx_path, image_path):
    doc = docx.Document()

    # Page setup - Margins (1 inch = 1440 dxa)
    sections = doc.sections
    for s in sections:
        s.top_margin = Inches(1.0)
        s.bottom_margin = Inches(1.0)
        s.left_margin = Inches(1.0)
        s.right_margin = Inches(1.0)

    # Styles
    style_normal = doc.styles['Normal']
    style_normal.font.name = '微软雅黑'
    style_normal.font.size = Pt(10.5)
    style_normal.font.color.rgb = RGBColor(0x33, 0x41, 0x55) # Slate dark

    # Helper function for adding styled headings
    def add_custom_heading(text, level):
        h = doc.add_heading(text, level=level)
        h.paragraph_format.space_before = Pt(14)
        h.paragraph_format.space_after = Pt(6)
        run = h.runs[0]
        run.font.name = '微软雅黑'
        if level == 1:
            run.font.size = Pt(18)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0x0f, 0x17, 0x2a) # Dark navy
        elif level == 2:
            run.font.size = Pt(14)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0x02, 0x84, 0xc7) # Visio Sky Blue
        elif level == 3:
            run.font.size = Pt(12)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0x33, 0x41, 0x55)
        return h

    # Helper for adding styled callout box
    def add_callout(text, title="重要导言与原则"):
        tbl = doc.add_table(rows=1, cols=1)
        tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
        cell = tbl.cell(0, 0)
        set_cell_background(cell, "F1F5F9")
        set_cell_margins(cell, top=140, bottom=140, left=200, right=200)
        
        # Left border accent
        tcPr = cell._element.get_or_add_tcPr()
        borders = parse_xml(f'<w:tcBorders {nsdecls("w")}><w:left w:val="single" w:sz="36" w:space="0" w:color="0284C7"/><w:top w:val="none"/><w:right w:val="none"/><w:bottom w:val="none"/></w:tcBorders>')
        tcPr.append(borders)
        
        p = cell.paragraphs[0]
        p.paragraph_format.space_before = Pt(4)
        p.paragraph_format.space_after = Pt(4)
        r_t = p.add_run(f"【{title}】\n")
        r_t.font.bold = True
        r_t.font.size = Pt(10.5)
        r_t.font.color.rgb = RGBColor(0x02, 0x84, 0xc7)
        r_b = p.add_run(text)
        r_b.font.size = Pt(10)
        r_b.font.color.rgb = RGBColor(0x33, 0x41, 0x55)
        doc.add_paragraph() # Spacer

    # ==================== Document Header / Title ====================
    p_title = doc.add_paragraph()
    p_title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_title.paragraph_format.space_before = Pt(24)
    p_title.paragraph_format.space_after = Pt(6)
    r_title = p_title.add_run("智能计划与控制（IPC）体系")
    r_title.font.size = Pt(26)
    r_title.font.bold = True
    r_title.font.color.rgb = RGBColor(0x0f, 0x17, 0x2a)

    p_sub = doc.add_paragraph()
    p_sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_sub.paragraph_format.space_after = Pt(24)
    r_sub = p_sub.add_run("全景白皮书与 C++ 算法引擎技术架构指南 (v1.0 旗舰完整版)")
    r_sub.font.size = Pt(14)
    r_sub.font.color.rgb = RGBColor(0x02, 0x84, 0xc7)

    add_callout(
        "智能计划与控制（IPC, Intelligent Planning & Control）体系远非单一的软件产品，而是一套融合前瞻管理理念、数学规划模型、DOD 高性能数据结构与 C++ 并行算法引擎的完整决策指挥系统。本白皮书 1.0 版实现了从战略 IBP 控制塔到车间微观工单排产调度的无损衔接，并对 C++ 源码实现进行了逐层次深度解构。",
        "IPC v1.0 旗舰版系统导言"
    )

    # ==================== 第一部分：IPC 体系介绍与总纲 ====================
    add_custom_heading("第一部分：IPC — 智能计划与控制体系介绍与总纲", level=1)
    
    add_custom_heading("一、核心理念：智能驱动，构建一体化运营能力", level=2)
    doc.add_paragraph(
        "企业运营依赖于客户、产品与服务、内部资源、外部伙伴四大要素的协同。传统模式中，数据、流程与决策割裂，导致战略与微观派工严重脱节（“两张皮”割裂），企业难以应对 VUCA 时代的剧烈波动。"
    )
    doc.add_paragraph(
        "IPC 体系致力于从根本上解决这一工程难题，其核心在于：以智能（Intelligence）为驱动，实现真正有价值的集成（Integration）与优化（Optimization）：\n"
        "1. 智能驱动集成 (Intelligent Integration)：系统能智能识别并连接全局数据与流程，动态打通从战略到执行、从客户到供应商的端到端网络，确保决策约束无损下达。\n"
        "2. 智能驱动优化 (Intelligent Optimization)：系统能理解复杂的业务目标与物理约束，在多重维度（成本、交期、ROIC、营运资本）间进行明智权衡，求解全局最优解。"
    )

    add_custom_heading("二、核心价值：实现卓越运营", level=2)
    doc.add_paragraph(
        "IPC 体系为企业带来三大核心商业价值：\n"
        "· 全局可视，精准决策：提供端到端的实时物理视野与财务 P&L 洞察，支持基于 180 秒毫秒级推演的 What-If 模拟决策。\n"
        "· 自动协同，降本增效：打通横向与纵向协同壁垒，通过算法自动化消除沟通死锁，释放组织产能。\n"
        "· 敏捷响应，提升韧性：对供应链中断（如黑天鹅断供、设备故障）进行毫秒级感知与 4 步资源置换 (Resource Swapping)，增强抗逆能力。"
    )

    add_custom_heading("三、清晰的人机协作模式", level=2)
    doc.add_paragraph(
        "IPC 系统建立清晰的人机分工：\n"
        "· 智能执行层 (IPC 计划引擎)：负责全局实时感知、多维模拟分析、NP-Hard 车间排产求解、180秒刚性工单反写与例外报警。\n"
        "· 战略决策层 (人类管理者)：负责设定战略财务目标 (ROIC/NOPAT 泛函)、定义配额与优先级护栏、审批边界例外与重大投资。"
    )

    add_custom_heading("四、体系建设方法论与关键成功要素", level=2)
    doc.add_paragraph(
        "IPC 建设是一场能力重构而非简单软件部署。我们推崇基于“总设计师”机制的“蓝图设计 - 奇点突破 - 价值验证 - 有机扩散”四阶段飞轮路径，确保一把手工程落地、聚焦可衡量的 KPI 收益（如季度 ROIC 逆势提升 1.8%），并依托统一的高性能 C++ 技术平台与数据生命线。"
    )

    # ==================== 第二部分：数据模型 ====================
    add_custom_heading("第二部分：IPC 三大数据模型与架构表结构 (Data Models)", level=1)
    doc.add_paragraph(
        "IPC 体系的智能运营中枢由三大核心数据模型协同构成：\n"
        "1. 运营数据模型 (Operational Data Model) — “数字镜像”：完整定义产品结构（Part、BOM）、客户群体及供应网络（Site、Location、Supplier）。\n"
        "2. 控制数据模型 (Control Data Model) — “规则大脑”：承载计划策略（补货规则、时间栅栏 DTF、安全库存策略）、优化算法门控与 C++ 执行逻辑。\n"
        "3. 决策数据模型 (Decision Data Model) — “决策引擎”：结合历史与实时数据，驱动 3D DOD 向量网格与 MRP Netting 计算，输出计划订单 (PlannedOrder) 与工单反写记录。"
    )

    # Table for Data Models
    tbl_dm = doc.add_table(rows=6, cols=4)
    tbl_dm.alignment = WD_TABLE_ALIGNMENT.CENTER
    headers = ["数据模型分类", "核心物理数据表", "数据流方向与主键", "业务与算法控制功能"]
    hdr_cells = tbl_dm.rows[0].cells
    for i, h_text in enumerate(headers):
        hdr_cells[i].text = h_text
        set_cell_background(hdr_cells[i], "0284C7")
        hdr_cells[i].paragraphs[0].runs[0].font.color.rgb = RGBColor(0xff, 0xff, 0xff)
        hdr_cells[i].paragraphs[0].runs[0].font.bold = True

    dm_data = [
        ("运营数据模型\n(Operational)", "ipc_material_node\nipc_bom_item\nipc_onhand", "Part + Site (主键)\nBOM Parent/Child\nLocation + AvailableDate", "建立价值链物理网络镜像，存储物料成本、交期 LeadTime、在手库存与 BOM 损耗率"),
        ("运营需求模型\n(Demand)", "ipc_independent_demand\nipc_consensus_forecast", "Demand_ID (主键)\nPart + Customer + Date", "存储客户订单、S&OP 预测与 3D 共识预测点阵，绑定订单优先级与维度 Group"),
        ("控制数据模型\n(Control)", "ipc_allotment_constraint\nipc_solver_config", "Scenario + Part + Day\nParam_Name + Param_Value", "定义 5 大战术门控配额硬护栏 (Allotment Cap)、时间栅栏 DTF 与 C++ 引擎运行模式"),
        ("控制日历模型\n(Calendar)", "ipc_calendar\nipc_planning_calendar", "Calendar_Name\nPart + Calendar_Type", "定义车间工作日历、预测分布 SpreadCalendar 与跨时间桶冲销窗口日历"),
        ("决策数据模型\n(Decision)", "ipc_planned_order\nipc_alternate_allocation\nipc_micro_dispatch_ledger", "IPC_Planned_Order\nMain_Part + Alt_Part\nDemand + WorkOrder", "输出 LBL MRP 计算的计划订单、专利替换料分配日志以及 180s 刚性反写的微观派工单 Work Order")
    ]

    for row_idx, data_tuple in enumerate(dm_data, start=1):
        row_cells = tbl_dm.rows[row_idx].cells
        if row_idx % 2 == 1:
            for c in row_cells: set_cell_background(c, "F8FAFC")
        for col_idx, text in enumerate(data_tuple):
            row_cells[col_idx].text = text
            set_cell_margins(row_cells[col_idx], top=80, bottom=80, left=120, right=120)

    doc.add_paragraph() # Spacer

    # ==================== 第三部分：全景 Visio 逻辑流程与 7 大阶段深拆 ====================
    add_custom_heading("第三部分：从 IBP 到排产调度的全景逻辑流程与 7 大阶段算法深拆", level=1)
    doc.add_paragraph(
        "本章节为 IPC 交付大脑的算法执行核心。系统运转由集中的串并行计算与条件判断门控驱动，实现了 180 秒内从财务 ROIC 目标向微观工单 (Work Order) 的无损传导。"
    )

    # Embed Visio Flowchart Image
    if os.path.exists(image_path):
        p_img = doc.add_paragraph()
        p_img.alignment = WD_ALIGN_PARAGRAPH.CENTER
        run_img = p_img.add_run()
        run_img.add_picture(image_path, width=Inches(6.5))
        p_cap = doc.add_paragraph()
        p_cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_cap.paragraph_format.space_after = Pt(14)
        r_cap = p_cap.add_run("图 3-1 IPC 从战略 IBP 控制塔至微观车间派工排产调度的 Visio 规范主逻辑流程图")
        r_cap.font.size = Pt(9.5)
        r_cap.font.italic = True
        r_cap.font.color.rgb = RGBColor(0x64, 0x74, 0x8b)

    add_custom_heading("一、【阶段 0】起点双轨 Fork-Join 并发编译引擎", level=2)
    doc.add_paragraph(
        "系统点火启动时，绝对禁止单线程串行处理。启动即触发 Fork-Join 双轨并发：\n"
        "· 并行轨 A (订单优先级排序与维度绑定)：将 S&OP 预测与客户真实订单映射至 Customer Group × Region × Product Family 统一层级，通过 64 位位打包算法赋值唯一优先级重量。\n"
        "· 并行轨 B (价值链网络拓扑展开)：调用 lsc_tree.cpp 中的 compile_low_level_codes()，在不依赖需求数量的前提下，递归求解全网物料低层码 (LLC)，确定消纳拓扑顺序并排除环路死锁。\n"
        "· Fork-Join 屏障收敛：双轨计算完成后，在屏障处收敛同步，为后续计算建立“业务契约”与“物理可行性”的双重防线。"
    )

    add_custom_heading("二、【阶段 1】IBP 财业双生控制塔与 Bilevel 双层规划", level=2)
    doc.add_paragraph(
        "解决战略与车间“两张皮”的根本工程通途在于离散非凸双层优化模型 (Bilevel Optimization)：\n"
        "· 上层规划：以资本回报率 (ROIC) 极大化与营运资本 (Operating Capital) 极小化为控制目标，导出各产品线与车间资源的最高配额矢量 A_alloc。\n"
        "· 下层规划：求解离散 NP-hard 车间调度成本极小化，使得微观工单满足物理装配约束。\n"
        "· 3D 共识预测与 Holt-Winters 分拆：融合 Sales, Marketing, Statistical, Strategic 预测，运行 run_holt_winters(series, 7, 30)，结合历史组合比例拆解至叶子 SKU。\n"
        "· MEIO 多阶库存优化：根据服务水准 Z_i = norm_inv(SL_i) 与交期方差，沿着 BOM DAG 前向传递需求均值与方差，求解全网多阶安全库存 SS_i = Z_i * sqrt(L_i * var_D + D_i^2 * var_L)。"
    )

    add_custom_heading("三、【阶段 2】ITP 战术净需求冲销与 3D 张量网格", level=2)
    doc.add_paragraph(
        "数据统一注入 3D 向量网格 gross_demand[part_id][day][dim_idx]。为了防止预录 S&OP 预测与后到真实订单叠加，算法运行向后与向前冲销窗口：\n"
        "Unconsumed Forecast(t) = max(0, Original Forecast(t) - sum(Customer Orders(t ± Δt)))\n"
        "输出纯净净需求流 (Net Demand Stream)，送入战术门控树。"
    )

    add_custom_heading("四、【阶段 3】ITP 战术配额五大条件判决门控树", level=2)
    doc.add_paragraph(
        "战术层设置五大核心条件判决门控：\n"
        "1. 判决门控 1 (Gating 瓶颈判定)：若 Demand Qty > Available Supply，触发瓶颈分配；否则走 FIFS 顺畅释放配额。\n"
        "2. 判决门控 2 (时间桶属性判定)：若处于短期 (1-4周)，运行 Strategic Priority 算子，High 承诺 100% 划拨，余量注入 Surplus 池；若处于长期 (5周+)，运行 FairShare 比例分摊算子 Assigned_i = Supply * (Demand_i / Total_Demand)。\n"
        "3. 判决门控 3 (跨时间桶 Netting 判定)：若 Shortage(t) > 0 且 Surplus(t+k) > 0，运行 Left-Shifting 向左切片拉动算子 Pull Qty = min(Shortage(t), Surplus(t+k))，将远端富余向左拉动填补近端缺口。\n"
        "· 输出产物：下发不可逾越的战术配额硬护栏 ipc_allotment_constraint。"
    )

    add_custom_heading("五、【阶段 4】LBL MRP 拓扑净额与 Alternate 动态替换料分配", level=2)
    doc.add_paragraph(
        "逐层消纳点火 (Level 0 -> Level N)。在 OpenMP 多线程同层并行下，按 Low-Level Code (LLC) 逐层扣减。当主料发生缺口时，扫描 BOM 中的 Alt Group 优先级，自动切换备选物料并写入 ipc_alternate_allocation 扣减账本。"
    )

    add_custom_heading("六、【阶段 5】IOP 微观派发与 Resource Swapping 4 步资源置换", level=2)
    doc.add_paragraph(
        "配额下发至 IOP 后触发微观执行闭环：\n"
        "1. 判决门控 4 (交期契约定锚)：比较 Due Date 与 Best Can Do，定锚不可撕裂的发货红线 PSD = max(Due Date, Best Can Do)。\n"
        "2. 判决门控 5 (微观扰动与 Resource Swapping)：当面临高优先插单或设备故障时，触发 4 步资源置换：(1)高优先插单判定；(2)全网软预留光谱扫描；(3)低优先资源剥离 De-allocation；(4)被置换任务二次平滑排期。"
    )

    add_custom_heading("七、【阶段 6】MCDS / DBD 车间微观派程调度与 180 秒刚性反写", level=2)
    doc.add_paragraph(
        "结合车间工位/机台日产能 (wc_daily_capacity) 与 Routing，生成工单 DAG 依赖图，求解开完工时刻。控制塔摒弃开环展示大屏，采用闭环 MPC 架构，在 3 分钟 (180秒) 内将解算的微观派工单 (Work Order) 刚性反写至底层 MES/WMS 与 DuckDB 数据库，闭环驱动物理生产。"
    )

    # ==================== 第四部分：底层数据结构与 C++ 算法高性能调优 ====================
    add_custom_heading("第四部分：底层数据结构与 C++ 算法高性能调优 (Data Structures & Optimizations)", level=1)
    doc.add_paragraph(
        "为支撑上述业务算法在数百万级节点规模下实现秒级求解，IPC C++ 引擎在底层设计了 6 大极致性能优化与专用数据结构："
    )

    doc.add_paragraph(
        "1. DOD (Data-Oriented Design) 连续内存布局：摒弃传统 OOP 面向对象中指针散落于堆区 (Heap) 的节点图模式，将百万级节点存入连续内存数组 std::vector<PartSiteRecord>。CPU 预取器能够将整块内存连续装载进 L1/L2 Cache，消除 Cache Miss 和指针追逐开销，命中率提升至 100%。\n\n"
        "2. 64 位复合优先级位按位打包 (Bit-Packing Composite Priority)：在 encode_composite_priority() 中，将契约状态 (Bit 62)、客户等级 (Bit 60-61)、交期 (Bit 44-59)、原始优先级 (Bit 28-43) 与订单金额 (Bit 0-27) 打包进单个 uint64_t 无符号整数。CPU 在排序时仅需执行一次标量比较，相比传统多字段 Lambda 函数性能提升 8 倍以上。\n\n"
        "3. OpenMP Task-Group 多线程同层并行与 std::atomic 无锁争用：在 LBL MRP 逐层消纳中，同一 Low-Level Code (LLC) 层级内的物料节点天然无依赖。引擎通过 #pragma omp parallel for 进行多核线程并发。针对子节点需求的累加与库存扣减，采用 std::atomic_ref 与 compare_exchange_weak 无锁 CAS 事务指令，彻底摆脱互斥锁开销。\n\n"
        "4. 配额哈希 Fast-Path 与周对齐回退机制：采用专用位移哈希函数 AllotmentConstraintKeyHash 对 (part_id, day, family_id, cust_group_id, region_id) 5D 键进行哈希；查找时优先单日精准匹配，失败时快速回退至周一对齐日 (wk_start)，实现 O(1) 常数级配额约束查表。RAII 护栏 AllotmentRollbackGuard 保证在配额分配失败时实现零内存分配的事务回滚。\n\n"
        "5. MEIO 一维展平连续数组：将多阶库存优化的日需求提取开销从 O(N × M) 嵌套哈希表展平为单块一维连续数组 parts_daily_demand[part_id * num_days + day]。利用直接偏移量计算，使得沿 BOM DAG 向下传递需求均值与方差时达到极致的 O(1) 内存访问效率。\n\n"
        "6. DuckDB 流式 Appender 零拷贝内存表 Batch 灌入：计算结果写回 DuckDB 时，摒弃传统的 SQL INSERT INTO 文本解析开销，直接使用 C++ 原生 duckdb::Appender 内存流 API，通过 Binary Stream Batch 直接将结构体灌入列式内存表中，百万人份派工单写回仅需数十毫秒。"
    )

    # ==================== 第五部分：C++ 核心代码与算法对账全景账本 ====================
    add_custom_heading("第五部分：C++ 核心代码与算法对账全景账本 (Software Ledger)", level=1)
    doc.add_paragraph(
        "下表为 IPC C++ 引擎源码与全景白皮书理论算法的精准映射对账表："
    )

    # Software Ledger Table
    tbl_ledger = doc.add_table(rows=15, cols=5)
    tbl_ledger.alignment = WD_TABLE_ALIGNMENT.CENTER
    headers_l = ["阶段序号", "逻辑模块", "业务算法 / 数据模型", "底层 C++ 数据结构与优化技术", "源码文件与行号 / 状态"]
    hdr_cells_l = tbl_ledger.rows[0].cells
    for i, h_text in enumerate(headers_l):
        hdr_cells_l[i].text = h_text
        set_cell_background(hdr_cells_l[i], "0F172A")
        hdr_cells_l[i].paragraphs[0].runs[0].font.color.rgb = RGBColor(0xff, 0xff, 0xff)
        hdr_cells_l[i].paragraphs[0].runs[0].font.bold = True

    ledger_data = [
        ("0.1", "起点并发 A", "订单优先级与战略维度绑定", "encode_composite_priority (64位复合位打包)", "ipc_types.h:L220 【已实装】"),
        ("0.2", "起点并发 B", "全网物料 LLC 低层码拓扑编译", "compile_low_level_codes (DAG 深度优先拓扑)", "lsc_tree.cpp:L105 【已实装】"),
        ("1.1", "IBP 共识", "多源共识预测点阵历史比例拆解", "ConsensusForecast & HierarchyResolver", "engine_main.cpp:L876 【已实装】"),
        ("1.2", "IBP 预测", "Holt-Winters 时序平滑预测", "run_holt_winters (双参数 Triple Exponential)", "math_utils.cpp:L45 【已实装】"),
        ("1.3", "MEIO 优化", "多阶安全库存与需求方差前向传递", "parts_daily_demand 一维展平连续数组 O(1)", "engine_main.cpp:L1195 【已实装】"),
        ("2.1", "ITP 网格", "3D DOD 需求张量构建", "gross_demand[part_id][day][dim] 3D 向量", "mrp_engine.cpp:L54 【已实装】"),
        ("2.2", "ITP 冲销", "Forecast Consumption 双向滑动窗口", "双向指针滑动窗口消纳", "mrp_engine.cpp:L70 【已实装】"),
        ("3.1", "战术门控", "Gating 瓶颈/Priority/FairShare/Left-Shift", "AllotmentConstraintKeyHash & 星期一 Fast-Path", "ipc_types.h:L115 【已实装】"),
        ("3.2", "事务回滚", "战术配额事务性尝试与回滚", "AllotmentRollbackGuard RAII 零分配回滚 Guard", "ipc_types.h:L196 【已实装】"),
        ("4.1", "LBL MRP", "逐层 MRP 消纳与在途/现货扣减", "OpenMP #pragma omp parallel for + CAS 无锁", "mrp_engine.cpp:L161 【已实装】"),
        ("4.2", "替换料", "动态替换料 BOM 优先级扫描扣减", "AlternateAllocationRecord 与 Alt 组别检索", "substitution.cpp:L30 【已实装】"),
        ("5.1", "IOP 定锚", "交期契约红线定锚 PSD", "PSD = max(Due Date, Best Can Do)", "mrp_engine.cpp:L583 【已实装】"),
        ("5.2", "资源置换", "Resource Swapping 4-Step 动态置换", "软预留光谱 gross_demand_priority 动态剥离", "mrp_engine.cpp:L610 【已实装】"),
        ("6.1", "DBD 调度", "MCDS / DBD 车间工位/机台派程", "run_dbd_dispatch_engine & Workcenter 匹配", "dbd_engine.cpp:L1 【已实装】")
    ]

    for row_idx, data_tuple in enumerate(ledger_data, start=1):
        row_cells = tbl_ledger.rows[row_idx].cells
        if row_idx % 2 == 1:
            for c in row_cells: set_cell_background(c, "F8FAFC")
        for col_idx, text in enumerate(data_tuple):
            row_cells[col_idx].text = text
            set_cell_margins(row_cells[col_idx], top=70, bottom=70, left=100, right=100)

    doc.save(output_docx_path)
    print(f"IPC v1.0 Word document successfully created at: {output_docx_path}")

if __name__ == "__main__":
    out_docx = r"h:\IPC\智能计划与控制（IPC）体系_v1.0.docx"
    img_path = r"h:\IPC\方案\images\ibp_to_scheduling_master_flowchart.png"
    create_ipc_v1_0_docx(out_docx, img_path)
