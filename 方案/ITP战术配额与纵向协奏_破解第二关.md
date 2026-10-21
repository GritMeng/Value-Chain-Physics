# 主计划怎么跟执行计划协同？——战术配额 (Allocation/Allotment) 与微观排产的纵向闭环方案

> **导言：承接第一关——让战术护栏与微观排产无缝对接**  
> 
> 在《第一关：怎么能交付得又准又快》中，我们解构了**运营执行计划（IOP / Operational Execution Planning）**的微观排产引擎——通过 **OTP (Optimized to Promise) 正向推演与反向拉动算法**、**动态软预留 (Soft Reservation)**、**紧急插单的资源置换 (Resource Swapping)** 以及**采购/生产/交付/调度/Call料五重流程的横向对齐**，解决了车间级的交付黑洞与微观排产僵局。
> 
> 然而，第一关在结尾留下了两个关键追问：
> 1. *单纯依赖微观 IOP 的优先级排序，怎么拯救未被提前卡位的晚到高优先订单？*  
> 2. *没有宏观战术边界护栏，微观排程跑得再快，是否会偏离企业的战略目标与利润底线？*
> 
> 解决这两个问题，绝不能抛弃第一关的 IOP 引擎，而是必须建立**主计划（ITP / Master Planning）与执行计划（IOP）的纵向闭环协同**。主计划与执行计划是同源而不同尺度的两套计划：主计划负责在战术时间窗（18个月/周/月）内，根据企业 S&OP 战略共识进行**配额分配（Allocation）与硬性上界下发（Allotment）**；执行计划（IOP）则接纳 Allotment 作为不可逾越的护栏（Hard Cap），调用第一关的 **OTP 推拉算法** 和 **软预留/置换机制**，在护栏内完成最优化工序匹配与极速交付。
> 
> 本文作为第二关方案，将第一关的微观 IOP 引擎与主计划的宏观配额有机集成，构建起“战术有护栏、微观有极速”的协同体系。

---

## 第一章 主计划 (ITP) 与执行计划 (IOP) 的协同原理

主计划（Intelligent Tactical Planning, ITP）与执行计划（Intelligent Operational Planning, IOP）的协同，本质上是复杂生产环境中**不同时间与空间尺度的分工与对接**。

```
                【主计划 ITP ↔ 执行计划 IOP 跨尺度协同架构】

       ┌──────────────────────────────────────────────────────────┐
       │             S&OP 战略共识层 (Consensus Forecast)         │
       └────────────────────────────┬─────────────────────────────┘
                                    │ 战术博弈 (FIFS / FairShare)
                                    ▼
       ┌──────────────────────────────────────────────────────────┐
       │        主计划 ITP (Master Planning / 战术配额节点)       │
       │        输出: Allocation (战术名义配额)                    │
       └────────────────────────────┬─────────────────────────────┘
                                    │ 下发硬性护栏 (Allotment Cap)
                                    ▼
       ┌──────────────────────────────────────────────────────────┐
       │   运营执行计划 IOP (第一关 OTP 推拉引擎 / 5重流程对齐)    │
       │   基于 Priority Spectrum 软预留 & Resource Swapping      │
       └────────────────────────────┬─────────────────────────────┘
                                    │ 反哺 WIP 状态 & 实际到货(GR/ETA)
                                    └─────────────────► 触发配额动态再平衡
```

![主计划与执行计划双螺旋关系图](file:///h:/IPC/方案/images/monograph_itp_double_helix.png)

### 1.1 同一套物理模型：数据与约束的无缝继承
ITP 不能脱离第一关建立的 IOP 机制孤立存在，二者建立在完全一致的物理基底上：
- **继承第一关的 BOM 与约束拓扑**：ITP 的战术分配与 IOP 的微观排产，共享相同的多层 BOM 结构、替代路径与设备/物料能力上限。
- **继承第一关的五重流程对齐**：IOP 在微观上实现的“采购、生产、交付、调度、Call料”五重流程横向对齐，是 ITP 战术配额能够在车间兑现的物理载体。若没有第一关的横向对齐，ITP 的配额下发将在微观摩擦中被彻底撕碎。

### 1.2 不同时间与空间尺度的分工与对接
二者在时间、空间与决策粒度上分工明确，通过标准接口实现无缝连接：

| 维度 | 战术主计划 (ITP / Master Planning) | 继承与对接的运营执行计划 (IOP / Execution) |
| :--- | :--- | :--- |
| **时间尺度 (Horizon)** | 滚动 18 个月，按周（Week）/ 月（Month）桶（Bucket）博弈 | 滚动 1-4 周，按天（Day）/ 班次（Shift）/ 小时（Hour）精准排产 |
| **空间/对象粒度** | 聚合层级：Segment / Region / Customer Group / Part Family | 微观实体：具体 Order ID / SKU Part Number / 物理设备 Line & Machine |
| **核心算法引擎** | Gating 分配博弈 (Strategic Priority / FairShare / Netting) | 第一关 **OTP 推拉结合引擎 (Forward Best-Can-Do & Backward Pull)** |
| **预留与控制形态** | 战术名义配额 (**Allocation**) 与约束上限 (**Allotment**) | 第一关 **动态软预留 (Soft Reservation) & 资源置换 (Swapping)** |

### 1.3 双向闭环机制
1. **下发硬性护栏 (Downward Hard Cap)**：ITP 将 S&OP 战略共识解构出的各 Segment 配额上限（Allotment），下发给 IOP 作为 OTP 推拉引擎求解时的不可逾越的物料/产能上界（Gating Barrier）。
2. **微观执行反馈 (Upward Micro Feedback)**：IOP 运行第一关的 OTP 推拉计算与软预留锁定，将实际订单消耗、车间 WIP 状态、WIP 滞留率以及供应商到货（GR/ETA）实况实时反哺给 ITP，触发战术配额的动态再平衡。

---

## 第二章 为什么传统 S&OP 到了车间会失灵？

为什么传统制造企业哪怕引进了 APS/MES 也依然存在“交期撕裂”与“大客户订单被抢占”？因为存在三大纵向断点。

### 2.1 三大常见痛点与断点

1. **只有 Forecast，没有 Allocation**：S&OP 产出的销售预测在进入排产时被当成纯粹的“参考数字”，车间排产按“先到先得（FIFO）”执行，导致战略客户的订单因为下单稍晚而被普通客户抢光配额。
2. **只有宏观配额，没有微观约束传递**：高层决定的“中国区保障 800 台，北美保障 500 台”无法自动转化为排产算法的数学约束，微观 IOP 无法识别组织层级（Hierarchy）约束。
3. **战略分配与实际物料预留脱节**：S&OP 无法精确计算物料分配的软预留与硬占用，导致配额承诺变成了空中楼阁。

```
                         【三大纵向断点与解法】

  [传统断点模式]
  S&OP 预测数字 ──► (人工参考, 无配额保护) ──► 车间排产按先到先得 ──► 晚到大单缺料瘫痪!

  [IPC 闭环模式]
  S&OP 预测数字 ──► ITP 配额划分 (Allocation) ──► Allotment 硬性上界 ──► IOP 在护栏内极速排产
```

![三层纵向协同架构图](file:///h:/IPC/方案/images/monograph_itp_vertical_synergy.png)

### 2.2 解法：构建“宏观战略—战术配额—微观排产”三层协同网格

为了彻底消灭断点，IPC 体系将第一关的**横向五重流程对齐**与第二关的**纵向三层协同**融合为全域协同网格：

1. **战略共识层 (Consensus Forecast)**：在客户层级（Customer）、区域层级（Region）与产品层级（Part）建立长期需求共识（Combine Hierarchy），确定承诺等级（Commitment Level: High/Med/Low）。
2. **战术主计划层 (Master Planning - Allocation Node)**：运行 Gating 分配博弈算法（FIFS / FairShare / EqualShare），将受限供应解构为各 Segment 在各时间桶（Bucket）内的 **Allocation 额度与 Allotment 硬界**。
3. **运营执行层 (IOP / DSP / Execution)**：**全面激活第一关的微观引擎**——在 Allotment 硬界约束下，运行 OTP 推拉算法定锚 PSD，应用动态软预留（Soft Reservation）保护战略订单，遇突发插单时调用资源置换（Resource Swapping）消化冲击，驱动车间极速流动。

---

## 第三章 资源分配的三种状态：Assignment、Allocation 与 Allotment

在价值链管理框架下，资源并非“有或无”的简单状态，而是在供应链流转中经历**三种形态的跃迁**。本章重点阐明战术配额如何映射并驱动第一关的 IOP 引擎。

![Allocation -> Allotment -> Execution 约束转换图](file:///h:/IPC/方案/images/monograph_itp_allocation_to_allotment.png)

### 3.1 资源分配的三种物理状态（状态机理论）

#### 1. Assignment（物理实物匹配 / 自由分配）
在手物理库存（On-hand Inventory）与具体工单/订单项之间的 1:1 物理绑定。属于第一关中物料扫码入库（GR）后的强排他性实体装配。

#### 2. Allocation（战术配额划分 / 软性配额）
主计划（ITP）在周/月桶维度，根据 Combine Hierarchy 以及 Commitment Level，将未来的预计可用供应（Total Supply）划分给特定战略 Segment 的**名义配额**。

#### 3. Allotment（约束性配额 / IOP 硬性上限）
将 Allocation 结果下发给第一关 IOP 引擎的**硬性消耗上限（Hard Cap）**：
$$\text{Supply assignment to Segment is free within Allotment cap, but Total Execution Quantity} \le \text{Allotment Limit}$$

---

### 3.2 Allotment 在微观 IOP 排产引擎中的控制演练

我们以具体案例展示 Allotment 如何注入并控制第一关的 **OTP 推拉算法** 与 **动态软预留机制**：

```
+-----------------------------------------------------------------------------------+
|                        ITP Master Planning Layer                                  |
|   Segment: NA-America | Part: B | Bucket: W1 (Week 1) | Allocation/Allotment = 200    |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼ (下发硬性配额上界 Allotment Cap = 200)
+-----------------------------------------------------------------------------------+
|                        IOP Execution Planning Layer (第一关引擎)                    |
|                                                                                   |
|  [Day 1 Supply: 700] ──► OTP Forward Phase (寻找物理极限 Best Can Do)              |
|                          └─► 强制叠加 Gating Barrier: Segment 累计消耗 <= 200     |
|                                                                                   |
|  [PSD 契约定锚]      ──► OTP Backward Phase (从 PSD 向左逆向倒推最晚投产)            |
|                          └─► 基于 Priority Spectrum 建立 软预留保护墙              |
|                                                                                   |
|  [车间实际执行]      ──► Day 1/2/3 物理配载装配不超过 200 个，剩余 500 留存通用池       |
+-----------------------------------------------------------------------------------+
```

#### 步骤 1：主计划配额下发 (Allocation $\to$ Allotment)
主计划 Allocation 节点计算得出：零部件 $B$ 在 $W_1$（第一周）针对 `NA-America` Segment 的 **Allocation 为 200**。该数值作为 Allotment 硬性指令下发给 IOP 引擎：
- **Part**: $B$, **Bucket**: $W_1$, **Segment**: `NA-America`, **Allotment Limit**: $200$.

#### 步骤 2：对第一关 OTP 正向推演 (Forward Phase) 的约束注入
当 IOP 接收到该 Segment 的具体订单（如 $D_6=100$, $D_7=150$）时，启动第一关的 **OTP 正向推演算法** 寻找最早可交付日期（Best Can Do）：
- 虽然 Day 1 物理到货高达 700 个，但 OTP 正向推演在扫描到物料 $B$ 的可用性时，**必须将可用性上限强制截断为 Allotment 限额（200 个）**！
- 因此，OTP 判定 $D_6 (100)$ 和 $D_7$ 中的 $100$ 个可以在 Day 1 达成齐套，而 $D_7$ 剩余的 $50$ 个被判定为受配额限制（Allotment Exceeded），推迟至后序时间桶或触发战术增补。

#### 步骤 3：对第一关 OTP 反向拉动 (Backward Phase) 与软预留 (Soft Reservation) 的保护
- 依据 OTP 算出的 Best Can Do 锚定 **计划发货日 (PSD)**，PSD 作为已承诺契约红线；
- 启动 **Backward 反向拉动**：以 PSD 为终点向左倒推最晚投产时刻，消灭车间 WIP 堆积；
- **软预留 Protection Wall 建立**：所有在 Allotment 200 额度内的订单，在 IOP 内部获得最高等级的软预留保护（Soft Reservation），防止被外大区或低优先级订单在微观排产中抢占。

---

## 第四章 供不应求时的配额分配规则与插单处理

当全局供应链陷入瓶颈（Gating）时，战术层如何通过“短期战略优先，长期公平公正 Fair Share”重新分配资源，并与第一关的 **紧急插单资源置换 (Resource Swapping)** 联动？

![多层级战术配额划分图](file:///h:/IPC/方案/images/monograph_itp_hierarchy_allocation.png)

### 4.1 配额分配规则与双重时间策略

1. **Commitment Level（承诺等级）**：High (战略大客户/核心阵地) / Med (标准成熟区) / Low (散户/现货)。
2. **三大 Allocation Rules**：
   - **FIFS (先到先得)**：适用于非瓶颈场景。
   - **FairShare (按比例切片)**：按需求比例分摊稀缺供应：
     $$\text{Assigned}_i = \text{Total Supply} \times \frac{\text{Demand}_i}{\sum_{j} \text{Demand}_j}$$
   - **EqualShare (均分资源)**：忽略规模差异平分供应：$\text{Assigned}_i = \frac{\text{Total Supply}}{N}$。
3. **双重时间策略**：
   - **短期（1–4周）—— 战略优先 (Strategic Priority)**：100% 优先保障 Commitment Level = High 的战略 Segment，剩余资源再分配给其他 Segment。
   - **长期（5–16周+）—— 公平份额 (Fair Share)**：按需求比例平滑切片，防止次要市场长期断供。

---

### 4.2 经典多层级博弈与微观资源置换 (Resource Swapping) 联动

设某主板零件 $B$ 总供应为 **1700** 个：

#### 1. 战术层 Allocation 博弈演算
- **GEO 层级**：`PRC` (High 承诺) 战略优先，划走 $800$；Surplus $900$ 留给 `Others` (Mid 承诺)。
- **Sub Geo 层级**：`Others` 的 $900$ 配额按 FairShare (比例 5:4) 分配给 `NA` ($500$) 与 `EUR` ($400$)。
- **Country 层级**：`NA` 的 $500$ 配额按 EqualShare 均分给 `US` ($250$) 与 `CA` ($250$)。

#### 2. 与第一关 IOP 资源置换 (Resource Swapping) 引擎的联动控制
当在周中发生突发战略插单或供应下调，导致战术层调整 Allotment 时，**IOP 引擎立即调用第一关第三章的 4 步资源置换流程**：

```
[战术 Allotment 变更] ──► 1. 确认新插单/战略Segment的高 Priority 等级
                           │
                           ▼
                      2. 全网扫描 IOP 已分配资源与软预留状态
                           │
                           ▼
                      3. 精准剥离 (De-allocation) 低 Priority 任务的物料与机台
                           │
                           ▼
                      4. 被置换任务平滑二次排期 (Smooth Rescheduling)
```

- **联动效果**：战术层的配额调整不需要人工开会协调，也不需要在 IOP 中“暴力插队瘫痪全厂”，而是通过资源置换机制，将冲击局限在局部，既守护了战略 Segment 的交期（准），又保障了整厂的流动速度（快）。

---

### 4.3 跨时间桶切片与净需求满足 (Netting)

在跨时间桶（Bucket 1~4）供需错配场景中：

| 指标 | Bucket 1 | Bucket 2 | Bucket 3 | Bucket 4 | 业务机制 |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **需求 (Demand)** | 200 | 240 | 220 | 140 | 总需求 = 800 |
| **供应 (Supply)** | 180 | 180 | 180 | 260 | 总供应 = 800 |
| **净缺口 (Netting)** | **-20** | **-60** | **-40** | **+120 (Surplus)** | 跨 Bucket 切片满足 |

**算法联动**：主计划识别出 Bucket 4 存在 120 个 Surplus 供应，启动向左切片（Left-Shifting Pull-forward），将其拉动分配给 Bucket 1(-20)、Bucket 2(-60)、Bucket 3(-40) 的 Netting 缺口。**IOP 引擎接纳该拉动指令后，自动调用第一关 OTP 的 Backward 反向拉动算法**，精准计算出这 120 个零部件在 Bucket 1~3 的最晚开工时间，消灭车间 WIP 呆滞。

---

## 第五章 主计划三大核心模块与微观排产的对接

```
                    +---------------------------------------+
                    |    Consensus Forecast (S&OP 层)        |
                    +---------------------------------------+
                                        │
                                        ▼
                    +---------------------------------------+
                    |  1. MRP Netting & Forecast Consumption |
                    |     (收敛第一关需求范围)              |
                    +---------------------------------------+
                                        │
                                        ▼
                    +---------------------------------------+
                    |  2. MPS & What-if Simulation Engine   |
                    |     (调用第一关 OTP 引擎进行极速试算)   |
                    +---------------------------------------+
                                        │
                                        ▼
                    +---------------------------------------+
                    | 3. Supplier Collaboration & Pegging   |
                    |    (PO/SR/ETA/GR 映射第一关软硬预留)    |
                    +---------------------------------------+
                                        │
                                        ▼
                    +---------------------------------------+
                    |   IOP 微观执行引擎 (第一关 OTP & 5重流程) |
                    +---------------------------------------+
```

### 5.1 MRP Netting 与 Forecast Consumption（预测冲销）

第一关第 1 章指出，若需求范围割裂，会导致“先到先得踩踏战略订单”。
- **Forecast Consumption（预测冲销）** 机制通过向后（Backward）与向前（Forward）冲销窗口，用真实的 Customer Order 动态冲销 Consensus Forecast：
  $$\text{Unconsumed Forecast}(t) = \max\left( 0, \text{Original Forecast}(t) - \sum \text{Customer Orders}(t \pm \Delta t) \right)$$
- 冲销后的净需求直接注入第一关的 IOP 订单池，确保 IOP 既能为未来高优先订单卡位，又不会发生需求重复叠加计算。

---

### 5.2 MPS（主生产计划模拟与 What-if 决策）

MPS 混合整数规划（MILP）在评估“大单插入”或“芯片断供”的 What-if 场景时，**直接调用第一关的 OTP 推拉引擎进行极速求解**：
- 计划员在战术界面发起 What-if 模拟；
- 战术层下发临时 Allotment 方案到 IOP 仿真沙盒；
- IOP 引擎在数秒内运行 OTP 正向/反向推演，精准反馈该方案对 **PSD 交期兑现率（准）**、**车间 WIP 堆积率与库存周转率（快）** 的量化影响。

---

### 5.3 供应商协同与 Pegging 链路（PO, SR, ETA, GR Pegging）

第一关第 1 章解析了“仅看仓库在手实物导致不齐套死锁”的痛点。ITP 的供应商协同模块通过端到端 Pegging 彻底破解：

```
Segment Allocation ──► PO (采购订单) ──► SR (发货通知) ──► ETA (预计到港) ──► GR (入库验收)
                                                                             │
                                                                             ▼
                                                                   IOP 硬物理锁定 (Assignment)
```

1. **PO / SR / ETA Pegging**：将战术 Allocation 与上游供应商的发货通知（SR）和物流在途（ETA）进行动态软绑定。
2. **GR (Goods Receipt) 瞬间状态跃迁**：当物料在海关清关并完成扫码入库（GR）时，第一关的 **动态软预留 (Soft Reservation)** 瞬间跃迁为 **硬性物理匹配 (Assignment)**，直接驱动车间装配排产。

---

## 第六章 主计划与执行排产闭环架构

### 6.1 主计划与执行排产闭环架构

将第一关的“横向五重流程对齐”与第二关的“纵向三层协同”融为一体，企业交付大脑的闭环架构如下：

```
                               【纵向三层协同】
                        S&OP 战略共识 Forecast
                                 │
                                 ▼ ( Allocation 算法 )
                        ITP 战术配额 Allotment Caps
                                 │
                                 ▼ ( 约束上界屏障 Gating )
┌────────────────────────────────────────────────────────────────────────┐
│                        IOP 运营执行层 (第一关微观引擎)                   │
│                                                                        │
│   【横向五重对齐】                                                       │
│   采购向量 ──► 生产向量 ──► 交付向量 ──► 调度向量 ──► Call料向量        │
│      │            │            │            │            │             │
│      └────────────┴──────┬─────┴────────────┴────────────┘             │
│                          │                                             │
│                          ▼                                             │
│               OTP 推拉算法 (Forward & Backward)                         │
│               软预留 (Soft Reservation) & 资源置换 (Swapping)            │
└────────────────────────────────────────────────────────────────────────┘
```

- **纵向控制流**：S&OP 共识 $\to$ ITP 战术 Allocation $\to$ IOP 硬性 Allotment Cap；
- **横向动力流**：采购、生产、交付、调度、Call料五重流程收敛同向；
- **微观计算流**：OTP 引擎在 Allotment Cap 护栏内，利用软预留与置换算法求解 PSD，消除 WIP 呆滞。

---

### 6.2 闭环 KPI 衡量体系

1. **Allocation Compliance Rate (配额遵从率)**：考核 IOP 是否在 Allotment 护栏内排产，严禁超配额越权排产。
2. **OTP Delivery On-Time Rate (OTP 交期兑现率)**：考核在 Allocation 护栏与 IOP OTP 算子协同下，战略订单的 100% 兑现。
3. **WIP Turnover Velocity (在制品周转速度)**：考核通过 IOP 反向拉动（Backward Phase），车间 WIP 压减与周转率提升效果。

---

### 6.3 核心结论

主计划（ITP）绝非独立于车间执行的空头政治，执行计划（IOP）也绝非缺乏战略眼光的“微观近视”。通过**同源而不同尺度的架构设计**、**Assignment $\to$ Allocation $\to$ Allotment 的状态跃迁**、**“短期战略优先、长期公平公正”的配额机制**，以及**与第一关 OTP 推拉算法、软预留、资源置换的深度联动**，企业才能建立起“战略有护栏、微观有极速、交付又准又快”的交付体系。
