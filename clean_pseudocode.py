import os, re, tarfile

work_dir = os.path.dirname(os.path.abspath(__file__))
target_main_tex = os.path.join(work_dir, "main.tex")
clean_tar = os.path.join(work_dir, "arXiv_clean_package.tar.gz")

print("Cleaning main.tex to be 100% isomorphic to the Occam's Razor Chinese Monograph (Zero Pseudocode, Zero Conference Draft Noise)...")

# Ultra-clean, pure, 1:1 isomorphic formal English monograph matching Volume I-IV
pure_canonical_tex = r"""\documentclass[11pt,a4paper]{article}
\usepackage[utf8]{inputenc}
\usepackage{amsmath,amssymb,amsthm,amsfonts}
\usepackage{geometry}
\usepackage{hyperref}
\usepackage{cite}
\usepackage{booktabs}
\usepackage{microtype}
\usepackage{graphicx}

\geometry{margin=1.0in}

\title{\textbf{System and Complexity Science: Generation, Persistence, and Evolution of Order}\\
\large --- The Physical Constitution for Open Complex Giant Systems}

\author{\textbf{Fanchun Meng (Grit Meng)}\\
\small Chief Architect of IPS Engine (2007--2020), Lenovo Group\\
\small Author of \textit{Physics of Value Chain Management} and \textit{Holographic Anti-Entropy}\\
\small Email: gritmeng@outlook.com | GitHub: \url{https://github.com/GritMeng/Value-Chain-Physics}}

\date{July 2026}

\newtheorem{axiom}{Axiom}
\newtheorem{definition}{Definition}
\newtheorem{theorem}{Theorem}
\newtheorem{proposition}{Proposition}

\begin{document}

\maketitle

\begin{abstract}
For over two centuries, the governance of open complex giant systems has suffered from the collapse of reductionist paradigms under factorial complexity $O(N!)$ and Wolfram computational irreducibility. 
In this monograph, we step back to the absolute first principles using Occam's razor. We establish a rigorous, minimal axiomatic foundation: 
starting from \textit{Disorder} (the unpartitioned whole) and \textit{Observer} (the self-sustaining steady state), we derive the \textit{Triple Realities} (Real, Ideal, Residual) and prove the \textbf{Three Laws of Order}: 
(1) \textit{Law of Generation}: Without an Ideal Framework, order collapses ($\Delta S \to \infty$); 
(2) \textit{Law of Preservation}: Without Deterministic Constraints ($\mathbf{\Pi}_\bot$), order dissipates through internal friction; 
(3) \textit{Law of Evolution}: Without Residual Feedback ($\mathbf{\Delta}$), evolution stalls into dogma. 
We formalize these laws into five irreducible work operators: Prior Partition Operator $\mathbf{\Pi} = \langle D, A \rangle$, Rigid Manifold Operator $\mathbf{\Pi}_\bot$, Residual Norm Difference $\mathbf{\Delta}$, Second-Order Evolution Operator $\mathbf{\Phi}$, and Conscience Tight-Support Operator $\mathbf{E}_{\mathrm{sp}}$. 
Using the Banach Contraction Mapping Theorem and Priority Netting, we prove that factorial complexity $O(N!)$ is algebra-pruned to polynomial solvable $O(N \log N)$. 
Finally, we report a 13-year empirical validation across Lenovo's global manufacturing network (scheduling 2 million parts in 5 minutes, 98\% response rate, 1.9$\times$ inventory turnover, human-out-of-the-loop self-healing), and establish a 1:1 isomorphism across 19 historical philosophies from Laozi and Wang Yangming to Qian Xuesen's Giant Systems, Wu Xuemou's Pansystems, and Longbing Cao's Non-IIDness.
\end{abstract}

\section{Preface \& Introduction: Practice Origin and Occam's Razor Reconstruction}
The emergence of this axiomatic system is not derived from abstract speculation, but from a 22-year trajectory of practice, reflection, and scientific elevation:
\begin{itemize}
    \item \textbf{2004--2007}: Early foundation in enterprise data structures, Bill of Materials (BOM), routing, and phase-space constraints.
    \item \textbf{2007--2020}: As Chief Architect at Lenovo Group, building the Integrated Planning Solution (IPS)---an automated decision engine scheduling global lighthouse factories (e.g., LCFC Hefei). IPS achieved human-out-of-the-loop cybernetic loops, boosting delivery response from 54\% to 98\%, on-time delivery accuracy by +32\%, and inventory turnover by 1.9$\times$, releasing billions in capital.
    \item \textbf{2020--2026}: Formulating Version 1 (v1) preprints of \textit{Physics of Value Chain Management} and \textit{Holographic Anti-Entropy}.
    \item \textbf{Present (v2 Revision)}: Applying Occam's razor to strip all redundant intermediary jargon, stepping back to the logical origin to construct this unabridged, rigorous Version 2 (v2) preprint and formal manuscript.
\end{itemize}

We strictly enforce Seven Iron Rules: (1) No Overdraft; (2) Shortest Logical Path; (3) Extreme Parsimony; (4) Empirical Validation; (5) Respect Prior Wisdom; (6) Establish Axioms First; (7) Falsifiability Deadlines.

\section{Volume I: The Origin of Order (Axiomatic Foundation)}

\subsection{Disorder, Observer, and Partition}
\begin{quote}
\textit{Without an observer, there is no world. Once disorder is observed, it manifests as the world. Beyond physics, the world is order.}
\end{quote}

\begin{definition}[Disorder]
The unpartitioned whole prior to any observer boundary is defined as Disorder. Disorder is not empty void, but the inherent state of everything unpartitioned.
\end{definition}

\begin{definition}[Observer, Awareness, and Construction]
Within disorder, any entity capable of maintaining a self-sustaining boundary separating internal from external is an Observer. Awareness is the clarity to discern (the source of partition); Construction is the power to build (the boundary of order).
\end{definition}

\begin{axiom}[Axiom 0: Observer Self-Sustaining Law]
The Observer is not an a priori assumption, but a natural self-sustaining steady state emerging within disorder. Without an observer, there is no world.
\end{axiom}

\begin{definition}[Partition and Order Construction]
The observer's action of dividing boundaries using Awareness is Partition; establishing rules within the boundary using Construction is Order Construction. Inside is the Ideal World; outside is Disorder Background.
\end{definition}

\subsection{Triple Realities and the Three Laws of Order}
Partitioning simultaneously manifests the \textbf{Triple Realities}:
\begin{enumerate}
    \item \textbf{Real}: The unobservable, infinite background.
    \item \textbf{Ideal}: The local order framework established within the boundary by the observer.
    \item \textbf{Residual} ($\mathbf{\Delta}$): The discrepancy computation interface between Ideal and Real.
\end{enumerate}

\begin{axiom}[Axiom 1: Generation Law / No Ideal, Collapse]
Without an Ideal Framework established by partition, nothing can sustain in chaos, and order collapses ($\Delta S \to \infty$).
\end{axiom}

\begin{axiom}[Axiom 2: Preservation Law / No Constraint, Dissipation]
Without Deterministic Constraints and self-consistent rules, internal elements collide and order dissipates into internal friction.
\end{axiom}

\begin{axiom}[Axiom 3: Evolution Law / No Residual, Stagnation]
Without identifying Residual Feedback ($\mathbf{\Delta}$), the ideal framework hardens into dogma, and intergenerational evolution stalls.
\end{axiom}

\subsection{Natural Emergence of Logic, Model, and Algorithm}
Once the ideal framework is established, the observer naturally derives three tools:
\begin{itemize}
    \item \textbf{Logic}: The structural spine maintaining internal self-consistency.
    \item \textbf{Model}: The structured, orthogonal representation of the ideal framework in cognition.
    \item \textbf{Algorithm}: The temporal steps executing rules and work.
\end{itemize}

\subsection{Falsifiability Deadlines}
This theory declares three explicit falsifiability deadlines:
\begin{enumerate}
    \item Order sustains stably without an observer or an ideal framework.
    \item Collaboration occurs without friction in the absence of constraints.
    \item Intergenerational evolution occurs while residual remains identically zero.
\end{enumerate}

\section{Volume II: Formal Work Operators and Decidability Proofs}

We formalize the core work operators:
\begin{enumerate}
    \item \textbf{Prior Partition Operator} $\mathbf{\Pi} = \langle D, A \rangle$: Projection from infinite phase space $\Xi$ to state space $\Omega$, satisfying idempotency $\mathbf{\Pi}^2 = \mathbf{\Pi}$ and $\dim(\Omega) \ll \dim(\Xi)$.
    \item \textbf{Rigid Manifold Operator} $\mathbf{\Pi}_\bot$: Enforces legal sub-manifold trajectory, $\mathbf{x}_\bot(t) = (\mathbf{I} - \mathbf{\Pi}_\bot)\mathbf{x}(t) \to 0$.
    \item \textbf{Residual Norm Difference} $\mathbf{\Delta} = \|\Omega_t - \Omega_{t-1}\|$.
    \item \textbf{Second-Order Meta-Cognitive Operator} $\mathbf{\Phi}: \mathbf{\Pi}_k \to \mathbf{\Pi}_{k+1}$.
    \item \textbf{Conscience Tight-Support Operator} $\mathbf{E}_{\mathrm{sp}}$: Clips heavy-tailed Goodhart distributions to sub-Gaussian: $\mathbf{E}_{\mathrm{sp}} \cdot p(\mathbf{\Delta}) \in \text{Sub-Gaussian}$.
\end{enumerate}

\begin{theorem}[Banach Contraction and Polynomial Decidability]
Let $\mathbf{T} = \mathbf{\Pi}_\bot \circ \mathbf{\Phi}$. Then $d(\mathbf{T}x, \mathbf{T}y) \le \gamma d(x,y)$ with $\gamma < 1$, guaranteeing convergence to a unique Pareto optimal state and reducing factorial complexity $O(N!)$ to polynomial $O(N \log N)$ via priority netting.
\end{theorem}

\section{Volume III \& IV: Physical Laws, Isomorphism, and Empirical Validation}

We formulate the physical phase-space clipping equation:
\begin{equation}
V = M \cdot \mathbf{\Pi} [ N \otimes T \otimes C_{st} ]
\end{equation}
and the mismatch angle work efficiency equation:
\begin{equation}
W_{\text{eff}} = W_{\text{total}} \cdot \cos\theta, \quad \cos\theta = \frac{\mathbf{V}_{\text{itn}} \cdot \mathbf{V}_{\text{logic}}}{\|\mathbf{V}_{\text{itn}}\| \|\mathbf{V}_{\text{logic}}\|}
\end{equation}

\begin{figure}[htbp]
\centering
\includegraphics[width=0.8\textwidth]{images/fig3_human_machine_double_loop.png}
\caption{Human-Machine Double Loop Cybernetic Architecture}
\end{figure}

We establish a 1:1 isomorphism across 19 historical paradigms:
\begin{itemize}
    \item \textbf{Daoism (Laozi/Zhuangzi)}: \textit{Tao} and Non-being $\equiv$ Unpartitioned Disorder; \textit{Being} $\equiv \mathbf{\Pi}$ Partition.
    \item \textbf{Wang Yangming's Mind Studies}: \textit{Liangzhi} $\equiv \mathbf{E}_{\mathrm{sp}}$ Tight-Support Operator; \textit{Knowledge-Action Unity} $\equiv$ Cybernetic Closed Loop.
    \item \textbf{Kant}: \textit{Human legislates for Nature} $\equiv$ A priori Partitioning $\mathbf{\Pi}$.
    \item \textbf{Qian Xuesen}: \textit{Open Complex Giant Systems} $\equiv$ Human-in-the-loop/Human-out-of-the-loop cybernetic system.
    \item \textbf{Wu Xuemou}: \textit{Pansystems Theory} $\equiv \mathbf{\Pi}$ Phase Space Truncation.
    \item \textbf{Longbing Cao}: \textit{Non-IID Learning} $\equiv$ Non-orthogonal Topology Matrix $\mathbf{A}$.
\end{itemize}

\bibliographystyle{plain}
\bibliography{GritMeng_Research_Outputs}

\end{document}
"""

# Strip any accidental CJK characters
def remove_non_ascii(text):
    return re.sub(r'[^\x00-\x7F]+', '', text)

cleaned_tex = remove_non_ascii(pure_canonical_tex)

with open(target_main_tex, "w", encoding="utf-8") as f:
    f.write(cleaned_tex)

print(f"Cleaned main.tex! Removed all pseudocode and draft noise. New size: {len(cleaned_tex)} bytes")

# Re-create ultra-clean tar.gz
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
images_dir = os.path.join(work_dir, "images")

if os.path.exists(clean_tar):
    os.remove(clean_tar)

with tarfile.open(clean_tar, "w:gz") as tar:
    tar.add(target_main_tex, arcname="main.tex")
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")
    if os.path.exists(images_dir):
        for root, dirs, files in os.walk(images_dir):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, work_dir)
                tar.add(full_path, arcname=rel_path)

print(f"Re-packaged clean tar.gz at: {clean_tar}")
with tarfile.open(clean_tar, "r:gz") as tar:
    print("Files in tar:", tar.getnames())
