---
table_name: "ipc_part_source"
alias: "part_source"
module: "6_Object_Data_Model_ODM"
cpp_struct: "SourcingRuleRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_part_source` (part_source)

> **业务说明**: 定义一个物料的供应源. 

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `source` | source | `VARCHAR(10)` | PK / NOT NULL | 来源
Reference Table:Source |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 启用日期 |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | 失效日期 |
| `max_qty` | max_qty | `DECIMAL(18,2)` | Nullable | 最大订单数量（在一个时间段内可以下达多份计划订单）。
如果 OrderPolicy.MaximumUsage 被设置为“use”或“average”，那么从该零部件来源生成的任何计划订单中的数量都不会超过此字段中的值（如果在一个时间段内达到每个时间段的最大订单数量，则最后一个计划订单除外）。
然而，在 Max_usage 被设置为“champain”时，此字段表示在一个生产活动中所有供应批次（订单）的最大计划订单数量，并且一旦达到该最大值，必须开始一个新的活动。换句话说，在给定的生产活动中允许的最大批次数量被定义为 MaximumQty/MultipleQty（必要时向上取整到下一个整数）。 |
| `min_qty` | min_qty | `DECIMAL(18,2)` | Nullable | 下订单的最小数量 |
| `mul_qty` | mul_qty | `DECIMAL(18,2)` | Nullable | 下订单的时候，每张订单的数量要是其倍数 |
| `dim_grp` | dim_grp | `VARCHAR(10)` | Nullable | 应对特殊需求时所用到维度组，例如某些特殊类型订单或客户指定供应源。
Reference Table: DimensionGroup |
| `ptf_intervals` | ptf_intervals | `DECIMAL(18,2)` | Nullable | 冻结期从RunDate开始的期间数 |
| `ptf_units` | ptf_units | `VARCHAR(10)` | Nullable | 冻结期的时间单位
Reference Table: Calendar |
| `source_type` | source_type | `VARCHAR(10)` | Nullable | 供应类型,如采买，制造，和补货
Reference Table: SourceType |
| `source_uom` | source_uom | `VARCHAR(10)` | Nullable | 获取该物料的单位。 |
| `dock_to_stock_lt` | dock_to_stock_lt | `DECIMAL(18,2)` | Nullable | 将物料从收货区搬运至可用库存所需的时长。此字段的设置会增加所有类型供应的有效前置时间；除非有特殊需要，否则请将其设置为 0。其测量单位为“part.planning_calendars.time_unit” |
| `var_lt` | var_lt | `DECIMAL(18,2)` | Nullable | 根据订单数量而变化的LT |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `priority` | priority | `INTEGER` | Nullable | 优先级，数值越小越优先 |
| `allotment` | allotment | `VARCHAR` | Nullable | 关联手工预留 |
| `alt_bom` | alt_bom | `INTEGER` | Nullable | BOM的替换号，非BOM ID |
| `coproduct_yield` | coproduct_yield | `DECIMAL(18,2)` | Nullable | 表示主·产品实际作为主产品供应的生产百分比。其余部分根据相关物料清单记录中提供的“每单位数量”值在副产品和副产品之间分配。
此字段遵循order_policy_yield_usage字段设置，在yield字段之后应用，并始终在Netting中予以考虑。如果不使用副产品和副产品逻辑，则应将此字段设置为 0（表示应忽略该字段）。
Default:0
当启用时间段时，将会应用time_phase_yield代替它。 |
| `fixed_lt` | fixed_lt | `DECIMAL(18,2)` | Nullable | order release 日期要比到其日推迟多久
 |
| `order_policy` | order_rule | `VARCHAR(10)` | Nullable | 计划单据产生的规则，关联order_rule |
| `par_site` | par_site | `VARCHAR(8)` | Nullable | 场所相当于SAP的Plant. |
| `par_part` | par_part | `VARCHAR(40)` | Nullable | Assemble物料号
Reference Table: [[ipc_part|Material]] |
| `bomid` | bomid | `VARCHAR(40)` | Nullable | BOM编号
Reference Table: [[ipc_bom_route|BOM]] |
| `mrp_rule` | mrp_rule | `VARCHAR(10)` | Nullable | 唯一标识 |
| `ptf` | ptf | `DECIMAL(18,2)` | Nullable | planning time fence, 用于确定使用此物料来源时能够最早规划新订单的时间（依据“order_rule.ptf_rule”字段中的设置）。此字段应以物料或来源的时间单位来表示（依据“order_rule.ptf_rule”字段中的设置）。 |
| `labor_cost` | labor_cost | `DOUBLE` | Nullable | 单位人力成本，单位在相关的supply_unit中。 |
| `part_cost` | part_cost | `DOUBLE` | Nullable | 单位物料成本，用在cost forecast以及销售成本中，在替换资源选择上也会考量 |
| `oh_cost` | oh_cost | `DOUBLE` | Nullable | 单位间接成本 |
| `alt_assignment_intervals` | alt_assignment_intervals | `INTEGER` | Nullable | 多少time units不能满足需求，去寻找替换的资源：
<0 - 直到所有资源都用完，才会考虑替换的资源。
=0 - 先看当前的替换资源，然后看更早期的资源，即N字形消耗
>0 - Z字形消耗，看多少时间单位的资源之后就会看替换资源 |
| `due_lt` | due_lt | `DECIMAL(18,2)` | Nullable | 需求日期减去此此提前期为供应计划开始考量的日期。此措施旨在防止供应和需求的波动，并会对计划的订单交付日期、EffLeadTime（平均交付周期）以及CumLeadTime（累计交付周期）的计算结果产生影响。这个值应以该物料的“planning_calendar.time_units”日历或“日常/公历”日历形式（具体取决于该部件的“部分类型.安全提前期日历规则”设置）来表示。
请注意，此字段的使用取决于“部分类型.使用安全提前期”字段的设置，此外还有部分类型的其他设置可用于指定针对特定类型需求应用安全提前期的方式。 |
| `shipping_cost` | shipping_cost | `DOUBLE` | Nullable | 此供应源的发货标准成本 |
| `pre_plan` | pre_plan | `DECIMAL(18,2)` | Nullable | 决定可以提前多久plan, 例如在主计划中forecast可以提前多少个周期生产

当维护<0时，代表不能提前 |
| `after_final_operation` | after_final_operation | `DECIMAL(18,2)` | Nullable | 屯多久的WIP |
| `before_first_operation` | before_first_operation | `DECIMAL(18,2)` | Nullable | 待料多久 |
| `safety_lt` | safety_lt | `DECIMAL(18,2)` | Nullable | due date 之后留出安全前置期开始supply plan. |
| `takt_time` | takt_time | `DECIMAL(18,2)` | Nullable | 此字段表示当某部件的需求量超过 MaximumQty 时，连续计划订单之间的时间间隔单位数（通常为工作日）。时间单位由 part.planning_calendars.time_units 定义。

字段值 < 0：从供应原定到期日向后分散计划订单。
字段值 > 0：从供应原定到期日向前分散计划订单。
字段值 = 0：允许在同一天创建多个计划订单以满足需求。
特殊场景：
若分散后的计划订单在达到 PTFDate（计划截止日期）时仍未完成全部需求量，则系统会在 PTFDate 当天生成多个订单 |
| `target` | target | `DECIMAL(18,2)` | Nullable | 此字段表示当存在多个有效部件来源（优先级相同）可满足某部件的计划需求时，该来源的目标分配值。

作用：根据目标值比例将计划需求拆分到不同来源。
分配规则：由 source_rule.assignment_rule 字段决定：
按比例分配：每个需求按来源的目标值比例拆分。
单一来源优先：尽可能由单个来源完全满足每个需求。
默认值：0
特殊规则：若所有有效来源的 Target 均为0，则需求均分到所有来源。 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：动态货源补货抉择与时空偏置
* **因果流向**：`ipc_part_source` 表是整个有限能力排程引擎（IOP）确定物料补货策略、提前期及约束条件的核心。
* **算法编排控制流**：
  1. 补货性质识别：根据 `source_type` 确定物料是“制造”（Make，触发 BOM 爆炸和工序排产）、“采买”（Buy，触发供应商 PO 生成，受 `dock_to_stock_lt` 搬运偏置影响）还是“转储”（Transfer，触发跨厂 STO，受调拨提前期偏置影响）。
  2. 交期偏置计算：根据 `fixed_lt` 与 `var_lt` 动态确定计划订单的开工期，并通过 `takt_time` 控制超量需求订单的分散铺排，规避瞬间负荷过载。
  3. 替代检索行为：读取 `alt_assignment_intervals` 天数：
     - 若小于 0，采用 N 字形消耗，依次将本地所有可用现有量与在途消化完，才看替代；
     - 若大于 0，采用 Z 字形消耗，超过设定天数后的需求直接路由至替代货源，降低近期的库存缺料风险。
  4. 比例拆分：在多货源同优先级下，读取 `target`，按配额比率分裂生成多笔计划订单。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 中，`ipc_part_source` 的逻辑属性被编译进 `PartSiteRecord`（物料站点配置）中，并在求解器初始化时绑定对应的工艺和 BOM 索引，消除了运行时的关系表关联：

```cpp
// 对应 ipc_part_source 在内存中的编译对齐体 (部分与 PartSiteRecord 融合)
struct SourcingRuleRecord {
    uint32_t part_id;
    uint32_t source_id;
    uint8_t source_type;          // 供应源类型 (MAKE=0, BUY=1, TRANSFER=2)
    double min_order_qty;          // 最小起订量 (对应 min_qty)
    double max_order_qty;          // 最大订单上限 (对应 max_qty)
    double multiple_qty;           // 包装订单倍数 (对应 mul_qty)
    int planning_time_fence_days;  // 计划时间屏障天数 (对应 ptf)
    int alternative_search_days;   // 替代料搜索天数窗 (对应 alt_assignment_intervals)
};
```