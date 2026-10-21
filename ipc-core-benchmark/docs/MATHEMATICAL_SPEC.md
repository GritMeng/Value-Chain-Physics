# IPC Core Engine 决策质量数学规范白皮书 (Mathematical Specification)

本规范阐述开源统御引擎在交付承诺 (ATP/CTP)、主计划与执行计划协同 (ITP/IOP Alignment) 及替代料动态决策 (Material Substitution) 中的数学推导与边界法则。

---

## 1. 交付可承诺 (ATP / CTP) 与零堆栈回滚算法

### 1.1 可承诺交付日计算模型

对于独立需求 \(D(p, t_{due}, Q)\)（其中 \(p\) 为目标物料，\(t_{due}\) 为期望交期，\(Q\) 为数量），IPC 引擎寻找满足约束的最小交期 \(t^* \ge t_{due}\)：

\[
t^* = \min \left\{ t \;\middle|\; S_{avail}(p, t) + C_{avail}(p, t - L_p) \ge Q \right\}
\]

其中：
- \(S_{avail}(p, t)\) 为 \(t\) 日及之前可用的现存量与已计划产出。
- \(L_p\) 为物料 \(p\) 的生产/采购提前期 (Lead Time)。
- \(C_{avail}\) 为对应瓶颈工作中心的剩余可用工时能力。

### 1.2 零堆内存回滚机制 (Zero-Heap Rollback)

在递归遍历多层 BOM 深度优先搜索（DFS）时，若第 \(k\) 层子件预留失败，引擎必须在 \(\mathcal{O}(1)\) 时间复杂度内复原先前的资源预留，避免在堆内存中动态分配/释放临时对象（Zero-Heap Allocation）。

回滚操作定义为：
\[
\text{AllocatedQty}(Node) \leftarrow \text{AllocatedQty}(Node) - \Delta Q_{temp}
\]
\[
\text{CapacityHours}(Wc) \leftarrow \text{CapacityHours}(Wc) - \Delta H_{temp}
\]

---

## 2. 主计划 (ITP) 与执行计划 (IOP) 协同阻断模型

### 2.1 战术防波堤配额生成 (ITP Buffer Generation)

ITP 主计划根据软约束缓冲区系数 \(\gamma \ge 1.0\) 生成宏观配额防波堤：

\[
\mathcal{A}(Key) = \gamma \cdot \sum_{i \in \text{MasterDemands}(Key)} Q_i
\]

其中约束键包含四维空间与时序标记：
\[
Key = \langle t, \text{FamilyID}, \text{CustGroupID}, \text{RegionID} \rangle
\]

### 2.2 执行计划刚性阻断校验 (IOP Rigid Quota Enforcement)

车间执行计划在微观排产时，必须严格受限于主计划下发的授权配额：

\[
\sum_{j \le k} Q_j^{IOP} \le \mathcal{A}(Key)
\]

当第 \(k\) 条执行需求突破配额上限（\(\sum > \mathcal{A}(Key)\)）时，引擎执行**强行拦截阻断 (Hard Blocking)**，防止车间越权抢料导致整体战略目标漂移。

---

## 3. 替代料三级决策与动态归一化模型

### 3.1 一类替代料：历史配额比例平衡 (Quota Balance)

针对替代料集合 \(\{i \in Group\}\)，目标为最小化历史分配量与目标比例的偏差：

\[
i^* = \arg\max_{i} \left| Q_i^{hist} - \left( \sum Q^{hist} + Q_{net} \right) \cdot \theta_i \right|
\]
其中 \(\theta_i\) 为目标配额比例。

### 3.2 二类替代料：组内固定优先级 (Priority Hierarchy)

\[
i^* = \arg\min_{i} \left( \frac{Q_i^{hist}}{\max(\theta_i, \epsilon)} \right)
\]

### 3.3 三类替代料：跨组动态归一化与水位消纳 (Dynamic Bucket Consumption)

当主物料存在净需求 \(Q_{net}\) 时，剩余替代料按当前剩余应分配量进行归一化计算：

\[
\theta_i^{current} = \frac{Q_i^{due}}{\sum_{j \in Active} Q_j^{due}}
\]

每次选优扣减量受限于实际批量倍数 \(\text{Lot}_i\) 与安全库存保护带 \(\text{SS}_i\)：

\[
Q_i^{actual} = \left\lceil \frac{Q_i^{current} \cdot Q_{net}}{\text{Lot}_i} \right\rceil \cdot \text{Lot}_i
\]
\[
Q_i^{consumed} = \min \left( Q_i^{actual}, \;\max(0, \text{OnHand}_i - \text{SS}_i), \; Q_{net} \right)
\]
