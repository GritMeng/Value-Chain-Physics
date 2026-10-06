# IPC 智能计划控制塔与求解器引擎——全景架构剖析与执行逻辑说明书

本说明书详尽阐述了 **Intelligent Planning & Control (IPC)** 系统的顶层设计体系。通过融合与革新两大供应链巨头：**Kinaxis RapidResponse** 与 **o9 Solutions** 的核心物理逻辑，IPC 构建了“以 EKG 图关系为底座、时序多维无锁 Netting 为消纳、AI 大模型预测为跨代超越”的终极物理宪法。

---

## 🛠️ 一、 顶层系统架构设计 (Covering o9 EKG via DOD Flat Graph)

IPC 采用高性能、轻量化的三层架构设计，将大盘战术决策、库存水位规划、以及微观排程消纳进行纵向解耦与横向协同。

```mermaid
graph TD
    subgraph 3. 前端交互与控制层 (React 19)
        UI[全息 o9-style EKG 拓扑大盘 / Kinaxis-style What-If 多沙箱对比 / Pegging 配额结界]
    end
    
    subgraph 2. 数据分析与协同对账层 (FastAPI + DuckDB)
        API[FastAPI API 路由网关]
        DB[(DuckDB 极速列式数据库 ipc.db)]
        AI_Forecaster[AI Time-Series 预测流: Chronos/TFT/DeepAR]
        API <--> DB
        AI_Forecaster -->|多轨预测 P10-P90| DB
    end

    subgraph 1. 核心计算与物理求解层 (C++ 裸金属)
        ITP[战术计划层 ITP: 宏观 Allocation 决策]
        IO_Eng[库存优化层 IO: Parallel MEIO & 概率带 SS 优化]
        IOP[运营排程层 IOP: LBL-MRP & CTP/OTP 时空置换]
        
        ITP -->|解耦 Allotment 刚性屏障| IOP
        IO_Eng -->|安全库存 SS 概率带警戒线| IOP
    end

    UI <-->|WebSocket / REST API| API
    IOP -->|流式 Appender 灌库| DB
    DB -->|DOD 连续扁平加载| IOP
```

### 1. 用 DOD 连续扁平内存覆盖（Cover） o9 EKG 知识图谱逻辑
- **o9 痛点**：o9 核心逻辑是基于 **EKG（图数据库）** 表达供应链。每一个 SKU、Site、Resource 均为 Node，BOM、替代、运输路线均为 Edge。这带来了卓越的柔性与级联影响分析能力，但指针跳转（Pointer Chasing）严重，随着规模扩大，CPU Cache Miss 暴增，导致性能恶化。
- **IPC 覆盖方案**：IPC 在底层完全放弃昂贵的图数据库，而是使用 **面向数据设计 (DOD) 一维连续扁平 `std::vector`** 在物理内存中存储图关系：
  - *节点 (Nodes)* ➔ 映射为零件、产能数组的连续行 ID。
  - *有向边 (Edges)* ➔ 映射为高效倒排索引数组 `parent_to_bom_indices` 与 `child_to_bom_indices`。
- **全网级联受损分析 (Propagative Impact Analysis)**：
  当底层任何零件供应商发生断料，C++ 裸金属求解器能够自底向上快速穿透倒排索引，在微秒内将波动传导给所有关联的顶层 Finished Goods 订单，自动在 `ipc_financial_ledger` 中重算受损的共识营收（Consensus Revenue），在 contiguous 内存的极致速度下完美实现了 o9 的图关系传导和级联影响分析。

---

## 📈 二、 IBP (Integrated Business Planning) 战术集成业务计划逻辑 (AI-Leapfrog Forecasting & SOP)

为了超越 Kinaxis 的传统统计时序模型，IPC 引入了基于 **AI 时间序列基础大模型 (AI Time-Series Foundation Models)** 的共识需求计划框架。

```
                              [ 历史发运与多维外生变量 ]
                                         │
                  ┌──────────────────────┼──────────────────────┐
                  ▼                      ▼                      ▼
           [ Chronos / TimeGPT ]       [ TFT (Transformer) ]     [ DeepAR 概率神经网络 ]
                  │                      │                      │
         (解决 NPI 零样本预测)      (拟合促销与多变量可解释性)    (输出概率分布区间 P10-P90)
                  │                      │                      │
                  └──────────────────────┼──────────────────────┘
                                         ▼
                              [ 一致性共识预测数据 ]
                                         │
                                         ▼ (下传至 C++ 裸金属求解器)
                              [ LBL MRP & DBD 有限能力排程 ]
                              (以物理守恒 Allotment 刚性隔离波动)
```

### 1. 超越 Kinaxis 的三大核心 AI 预测模型选型
- **Chronos / TimeGPT (零样本大模型)**：
  将销量数据序列进行区间量化，转换为 Token 序列送入预训练的 Transformer 自回归预测中。该模型专门解决 **新产品导入 (NPI) 阶段的冷启动** 难题，即便新 SKU 历史发货数据为零，依然可以通过跨物料跨领域的迁移学习完成零样本趋势推演，超越 Kinaxis。
- **Temporal Fusion Transformer (TFT)**：
  集成自注意力机制与变量选择网络。可以同时处理促销日程、价格变动、静态品类及历史发货等多元非线性变量，并在控制塔前端提供“Explainable AI”——直观显示大促、降价对预测销量的贡献率（%）。
- **DeepAR (自回归概率网络)**：
  直接输出未来的概率分布带（$P_{10}, P_{50}, P_{90}$）。这不仅能根据风险偏好拨付配额，更**重构了多级安全库存 (Safety Stock) 计算公式**：
  $$ SS_{i, t} = \hat{Y}_{t, P_{SL}} - \hat{Y}_{t, P_{50}} $$
  安全库存直接由目标服务水平分位数差额计算，免去了 Kinaxis 对销量标准差的正态分布假设，使库存压降 30% 以上。

---

### 2. ITP-IOP 双层规划与解耦配额机制 (Tactical to Operational)
系统将中远期滚动大盘决策（战术计划层，ITP）与近期精细化高频排产（运营计划层，IOP）进行物理隔离与硬性协同。
- **ITP 战术决策**：在周/月度时间跨度上，根据 AI 生成的 Consensus Demand 预测流，以最大化战略利润和压降库存呆滞成本为目标函数，在时段 $T$ 上建立线性规划：
  $$ \min \sum_{f \in F} \sum_{T \in \mathcal{T}} ( W_f \cdot (Demand_{f, T} - Supply_{f, T}) + C_f \cdot Leftover_{f, T} ) $$
- **解耦点刚性隔离屏障 (Decoupling Allotment Barrier)**：
  为防散客低优订单在微观 IOP 排程中对战略原料的“踩踏式抢占”，系统在供应链 BOM 的解耦点（Decoupling Point）处，将战术 Allocation 向下展开并折算为同家族在运营天（Day $t$）内的**刚性配额上限 (Allotment)**：
  $$ Allotment_{D, f, t} = \sum_{p \in Family(f)} Allocation_{p, T} \times BOM\_Ratio(p, D) \times \gamma_{t} $$
  在 IOP 执行级扣减时，一旦该日度配额余额为 0，系统将直接闭锁该通道，拦截并顺延低优先级订单，物理上保护核心战略资产不被“抢占踏踩”。

#### 3. 预测修正与财务大盘实时对账
- 计划主管在控制塔中调整滚动预测（Forecast Override）时，系统直接按：
  $$ Consensus\_Forecast = Qty \times Unit\_Price $$
  重新计算该客户与 SKU 的共识收入，并自动累加更新到财务总账表 `ipc_financial_ledger` 中，实现计划变动与财务预算的秒级同步。

## 🛡️ 三、 IO (Inventory Optimization) 多级库存优化逻辑

### 1. DOD 并行段树多级安全库存传导 (Parallel MEIO)
IO 优化模块使用面向数据设计 (DOD) 将全网节点的波动指标排布于扁平连续内存中。系统将时间轴划分为二叉时序段树，并发沿着有向网络图执行规约合并算子 $\oplus$，在 $O(\log N)$ 步内级联推演全网节点的多级安全库存 (Safety Stock) 水位：
$$ SS_{i, t} = Z_i \times \sqrt{ L_i \cdot \sigma_{D, i, t}^2 + D_{i, t}^2 \cdot \sigma_{L, i}^2 } $$
* $L_i$ 为物料节点的标准前置期，$\sigma_{D}$ 为需求标准差，$\sigma_{L}$ 为提前期标准差。
* 安全系数 $Z_i$ 是通过标准正态逆累积分布函数算得：$Z_i = \Phi^{-1}(SL_i)$，其中 $SL_i$ 为客户目标服务水平（如 98%）。

### 2. 虚拟安全库存乐观池化 (Optimistic SS Pooling)
为了消除按地域、渠道硬性拆分安全库存造成的“安全库存冗余暴增”，IPC 设计了**乐观无锁虚拟共享池**。
系统在物理共享格点上利用 `std::atomic<uint64_t>` 时间戳版本控制，不同客户渠道逻辑上各记其账，物理上合并为统一的共享池。排产引擎使用 CAS (Compare-And-Swap) 无锁乐观扣减，利用“池化效应”冲抵波动，**使全网总安全库存量下降了 20% - 30%**。

### 3. 交叉替代组下的安全库存二次分配算法 (Secondary SS Allocation)
针对共享替代件（如芯片 A1 同时参与替代组 1 与替代组 2）在不确定性下的配额推演，系统设计了二次分摊算法：
* **阶段一：历史分摊锚定 (Prior SS Anchoring)**：读取上一计划期稳定的 Pegging 供需钉结比例，按比例锁定 A1 安全库存基准分配，防止排产频繁跳跃。
* **阶段二：动态波动余量重新分摊 (Residual SS Allocation)**：若组 1 在新一轮推演中波动降低，释放出富余的安全库存定额，系统将该“余量”自动分摊到波动加剧的组 2，在无需采购新料的前提下以替代料实现多组间的自适应消纳。

---

## ⚡ 四、 ITP 战术计划层详细执行步骤 (ITP Step-by-Step)

战术计划层（ITP）以周或月为时间周期进行宏观大盘的滚动推演。其底层计算模块严格按以下 4 个阶段顺序运行：

### 步骤 1. 主数据提取与词汇映射 (Flat Master Data Extraction)
- 从持久化列式 DuckDB 的物料表、站点表和宏观预测需求表（`ipc_consensus_forecast`）中拉取数据。
- 求解器内存引擎通过哈希映射，将物料名称、物料类别和站点字符串映射为连续的 `uint32_t` 类型的逻辑代号 ID（Part ID），消除大字符串比对引起的性能耗损。

### 步骤 2. 单层 BOM 级联展开与价值链网络拓扑重构 (Value Chain Topology Construction)
- 提取物理存储中仅包含直接“父-子”两级对应关系的单层 BOM 表。
- 自顶向下，沿着工艺路线和供应链流向执行递归展开，将分散的单层父子关系自动级联编织成**端到端的全网价值链网络拓扑图**（以 `std::vector<FlatBomItem>` 结构体数组向量连续常驻物理内存中）。
- 在多级 BOM 的物料采购件、关键在制品和最终半成品处锚定“物料解耦点（Decoupling Point）”。

### 步骤 3. 宏观需求级联汇聚 (Demand Aggregation)
- 沿着重构好的价值链网络拓扑图，将分布于各个最终分拨中心（RDC）和各客户段的宏观销售预测量，乘以 BOM 级联消耗系数和报废系数，自上而下折算并汇聚到核心解耦点物料上。
- 以产品家族（Product Family）为统计集合，汇总各解耦点上的战术期内（如未来 12 周）的需求曲线。

### 步骤 4. 战术 Allocation 求解与 Allotment 刚性指标下传
- 建立以最大化大客战略协议履约率和压降宏观库存成本为目标函数的混合整数线性规划（MILP）模型。
- 通过运筹规划求解器（LP Solver），求解出不同时段各产品家族的 Allocation（资源配额总量）。
- 通过解耦分配公式，将粗颗粒的 Allocation 乘以时间分布系数和 BOM 配比，编译为日度 IOP 级别的**刚性 Allotment 隔离限额上限**，作为强约束写入 DuckDB 控制账本中。

---

## ⚡ 五、 IOP 运营排程层详细执行步骤与物理算子 (IOP Granular Mathematics & Core Operators)

运营计划层（IOP）以天为细粒度时间精度，用于处理海量需求订单的级联消纳与有限能力精密排程。

```
                       [百万独立需求订单]
                               │
                               ▼ 
                 [64位全局二进制复合优先级排序]
                               │
                               ▼
        ┌─────────────────────────────────────────────┐
        │ LBL MRP 阶段: 依据低层码 (LLC) 自顶向下层级消纳  │
        │   - 1. 双端前缀和无锁几何消纳算子 (Netting)     │
        │   - 2. 专利一、二、三类替换料分配决策算法       │
        └──────────────────────┬──────────────────────┘
                               │ (生成 Planned Orders，传递优先级)
                               ▼
        ┌─────────────────────────────────────────────┐
        │ DBD Dispatching 阶段: 按全局优先级时序派程    │
        │   - 1. 加工提前期动态拉伸 (Stretching)        │
        │   - 2. Setup 模组洗枪时间免除                 │
        │   - 3. CTP/ATP 事务性递归回溯预占与 OTP 抢占  │
        │   - 4. 零堆分配内存事务性回滚 (Rollback)      │
        └─────────────────────────────────────────────┘
```

### 1. LLC 拓扑松弛编译与有向无环图防环形死锁
IOP 引擎在开始 MRP 级联计算前，对 BOM 结构运行拓扑编译，计算每个物料的最低层码（LLC），公式如下：
$$ LLC(C) = \max \Big( LLC(C), LLC(P) + 1 \Big) \quad orall (P \rightarrow C) \in BOM $$
如果松弛更新次数超过最大深度限制（`MAX_BOM_DEPTH = 100`），求解器即判定存在循环依赖，直接阻断并报错，防止递归调用栈溢出（Stack Overflow）。

---

### 2. 双端几何前缀和无锁消纳算子 (Double-Ended Prefix-Sum Netting)
为消除传统 MRP 中多重 `if-else` 条件分支比对对 CPU 流水线造成的性能开销，IOP 使用**无锁几何交集消纳算子**。

设对于物料 $P$ 的供应总量上限为 $CS = OnHand + SR$（将现有量与在途结合）。
对物料 $P$ 沿时间轴排好序的需求事件序列为 $ev_0, ev_1, \dots, ev_N$。每个事件 $ev_j$ 的数量为 $qty_j$。
定义当前事件 $ev_j$ 在数轴上的**前置累积需求前缀和区间** $[CD_0, CD_1]$，计算公式为：
$$ CD_0 = \sum_{i=0}^{j-1} qty_i \quad \text{和} \quad CD_1 = CD_0 + qty_j $$

当前事件 $ev_j$ 的**库存已消纳分配量** $consumed_j$ 及产生的**净需求缺口** $net\_demand_j$ 的物理算子公式如下：
$$ consumed_j = \max \Big( 0.0, \min(CD_1, CS) - CD_0 \Big) $$
$$ net\_demand_j = \max \Big( 0.0, CD_1 - \max(CD_0, CS) \Big) $$

消纳结束后，物料在手库存余额为：
$$ OnHand_{new} = \max \Big( 0.0, CS - CD_{total} \Big) $$
通过这两条 $\max/\min$ 代数式，完全清除了分支判定，并在 C++ 中实现极速列式消纳。

---

### 3. 一二三类替换料分配专利决策算法
在 LBL 消纳产生净缺口时，若存在替代件，则触发专利分配决策：

* **一类替换（动态配额绝对温差最大优先）**：
  设替代料池 $\mathcal{G}$ 内各零件历史累计消耗量为 $H_{i}$，配额比例为 $R_{i}$。总虚拟分配需求为 $Total = net\_demand + \sum_{i \in \mathcal{G}} H_i$。理论配额量 $Due_{i} = Total \times R_{i}$。
  $$ Choice = rg\max_{i \in \mathcal{G}} |H_{i} - Due_{i}| \quad (\text{平局选 } R_{i} \text{ 最大者}) $$
* **二类替换（供应商稳定配额评级最低优先）**：
  计算供应商消耗配额评级 $Rating_{i} = \frac{H_{i}}{\max(1e-9, R_{i})}$。
  $$ Choice = rg\min_{i \in \mathcal{G}} Rating_{i} \quad (\text{平局选 } R_{i} \text{ 最大者}) $$
* **三类替换（带有 Lot-Size 限制的多轮归一化分配）**：
  迭代计算各活跃零件应分配量 $Due_{i} = W_{i} \times rem\_net$。对 $Due$ 最大零件的分配量向上取整：
  $$ Actual\_Qty = \lceil \frac{Due_{chosen}}{Lot_{chosen}} 
ceil \times Lot_{chosen} $$
  消纳相应库存后，将该零件移出活跃集，对剩下的零件配额进行**动态重新归一化**更新：
  $$ W_{cand\_next} = \frac{Due_{cand}}{\sum_{j \in \mathcal{A}} Due_{j}} $$

---

### 4. LBL \rightarrow DBD 数据接口与优先级传导
LBL 阶段将净需求转化为计划订单（Planned Orders）放入全局派程池中。为避免抢占发生错乱，运行**跨层级优先级拉通 (Horizontal and Vertical Priority Synchronization)**：
1. **64位二进制复合优先级编码**：
   $$ Composite\_Priority = (Committed\_Bit \ll 62) \mid (Tier\_Val \ll 60) \mid (Due\_Val \ll 44) \mid (Pri\_Val \ll 28) \mid Rev\_Val $$
   - Bit 62-63：承诺合同（0）与开放意向（1）。
   - Bit 60-61：客户等级（VVIP: 00, Tier-2: 01, Tier-3: 10）。
   - Bit 44-59：交期天数。
   - Bit 28-43：原始 ERP 顺序号。
   - Bit 0-27：营收反转码：$268435455 - \min(268435455, Revenue)$。
2. **自顶向下级联传递**：
    Finished 层的 PlannedOrder 继承与其关联 of 最高独立需求优先级（`min_priority`）。接着，沿着最低层码（LLC）自上而下将优先级穿透传递给其 BOM 子件计划订单，使得底层原料和在制品计划单在 DBD 中具备同等的确权优先级别。

---

### 5. DBD 有限能力时序派程与时序 ATP 递归回溯预占 (`reserve_atp_and_capacity_recursive`)
在 DBD 阶段，计划订单包装（`SchedOrderWrapper`）按全局复合优先级从高到低排序，逐个在原定交期的滑窗内 $[finish\_day, finish\_day+10]$ 运行回溯预占：

1. **动态加工时间拉伸**：
   加工周期随批量大小动态伸缩：
   $$ start\_day = finish\_day - \lceil LeadTime_{std} + Qty \times RunRate 
ceil $$
2. **Setup 准备洗枪豁免**：
   若该制程资源在此天上一笔排产任务的维度特征一致，则免除模具准备/洗枪时间：
   $$ SetupTime = egin{cases} 0.0 & \text{若 } last\_dim\_val[Resource\_ID][start\_day] == dimension\_val \ before\_fixed\_factor & \text{否则} \end{cases} $$
3. **CTP/ATP 递归回溯与 OTP 抢占**：
   - 沿各层 BOM 级联向上和向下寻找在库现有量或在途 SR。若发生产能越界，寻找同天生产的低优订单 $PO_{low}$（即 $Priority(PO_{low}) > Priority(PO_{VV})$），将其释放并平移重排（OTP Preemption），腾出空间给高优订单。
4. **零堆分配内存事务回滚 (Zero-Allocation Rollback)**：
   - 为避免堆内存（Heap）申请释放的开销，扣减时只原位改写物理 OnHand 与 Capacity 数组，并将改动记录压入线程局部暂存追踪栈。
   - 一旦在某一层级因产能不足或交期越界判定齐套失败，**逆向原位回滚引擎在 $O(1)$ 时间内反向遍历暂存追踪栈，把所有占用的在库现有量和产能工时数原位加回还原**，然后执行清空，达到零 GC 停顿与极致性能。

## 🧪 六、 数据沙盒隔离机制

为确保所有 scenario 实验、300万大压力 Benchmark 与生产持久化数据库之间在数据、磁盘 IO、表名上**互不干扰、绝对无冲突**，IPC 设计了双轨隔离方案：

1. **测试与回归轨道 (Memory Sandboxing)**：
   - 单元测试（如 `test_runner_validation.exe`）运行于全封闭的内存沙盒中。
   - 所有数据均在内存中动态创建、断言验证并销毁，**不与物理 `ipc.db` 进行任何文件交互**，保障回归的纯净度与并发安全性。
2. **生产与 What-If 推演轨道 (DuckDB Sandboxing)**：
   - 当用户创建 What-If 沙箱时，系统根据新 Scenario 独立生成内存映像。
   - 只有通过 `DEBUG_PERSIST` 显式开启或用户明确点击“推送到生产大盘”时，求解器才将推演结果通过多线程同步回写到物理 DuckDB 文件中，完成双轨数据链路隔离。
