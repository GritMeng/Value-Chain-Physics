# IPC Core Open Benchmarks v2.2 数学规范 (Mathematical Specification)

本规范给出公开无菌验证套件（`ipc-core-benchmark` v2.2）所覆盖的九大供应链决策与网络代数机理的形式化定义与对账方程。所有算子均采用洁净室原生三态命名（`SkuNodeRecord` / `BomEdgeRecord` / `DemandOrderRecord` / `QuotaFirewallKey`），不含任何商业引擎底层内存实现细节。

---

## 1. 《排序耦合定律》最小完备反例、通用两阶段正交解耦与零堆内存回滚

在 100% 预测准确、100% 主数据完备条件下，高优订单 $D_1$（需 $M_1(t=4) + M_2$）与常规订单 $D_2$（需 $M_2 + M_3(t=2)$）共享通用件 $M_2$（在手现货 $t=1$，在途 $t=3$）：
- **传统静态优先级排他锁库**：$D_1$ 锁走 $M_2(t=1)$ 闲置等待 $M_1(t=4)$，导致 $D_2$ 被迫延期至 $t=3$（无故延期 1 天、现货闲置锁死 3 天）；
- **通用多需求两阶段正交解耦（`run_two_stage_orthogonal_pegging`）**：第一阶段先扫描各订单全套专用与共享子件物理最早齐套下界 $T_{\min}(d)$，第二阶段按 $\langle T_{\min}(d),\, \text{Priority}(d)\rangle$ 顺序执行就近水位晚绑定，使 $D_2$ 于 $t=2$、$D_1$ 于 $t=4$ 双双准时交付；
- **CSR 邻接索引与预分配游标真零堆内存回滚（`CsrBomGraph` + `WatermarkRollbackArena`）**：BOM 图按压缩稀疏行（CSR）连续存储，深度优先预留前记录物料与产能日志水位游标 `(supply_mark, cap_mark)`，任意叶节点缺料时直接逆序退回水位游标，搜索热路径保持 0 次堆内存分配与失败零残留。

---

## 2. 成套组替代沉没长板激活（11,000 vs. 1,000，CFO 账面守恒口径）

设主替代组 $G_1$ 持有高价值长板沉没库存 $V_{\text{sunk}} = 11{,}000$、缺短板小料（定向拉快成本 $C_{\text{exp}} = 1{,}000$），新替代组 $G_2$ 整套新购成本 $C_{\text{new}} = 12{,}000$：
- 现有 SCP / APS 跳过短板组直采 $G_2$：新掏现金 $12{,}000$ 交货 + 库内 $11{,}000$ 长板憋成呆滞报废，总账面成本 $23{,}000$；
- IPC 按增量新购现金流裁决：因 $C_{\text{exp}} < C_{\text{new}}$，定向拉快短板激活长板，仅新掏现金 $1{,}000$ + 正常领用库内 $11{,}000$ 主芯片（呆滞报废为 $0$），总账面成本 $12{,}000$；单套真实净省现金 $23{,}000 - 12{,}000 = 11{,}000$（严守资产负债表前后余额守恒，不重复计沉没存货）。

---

## 3. 多源配额平衡与最小包装倍数（MPQ）动态重归一化

对目标比例为 $\theta_i$、最小包装倍数为 $L_i$ 的候选供应商集合，按当前剩余净需求 $R$ 逐轮选出应分配量 $Q_i^{\text{due}} = \theta_i^{\text{cur}} \cdot R$ 最大者执行向上圆整 $Q_i^{\text{act}} = \lceil Q_i^{\text{due}} / L_i \rceil \cdot L_i$ 并受限于可用现货与剩余净需求；每轮结束后对剩余候选人按未分配应分份额重归一化：

$$\theta_j^{\text{next}} = \frac{Q_j^{\text{due}}}{\sum_{k \in \text{Active}} Q_k^{\text{due}}}$$

在 `1,000` 颗需求、比例 `50%:30%:20%`、包装倍数 `300` 下精确守恒收敛为 `600 + 300 + 100 = 1,000`。

---

## 4. 推拉解耦点（CODP）两层非补偿定位与二阶修宪 $\Phi$ 影子收益

1. **一阶界内定位（三道非补偿硬门控）**：
   - **门控一（时间因果硬界）**：$L_{\text{down}}(v) \le D_{\text{tol}}$（违背者一票淘汰，统一收敛 ETO/MTO/ATO/CTO/MTS）；
   - **门控二（物理可存与未固化差异）**：须具备存储条件且未打上不可逆客户/国别标签；
   - **门控三（沙漏腰部减熵割集）**：同等交付承诺 $S_{\text{FG}}$ 下，向高扇出度 $K$ 沙漏腰部收敛时，汇聚标准差按 $\sigma_{\text{pool}} = \sqrt{K}\sigma$ 收缩（变异系数降为 $1/\sqrt{K}$）、累计单件成本与跨国 E&O 同步下降。
2. **二阶修宪（$\Phi$ 移点影子收益）**：当旧工艺提前固化国别差异迫使 CODP 退化至成品层时，量化输出移点释放的常态安全库存资金 $\Lambda_{\text{CODP}} = J^*(\mathcal{C}_k) - J^*(\mathcal{C}_{k+1})$（3 层 6 节点标准算例中为 `¥512,000 - ¥192,000 = ¥320,000`），驱动研发 DFSC 延迟差异化。

---

## 5. 零预测冲减：推式防独占日切割与拉式超额三态路由

- **推式段（零预测冲减）**：周度需求在供应上限下按比例切分有效配额（欠账自动顺延至下周首日），再按各工作日有效产能比例切割到天，杜绝单一渠道在首日独吞全天产能；
- **拉式段（越界窗口三态路由）**：实单在当周配额桶内静默兑现（新增 `MRP = 0`）；产生超额 $\Delta O > 0$ 时按月包络线 $E_{\text{month}}$ 与采购提前期 $L_{\text{proc}}$ 自动分流：
  - `Mode A`（月累计 $\le E_{\text{month}}$）：节奏前移，扣减下周配额 $\Delta O$，新增 `MRP = 0`；
  - `Mode B`（月累计 $> E_{\text{month}}$ 且剩余交期 $\ge L_{\text{proc}}$）：锁死下周配额，直发增量 `MRP = +\Delta O`；
  - `Mode C`（月累计 $> E_{\text{month}}$ 且剩余交期 $< L_{\text{proc}}$）：从下周配额现货借调 $\Delta O$ 救急并同步注入回填 `MRP = +\Delta O`。

---

## 6. ITP 齐套门控最紧瓶颈注水切分、逆向重展净算与 IOP 平坦防波堤闭环

设 $R$ 个区域需求流 $r \in \{1,\dots,R\}$ 原始预测为 $F_r$、专用受限子件可用上限集合为 $\{S_{r,u}\}$、战略保底锁定量为 $L_r$，并共同依赖 $M$ 类跨区共用件池 $\{M_m\}$（总供给 $S_m^{\text{shared}}$，单耗 $\eta_m$）：
1. **专用件齐套包络先行**：先计算各需求流独立齐套上限 $K_r = \min\!\left(F_r,\, \min_u S_{r,u}\right)$，剥离缺专用件的虚高预测，禁止未齐套虚量挤占共用瓶颈件；
2. **最紧瓶颈（`Most-Gating`）自动识别与触顶循环注水公平切分**：自动定位稀缺比最小的共用瓶颈 $m^* = \arg\min_m \frac{S_m^{\text{shared}} / \eta_m}{\sum_r K_r}$；当总齐套需求超越等效供给上限 $C^* = S_{m^*}^{\text{shared}} / \eta_{m^*}$ 时，先按战略等级顺序兑现保底 $L_r^{\text{grant}} \le \min(L_r, K_r)$，再对剩余供给 $R_{\text{sup}} = C^* - \sum_r L_r^{\text{grant}}$ 按各区剩余齐套空间 $\Delta K_r = K_r - L_r^{\text{grant}}$ 执行触顶循环注水切分（任一区域触及 $K_r$ 后将溢出余量二次按比例重分），生成保护配额 $\mathcal{A}_r$（双区域标准算例精确收敛为 $\mathcal{A}_A = 4{,}000, \mathcal{A}_D = 2{,}000$；四区域三共用件算例自动锁定 `RF_PA_2X` 并收敛至总配额 `7,500`）；
3. **逆向重展净算（消除超买呆滞）**：以获批配额 $\mathcal{A}_r$ 封顶逆向重展非瓶颈外购件净需求，使采购总量由原始直展 $\sum_r F_r \gamma_r$ 收敛至 $\sum_r \mathcal{A}_r \gamma_r$（双区算例消除 `3,000` 颗呆滞，四区算例消除 `8,000` 颗呆滞）；
4. **连续平坦开放寻址防波堤（`FlatQuotaFirewallTable`）执行隔离**：IOP 执行层将四维键 $\langle t, \text{Family}, \text{CustGroup}, \text{Region} \rangle$ 打包为 64 位整数并在连续平坦槽位内核验放行，超额实单仅拦截越界增量，保住常规区域 `100%` 齐套交付。

---

## 7. 不完全替代：单向属性门槛与主料齐套锚定混用红线

当工艺或法规要求替代料在总产出中的占比不得超过 $\theta \in (0, 1)$（如 `30%`）时：
- **单向/精确属性门控**：先检验 `GE`（高规代低规 $d_{\text{cand}} \ge d_{\text{req}}$）或 `EQ`（认证一致 $d_{\text{cand}} = d_{\text{req}}$），未通过者直接截断为 `0`；
- **主料锚定封顶不等式**：当主料缺料仅齐套 $Q_{\text{main}}$ 时，现有系统机械按总需求毛量 $D_{\text{gross}}$ 计算 $\lfloor \theta D_{\text{gross}} \rfloor$ 会导致分母坍缩越线；IPC 严格求解 $\frac{Q_{\text{sub}}}{Q_{\text{main}} + Q_{\text{sub}}} \le \theta$，给出主料锚定上界：

$$Q_{\text{sub}} \le \min\left(D_{\text{gross}} - Q_{\text{main}},\; S_{\text{sub}},\; \lfloor \theta D_{\text{gross}} \rfloor,\; \left\lfloor \frac{\theta}{1 - \theta} Q_{\text{main}} \right\rfloor\right)$$

在 $D_{\text{gross}}=1{,}000, Q_{\text{main}}=500, \theta=0.30$ 下精准截断为 `214` 颗（`29.97% <= 30%`）。

---

## 8. 跨解耦点两阶段受限推拉排产与价值链网络先序区间代数

1. **两阶段推拉排产（`Push-IOP + Pull-IOP`）**：
   - **第一阶段（`Push-IOP` 预测受限预投）**：在推拉解耦点（CODP）上游按日有限产能 $C_t$ 与关键长周期物料在手/在途供给 $S_t^{\text{chip}}$ 双重硬约束，预投不带客户/国别死签的通用半成品时间桶池 $P_t^{\text{bare}}$；
   - **第二阶段（`Pull-IOP` 实单配额门控与动态齐套承诺）**：下游客户实单先经配额防波堤拦截超额抢料，再按 $\text{PromisedDate} = \max\left(\text{DueDate},\; \text{AvailDate}^*(P^{\text{bare}}) + L_{\text{post}}\right)$ 给出零爽约诚实交期，彻底消除传统仅按后道静态提前期 $L_{\text{post}}$ 盲目承诺造成的断料违约。
2. **价值链网络先序区间与维度规划代数**：
   - **无递归 SQL 连接的 $O(1)$ 子树判定**：在生成森林上一次构建先序进入序号 $\text{rank}(u)$ 与子树跨度 $\text{size}(u)$，任意节点 $v$ 属于 $u$ 子树当且仅当 $\text{rank}(u) \le \text{rank}(v) < \text{rank}(u) + \text{size}(u)$；
   - **展开期剪枝与菱形守恒**：在多阶网络正向展开时同步执行规格偏序算子（`PASS / EQ / GE / LE`）与国别安规位掩码（$(\text{mask}_{\text{cand}} \mathbin{\&} \text{mask}_{\text{req}}) = \text{mask}_{\text{req}}$）剪枝；同时支持全路径实例树（`PATH_INSTANCE`）与菱形共用件流量守恒合并（`DISTINCT_NODE`）双视图及逆向 `Where-Used` 顶层受影响成品反查。
