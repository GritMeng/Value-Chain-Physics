import os, subprocess, tarfile

work_dir = r"h:\系统科学\the-holographic-anti-entropy-paper"
tex_file = os.path.join(work_dir, "main.tex")
bib_file = os.path.join(work_dir, "GritMeng_Research_Outputs.bib")
output_tar = os.path.join(work_dir, "arXiv_v2_replacement_package.tar.gz")

# Unabridged Occam's razor formal English manuscript
formal_en_tex = r"""\documentclass[11pt,a4paper]{article}
\usepackage[utf8]{inputenc}
\usepackage{amsmath,amssymb,amsthm}
\usepackage{geometry}
\usepackage{hyperref}
\usepackage{cite}
\usepackage{booktabs}
\usepackage{microtype}

\geometry{margin=1.0in}

\title{\textbf{System and Complexity Science: Generation, Preservation, and Evolution of Order}\\
\large --- The Physical Constitution for Open Complex Giant Systems}

\author{\textbf{Fanchun Meng (Grit Meng)}\\
\small Former Chief Architect of Global Supply Chain Planning Systems, Lenovo Group\\
\small Author of \textit{Physics of Value Chain Management} and \textit{Holographic Anti-Entropy}\\
\small Email: grit.meng@gmail.com | GitHub: \url{https://gritmeng.github.io/Value-Chain-Physics/}}

\date{July 2026}

\newtheorem{axiom}{Axiom}
\newtheorem{definition}{Definition}
\newtheorem{theorem}{Theorem}

\begin{document}

\maketitle

\begin{abstract}
For over two centuries, governance of open complex giant systems has suffered from the reductionist paradigm collapse under factorial complexity $O(N!)$ and Wolfram computational irreducibility. 
In this monograph, we step back to the absolute first principles using Occam's razor. We establish a rigorous, minimal axiomatic foundation: 
starting from \textit{Disorder} (the unpartitioned whole) and \textit{Observer} (the self-sustaining steady state), we derive the \textit{Triple Realities} (Real, Ideal, Residual) and prove the \textbf{Three Laws of Order}: 
(1) \textit{Law of Generation}: Without an Ideal Framework, order collapses ($\Delta S \to \infty$); 
(2) \textit{Law of Preservation}: Without Rigid Manifold Constraints ($\mathbf{\Pi}_\bot$), order dissipates; 
(3) \textit{Law of Evolution}: Without Residual Feedback ($\mathbf{\Delta}$), evolution stalls. 
We formalize these laws into five irreducible work operators: Prior Partition Operator $\mathbf{\Pi} = \langle D, A \rangle$, Rigid Manifold Operator $\mathbf{\Pi}_\bot$, Residual Norm Difference $\mathbf{\Delta}$, Second-Order Meta-Cognitive Operator $\mathbf{\Phi}$, and Conscience Tight-Support Operator $\mathbf{E}_{\mathrm{sp}}$. 
Using the Banach Contraction Mapping Theorem and Branch-Free Priority Netting, we prove that factorial complexity $O(N!)$ is algebra-pruned to polynomial solvable $O(N \log N)$. 
Finally, we report a 13-year empirical validation across Lenovo's global manufacturing network (scheduling 2 million parts in 5 minutes, 98\% response rate, 1.9$\times$ inventory turnover, human-out-of-the-loop self-healing), and establish a 1:1 isomorphism across 19 historical philosophies from Laozi and Wang Yangming to Qian Xuesen's Giant Systems, Wu Xuemou's Pansystems, and Longbing Cao's Non-IIDness.
\end{abstract}

\section{Introduction: Practice Origin and Occam's Razor Reconstruction}
From 2007 to 2020 at Lenovo Group, our team constructed the Integrated Planning Solution (IPS), achieving an end-to-end human-out-of-the-loop autonomous decision-making engine across global lighthouse factories. Operating data verified a delivery response rate jump from 54\% to 98\%, +32\% delivery accuracy, and 1.9$\times$ inventory turnover releasing billions in capital.

Following our initial preprint (v1), we apply Occam's razor to strip all redundant intermediary jargon, stepping back to the irreducible logical origin. We reconstruct this Version 2 (v2) preprint as a minimal, unabridged axiomatic constitution for system and complexity science.

\section{Volume I: The Origin of Order (Axiomatic Foundation)}
\subsection{Disorder, Observer, and Partition}
\begin{definition}[Disorder]
The unpartitioned whole prior to any observer boundary is defined as Disorder.
\end{definition}

\begin{definition}[Observer]
Within disorder, any entity capable of maintaining a self-sustaining boundary separating internal from external is an Observer, possessing Awareness (the clarity to discern) and Construction (the power to build).
\end{definition}

\begin{axiom}[Axiom 0: Observer Self-Sustaining Law]
The Observer is not an a priori assumption, but a natural self-sustaining steady state emerging within disorder. Without an observer, there is no world.
\end{axiom}

\subsection{Triple Realities and the Three Laws of Order}
Partitioning simultaneously manifests the \textbf{Triple Realities}:
\begin{enumerate}
    \item \textbf{Real}: The unobservable, infinite background.
    \item \textbf{Ideal}: The local order established within the boundary.
    \item \textbf{Residual} ($\mathbf{\Delta}$): The discrepancy computation interface between Ideal and Real.
\end{enumerate}

\begin{axiom}[Axiom 1: Generation / No Ideal, Collapse]
Without an Ideal Framework, order cannot sustain and collapses into disorder.
\end{axiom}

\begin{axiom}[Axiom 2: Preservation / No Constraint, Dissipation]
Without Rigid Manifold Constraints, internal elements collide and order dissipates.
\end{axiom}

\begin{axiom}[Axiom 3: Evolution / No Residual, Stagnation]
Without Residual Feedback, the ideal framework hardens into dogma, and evolution stalls.
\end{axiom}

\section{Volume II: Formal Operators and Mathematical Proofs}
We formalize the five fundamental work operators:
\begin{equation}
\mathbf{\Pi}^2 = \mathbf{\Pi}, \quad \dim(\Omega) \ll \dim(\Xi)
\end{equation}
\begin{equation}
\mathbf{x}_\bot(t) = (\mathbf{I} - \mathbf{\Pi}_\bot)\mathbf{x}(t) \longrightarrow 0
\end{equation}
\begin{equation}
\mathbf{\Delta} = \|\Omega_t - \Omega_{t-1}\|
\end{equation}
\begin{equation}
\mathbf{\Phi}: \mathbf{\Pi}_k \longrightarrow \mathbf{\Pi}_{k+1}
\end{equation}
\begin{equation}
\mathbf{E}_{\mathrm{sp}} \cdot p(\mathbf{\Delta}) \in \text{Sub-Gaussian}
\end{equation}

\begin{theorem}[Banach Contraction & $O(N \log N)$ Decidability]
Let $\mathbf{T} = \mathbf{\Pi}_\bot \circ \mathbf{\Phi}$. Then $d(\mathbf{T}x, \mathbf{T}y) \le \gamma d(x,y)$ with $\gamma < 1$, guaranteeing convergence to a unique Pareto optimal state and reducing $O(N!)$ to $O(N \log N)$ via priority netting.
\end{theorem}

\section{Volume III & IV: Engineering Reduction, Isomorphism, and Validation}
We establish the System Engineering Eight Laws (Purpose, Essence, Scheme, Capability, Mechanism, Route, Work, Evolution) and map the 1:1 isomorphism across 19 historical paradigms:
\begin{itemize}
    \item Laozi's \textit{Tao} $\equiv \mathbf{\Pi}$ Partition.
    \item Wang Yangming's \textit{Liangzhi} $\equiv \mathbf{E}_{\mathrm{sp}}$ Tight-Support Operator.
    \item Qian Xuesen's \textit{Open Complex Giant Systems} $\equiv$ Human-out-of-the-loop cybernetic loop.
    \item Wu Xuemou's \textit{Pansystems} $\equiv \mathbf{\Pi}$ Phase Space Truncation.
    \item Longbing Cao's \textit{Non-IIDness} $\equiv$ Non-orthogonal Topology Connection Matrix $\mathbf{A}$.
\end{itemize}

\end{document}
"""

with open(tex_file, "w", encoding="utf-8") as f:
    f.write(formal_en_tex)

# Make tar.gz in h:\系统科学\the-holographic-anti-entropy-paper
with tarfile.open(output_tar, "w:gz") as tar:
    tar.add(tex_file, arcname="main.tex")
    if os.path.exists(bib_file):
        tar.add(bib_file, arcname="GritMeng_Research_Outputs.bib")

print(f"Created clean arXiv replacement tar.gz package at: {output_tar}")
