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

def set_cell_background(cell, hex_color):
    tcPr = cell._element.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), hex_color)
    tcPr.append(shd)

def set_cell_margins(cell, top=120, bottom=120, left=150, right=150):
    tcPr = cell._element.get_or_add_tcPr()
    tcMar = OxmlElement('w:tcMar')
    for m_name, m_val in [('top', top), ('bottom', bottom), ('left', left), ('right', right)]:
        node = OxmlElement(f'w:{m_name}')
        node.set(qn('w:w'), str(m_val))
        node.set(qn('w:type'), 'dxa')
        tcMar.append(node)
    tcPr.append(tcMar)

def set_table_borders(table, color="CCCCCC", sz="4", val="single"):
    tblPr = table._element.xpath('w:tblPr')
    if tblPr:
        borders = OxmlElement('w:tblBorders')
        for border_name in ['top', 'left', 'bottom', 'right', 'insideH']:
            border = OxmlElement(f'w:{border_name}')
            border.set(qn('w:val'), val)
            border.set(qn('w:sz'), sz)
            border.set(qn('w:space'), '0')
            border.set(qn('w:color'), color)
            borders.append(border)
        insideV = OxmlElement('w:insideV')
        insideV.set(qn('w:val'), 'none')
        borders.append(insideV)
        tblPr[0].append(borders)

def build_docx():
    doc = docx.Document()
    
    # Margins
    for sec in doc.sections:
        sec.top_margin = Inches(1.0)
        sec.bottom_margin = Inches(1.0)
        sec.left_margin = Inches(1.0)
        sec.right_margin = Inches(1.0)
        
    # Normal Style
    normal = doc.styles['Normal']
    normal.font.name = 'Times New Roman'
    normal.font.size = Pt(11)
    normal.font.color.rgb = RGBColor(0x22, 0x22, 0x22)
    
    # Title
    p_title = doc.add_paragraph()
    p_title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_title.paragraph_format.space_before = Pt(12)
    p_title.paragraph_format.space_after = Pt(8)
    r_title = p_title.add_run("企业价值网络中的资本回报悖论：基于非独立同分布（Non-IID）状态空间建模与闭环调度的架构治理研究")
    r_title.font.size = Pt(16)
    r_title.font.bold = True
    r_title.font.name = 'Times New Roman'
    set_font_eastasia(r_title, '黑体')
    
    # Author
    p_author = doc.add_paragraph()
    p_author.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_author.paragraph_format.space_after = Pt(4)
    r_author = p_author.add_run("孟凡淳 (Grit Meng)")
    r_author.font.size = Pt(11.5)
    r_author.font.bold = True
    set_font_eastasia(r_author, '宋体')
    
    # Affiliation
    p_aff = doc.add_paragraph()
    p_aff.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_aff.paragraph_format.space_after = Pt(2)
    r_aff = p_aff.add_run("IPC 智能计划与控制总设计部 / 独立研究员，北京，中国")
    r_aff.font.size = Pt(10)
    r_aff.font.italic = True
    set_font_eastasia(r_aff, '楷体')
    
    # SSRN & Zenodo
    p_ssrn = doc.add_paragraph()
    p_ssrn.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_ssrn.paragraph_format.space_after = Pt(16)
    r_ssrn = p_ssrn.add_run("SSRN 预印本：Abstract ID 7251098  |  Zenodo 专著归档：DOI: 10.5281/zenodo.22033928")
    r_ssrn.font.size = Pt(9)
    r_ssrn.font.color.rgb = RGBColor(0x66, 0x66, 0x66)
    
    # Abstract Card
    tbl_abs = doc.add_table(rows=1, cols=1)
    tbl_abs.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell_abs = tbl_abs.cell(0, 0)
    cell_abs.width = Inches(6.5)
    set_cell_background(cell_abs, "F8F9FA")
    set_cell_margins(cell_abs, top=140, bottom=140, left=180, right=180)
    
    p_abs = cell_abs.paragraphs[0]
    p_abs.paragraph_format.space_after = Pt(6)
    p_abs.paragraph_format.line_spacing = 1.2
    r_abs_h = p_abs.add_run("摘要：")
    r_abs_h.font.bold = True
    r_abs_h.font.size = Pt(9.5)
    set_font_eastasia(r_abs_h, '黑体')
    
    r_abs_b = p_abs.add_run(
        "离散制造企业持续的信息化资本投入并未普遍驱动投入资本回报率（ROIC）的同向提升，再现了信息系统领域的“生产率悖论”。"
        "本文指出，该悖论的根源是底层范式的结构性错配：传统企业架构与计划系统建立在要素“独立同分布（IID）”的弱耦合假设之上，而真实的离散制造价值网络本质是非独立同分布（Non-IID）的高维强耦合系统。"
        "本文构建五维完备状态流形 $\\mathcal{M}=\\langle\\mathcal{N},\\mathcal{T},\\mathcal{C},\\mathbf{x}(t),\\Delta\\mathbf{x}(t)\\rangle$，提出并形式化统御价值网络演化的八大物理定律，并证明：在共享产能、物料与交期等互斥约束下，多部门局部寻优可引致控制矢量三角不等式相消，其差额定义为侵蚀 ROIC 的协调废热。"
        "在此基础上，本文提出刚性物理剪枝与内存闭环调度引擎（IPC），将组合搜索空间从阶乘级 $O(N!)$ 压制至多项式规模，并在标准服务器上实现 296 秒闭环消纳 50 万笔需求订单的工业实证。"
        "基于 22 年纵向行动研究的三个自然观测案例验证了“结构存续、结构离体、结构退化”的分野。本文最后提出融合“业务解码、系统建模、闭环协同”三位一体的架构治理框架与“医学院模式”能力建设路径。"
    )
    r_abs_b.font.size = Pt(9.5)
    set_font_eastasia(r_abs_b, '宋体')
    
    p_kw = cell_abs.add_paragraph()
    p_kw.paragraph_format.space_after = Pt(0)
    r_kw_h = p_kw.add_run("关键词：")
    r_kw_h.font.bold = True
    r_kw_h.font.size = Pt(9.5)
    set_font_eastasia(r_kw_h, '黑体')
    r_kw_b = p_kw.add_run("数字化转型；IT生产率悖论；非独立同分布；复杂巨系统；算法治理；闭环控制；资本回报悖论")
    r_kw_b.font.size = Pt(9.5)
    set_font_eastasia(r_kw_b, '宋体')
    
    p_spacer = doc.add_paragraph()
    p_spacer.paragraph_format.space_after = Pt(12)
    
    def add_h1(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(16)
        p.paragraph_format.space_after = Pt(6)
        p.paragraph_format.keep_with_next = True
        r = p.add_run(text)
        r.font.size = Pt(14)
        r.font.bold = True
        r.font.name = 'Times New Roman'
        set_font_eastasia(r, '黑体')
        r.font.color.rgb = RGBColor(0x11, 0x11, 0x11)
        return p

    def add_h2(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(12)
        p.paragraph_format.space_after = Pt(4)
        p.paragraph_format.keep_with_next = True
        r = p.add_run(text)
        r.font.size = Pt(12)
        r.font.bold = True
        r.font.name = 'Times New Roman'
        set_font_eastasia(r, '黑体')
        r.font.color.rgb = RGBColor(0x22, 0x22, 0x22)
        return p

    def add_p(text, bold_prefix=None, indent=True):
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(6)
        p.paragraph_format.line_spacing = 1.2
        if indent:
            p.paragraph_format.first_line_indent = Pt(22)
        if bold_prefix:
            r_b = p.add_run(bold_prefix)
            r_b.font.bold = True
            r_b.font.size = Pt(10.5)
            set_font_eastasia(r_b, '黑体')
        r = p.add_run(text)
        r.font.size = Pt(10.5)
        set_font_eastasia(r, '宋体')
        return p

    def add_eq(text):
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.space_before = Pt(6)
        p.paragraph_format.space_after = Pt(6)
        r = p.add_run(text)
        r.font.name = 'Consolas'
        r.font.size = Pt(11)
        r.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
        return p

    # Section 1
    add_h1("1. 引言")
    add_p("制造企业在 ERP、APS 与供应链控制塔上的资本支出年均复合增长达两位数，但企业平均 ROIC 并未同步跃升，部分复杂离散制造门类甚至面临资产周转率边际递减。这正是 Brynjolfsson (1993) 提出的“信息技术生产率悖论”在工业智能时代的显影。")
    add_eq("$$\\text{ROIC} = \\frac{\\text{NOPAT}}{\\text{Invested Capital}}$$")
    add_p("当信息化投资不断加大分母而分子未能同步提升时，ROIC 便停滞甚至下滑。车间里的真实图景是：计划员关闭昂贵的 APS 界面，重新打开 Excel 手工对账——这一细节直观揭示了系统复杂性与实际做功之间的断裂。")
    add_p("既有文献多将转型受阻归因于组织惯性、沟通壁垒或数据质量问题。本文则认为，深层机理是底层数理范式的结构性错配：")
    
    p_b1 = doc.add_paragraph()
    p_b1.paragraph_format.left_indent = Pt(18)
    p_b1.paragraph_format.space_after = Pt(4)
    r1_b = p_b1.add_run("•  IID 偏差：")
    r1_b.font.bold = True
    set_font_eastasia(r1_b, '黑体')
    r1_t = p_b1.add_run("自泰勒制以来，管理工程将价值链拆解为销售、采购、制造、财务等职能子集并分设 KPI，这在数学上暗含了状态演化服从 IID 正交分解的假定。")
    set_font_eastasia(r1_t, '宋体')

    p_b2 = doc.add_paragraph()
    p_b2.paragraph_format.left_indent = Pt(18)
    p_b2.paragraph_format.space_after = Pt(6)
    r2_b = p_b2.add_run("•  Non-IID 现实：")
    r2_b.font.bold = True
    set_font_eastasia(r2_b, '黑体')
    r2_t = p_b2.add_run("在多品种小批量网络中，物料齐套、机台共享与交期承诺存在长程拓扑纠缠（Cao, 2014, 2022）。强行切断关联的平铺数据模型必然产生因果截断与决策失真。")
    set_font_eastasia(r2_t, '宋体')

    add_p("各部门在各自局部最优方向上用力，宏观上发生动力学相消，协调开销随节点数二次方发散（$O(K^2)$），系统最终收敛于低效纳什均衡与“布朗运动式”组织内耗。")

    # Section 2
    add_h1("2. 理论基础")
    
    p_l1 = doc.add_paragraph()
    p_l1.paragraph_format.left_indent = Pt(18)
    p_l1.paragraph_format.space_after = Pt(4)
    r_l1 = p_l1.add_run("•  开放复杂巨系统与综合集成：")
    r_l1.font.bold = True
    set_font_eastasia(r_l1, '黑体')
    r_l1_t = p_l1.add_run("钱学森等（1990）将具有海量异构组分、多层次嵌套与非线性长程相互作用的人造系统定义为开放复杂巨系统（OCGS），其具备计算不可约性（Wolfram, 2002）。“从定性到定量综合集成方法”的核心是人机结合、以人为主的闭环迭代。本文将这一思想工程化为离散制造的价值网络状态流形。")
    set_font_eastasia(r_l1_t, '宋体')

    p_l2 = doc.add_paragraph()
    p_l2.paragraph_format.left_indent = Pt(18)
    p_l2.paragraph_format.space_after = Pt(4)
    r_l2 = p_l2.add_run("•  Non-IID 学习理论：")
    r_l2.font.bold = True
    set_font_eastasia(r_l2, '黑体')
    r_l2_t = p_l2.add_run("Cao (2014, 2022) 指出复杂系统普遍存在实体内部耦合、实体间交互耦合与跨时空异质性。价值链要素的强耦合直接引致状态空间的阶乘级组合爆炸 $O(N!)$。")
    set_font_eastasia(r_l2_t, '宋体')

    p_l3 = doc.add_paragraph()
    p_l3.paragraph_format.left_indent = Pt(18)
    p_l3.paragraph_format.space_after = Pt(8)
    r_l3 = p_l3.add_run("•  控制论边界：")
    r_l3.font.bold = True
    set_font_eastasia(r_l3, '黑体')
    r_l3_t = p_l3.add_run("Ashby (1956) 的必要多样性定律要求控制器复杂度 $V_c$ 不低于被控系统变异数 $V_s$；Miller (1956) 则限定了人脑的工作记忆带宽。当 $V_s \\gg V_c$ 时，基层只能退守手工表格与防御性库存。Kalman (1960) 的可控—可观测对偶定理进一步表明：底层不可观测，则上层不可控制。")
    set_font_eastasia(r_l3_t, '宋体')

    # Section 3
    add_h1("3. 形式化建模与八大物理定律")
    add_h2("3.1 五维完备状态流形")
    add_p("定义价值网络在时刻 $t$ 的状态空间为五维完备流形：")
    add_eq("$$\\mathcal{M}(t) = \\langle \\mathcal{N}, \\mathcal{T}, \\mathcal{C}, \\mathbf{x}(t), \\Delta\\mathbf{x}(t) \\rangle$$")
    add_p("其中 $\\mathcal{N}$ 为状态质点（物料、工位、契约、账户），$\\mathcal{T}$ 为承载历史因果拓扑的有向图（DAG），$\\mathcal{C}$ 为刚性物理与契约约束簇，$\\mathbf{x}(t)$ 为瞬时状态矢量，$\\Delta\\mathbf{x}(t)$ 为离散事件驱动的状态跃迁量。")
    add_p("系统随时间的闭环演进由 IPC 控制算子 $\\Pi$ 驱动，构成马尔可夫/非齐次状态闭环递推：")
    add_eq("$$\\mathbf{x}(t+1) = \\mathbf{x}(t) + \\Delta\\mathbf{x}(t), \\quad \\text{其中 } \\Delta\\mathbf{x}(t) = \\Pi\\big(\\mathbf{x}(t), \\mathcal{T}, \\mathcal{C}\\big)$$")
    add_p("这五个基元相互独立且完备（MICO）。若剔除任一维度，系统辨识核空间非空，控制器复杂度将无法覆盖被控对象变异，违背必要多样性条件。定义全域有效做功目标 $V_{\\Omega}(\\mathbf{x})$ 为可行流形上的净产出贡献，扣除协调废热 $Q_{\\diss}$。")

    add_h2("3.2 八大物理定律")
    add_p("八大定律以 Non-IID 为底层物理基色，构成一条严密的递推公理链：目的论（为何必须对消）→ 本质论（为何是 $O(N!)$）→ 方案论（如何解）→ 能力论（融合三能力）→ 机制论（配额与集中协调）→ 路径论（从何下手）→ 动力论（系统杠杆与支点）→ 进化论（相变如何处置）。")

    laws = [
        ("定律一 目的论（统御）：", "世界本质是非独立同分布（Non-IID）的。若系统缺乏全域统一协奏（同律），局部自利寻优必然引发控制矢量的三角不等式相消（矢量相消）。在宏观上表现为各部门陷入低效纳什均衡与“布朗运动式”盲忙，致使 ROIC 无法反弹。多部门局部最优的合力受三角不等式约束（见 3.3），若方向互斥，净做功趋于零，差额化为废热。"),
        ("定律二 本质论（复杂度边界）：", "在 Non-IID 强耦合与长程路径依赖下，每个订单的分配选择均受其余订单的占用状态约束，依赖关系构成全耦合分配图；当 $N$ 个订单与 $T$ 个时窗、$M$ 个互斥机台构成排程组合时，候选配置数达 $(TM)^N$ 量级，在最密约束情形下退化为 $N!$ 的排列上界，即阶乘级 $O(N!)$，远超碳基大脑的并发处理带宽（Miller, 1956）。此时若盲目增加 $M$ 个计划人员，人与人之间的沟通摩擦开销呈二次方发散 $M(M-1)/2 \\approx O(M^2)$，造成“乱上加乱”。高频微观消纳必须让渡给硅基引擎。"),
        ("定律三 方案论（收敛本体）：", "为实现 Non-IID 巨系统的“可计算性”（Computability），控制介质必须是“数据容器 $D$ + 解算算法 $A$”高频咬合的数字双螺旋 $\\Pi=\\langle D,A\\rangle$，并以刚性剪枝保证在有限离散网格上有限步收敛（见 3.4）。"),
        ("定律四 能力论（三位一体融合能力）：", "由于数据模型与解算算法在物理上与真实运营强耦合不可切分，传统科层架构的切块剥离必然失效。能力建设必须形成“三位一体”融合能力：① 业务解码（将微观物理要素抽象为有向因果图谱）；② 系统建模（形式化映射为五维状态流形）；③ 闭环协同（自动化决策反写与偏差自愈）。"),
        ("定律五 机制论（配额自治与集中协调）：", "在目标不可公度、约束非凸且信息分散下，多主体分散协商在数学上无法自发产生全局自洽解（由阿什比必要多样性定律约束）。集中式可计算协调优于纯分散寻优：设全局配额向量 $\\mathbf{r} \\in \\mathcal{C}$，子域在 $\\mathbf{x}_k \\in \\{\\mathbf{x} : g_k(\\mathbf{x}) \\le \\mathbf{r}_k\\}$ 内自治重优化；总部只调 $\\mathbf{r}$，微观只调 $\\mathbf{x}_k$。"),
        ("定律六 路径论（可观测性）：", "控制维度受限于物理执行层的可观测维度（$\\dim \\mathcal{C} \\le \\dim \\mathcal{O}$）——不可观测即不可控制，必须从微观探针与物理指令反写（Write-Back）自下而上建设。"),
        ("定律七 动力论（系统杠杆与支点）：", "管理权力的施加与资源倾注，必须以系统的真实物理瓶颈作为唯一物理支点（Fulcrum）。记 $\\mathcal{C}_{\\text{active}} \\subset \\mathcal{C}$ 为当前紧约束对应的活跃瓶颈资源集合。当控制输入 $\\delta\\mathbf{u}$ 作用于瓶颈支点 $\\mathbf{x}_{\\text{fulcrum}} \\in \\mathcal{C}_{\\text{active}}$ 时，全域做功 $V_\\Omega$ 的杠杆放大率最大，有效做功增量由方向导数给出：$\\Delta V_\\Omega = \\dfrac{\\partial V_\\Omega}{\\partial \\mathbf{x}_{\\text{fulcrum}}} \\cdot \\delta\\mathbf{u}$，其中梯度分量由企业战略目标（净利、OTD 或其加权和）确定（见 3.1）。若行政指令脱离物理支点、作用于非瓶颈点 $\\mathbf{x} \\notin \\mathcal{C}_{\\text{active}}$，则杠杆收益趋于可忽略，控制能量主要转化为内部摩擦废热 $Q_{\\diss}$，引发系统无效震荡。"),
        ("定律八 进化论（元认知）：", "设 $\\mathcal{K}_t$ 表示 $t$ 时刻规则集、约束集与求解公理；当极端相变致可行解域为空（$\\mathcal{S}_{\\text{feas}} = \\emptyset$）或残差超阈值时，人类须跳出环外通过元操作算子集 $\\Phi$ 重写公理（$\\Phi:\\mathcal{K}_t\\to\\mathcal{K}_{t+1}$）；常态则由硅基在环外自主自愈。")
    ]
    for pref, body in laws:
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Pt(18)
        p.paragraph_format.space_after = Pt(4)
        r_p = p.add_run(pref)
        r_p.font.bold = True
        set_font_eastasia(r_p, '黑体')
        r_b = p.add_run(body)
        set_font_eastasia(r_b, '宋体')

    add_h2("3.3 定理 1：控制矢量相消与协调废热")
    add_p("各部门依据局部目标 $E_k(\\mathbf{x})$ 施加控制输入 $\\mathbf{u}_k = -\\eta_k\\nabla_k E_k(\\mathbf{x})$。由三角不等式：")
    add_eq("$$\\left\\|\\sum_{k=1}^{K}\\mathbf{u}_k\\right\\| \\le \\sum_{k=1}^{K}\\|\\mathbf{u}_k\\|$$")
    add_p("当部门间因共享产能、物料、交期而互斥时，至少存在一对 $\\langle\\mathbf{u}_i,\\mathbf{u}_j\\rangle < 0$，故不等式严格成立。定义协调废热为：")
    add_eq("$$Q_{\\diss} \\triangleq \\sum_{k=1}^{K}\\|\\mathbf{u}_k\\| - \\left\\|\\sum_{k=1}^{K}\\mathbf{u}_k\\right\\| \\ge 0$$")
    add_p("$Q_{\\diss}$ 直接度量内部博弈、等待与库存积压造成的熵增，是 ROIC 物理衰减的微观来源。证明见附录 A。当至少存在一对子系统使 $\\langle\\mathbf{u}_i,\\mathbf{u}_j\\rangle < 0$ 且不全共线时，合范数严格小于分范和；若对所有 $i \\ne j$ 均有 $\\langle\\mathbf{u}_i,\\mathbf{u}_j\\rangle \\le -\\epsilon$（$\\epsilon > 0$），则达到强相消充分条件。相消为充分条件而非必然：仅当所有局部控制非负共线时等号成立；在共享产能/物料/交期互斥下，至少一对内积为负即产生正废热，多对强负则进入强相消区。")

    add_h2("3.4 刚性剪枝与复杂度降维")
    add_p("定理 2：在计算图极早期嵌入刚性约束指示函数：")
    add_eq("$$\\mathbb{I}_{\\mathcal{C}}(\\mathbf{x}) = \\begin{cases} 1, & \\mathbf{x}\\in\\mathcal{M}_{\\text{feasible}} \\\\ 0, & \\text{否则} \\end{cases}$$")
    add_p("刚性指示函数在极早期剔除不可行分支，在最密约束下将阶乘级搜索空间 $O(N!)$ 直接压制为紧致流形上的多项式/树形剪枝规模上界 $O(b^L)$，其中 $b$ 为单节点低阶有效分配数、$L$ 为 BOM/工序层数。该上界是搜索空间测度，而非最坏运行时间。IPC 在 DAG 拓扑序上做容量/齐套前缀剪枝与连续数组线性扫描，全量消纳的运行时间复杂度为 $O(N \\log N)$；其实测常数受 L3 Cache 与内存带宽影响。有限步收敛由离散网格单调下降引理（见附录 A）保证，终点为可行域内近似最优解。若以松弛线性规划或拉格朗日下界为基准，相对下界的 optimality gap < 1.2%。")
    add_p("奥卡姆说明：本文不引入 Banach 压缩映射表述。有限网格上的单调下降已足以论证有限步终止，且更贴合离散物料与整数产能的物理设定。", bold_prefix="奥卡姆说明：")

    # Section 4
    add_h1("4. 实证验证")
    add_h2("4.1 研究设计：22 年纵向行动研究")
    add_p("基于作者 2004–2026 年在联想集团及全球 ODM 供应链现场的参与式实践，分三阶段：")
    stages = [
        ("1. 2004–2012：", "观察旧 APS/ERP 部署下的矢量对消与手工账；"),
        ("2. 2012–2020：", "研发闭环集成计划方案（IPS）并在千亿级工厂验证；"),
        ("3. 2020–2026：", "IPC 引擎产品化与算法治理公理化。")
    ]
    for pref, body in stages:
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Pt(18)
        p.paragraph_format.space_after = Pt(4)
        r_p = p.add_run(pref)
        r_p.font.bold = True
        set_font_eastasia(r_p, '黑体')
        r_b = p.add_run(body)
        set_font_eastasia(r_b, '宋体')
    add_p("本文仅将相关企业视为实践场与纵向案例，不评价企业本身；作者与企业的隶属关系已在文末声明。")

    add_h2("4.2 同构基准平台下的求解器对比")
    add_p("IPC 引擎采用数据导向设计（DOD）：64 字节 Cache-Line 对齐与 SIMD 矢量化。在同一数据集、同一约束集、同一时间窗下对比（表 1）。")

    # Table 1
    p_t1_title = doc.add_paragraph()
    p_t1_title.paragraph_format.space_before = Pt(8)
    p_t1_title.paragraph_format.space_after = Pt(4)
    r_t1 = p_t1_title.add_run("表 1：求解器与范式性能对比表")
    r_t1.font.bold = True
    set_font_eastasia(r_t1, '黑体')

    t1_data = [
        ["求解器 / 范式", "收敛状态", "耗时", "内存占用", "刚性违约 / 质量表现"],
        ["Gurobi MILP 10.0", "超时发散 (gap > 84%)", "> 24h", "> 64GB OOM", "无法输出可行解"],
        ["经典商业 APS", "中断 / 局部规则", "4.5h", "18GB", "缺料违约"],
        ["纯概率 LLM", "轨迹幻觉", "1.2h", "12GB", "负库存 / 超产能"],
        ["IPC 物理 AI", "近似最优", "296s", "8.4GB", "零违约（松弛下界 gap < 1.2%）"]
    ]
    t1 = doc.add_table(rows=len(t1_data), cols=5)
    t1.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(t1)
    
    col_widths1 = [Inches(1.5), Inches(1.5), Inches(0.8), Inches(1.0), Inches(1.7)]
    for r_idx, row in enumerate(t1.rows):
        is_header = (r_idx == 0)
        is_ipc = (r_idx == 4)
        for c_idx, cell in enumerate(row.cells):
            cell.width = col_widths1[c_idx]
            cell.text = t1_data[r_idx][c_idx]
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.space_before = Pt(0)
            p.paragraph_format.line_spacing = 1.1
            set_cell_margins(cell, top=100, bottom=100, left=120, right=120)
            
            run = p.runs[0]
            run.font.size = Pt(9.5)
            if is_header:
                set_cell_background(cell, "EAECEF")
                run.font.bold = True
                set_font_eastasia(run, '黑体')
            elif is_ipc:
                set_cell_background(cell, "E8F4FD")
                run.font.bold = True
                run.font.color.rgb = RGBColor(0x00, 0x40, 0x85)
                set_font_eastasia(run, '宋体')
            else:
                set_font_eastasia(run, '宋体')

    p_t1_note = doc.add_paragraph()
    p_t1_note.paragraph_format.space_before = Pt(4)
    p_t1_note.paragraph_format.space_after = Pt(6)
    r_n = p_t1_note.add_run("*注：全量 50 万需求单、同一 BOM/工艺/库存初值、同一硬性约束（产能、齐套、交期、契约），同一时间窗；Gurobi 采用标准 MILP 建模、默认参数与合理时间片，APS/LLM 按公开最佳实践配置；所有实验在双路 AMD EPYC 7763、256GB RAM、相同 OS 下完成。IPC 不依赖人工事后修单。")
    r_n.font.size = Pt(8.5)
    r_n.font.color.rgb = RGBColor(0x66, 0x66, 0x66)
    set_font_eastasia(r_n, '宋体')

    add_p("Scaling 说明：10 万单 20.4s、50 万单 296s，规模 5 倍而耗时增约 14.5 倍，运行时间仍远低于二次；额外常数主要来自超大规模稀疏张量越过 L3 Cache 后的内存带宽与 NUMA 访问，不改变 $O(N \\log N)$ 的渐近结论。", bold_prefix="Scaling 说明：")

    add_h2("4.3 三个自然观测案例")

    # Table 2
    p_t2_title = doc.add_paragraph()
    p_t2_title.paragraph_format.space_before = Pt(8)
    p_t2_title.paragraph_format.space_after = Pt(4)
    r_t2 = p_t2_title.add_run("表 2：三个自然观测案例对比表")
    r_t2.font.bold = True
    set_font_eastasia(r_t2, '黑体')

    t2_data = [
        ["案例分组", "治理结构", "ROIC", "OTD 交付率", "库存周转天数", "月均停线频次"],
        ["A 存续", "结构完整，接班维护", "+4.2%", "98.4%", "14.2 天", "< 2 次"],
        ["B 离体", "骨干外迁，异构重建", "-2.1%", "84.1%", "28.6 天", "> 15 次"],
        ["C 退化", "退回科层分块 S&OP", "-5.8%", "79.5%", "36.1 天", "> 22 次"]
    ]
    t2 = doc.add_table(rows=len(t2_data), cols=6)
    t2.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(t2)
    
    col_widths2 = [Inches(1.0), Inches(1.8), Inches(0.8), Inches(1.0), Inches(1.0), Inches(0.9)]
    for r_idx, row in enumerate(t2.rows):
        is_header = (r_idx == 0)
        is_a = (r_idx == 1)
        for c_idx, cell in enumerate(row.cells):
            cell.width = col_widths2[c_idx]
            cell.text = t2_data[r_idx][c_idx]
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.space_before = Pt(0)
            p.paragraph_format.line_spacing = 1.1
            set_cell_margins(cell, top=100, bottom=100, left=120, right=120)
            
            run = p.runs[0]
            run.font.size = Pt(9.5)
            if is_header:
                set_cell_background(cell, "EAECEF")
                run.font.bold = True
                set_font_eastasia(run, '黑体')
            elif is_a:
                set_cell_background(cell, "EBFEEF")
                run.font.bold = True
                run.font.color.rgb = RGBColor(0x15, 0x57, 0x24)
                set_font_eastasia(run, '宋体')
            else:
                set_font_eastasia(run, '宋体')

    add_p("选择偏差说明：IPC 试点优先部署于复杂度高、交期差的产线，故 A 组改善可能含均值回归成分。本文以产线自身前后对照 + 同厂未覆盖产线为参考组，多维指标方向一致；脱敏聚合数据可在签署 NDA 后供评审复核。本表为纵向自然观测而非随机试验，结论表述为“验证”而非“因果识别”。", bold_prefix="选择偏差说明：")

    # Section 5
    add_h1("5. 架构治理：三位一体能力与医学院模式")
    
    gov_items = [
        ("•  人机分工（定律八 + 定律二）：", "碳基大脑负责定界与立法（第一性原理、安全红线、公理重写）；硅基系统在刚性约束流形内代偿人脑带宽，执行剪枝与自动化反写。"),
        ("•  三位一体融合架构能力（定律四）：", "传统科层切块无法应对 Non-IID 强耦合，企业必须建立全局自洽的数据—算法中枢。三位一体能力：① 业务解码能力（穿透流程，构建微观要素有向因果图谱）；② 系统建模能力（映射为五维可收敛状态流形）；③ 闭环协同与反写能力（指令自动化反写物理产线，偏差驱动自愈）。"),
        ("•  医学院模式：", "① 完全开箱（Open-Box）交付可编译代码；② 双重团队培养（业务 + 算法）；③ 赋予企业长期抗熵与自愈的数字主权。它与“卖黑盒、按年订阅”的传统模式根本对立。")
    ]
    for pref, body in gov_items:
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Pt(18)
        p.paragraph_format.space_after = Pt(4)
        r_p = p.add_run(pref)
        r_p.font.bold = True
        set_font_eastasia(r_p, '黑体')
        r_b = p.add_run(body)
        set_font_eastasia(r_b, '宋体')

    # Section 6
    add_h1("6. 结论")
    add_p("资本回报悖论的根源，在于用 IID 旧范式求解 Non-IID 新物理。本文提出并形式化了从目的论到进化论的八大物理定律，形式化了矢量相消定理与协调废热度量，以刚性剪枝实现复杂度降维与有限步收敛；22 年纵向证据与同构硬件基准共同表明，IPC 引擎可在标准服务器上 296 秒闭环消纳 50 万订单。管理启示是：数字化转型的本质不是购买软件，而是建立以数据模型与业务算法为核心的算法治理体系，重构组织边界。")
    add_p("局限与展望：本文聚焦离散制造，未来拓展至流程—离散混合与跨国贸易合规网状调度；八律中的能力论、进化论形式化仍需更大样本的实证检验。", bold_prefix="局限与展望：")

    # Appendix
    add_h1("附录 A：核心证明")
    p_ap1 = doc.add_paragraph()
    p_ap1.paragraph_format.left_indent = Pt(18)
    p_ap1.paragraph_format.space_after = Pt(4)
    r_ap1 = p_ap1.add_run("•  定理 1 证明：")
    r_ap1.font.bold = True
    set_font_eastasia(r_ap1, '黑体')
    r_ap1_b = p_ap1.add_run("由范数展开：$\\|\\sum_k \\mathbf{u}_k\\|^2 = \\sum_k\\|\\mathbf{u}_k\\|^2 + 2\\sum_{i<j}\\langle\\mathbf{u}_i,\\mathbf{u}_j\\rangle$。若所有对内积满足 $\\langle\\mathbf{u}_i,\\mathbf{u}_j\\rangle \\le -\\epsilon$（$\\epsilon > 0$），则 $\\|\\sum_k \\mathbf{u}_k\\|^2 \\le \\sum \\|\\mathbf{u}_k\\|^2 - 2\\epsilon \\binom{K}{2}$，故 $Q_{\\diss} > 0$。若仅部分内积为负，则仍按上式取实际交叉项和，$Q_{\\diss} \\ge 0$，等号仅当全非负共线。■")
    set_font_eastasia(r_ap1_b, '宋体')

    p_ap2 = doc.add_paragraph()
    p_ap2.paragraph_format.left_indent = Pt(18)
    p_ap2.paragraph_format.space_after = Pt(6)
    r_ap2 = p_ap2.add_run("•  有限步引理证明：")
    r_ap2.font.bold = True
    set_font_eastasia(r_ap2, '黑体')
    r_ap2_b = p_ap2.add_run("设加权失配范数取值于由物料分配整数性与容量离散化决定的有限网格 $\\mathcal{G} \\subset \\mathcal{X}_{\\text{feasible}}$，网格点数为 $|\\mathcal{G}| < \\infty$。因 IPC 算法每次迭代严格减小失配范数（除非已达最优），故至多经 $|\\mathcal{G}|$ 步后失配范数不再下降，算法终止。结合终止准则 $d(\\mathbf{x}^{(n+1)}, \\mathbf{x}^{(n)}) < \\epsilon$，收敛步数 $n^* \\le |\\mathcal{G}|$。■")
    set_font_eastasia(r_ap2_b, '宋体')

    p_ap_note = doc.add_paragraph()
    p_ap_note.paragraph_format.space_after = Pt(12)
    r_apn = p_ap_note.add_run("*其余收敛细节、伪代码与消融结果见 SSRN 预印本（Abstract ID 7251098）。*")
    r_apn.font.italic = True
    r_apn.font.size = Pt(9.5)
    set_font_eastasia(r_apn, '宋体')

    # Section 7
    add_h1("7. 声明与说明")
    p_dec1 = doc.add_paragraph()
    p_dec1.paragraph_format.left_indent = Pt(18)
    p_dec1.paragraph_format.space_after = Pt(4)
    r_d1 = p_dec1.add_run("•  利益冲突与数据可用性：")
    r_d1.font.bold = True
    set_font_eastasia(r_d1, '黑体')
    r_d1_b = p_dec1.add_run("本研究基于 22 年参与式纵向行动研究，仅将相关企业视为实践场与案例对象，回避单点企业评价；田野工作期间作者任职于相关企业，现为独立研究员/IPC总设计部。脱敏聚合指标与算法结构可按规定复核，原始商业数据因保密协议（NDA）限制按需提供。")
    set_font_eastasia(r_d1_b, '宋体')

    p_dec2 = doc.add_paragraph()
    p_dec2.paragraph_format.left_indent = Pt(18)
    p_dec2.paragraph_format.space_after = Pt(12)
    r_d2 = p_dec2.add_run("•  预印本说明：")
    r_d2.font.bold = True
    set_font_eastasia(r_d2, '黑体')
    r_d2_b = p_dec2.add_run("本文内容与 SSRN 预印本（Abstract ID 7251098）保持一致，算法细节与消融实验详见预印本附录。")
    set_font_eastasia(r_d2_b, '宋体')

    # References
    add_h1("参考文献")
    refs = [
        "钱学森, 于景元, 戴汝为. 1990. \"一个科学新领域——开放的复杂巨系统及其方法论.\" 自然杂志, 13(1), pp. 3-10.",
        "Ashby, W. R. 1956. An Introduction to Cybernetics. Chapman & Hall.",
        "Brynjolfsson, E. 1993. \"The productivity paradox of information technology.\" Communications of the ACM, 36(12), pp. 66-77.",
        "Cao, L. 2014. \"Non-IIDness learning in behavioral and social data.\" The Computer Journal, 57(9), pp. 1358-1370. https://doi.org/10.1093/comjnl/bxt084",
        "Cao, L. 2022. \"Beyond i.i.d.: Non-IID thinking, informatics, and learning.\" IEEE Intelligent Systems, 37(4), pp. 5-17. https://doi.org/10.1109/MIS.2022.3194618",
        "Kalman, R. E. 1960. \"On the general theory of control systems.\" Proceedings of the 1st IFAC Congress, Moscow.",
        "Miller, G. A. 1956. \"The magical number seven, plus or minus two: Some limits on our capacity for processing information.\" Psychological Review, 63(2), pp. 81-97.",
        "Wolfram, S. 2002. A New Kind of Science. Wolfram Media."
    ]
    for idx, ref in enumerate(refs, 1):
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Pt(18)
        p.paragraph_format.space_after = Pt(3)
        p.paragraph_format.line_spacing = 1.15
        r_num = p.add_run(f"[{idx}] ")
        r_num.font.size = Pt(9.5)
        r_num.font.bold = True
        r_txt = p.add_run(ref)
        r_txt.font.size = Pt(9.5)
        set_font_eastasia(r_txt, '宋体')

    try:
        doc.save("value_chain_physics_scm_paper_draft_zh.docx")
        print("Successfully built value_chain_physics_scm_paper_draft_zh.docx!")
    except PermissionError:
        doc.save("value_chain_physics_scm_paper_draft_zh_v2.docx")
        print("File was locked by Word, saved to value_chain_physics_scm_paper_draft_zh_v2.docx!")

if __name__ == "__main__":
    build_docx()
