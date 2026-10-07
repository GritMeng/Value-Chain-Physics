import os
import matplotlib.pyplot as plt
import matplotlib.patches as patches

base_dir = r"H:\系统科学\价值链物理学"
zh_img_path = os.path.join(base_dir, "vcp_axioms_logic_chain_zh.png")
en_img_path = os.path.join(base_dir, "vcp_axioms_logic_chain_en.png")

plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei', 'Arial']
plt.rcParams['axes.unicode_minus'] = False

def create_diagram(is_english=False):
    # Slightly taller figure for ample spacing
    fig, ax = plt.subplots(figsize=(11, 15), dpi=300)
    fig.patch.set_facecolor('#FFFFFF')
    ax.set_facecolor('#FFFFFF')
    ax.axis('off')
    
    # Header Title
    title_text = "Logic Chain of Value Chain Physics (VCP) Eight-Axiom System" if is_english else "价值链物理学定性八大公理体系逻辑推演链条"
    subtitle_text = "From Teleology to Evolution: Formal Structural Derivation" if is_english else "从目的论到进化论：开放复杂巨系统的形式化结构推演"
    
    plt.text(0.5, 0.965, title_text, ha='center', va='center', fontsize=16, fontweight='bold', color='#0F2C59')
    plt.text(0.5, 0.942, subtitle_text, ha='center', va='center', fontsize=11, fontstyle='italic', color='#415A77')
    
    # First Principle Box at Top
    top_title = "FIRST PRINCIPLE: Non-IID Strongly Coupled Physical Reality" if is_english else "第一性原理：非独立同分布 (Non-IID) 强相干物理真实"
    p_top = patches.FancyBboxPatch((0.08, 0.87), 0.84, 0.045, boxstyle="round,pad=0.01", fc='#0F2C59', ec='#0F2C59', lw=1.5)
    ax.add_patch(p_top)
    plt.text(0.5, 0.8925, top_title, ha='center', va='center', fontsize=11.5, fontweight='bold', color='#FFFFFF')
    
    # Connector Arrow from Top
    ax.annotate('', xy=(0.5, 0.835), xytext=(0.5, 0.87),
                arrowprops=dict(facecolor='#0F2C59', edgecolor='#0F2C59', width=2, headwidth=7))
    
    # 8 Axioms Items Definition
    axioms_zh = [
        ("公理 3.1 目的论 (Teleology: Effective Work Maximization)", "消除组织内耗废热，实现全局统御，最大化全域抗熵有效做功 W_eff"),
        ("公理 3.2 本质论 (Ontology: Non-IID Coherent Coupling)", "认清治理面临的底层物理障碍是【非独立同分布 Non-IID】强相干耦合"),
        ("公理 3.3 方案论 (Methodology: Digital Dual-Helix Self-Healing)", "构建【数字双螺旋】高频自愈算子，要求求解重算频率 f_compute > f_perturbation"),
        ("公理 3.4 能力论 (Capability: Single-Brain Decision Singularity)", "确立【单脑决策奇点】，以硅基算力代偿碳基信道极限 (沟通博弈从 O(K^2) 降至 O(1))"),
        ("公理 3.5 机制论 (Mechanism: Topological Fractal Isomorphism)", "实施【拓扑分形同构】，实现中央配额隔离确权与微观节点自适应自治"),
        ("公理 3.6 路径论 (Pathology: Wiener Control Boundary)", "坚守【维纳边界】，保证计划控制维度受限于物理反写硬性控制权 C_control"),
        ("公理 3.7 动力论 (Dynamics: Shadow Price Resonance)", "保持【影子价格共振】，引导行政管理资源配置与拉格朗日影子价格 λ_j 共振"),
        ("公理 3.8 进化论 (Evolution: Meta-Cognitive Reconstruction)", "引入【元认知重构算子 Φ】，当可行解域 C_D 为空集时从环外跳入环中驱动公理演化")
    ]
    
    axioms_en = [
        ("Axiom 3.1 Teleology (Effective Work Maximization)", "Eliminate organizational friction heat, achieve global governance, and maximize anti-entropy work W_eff."),
        ("Axiom 3.2 Ontology (Non-IID Coherent Coupling)", "Recognize that the fundamental physical barrier is Non-IID topological strong coupling."),
        ("Axiom 3.3 Methodology (Digital Dual-Helix Self-Healing)", "Deploy high-frequency self-healing operators, ensuring f_compute > f_perturbation."),
        ("Axiom 3.4 Capability (Single-Brain Decision Singularity)", "Establish central planning singularity, reducing multi-agent negotiation noise from O(K^2) to O(1)."),
        ("Axiom 3.5 Mechanism (Topological Fractal Isomorphism)", "Maintain central quota boundary entitlement while enabling microscopic node adaptive optimization."),
        ("Axiom 3.6 Pathology (Wiener Control Boundary)", "Enforce control capability bounded by physical write-back authority: dim C_control <= dim O_observability."),
        ("Axiom 3.7 Dynamics (Shadow Price Resonance)", "Align executive management resource allocation with Lagrangian shadow prices lambda_j."),
        ("Axiom 3.8 Evolution (Meta-Cognitive Reconstruction)", "Trigger meta-cognitive operator Phi when feasible domain is empty to reconstruct axiom set.")
    ]
    
    axioms = axioms_en if is_english else axioms_zh
    
    start_y = 0.77
    box_height = 0.058
    gap_y = 0.038
    
    for i, (head, desc) in enumerate(axioms):
        curr_y = start_y - i * (box_height + gap_y)
        
        # Alternate box background styling
        bg_color = '#F8FAFC' if i % 2 == 0 else '#F1F5F9'
        border_color = '#0F2C59' if i % 2 == 0 else '#334155'
        
        card = patches.FancyBboxPatch((0.08, curr_y), 0.84, box_height, boxstyle="round,pad=0.008", 
                                     fc=bg_color, ec=border_color, lw=1.2)
        ax.add_patch(card)
        
        # Top Line: Bold Title Header
        plt.text(0.105, curr_y + 0.039, head, ha='left', va='center', 
                 fontsize=10.5, fontweight='bold', color='#0F2C59')
        
        # Bottom Line: Wrapped Description
        plt.text(0.105, curr_y + 0.017, desc, ha='left', va='center', 
                 fontsize=9, color='#334155')
        
        # Arrow to Next Card (if not last)
        if i < len(axioms) - 1:
            arrow_start_y = curr_y
            arrow_end_y = curr_y - gap_y
            ax.annotate('', xy=(0.5, arrow_end_y + 0.003), xytext=(0.5, arrow_start_y),
                        arrowprops=dict(facecolor='#0F2C59', edgecolor='#0F2C59', width=1.5, headwidth=5))

    out_file = en_img_path if is_english else zh_img_path
    plt.savefig(out_file, bbox_inches='tight', dpi=300)
    plt.close()
    print(f"Generated publication diagram without overlap: {out_file}")

def main():
    create_diagram(is_english=False)
    create_diagram(is_english=True)

if __name__ == "__main__":
    main()
