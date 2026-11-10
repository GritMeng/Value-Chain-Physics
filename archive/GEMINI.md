# 跨 Session 第一性原理与不可置疑五大铁律 (GEMINI.md)

> [!CAUTION]
> **绝对指令 (Mandatory Rules for All AI Agents)**  
> 本文件为本工作区的最高治理规约。任何 AI Agent（无论开启哪个 session，无论使用何种大模型）在对本目录下的文件进行读取、编写、修改、转换时，必须 100% 严格遵守以下五大铁律：

---

### 铁律一：零数据丢失与防误删原则（Zero Data Loss）
1. **严禁无预警整文件覆盖或删除**：严禁使用全量覆写（`Overwrite: true`）直接覆盖大篇幅 IPC 引擎源码、架构设计或算法文档。
2. **强制使用增量局部替换**：修改代码或文档必须使用局部替换算子，严禁将多千字节的源码或文档简化替换为几行概要。
3. **重要文件备份机制**：若用户明确要求进行大规模重构，必须先创建 `.bak` 备份文件（例如 `原文件名.py.bak` / `原文件名.md.bak`），并在对话中明确说明。

---

### 铁律二：Markdown 与代码源码为唯一真理源（Single Source of Truth, SSOT）
1. 所有 IPC 计划与控制引擎的架构设计、算法逻辑与源码，必须以本地纯文本为唯一真理源。
2. 绝对不允许直接在 Word 或 PDF 中做不可追溯的修改，所有导出仅能从源文件编译。

---

### 铁律三：确定性离线 Pandoc 编译（禁用简并脚本）
1. 转换文档为 Word (`.docx`) 严禁调用 `python-docx` 或第三方网页抓取脚本。
2. 必须直接调用本地已安装的 `pandoc` 编译器（`pandoc input.md -o output.docx`），借助 Pandoc 的抽象语法树（AST）将公式与代码块直接编译为 Word 原生矢量表达式对象，保证零乱码、零缺字。

---

### 铁律四：IPC 智能计划与控制引擎矢量相角自洽与 DAG 确定性
1. **原子算子规范**：算子推演只执行向量化映射 $O(N)$、并行排序 $O(N \log N)$ 与前缀扫描 $O(N)$，严禁无界的非线性组合搜索。
2. **消灭做功内耗**：控制矢量相角完全对齐（$\langle \vec{v}_i, \vec{v}_j \rangle \ge 0 \implies \Delta W_{\text{heat}} \to 0$）。
3. **有限因果 DAG**：严格控制推演因果链深度 $K \le \tau_{\text{phy}} / \Delta t_{\text{clock}} < +\infty$。

---

### 铁律五：严禁凭空幻觉（Grounding on Empirical Code & Text Only）
1. 严禁凭空捏造不存在的代码路径、API 接口、架构方法或算法函数。
2. 任何修改必须基于对原有代码或 Markdown 文档的真实查看后再进行。
