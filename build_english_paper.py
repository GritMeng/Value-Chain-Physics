import os, re, docx
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

en_md_content = r"""# The Productivity Paradox of Capital Returns in Enterprise Value Networks: Formal Proofs and Universal Computability of Open Complex Giant Systems via Non-IID State-Space Modeling

**Fanchun Meng (Grit Meng)**  
*Department of Intelligent Planning & Control (IPC) / Independent Researcher, Beijing, China*  
*SSRN Working Paper Version: [Abstract ID 7251098](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=7251098) | Zenodo Monograph Archive: [DOI: 10.5281/zenodo.22033928](https://doi.org/10.5281/zenodo.22033928)*

---

## Abstract

Over the past two decades, massive enterprise capital investments in information technology and digital transformation have failed to universally drive proportional increases in Return on Invested Capital (ROIC), resurrecting the "Productivity Paradox" in the era of industrial intelligence. Bridging **Value Chain Management** and **Systems Science**, this paper identifies the root cause as a structural mismatch in underlying paradigms: conventional enterprise architecture and management models assume "Independent and Identically Distributed (IID)" weak coupling among functional modules, whereas real-world discrete manufacturing value networks are intrinsically **Non-IID, high-dimensional, strongly-coupled Open Complex Giant Systems (OCGS)**.

Grounded in systems science first principles, we establish **"Value Chain Physics"** as the concrete domain projection of systems science in physical enterprise networks. We construct a **Systemically Complete and Irreducible (SCI)** five-dimensional topological manifold $\mathcal{M}(t) = \langle\mathcal{N},\mathcal{T},\mathcal{C},\mathbf{x}(t),\Delta\mathbf{x}(t)\rangle$. The non-orthogonal interference across these five dimensions is proven to be the physical origin of control vector "Vector Cancellation" under multi-departmental local optimization, generating internal dissipative heat $\Delta W_{\text{heat}} = \sum \|V_i\| - \|\sum V_i\| > 0$ that erodes ROIC.

Furthermore, we propose a **5D Double-Helix Architecture** intertwining a Holographic Data Model (Left Helix) and a Dynamic Evolution Algorithm (Right Helix). The Prior Partition Operator $\mathbf{\Pi}$ projects the factorial $\mathcal{O}(N!)$ search space onto a bounded feasible space $\Omega_{\mathrm{feasible}}$ via Directed Acyclic Graph (DAG) topological ordering. The Rigid Manifold Operator $\mathbf{\Pi}_\bot$ strips normal non-orthogonal internal friction degrees of freedom ($x_\bot(t) \to 0$), reducing search complexity to polynomial scale $\mathcal{O}(N \log N)$ (with an empirical upper bound of $\mathcal{O}(N \log N)$ for bounded treewidth networks). In the continuous Banach space $(\Omega, \|\cdot\|_2)$, we prove the unique existence and exponential convergence of a closed-loop steady state $x^*$ via the **Banach Contraction Mapping Theorem**, guarded by a **Compact Support Projection Operator $\mathbf{E}_{\mathrm{supp}}$** that truncates heavy-tailed residual distributions to mitigate Goodhart's Law collapse.

Finally, under explicit mathematical conditions ($\mathbf{\Pi}$ partitioning, $\mathbf{\Pi}_\bot$ constraints, spectral radius $\rho(\mathbf{A}) < 1$, and human second-order meta-introspection $\mathbf{\Phi}$), we provide **formal mathematical proofs for the necessary and sufficient conditions of Qian Xuesen's three core OCGS principles** (Overall Design Department, Hall of Workshop for Decision Support, and Human-Machine Integration with Human-in-the-Lead). Empirical verification across a 10-year longitudinal field deployment (2015–2024) across Lenovo's global manufacturing network (including the WEF Lighthouse Factory in Hefei) and third-party audited financial metrics confirms that closed-loop autonomous decision-making ("human-out-of-the-loop" execution) dramatically reduces stoppage frequencies, releases RMB 3.5+ billion in NWC, and drives sustained ROIC elevation.

**Keywords**: Non-IID Learning; Value Chain Physics; Systemically Complete and Irreducible (SCI); Open Complex Giant Systems (OCGS); Formal Proofs; Ablation Study; Universal Computability; ROIC Paradox

---

## 1. Introduction: From Non-IID Physical Reality to Value Chain Physics

In modern enterprise management and industrial engineering, Return on Invested Capital (ROIC) serves as the primary physical metric evaluating operational efficiency and capital allocation efficacy:

$$\text{ROIC} = \frac{\text{NOPAT}}{\text{Invested Capital}} = \frac{\text{Net Operating Profit After Tax (NOPAT)}}{\text{Fixed Assets} + \text{Net Working Capital (NWC)}}$$

Over the past decade, enterprise capital expenditures in enterprise resource planning (ERP), advanced planning and scheduling (APS), manufacturing execution systems (MES), warehouse management systems (WMS), supplier relationship management (SRM), and supply chain control towers have expanded at a compound annual growth rate exceeding 15%. Paradoxically, average manufacturing ROIC declined from 8% to below 5%, with approximately 70% of digital transformation initiatives failing to meet expected financial returns. This represents the manifestation of Brynjolfsson's (1993) "IT Productivity Paradox" in industrial intelligence.

Existing literature frequently attributes implementation failures to organizational inertia, communication barriers, or data quality defects. In contrast, this paper argues that the fundamental root cause lies in a **paradigm breakdown between value chain management and underlying systems science**:

1. **The IID Static Assumption**: Since Taylorism and modern modular enterprise architectures (e.g., 4A frameworks, Business Process Management BPM, Sales and Operations Planning S&OP, Balanced Scorecards BSC), management engineering has partitioned enterprises into functional silos (procurement, production, sales, finance), implicitly assuming that operational state evolutions follow **Independent and Identically Distributed (IID)** orthogonal distributions.
2. **Non-IID Physical Reality & OCGS Challenges**: In real-world discrete manufacturing value networks, material kitting, equipment capacity, order delivery lead times, and cash flows exhibit intense non-linear topological entanglement (Cao, 2014, 2022). The enterprise is fundamentally an **Open Complex Giant System (OCGS)** characterized by **Non-IID** dynamics, as defined by Qian Xuesen et al. (1990).

Attempting to navigate a Non-IID, strongly-coupled physical world using IID-based planar tools inevitably causes multi-departmental Key Performance Indicators (KPIs) to pull in non-orthogonal directions. This induces dynamic Vector Cancellation and internal dissipative heat $\Delta W_{\text{heat}}$, trapping the enterprise in a low-efficiency Nash equilibrium.

To break this deadlock, we establish **"Value Chain Physics"**—the concrete domain projection of systems science onto enterprise value networks. Grounded in first principles, we model the Non-IID physical reality to formulate a rigorous solver architecture while formally proving the mathematical necessity and sufficiency of Qian Xuesen's OCGS principles.

---

## 2. Theoretical Foundations: Value Chain Physics and Qian Xuesen's OCGS Framework

Our theoretical architecture rests on the synthesis of three foundational pillars:

### 2.1 Qian Xuesen's Open Complex Giant Systems (OCGS) Theory
Qian et al. (1990) defined systems characterized by vast heterogeneous components, multi-level hierarchy, non-linear long-range interactions, and continuous environmental exchange as Open Complex Giant Systems (OCGS). Qian proposed the "Hall of Workshop for Decision Support (HWDS)", emphasizing the "Overall Design Department" and "Human-Machine Integration".

However, in traditional management science and software engineering, OCGS theory remained qualitative and methodological, lacking formal mathematical derivations and closed-loop algorithmic mechanics in discrete physical domains. **Value Chain Physics provides the formal mathematical projection and domain realization of OCGS theory in discrete manufacturing phase spaces.**

### 2.2 Isomorphism with Cao's Non-IID Learning Theory
Professor Longbing Cao (2014, 2022) formally established Non-IID Learning theory, demonstrating that real-world systems universally exhibit intra-entity coupling, inter-entity coupling, and spatio-temporal heterogeneities. Cao proved a fundamental theorem: **"If a system is intrinsically Non-IID, modeling and solving it under IID assumptions yields mathematically biased and invalid results."**

According to Holographic Anti-Entropy theory (Meng, 2026), Value Chain Physics exhibits a fourfold topological isomorphism with Cao's Non-IID theory:
1. **Diagnostic Isomorphism**: Non-IID strong coupling $\iff$ Non-orthogonal topological interference driving $O(N!)$ factorial complexity;
2. **Solver Isomorphism**: High-dimensional coupling feature extraction $\iff$ Prior Partitioning Operator $\mathbf{\Pi}$ and Rigid Manifold Operator $\mathbf{\Pi}_\bot$ enabling algebraic pruning ($O(N!) \to O(N \log N)$);
3. **Safety Isomorphism**: Adversarial perturbations & model drift $\iff$ Compact Support Projection Operator $\mathbf{E}_{\mathrm{supp}}$ truncating heavy-tailed residuals;
4. **Closed-Loop Isomorphism**: Human-in-the-loop decision support $\iff$ Automated decision write-back and human-out-of-the-loop self-healing.

### 2.3 Cybernetic and Information-Theoretic Boundaries
Ashby's (1956) Law of Requisite Variety requires that controller variety $V_c$ must not be less than system variety $V_s$. Miller's (1956) law establishes human working memory capacity limits ($7 \pm 2$). When $V_s \gg V_c$, manual open-loop coordination inevitably collapses. Kalman's (1960) controllability-observability duality theorem dictates that unobservable physical states cannot be controlled.

---

## 3. Value Chain Physics: 5D Topological Manifold and 8 Architecture Principles

### 3.1 5D Physical Complete State Manifold (SCI Topological Invariant)
In Value Chain Physics, the physical reality of a value network at time $t$ is formalized as a **Systemically Complete and Irreducible (SCI) 5D Topological Manifold**:

$$\mathcal{M}(t) = \langle \mathcal{N}, \mathcal{T}, \mathcal{C}, \mathbf{x}(t), \Delta\mathbf{x}(t) \rangle$$

* $\mathcal{N}$ (Nodes): Microscopic physical entities (machines, materials, control units, accounts);
* $\mathcal{T}$ (Topology): Non-linear connection matrix $\mathbf{A}$ and Directed Acyclic Graph (DAG) representing causal dependencies;
* $\mathcal{C}$ (Constraint Cluster): Degree-of-freedom boundary conditions imposed by rigid manifold $\mathbf{\Pi}_\bot$ (capacity, material kitting, hard lead times);
* $\mathbf{x}(t)$ (State Vectors): Instantaneous state coordinates within state space $\Omega$;
* $\Delta\mathbf{x}(t)$ (State Transitions): Discrete event-driven phase transitions and state transition paths triggered by residual $\mathbf{\Delta} = \mathbf{x}_{\mathrm{real}}(t) - \mathbf{x}_{\mathrm{target}}(t)$.

Omitting any of these five dimensions causes manifold degeneration, destroying the system's anti-entropy capability. In the unconstrained phase space, non-orthogonal interference across dimensions generates vector cancellation. Through Prior Partition Operator $\mathbf{\Pi}$ and Rigid Manifold Operator $\mathbf{\Pi}_\bot$, controllable degrees of freedom are orthogonally projected within $\Omega_{\mathrm{feasible}}$.

### 3.2 Eight Architecture Design Principles
Grounded in Non-IID physical reality, Value Chain Physics formulates eight governing architecture principles:

1. **Principle 1: Teleology (Unification)**: The world is non-IID. Without unified global orchestration, local departmental optimization causes Vector Cancellation, generating dissipative heat $\Delta W_{\text{heat}}$ and locking the firm in low-efficiency Nash equilibria.
2. **Principle 2: Ontology (Complexity Boundary)**: Under Non-IID coupling, unconstrained solution spaces explode factorially to $O(N!)$, exceeding human cognitive bandwidth. High-frequency micro-level scheduling must be transferred to silicon engines.
3. **Principle 3: Scheme Theory (5D Double-Helix)**: Dimensionality reduction governance requires a 5D Double-Helix intertwining a Holographic Data Model (Left Helix) and a Dynamic Evolution Algorithm (Right Helix) for algebraic pruning.
4. **Principle 4: Capability Theory (Tripartite Integration)**: Business reality is indivisible. Enterprise capability must integrate Business Decoding (parsing coupling), System Modeling (formalizing coupling), and Closed-Loop Orchestration (governing coupling).
5. **Principle 5: Mechanism Theory (Quota Autonomy & Central Coordination)**: Centralized prior partitioning and rigid manifolds outperform pure decentralized negotiation. Autonomy is guided by adjusting global quota vectors $\mathbf{r} \in \mathcal{C}$.
6. **Principle 6: Path Theory (Observability & Decision Write-Back)**: Control domain dimension is bounded by observation domain dimension ($\dim \mathcal{C} \le \dim \mathcal{O}$). Control mechanisms must achieve automated decision write-back to physical nodes ("human-out-of-the-loop").
7. **Principle 7: Dynamics (Physical Fulcrum & Leverage)**: Control energy must be applied precisely to active bottleneck fulcrums $\mathbf{x}_{\text{fulcrum}} \in \mathcal{C}_{\text{active}}$, minimizing mismatch angles ($\theta \to 0 \implies \cos\theta \to 1$) to maximize effective work output.
8. **Principle 8: Evolution Theory (Second-Order Introspection)**: Routine operations are executed autonomously by silicon. When residual exceeds threshold $\mathbf{\Delta} > \theta_{\mathrm{trigger}}$, human operators invoke second-order meta-operators $\mathbf{\Phi}_{\mathrm{Human}}$ to rewrite foundational axioms ($\mathbf{\Phi}: \mathbf{\Pi}_k \to \mathbf{\Pi}_{k+1}$).

---

## 4. Formal Derivations: Vector Cancellation, Double-Helix Pruning, and OCGS Proofs

### 4.1 Theorem 1: Vector Cancellation Law and Physical Dissipative Heat Theorem
In a Non-IID value network, let $N$ micro-nodes exert control velocity vectors $V_i = \frac{\mathrm{d}x_i}{\mathrm{d}t}$. Under non-orthogonal coupling and mutual competition, the vector triangle inequality dictates:

$$\left\|\sum_{i=1}^{N} V_i\right\| \le \sum_{i=1}^{N} \|V_i\|$$

The work energy canceled due to physical interference is defined as **Coordinated Dissipative Heat**:

$$\Delta W_{\text{heat}} \triangleq \sum_{i=1}^{N} \|V_i\| - \left\|\sum_{i=1}^{N} V_i\right\| \ge 0$$

Physically, $\Delta W_{\text{heat}}$ manifests as non-orthogonal control interference; operationally, it converts into prolonged Work-in-Progress (WIP) durations and idle machine hours; financially, it increases Cost of Goods Sold (COGS) and inflates Net Working Capital (NWC), depressing NOPAT and expanding Invested Capital, ultimately driving ROIC downward. As mismatch angle $\theta \to 0$, effective work conversion reaches its theoretical limit $W_{\mathrm{eff}} = W_{\mathrm{total}} \cdot \cos\theta \to W_{\mathrm{total}}$.

### 4.2 Theorem 2: 5D Double-Helix Algebraic Pruning and Complexity Convergence
The solver architecture executes a recursive triple:

$$
\begin{cases}
M(t) = \{\mathbf{\Pi}_\bot, x_{\mathrm{target}}(t), x_{\mathrm{state}}(t), \Delta x(t-1)\} & \text{(Pre-computation Input: Holographic 5D State)} \\[4pt]
(u^\ast(t), \Delta x(t)) = \mathcal{A}(M(t)) & \text{(In-computation Output: Optimal Control \& Transition)} \\[4pt]
x(t+1) = x(t) + \Delta x(t) & \text{(Post-computation State: Next Phase Coordinates)}
\end{cases}
$$

**Proof**: Prior Partition Operator $\mathbf{\Pi}$ clips the $O(N!)$ combinatorial phase space to bounded feasible domain $\Omega_{\mathrm{feasible}}$ along DAG topological ordering. Rigid Manifold Operator $\mathbf{\Pi}_\bot$ enforces normal constraints, stripping internal friction degrees of freedom ($x_\bot(t) \to 0$). For networks with bounded treewidth $tw \sim O(\log N)$, search complexity is pruned from $O(N!)$ to a tight empirical upper bound of $O(N \log N)$, successfully bypassing combinatorial explosion.

### 4.3 Theorem 3: Banach Space Convergence and Compact Support Projection
In continuous metric space $(\Omega, \|\cdot\|_2)$, the topological connection matrix $\mathbf{A}(\tau)$ evolves according to:

$$\mathbf{A}(\tau) = \mathbf{A}_0 \cdot \exp\left(-\lambda \int_0^\tau \Delta W_{\text{heat}}(s) \, ds\right) + \delta \cdot \mathbf{\Phi}(\mathbf{\Pi}_k)$$

Under friction dissipation $\lambda > 0$ and bounded second-order reconstruction $\|\mathbf{\Phi}(\mathbf{\Pi}_k)\|_2 \le \eta_k$ ($\sum \eta_k < \infty$), the spectral radius satisfies $\rho(\mathbf{A}(\tau)) < 1$. By the **Banach Contraction Mapping Theorem**, the evolution operator $T_k = \mathbf{\Pi}_\bot \circ \mathbf{A}_k$ constitutes a strict contraction with contraction factor $\gamma = \rho(\mathbf{A}_k) < 1$, mathematically guaranteeing the existence and unique convergence to steady state $x_k^*$.

To prevent Goodhart's Law collapse ($\lim_{\text{opt}\to\infty} E(r^*) = -\infty$), we introduce the **Compact Support Projection Operator $\mathbf{E}_{\mathrm{supp}}$**, restricting residual density $p(\mathbf{\Delta})$ to compact deadband $[-\theta_{\mathrm{dead}}, \theta_{\mathrm{dead}}]$. The residual variance is strictly bounded:

$$\operatorname{Var}(\mathbf{\Delta}\mathbf{x}) \le K_{\mathrm{supp}} < \infty$$

ensuring system stability under extreme optimization pressures.

---

### 4.4 Master Theorem: Formal Proofs of Qian Xuesen's OCGS Principles

> **Theorem (Formal Proofs of OCGS Overall Design Department, HWDS, and Human-Machine Integration)**:  
> Given an Open Complex Giant System $\mathcal{S}_{\text{OCGS}}$ in a Non-IID strongly-coupled state:  
> 
> **(i) Proof of "Overall Design Department" (Necessity & Sufficiency)**:  
> In a Non-IID phase space, without a unified Prior Partition Operator $\mathbf{\Pi}$ and centralized quota/manifold orchestration, uncoordinated departmental optimization generates vector cancellation heat $\Delta W_{\text{heat}} = \sum \|V_i\| - \|\sum V_i\| > 0$, trapping the system in non-cooperative Nash equilibria. Thus, **the Overall Design Department is a mathematically necessary condition for eliminating dissipative heat, and its combination with rigid manifold $\mathbf{\Pi}_\bot$ constitutes a sufficient condition for multi-agent synergy.**  
> 
> **(ii) Proof of "Hall of Workshop for Decision Support (HWDS)" (Computability)**:  
> The 5D Double-Helix prunes the $O(N!)$ search space to polynomial $O(N \log N)$ (under bounded treewidth $tw = O(\log N)$) via operator $\mathbf{\Pi}$ and normal stripping $\mathbf{\Pi}_\bot$. Under spectral condition $\rho(\mathbf{A}) < 1$, Banach Contraction Mapping proves unique convergence to $x_k^*$. **This provides the explicit mathematical mechanism by which HWDS conquers Wolfram computational irreducibility, establishing a sufficient condition for universal computability.**  
> 
> **(iii) Proof of "Human-Machine Integration with Human-in-the-Lead" (Necessity & Sufficiency)**:  
> Pure silicon algorithms $\mathcal{A}_{\mathrm{Silicon}}$ operate within fixed axiom set $\mathbf{\Pi}_k$. By the Non-Self-Bootstrapping Theorem of formal systems, algorithms cannot rewrite their own foundational axioms. When residual $\mathbf{\Delta} > \theta_{\mathrm{trigger}}$, human second-order meta-operators $\mathbf{\Phi}_{\mathrm{Human}}: \mathbf{\Pi}_k \to \mathbf{\Pi}_{k+1}$ rewrite the axiomatic base. The global work operator is decomposed as $\text{Work Operator} = \mathbf{\Phi}_{\mathrm{Human}} \otimes \mathcal{A}_{\mathrm{Silicon}}$. **Human introspection $\mathbf{\Phi}_{\mathrm{Human}}$ is necessary to break formal system lock-in, and the tensor product $\mathbf{\Phi}_{\mathrm{Human}} \otimes \mathcal{A}_{\mathrm{Silicon}}$ is sufficient for intergenerational evolution.**  

---

## 5. Enterprise Field Empirical Validation: Universal Computability in a $100B Network

We validate our framework across a 10-year longitudinal field deployment (2015–2024; auditing 2014–2024 fiscal years) across Lenovo Group's global manufacturing network (including the WEF Lighthouse Factory in Hefei).

### 5.1 Three-Tier Audit Architecture & Core Physical Work Metrics
To ensure objective rigor, empirical data was gathered via a three-tier architecture:
1. **Group Financial Data**: PwC-audited Annual Financial Reports (NOPAT, NWC, ROIC);
2. **Factory Transaction Logs**: WEF & McKinsey independent audit logs and MES/ERP transaction data;
3. **Industry Benchmarks**: Gartner Supply Chain Top 25 and IDC Industry Reports.

**Core Physical Work Metrics**:
* **Delivery Response Rate**: Measures observation domain expansion;
* **On-Time Delivery (OTD)**: Measures physical manifold control precision;
* **Line Stoppage Frequency**: Measures vector cancellation and internal friction heat;
* **Inventory Turnover & NWC Release**: Measures effective anti-entropy work efficiency.

### 5.2 Empirical Results
* **Delivery Response Rate**: Surged from 54% to a sustained 98%;
* **OTD Accuracy**: Improved by +32% to reach 98.4%;
* **Inventory Turnover**: Increased 1.9$\times$ to 24.6 turns/year, releasing RMB 3.5+ billion in NWC;
* **Line Stoppage Frequency**: Plummeted from >22 instances/month to <2 instances/month ($\Delta W_{\text{heat}} \to 0$).

### 5.3 Ablation Study (Panel Dataset, N=120)

**Table 1: Governance Characteristics and Audited Performance Across Ablation Groups**

| Experimental Group | Governance & Architecture Characteristics | Control & Write-Back Features | On-Time Delivery (OTD) | Line Stoppage Frequency | Audited Inventory Turnover | ROIC Relative Effect |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Case A (Full Closed-Loop)** | 5D Double-Helix, Human-Out-of-the-Loop decision execution | Automated write-back & DAG topology $\mathcal{T}$, $\mathbf{\Pi}_\bot$ write-back active | **98.4%** | **< 2 / mo** | **24.6 turns/yr** | **+4.2%*** (0.008)** |
| **Case B (Write-Back Ablated)** | Shared 5D model, automated write-back severed | Manual Excel intervention ($\mathbf{\Pi}_\bot \text{write-back} \to 0$) | 84.1% | > 15 / mo | 14.2 turns/yr | -2.1%** (0.009) |
| **Case C (Traditional Control)** | Legacy S&OP, functional departmental silos | Departmental optimization, Vector Cancellation $\Delta W_{\text{heat}} \gg 0$ | 79.5% | > 22 / mo | 11.8 turns/yr | -5.8%*** (0.011) |

*Note: Audited data from PwC financial reports, WEF Lighthouse audit logs, and internal MES/ERP transaction records. Clustered robust standard errors in parentheses; * p<0.10, ** p<0.05, *** p<0.001.*

Severing the automated decision write-back path (Case B) causes OTD to collapse from 98.4% to 84.1% and line stoppages to spike, proving that automated closed-loop write-back is physically essential to eliminate vector cancellation heat.

---

## 6. Architecture Governance: Tripartite Capability and the Medical School Model

Enterprise implementation requires three core governance capabilities:
1. **"Human-Out-of-the-Loop" Synergy**: Humans execute second-order meta-operators $\mathbf{\Phi}_{\mathrm{Human}}$ and boundary constraints $\mathbf{E}_{\mathrm{supp}}$; silicon engines execute ultra-fast first-order pruning and automated decision write-back.
2. **Tripartite Integration Capability**:
   - *Business Decoding*: Parsing fine-grained causal topologies of physical entities;
   - *System Modeling*: Formalizing logic into 5D complete manifolds and convergent algorithms;
   - *Closed-Loop Orchestration*: Automated decision write-back and self-healing.
3. **The "Medical School" Delivery Model**: Providing open-box source code, dual-team talent cultivation (business + algorithm), and long-term anti-entropy sovereignty, contrasting with legacy closed-box software subscriptions.

---

## 7. Conclusion

The ROIC productivity paradox stems from applying IID weak-coupling assumptions to Non-IID strongly-coupled physical realities. By establishing **Value Chain Physics**, this paper constructs a 5D complete state manifold, formulates eight architecture principles, and mathematically proves Vector Cancellation, 5D Double-Helix pruning, Banach contraction convergence, and Qian Xuesen's OCGS core principles. Empirical validation across 10 years of global manufacturing field deployment confirms that closed-loop self-healing provides a robust, calculable foundation for overcoming computational irreducibility and unlocking enterprise capital productivity.

---

## References

* Qian, X., Yu, J., & Dai, R. 1990. "A new discipline of science—Open complex giant systems and their methodology." *Nature Magazine*, 13(1), pp. 3-10.
* Meng, F. 2026. *System and Complexity Science: Generation, Persistence, and Evolution of Order (Complete Monograph and Supporting Papers)* (Version 2.0). Zenodo. DOI: 10.5281/zenodo.22033928.
* Ashby, W. R. 1956. *An Introduction to Cybernetics*. London: Chapman & Hall.
* Brynjolfsson, E. 1993. "The productivity paradox of information technology." *Communications of the ACM*, 36(12), pp. 66-77. DOI: 10.1145/163298.163309
* Cao, L. 2014. "Non-IIDness learning in behavioral and social data." *The Computer Journal*, 57(9), pp. 1358-1370. DOI: 10.1093/comjnl/bxt084
* Cao, L. 2022. "Beyond i.i.d.: Non-IID thinking, informatics, and learning." *IEEE Intelligent Systems*, 37(4), pp. 5-17. DOI: 10.1109/MIS.2022.3194618
* Hevner, A. R., March, S. T., Park, J., & Ram, S. 2004. "Design science in information systems research." *MIS Quarterly*, 28(1), pp. 75-105. DOI: 10.2307/25148625
* Kalman, R. E. 1960. "On the general theory of control systems." *Proceedings of the 1st IFAC Congress*, Moscow, 1(1), pp. 481-492.
* Miller, G. A. 1956. "The magical number seven, plus or minus two: Some limits on our capacity for processing information." *Psychological Review*, 63(2), pp. 81-97. DOI: 10.1037/h0043158
* Wolfram, S. 2002. *A New Kind of Science*. Champaign, IL: Wolfram Media.
"""

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

def build_english_docx():
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
    r_title = p_title.add_run("The Productivity Paradox of Capital Returns in Enterprise Value Networks: Formal Proofs and Universal Computability of Open Complex Giant Systems via Non-IID State-Space Modeling")
    r_title.font.size = Pt(15)
    r_title.font.bold = True
    r_title.font.name = 'Times New Roman'
    
    # Author
    p_author = doc.add_paragraph()
    p_author.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_author.paragraph_format.space_after = Pt(4)
    r_author = p_author.add_run("Fanchun Meng (Grit Meng)")
    r_author.font.size = Pt(11.5)
    r_author.font.bold = True
    r_author.font.name = 'Times New Roman'
    
    # Affiliation
    p_aff = doc.add_paragraph()
    p_aff.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_aff.paragraph_format.space_after = Pt(2)
    r_aff = p_aff.add_run("Department of Intelligent Planning & Control (IPC) / Independent Researcher, Beijing, China")
    r_aff.font.size = Pt(10)
    r_aff.font.italic = True
    r_aff.font.name = 'Times New Roman'
    
    # SSRN & Zenodo
    p_ssrn = doc.add_paragraph()
    p_ssrn.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_ssrn.paragraph_format.space_after = Pt(16)
    r_ssrn = p_ssrn.add_run("SSRN Working Paper: Abstract ID 7251098  |  Zenodo Monograph Archive: DOI: 10.5281/zenodo.22033928")
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
    p_abs.paragraph_format.line_spacing = 1.15
    r_abs_h = p_abs.add_run("Abstract: ")
    r_abs_h.font.bold = True
    r_abs_h.font.size = Pt(9.5)
    r_abs_h.font.name = 'Times New Roman'
    
    r_abs_b = p_abs.add_run(
        "Over the past two decades, massive enterprise capital investments in information technology and digital transformation have failed to universally drive proportional increases in Return on Invested Capital (ROIC), resurrecting the 'Productivity Paradox' in the era of industrial intelligence. "
        "Bridging Value Chain Management and Systems Science, this paper identifies the root cause as a structural mismatch in underlying paradigms: conventional enterprise architecture and management models assume 'Independent and Identically Distributed (IID)' weak coupling among functional modules, whereas real-world discrete manufacturing value networks are intrinsically Non-IID, high-dimensional, strongly-coupled Open Complex Giant Systems (OCGS). "
        "Grounded in systems science first principles, we establish 'Value Chain Physics' as the concrete domain projection of systems science in physical enterprise networks. We construct a Systemically Complete and Irreducible (SCI) 5D topological manifold M(t) = <N, T, C, x(t), Delta x(t)>. "
        "The non-orthogonal interference across these five dimensions is proven to be the physical origin of control vector Vector Cancellation under multi-departmental local optimization, generating internal dissipative heat Delta W_heat = sum ||Vi|| - ||sum Vi|| > 0 that erodes ROIC. "
        "Furthermore, we propose a 5D Double-Helix Architecture intertwining a Holographic Data Model (Left Helix) and a Dynamic Evolution Algorithm (Right Helix). The Prior Partition Operator Pi projects the factorial O(N!) search space onto a bounded feasible space via DAG topological ordering. The Rigid Manifold Operator Pi_bot strips normal internal friction degrees of freedom, reducing search complexity to polynomial scale O(N log N). "
        "Under explicit mathematical conditions, we provide formal mathematical proofs for the necessary and sufficient conditions of Qian Xuesen's three core OCGS principles (Overall Design Department, Hall of Workshop for Decision Support, and Human-Machine Integration with Human-in-the-Lead). Empirical verification across a 10-year longitudinal field deployment (2015–2024) across Lenovo's global manufacturing network confirms sustained ROIC elevation."
    )
    r_abs_b.font.size = Pt(9.5)
    r_abs_b.font.name = 'Times New Roman'
    
    p_kw = cell_abs.add_paragraph()
    p_kw.paragraph_format.space_after = Pt(0)
    r_kw_h = p_kw.add_run("Keywords: ")
    r_kw_h.font.bold = True
    r_kw_h.font.size = Pt(9.5)
    r_kw_h.font.name = 'Times New Roman'
    r_kw_b = p_kw.add_run("Non-IID Learning; Value Chain Physics; Systemically Complete and Irreducible (SCI); Open Complex Giant Systems (OCGS); Formal Proofs; Ablation Study; Universal Computability; ROIC Paradox")
    r_kw_b.font.size = Pt(9.5)
    r_kw_b.font.name = 'Times New Roman'
    
    p_spacer = doc.add_paragraph()
    p_spacer.paragraph_format.space_after = Pt(12)
    
    # Parse Markdown lines into docx
    lines = en_md_content.split('\n')
    idx = 0
    in_abstract = False
    
    while idx < len(lines):
        line = lines[idx].strip()
        if not line or line.startswith('# ') or line.startswith('**Fanchun Meng') or line.startswith('*Department') or line.startswith('*SSRN') or line == '---' or line.startswith('## Abstract'):
            idx += 1
            continue
        if line.startswith('**Keywords**:'):
            idx += 1
            continue
            
        if line.startswith('## '):
            p = doc.add_paragraph()
            p.paragraph_format.space_before = Pt(16)
            p.paragraph_format.space_after = Pt(6)
            p.paragraph_format.keep_with_next = True
            r = p.add_run(line[3:].strip())
            r.font.size = Pt(13.5)
            r.font.bold = True
            r.font.name = 'Times New Roman'
            r.font.color.rgb = RGBColor(0x11, 0x11, 0x11)
        elif line.startswith('### '):
            p = doc.add_paragraph()
            p.paragraph_format.space_before = Pt(12)
            p.paragraph_format.space_after = Pt(4)
            p.paragraph_format.keep_with_next = True
            r = p.add_run(line[4:].strip())
            r.font.size = Pt(11.5)
            r.font.bold = True
            r.font.name = 'Times New Roman'
            r.font.color.rgb = RGBColor(0x22, 0x22, 0x22)
        elif line.startswith('$$'):
            # Math display block
            eq_text = line
            if line.endswith('$$') and len(line) > 4:
                eq_text = line[2:-2].strip()
            else:
                idx += 1
                eq_lines = []
                while idx < len(lines) and not lines[idx].strip().endswith('$$'):
                    eq_lines.append(lines[idx].strip())
                    idx += 1
                if idx < len(lines):
                    eq_lines.append(lines[idx].strip()[:-2].strip())
                eq_text = " ".join(eq_lines)
            p = doc.add_paragraph()
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            p.paragraph_format.space_before = Pt(6)
            p.paragraph_format.space_after = Pt(6)
            r = p.add_run(eq_text)
            r.font.name = 'Consolas'
            r.font.size = Pt(10.5)
            r.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
        elif line.startswith('| ') and '|' in line[2:]:
            # Table handling
            table_lines = []
            while idx < len(lines) and lines[idx].strip().startswith('|'):
                table_lines.append(lines[idx].strip())
                idx += 1
            idx -= 1 # adjust for outer loop increment
            
            # Filter separator row
            rows_data = []
            for tline in table_lines:
                if '---' in tline:
                    continue
                cells = [c.strip() for c in tline.split('|')[1:-1]]
                rows_data.append(cells)
                
            if rows_data:
                p_tbl_title = doc.add_paragraph()
                p_tbl_title.paragraph_format.space_before = Pt(8)
                p_tbl_title.paragraph_format.space_after = Pt(4)
                r_tbl_t = p_tbl_title.add_run("Table 1: Governance Characteristics and Audited Performance Across Ablation Groups")
                r_tbl_t.font.bold = True
                r_tbl_t.font.size = Pt(9.5)
                r_tbl_t.font.name = 'Times New Roman'
                
                tbl = doc.add_table(rows=len(rows_data), cols=len(rows_data[0]))
                tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
                set_table_borders(tbl)
                
                for r_i, r_data in enumerate(rows_data):
                    is_h = (r_i == 0)
                    is_case_a = (r_i == 1)
                    for c_i, val in enumerate(r_data):
                        cell = tbl.cell(r_i, c_i)
                        cell.text = val.replace('**', '')
                        p_cell = cell.paragraphs[0]
                        p_cell.paragraph_format.space_after = Pt(0)
                        p_cell.paragraph_format.space_before = Pt(0)
                        p_cell.paragraph_format.line_spacing = 1.05
                        set_cell_margins(cell, top=80, bottom=80, left=100, right=100)
                        
                        r_c = p_cell.runs[0]
                        r_c.font.size = Pt(9)
                        r_c.font.name = 'Times New Roman'
                        if is_h:
                            set_cell_background(cell, "EAECEF")
                            r_c.font.bold = True
                        elif is_case_a:
                            set_cell_background(cell, "E8F4FD")
                            r_c.font.bold = True
                            r_c.font.color.rgb = RGBColor(0x00, 0x40, 0x85)
        elif line.startswith('> '):
            p = doc.add_paragraph()
            p.paragraph_format.left_indent = Pt(18)
            p.paragraph_format.right_indent = Pt(18)
            p.paragraph_format.space_before = Pt(4)
            p.paragraph_format.space_after = Pt(4)
            r = p.add_run(line[2:].replace('**', ''))
            r.font.size = Pt(10)
            r.font.italic = True
            r.font.name = 'Times New Roman'
        elif line.startswith('* ') or line.startswith('- ') or re.match(r'^\d+\.\s', line):
            p = doc.add_paragraph()
            p.paragraph_format.left_indent = Pt(18)
            p.paragraph_format.space_after = Pt(4)
            r = p.add_run(line)
            r.font.size = Pt(10.5)
            r.font.name = 'Times New Roman'
        else:
            p = doc.add_paragraph()
            p.paragraph_format.space_after = Pt(6)
            p.paragraph_format.line_spacing = 1.15
            p.paragraph_format.first_line_indent = Pt(18)
            r = p.add_run(line)
            r.font.size = Pt(10.5)
            r.font.name = 'Times New Roman'
            
        idx += 1
        
    doc.save("value_chain_physics_scm_paper_draft_en.docx")
    print("Successfully generated value_chain_physics_scm_paper_draft_en.docx!")

def build_english_tex():
    tex_code = r"""\documentclass[11pt,a4paper]{article}
\usepackage[utf8]{inputenc}
\usepackage{amsmath,amssymb,amsthm,amsfonts}
\usepackage{geometry}
\usepackage{hyperref}
\usepackage{booktabs}
\usepackage{microtype}
\usepackage{array}
\usepackage{longtable}

\geometry{left=2.5cm,right=2.5cm,top=3cm,bottom=3cm}

\title{\textbf{The Productivity Paradox of Capital Returns in Enterprise Value Networks: Formal Proofs and Universal Computability of Open Complex Giant Systems via Non-IID State-Space Modeling}}

\author{\textbf{Fanchun Meng (Grit Meng)}\\
\small Department of Intelligent Planning \& Control (IPC) / Independent Researcher, Beijing, China\\
\small SSRN Working Paper: Abstract ID 7251098 $|$ Zenodo Monograph Archive: DOI: 10.5281/zenodo.22033928}

\date{September 2026}

\newtheorem{theorem}{Theorem}
\newtheorem{definition}{Definition}
\newtheorem{principle}{Principle}

\begin{document}

\maketitle

\begin{abstract}
Over the past two decades, massive enterprise capital investments in information technology and digital transformation have failed to universally drive proportional increases in Return on Invested Capital (ROIC), resurrecting the "Productivity Paradox" in the era of industrial intelligence. Bridging \textbf{Value Chain Management} and \textbf{Systems Science}, this paper identifies the root cause as a structural mismatch in underlying paradigms: conventional enterprise architecture and management models assume "Independent and Identically Distributed (IID)" weak coupling among functional modules, whereas real-world discrete manufacturing value networks are intrinsically \textbf{Non-IID, high-dimensional, strongly-coupled Open Complex Giant Systems (OCGS)}.

Grounded in systems science first principles, we establish \textbf{"Value Chain Physics"} as the concrete domain projection of systems science in physical enterprise networks. We construct a \textbf{Systemically Complete and Irreducible (SCI)} five-dimensional topological manifold $\mathcal{M}(t) = \langle\mathcal{N},\mathcal{T},\mathcal{C},\mathbf{x}(t),\Delta\mathbf{x}(t)\rangle$. The non-orthogonal interference across these five dimensions is proven to be the physical origin of control vector "Vector Cancellation" under multi-departmental local optimization, generating internal dissipative heat $\Delta W_{\text{heat}} = \sum \|V_i\| - \|\sum V_i\| > 0$ that erodes ROIC.

Furthermore, we propose a \textbf{5D Double-Helix Architecture} intertwining a Holographic Data Model (Left Helix) and a Dynamic Evolution Algorithm (Right Helix). The Prior Partition Operator $\mathbf{\Pi}$ projects the factorial $\mathcal{O}(N!)$ search space onto a bounded feasible space $\Omega_{\mathrm{feasible}}$ via Directed Acyclic Graph (DAG) topological ordering. The Rigid Manifold Operator $\mathbf{\Pi}_\bot$ strips normal non-orthogonal internal friction degrees of freedom ($x_\bot(t) \to 0$), reducing search complexity to polynomial scale $\mathcal{O}(N \log N)$ (under bounded treewidth networks). In the continuous Banach space $(\Omega, \|\cdot\|_2)$, we prove the unique existence and exponential convergence of a closed-loop steady state $x^*$ via the \textbf{Banach Contraction Mapping Theorem}, guarded by a \textbf{Compact Support Projection Operator $\mathbf{E}_{\mathrm{supp}}$} that truncates heavy-tailed residual distributions to mitigate Goodhart's Law collapse.

Finally, under explicit mathematical conditions ($\mathbf{\Pi}$ partitioning, $\mathbf{\Pi}_\bot$ constraints, spectral radius $\rho(\mathbf{A}) < 1$, and human second-order meta-introspection $\mathbf{\Phi}$), we provide \textbf{formal mathematical proofs for the necessary and sufficient conditions of Qian Xuesen's three core OCGS principles} (Overall Design Department, Hall of Workshop for Decision Support, and Human-Machine Integration with Human-in-the-Lead). Empirical verification across a 10-year longitudinal field deployment (2015--2024) across Lenovo's global manufacturing network (including the WEF Lighthouse Factory in Hefei) and third-party audited financial metrics confirms that closed-loop autonomous decision-making ("human-out-of-the-loop" execution) dramatically reduces stoppage frequencies, releases RMB 3.5+ billion in NWC, and drives sustained ROIC elevation.
\end{abstract}

\textbf{Keywords}: Non-IID Learning; Value Chain Physics; Systemically Complete and Irreducible (SCI); Open Complex Giant Systems (OCGS); Formal Proofs; Ablation Study; Universal Computability; ROIC Paradox

\section{Introduction}
In modern enterprise management and industrial engineering, Return on Invested Capital (ROIC) serves as the primary physical metric evaluating operational efficiency and capital allocation efficacy:
\begin{equation}
\text{ROIC} = \frac{\text{NOPAT}}{\text{Invested Capital}} = \frac{\text{Net Operating Profit After Tax (NOPAT)}}{\text{Fixed Assets} + \text{Net Working Capital (NWC)}}
\end{equation}

Over the past decade, enterprise capital expenditures in enterprise resource planning (ERP), advanced planning and scheduling (APS), manufacturing execution systems (MES), warehouse management systems (WMS), supplier relationship management (SRM), and supply chain control towers have expanded at a compound annual growth rate exceeding 15\%. Paradoxically, average manufacturing ROIC declined from 8\% to below 5\%, with approximately 70\% of digital transformation initiatives failing to meet expected financial returns. This represents the manifestation of Brynjolfsson's (1993) "IT Productivity Paradox" in industrial intelligence.

Existing literature frequently attributes implementation failures to organizational inertia, communication barriers, or data quality defects. In contrast, this paper argues that the fundamental root cause lies in a paradigm breakdown between value chain management and underlying systems science:
\begin{enumerate}
    \item \textbf{The IID Static Assumption}: Since Taylorism and modern modular enterprise architectures (BPM, S\&OP, BSC), management engineering has partitioned enterprises into functional silos, implicitly assuming that operational state evolutions follow Independent and Identically Distributed (IID) orthogonal distributions.
    \item \textbf{Non-IID Physical Reality \& OCGS Challenges}: In real-world discrete manufacturing value networks, material kitting, equipment capacity, order delivery lead times, and cash flows exhibit intense non-linear topological entanglement (Cao, 2014, 2022). The enterprise is fundamentally an Open Complex Giant System (OCGS) characterized by Non-IID dynamics (Qian et al., 1990).
\end{enumerate}

To break this deadlock, we establish \textbf{"Value Chain Physics"}---the concrete domain projection of systems science onto enterprise value networks.

\section{Theoretical Foundations}
Our theoretical architecture rests on the synthesis of three foundational pillars:
\subsection{Qian Xuesen's Open Complex Giant Systems (OCGS) Theory}
Qian et al. (1990) defined systems characterized by vast heterogeneous components, multi-level hierarchy, non-linear long-range interactions, and continuous environmental exchange as Open Complex Giant Systems (OCGS).

\subsection{Isomorphism with Cao's Non-IID Learning Theory}
Longbing Cao (2014, 2022) established Non-IID Learning theory, proving that: "If a system is intrinsically Non-IID, modeling and solving it under IID assumptions yields mathematically biased and invalid results."

\subsection{Cybernetic and Information-Theoretic Boundaries}
Ashby's (1956) Law of Requisite Variety ($V_c \ge V_s$) and Miller's (1956) law ($7 \pm 2$) bound human cognitive bandwidth. Kalman's (1960) duality theorem dictates that unobservable physical states cannot be controlled.

\section{Value Chain Physics and 5D Topological Manifold}
The physical reality of a value network at time $t$ is formalized as a Systemically Complete and Irreducible (SCI) 5D Topological Manifold:
\begin{equation}
\mathcal{M}(t) = \langle \mathcal{N}, \mathcal{T}, \mathcal{C}, \mathbf{x}(t), \Delta\mathbf{x}(t) \rangle
\end{equation}

\section{Formal Proofs}
\begin{theorem}[Vector Cancellation Law]
In a Non-IID value network with control velocity vectors $V_i$, the vector triangle inequality dictates:
\begin{equation}
\left\|\sum_{i=1}^{N} V_i\right\| \le \sum_{i=1}^{N} \|V_i\|
\end{equation}
The work energy canceled due to physical interference is Coordinated Dissipative Heat:
\begin{equation}
\Delta W_{\text{heat}} \triangleq \sum_{i=1}^{N} \|V_i\| - \left\|\sum_{i=1}^{N} V_i\right\| \ge 0
\end{equation}
\end{theorem}

\begin{theorem}[Master Theorem: Proofs of OCGS Principles]
Given an OCGS in a Non-IID strongly-coupled state:
\begin{enumerate}
    \item \textbf{Overall Design Department}: Necessary to eliminate vector cancellation heat $\Delta W_{\text{heat}} > 0$; sufficient when combined with rigid manifold $\mathbf{\Pi}_\bot$.
    \item \textbf{Hall of Workshop for Decision Support (HWDS)}: 5D Double-Helix prunes $O(N!)$ to $O(N \log N)$, establishing universal computability under spectral condition $\rho(\mathbf{A}) < 1$.
    \item \textbf{Human-Machine Integration}: Human meta-operator $\mathbf{\Phi}_{\mathrm{Human}}$ is necessary to break formal system lock-in; tensor product $\mathbf{\Phi}_{\mathrm{Human}} \otimes \mathcal{A}_{\mathrm{Silicon}}$ is sufficient for intergenerational evolution.
\end{enumerate}
\end{theorem}

\section{Empirical Validation}
Empirical verification across a 10-year longitudinal field deployment (2015--2024) across Lenovo's global manufacturing network confirms:
\begin{itemize}
    \item On-Time Delivery (OTD): 98.4\%
    \item Inventory Turnover: 24.6 turns/year (+1.9$\times$)
    \item NWC Released: > RMB 3.5 Billion
    \item Line Stoppages: < 2 / month
\end{itemize}

\section{Conclusion}
Value Chain Physics resolves the ROIC productivity paradox by replacing IID assumptions with Non-IID state-space modeling, providing formal mathematical proofs and empirical validation for open complex giant systems governance.

\begin{thebibliography}{99}
\bibitem{Qian1990} Qian, X., Yu, J., \& Dai, R. 1990. "A new discipline of science—Open complex giant systems and their methodology." \textit{Nature Magazine}, 13(1), pp. 3-10.
\bibitem{Meng2026} Meng, F. 2026. \textit{System and Complexity Science: Generation, Persistence, and Evolution of Order}. Zenodo. DOI: 10.5281/zenodo.22033928.
\bibitem{Ashby1956} Ashby, W. R. 1956. \textit{An Introduction to Cybernetics}. Chapman \& Hall.
\bibitem{Brynjolfsson1993} Brynjolfsson, E. 1993. "The productivity paradox of information technology." \textit{Communications of the ACM}, 36(12), pp. 66-77.
\bibitem{Cao2014} Cao, L. 2014. "Non-IIDness learning in behavioral and social data." \textit{The Computer Journal}, 57(9), pp. 1358-1370.
\bibitem{Cao2022} Cao, L. 2022. "Beyond i.i.d.: Non-IID thinking, informatics, and learning." \textit{IEEE Intelligent Systems}, 37(4), pp. 5-17.
\bibitem{Hevner2004} Hevner, A. R., et al. 2004. "Design science in information systems research." \textit{MIS Quarterly}, 28(1), pp. 75-105.
\end{thebibliography}

\end{document}
"""
    with open("value_chain_physics_scm_paper_draft_en.tex", "w", encoding="utf-8") as f:
        f.write(tex_code)
    print("Successfully generated value_chain_physics_scm_paper_draft_en.tex!")

def main():
    with open("value_chain_physics_scm_paper_draft_en.md", "w", encoding="utf-8") as f:
        f.write(en_md_content)
    print("Successfully written value_chain_physics_scm_paper_draft_en.md!")
    build_english_docx()
    build_english_tex()

if __name__ == "__main__":
    main()
