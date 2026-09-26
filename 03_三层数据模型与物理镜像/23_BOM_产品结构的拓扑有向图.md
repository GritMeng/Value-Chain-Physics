# 🧬 23_BOM_产品结构的拓扑有向图：$O(1)$ 密集倒排索引与 LLC 拓扑编译

### 突触神经链接
- 👉 **上一层神经元**：[[22_Part_物料实体的物理属性]]
- 👉 **下一层神经元**：[[24_Demand_需求实体的时空流量]]
- 👉 **数学算法链接**：[[04_专利数学与消纳算法/28_1_LSC树多维展开与LLC编译]]

---

## 🏛️ 1. $O(1)$ 密集双向倒排索引 (Inverted Index Arrays)

在多级有向无环图（BOM DAG）中，传统系统在跑需求炸裂时，需要频繁递归查找“当前物料的所有子组件”或反向查找“该子件隶属的所有父项组”。这在数据库层面需要频繁执行高昂的 `SELECT JOIN` 开销。
- **DOD 倒排索引设计技巧**：
  为了实现零延迟炸裂，IPC 引擎在密集内存中常驻两组扁平的**二维整型索引阵列**：
  
### 📐 双向倒排索引数组设计：
1.  **正向炸裂索引 `parent_to_boms`**：
    `std::vector<std::vector<size_t>> parent_to_boms;`
    `parent_to_boms[parent_part_id]`：使用父项的 `part_id` 整数作为数组下标，**瞬时（小于1纳秒）** 返回该物料作为父项的所有子 BOM 密集索引。消除了任何递归搜索与 Map 比对，实现极速“毛需求爆破”。
2.  **反向追溯与替代定位索引 `child_to_boms`**：
    `child_to_boms[child_part_id]`：使用子项的 `part_id` 作为下标，**瞬时** 返回该物料作为子项的所有父 BOM 密集关系。这为 `[[04_专利数学与消纳算法/28_3_Swap不完全替代消纳]]` 快速定位组替代物料提供了极致的底层硬件通道。

---

## 📐 2. LLC 拓扑防环编译算法

拓扑排序是确保 MRP 物理计算正确的“数字铁律”。在系统初始化或 ECN 工程变更触发时，全网 DAG 自动执行 LBL（Level-by-Level）编译：

```
       【 根节点 LLC = 0 (独立订单) 】
                   │
                   ▼ (炸裂: 更新子件最大层级)
       【 LLC(Child) = max(LLC(Child), LLC(Parent) + 1) 】
                   │
                   ▼
       【 彻底消除计算乱序与死锁 】
```

*   **最大 LLC 深度合并规则**：
    如果一个共享组件 $C$ 同时属于多个不同的父项，在拓扑编译时，系统遍历所有的依赖路径，强制取其在所有路径中的**最大深度层级**：
    $$\text{LLC}(C) = \max_{e \in \text{All_Paths}} \left( \text{LLC}(\text{Parent}_e) + 1 \right)$$
    这保证了在消纳结算时，所有向该组件汇总的毛需求已经完全坍缩，实现算法的绝对自洽。
*   **拓扑环路熔断**：编译过程中，如果检测到首尾相接的闭环（如 $A \rightarrow B \rightarrow A$ 的恶性数据错误），拓扑编译器立即执行**“立法熔断”**并报错，防止 CPU 陷入死循环。

---

## ⏳ 3. ECN 时间切片工程变更与生命周期控制

在物理制造中，BOM 有向图并不是一成不变的，而是随着工程变更（ECN - Engineering Change Note）沿时间轴动态演进：
* **时效窗口过滤 (`eff_start_day`, `eff_end_day`)**：每个 BOM 关系边缘携带了时效起止日期。在需求爆爆过程中，引擎根据当前爆破日（Child Due Day）对 BOM 关系进行时间戳切片过滤：
  $$\text{Is_Active} = ( \text{child_due_day} \ge \text{eff_start_day} ) \land ( \text{child_due_day} \le \text{eff_end_day} )$$
  只有落在时效窗口内的子组件才会被拉动生产，未起效或已过期的组件会自动被排除，避免因工程更替引发的错误呆滞库存。

---

## ⚙️ 4. 考虑 Lot-Sizing 溢出量滚动消纳的 LBL MRP 净需求递推

在级联 MRP (Level-by-Level) 的供需消纳中，Lot-sizing 对需求爆破和物料消纳提出了严苛的约束：
1. **净需求计算与 Lot 进位**：
   在物料 $i$ 处，当期毛需求（Gross Demand）扣减其物理在手存量与在途订单后，形成净需求 $ND(t)$。若对应的 BOM 组件关系中定义了 `lot_size`，计划订单数量 $PO_Qty$ 将依据 lot 进位公式进行向上规整：
   $$PO_Qty(t) = \lceil \frac{ND(t)}{\text{lot_size}} \rceil \times \text{lot_size}$$
2. **溢出量回灌（Carryover Excess Registration）**：
   由于向上取整，会产生额外的溢出供给量：
   $$\text{Excess}(t) = PO_Qty(t) - ND(t)$$
   引擎的 `run_lbl_mrp_engine` 模块会将此 $\text{Excess}(t)$ 自动追加回物料 $i$ 的**时序累积供应曲线（Cumulative Supply $CS(t)$）**中：
   $$CS(k) \leftarrow CS(k) + \text{Excess}(t) \quad (\forall k \ge t)$$
3. **后续需求自动消化**：
   在随后的时段 $t' > t$ 的净需求消纳过程中，由于 $CS(t')$ 中已沉淀了之前的溢出量，随后的订单将优先消耗这部分“免费”的虚拟存量，直到溢出存量全部扣减归零后，才会触发新的计划订单。这确保了 LBL 计划的物理自洽，避免了传统的“小批量订单导致天量备料库存”的膨胀缺陷。

---
> 💡 **突触反向链接**：[[01_系统科学与时空定律/07_全息时空阻抗积分算子]] | [[05_工程天堑与AI审判/29_交付黑洞与时移阻尼天堑]]
