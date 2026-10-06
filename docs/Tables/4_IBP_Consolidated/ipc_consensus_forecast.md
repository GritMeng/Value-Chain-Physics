---
table_name: "ipc_consensus_forecast"
alias: "consensus_forecast"
module: "4_IBP_Consolidated"
cpp_struct: "ConsensusForecastHeader"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_consensus_forecast` (consensus_forecast)

> **业务说明**: IBP 共识需求预测表。多部门（销售、财务、运营）共识后的滚动预测需求，支持单价及 override 数量修改，动态联动重新计算营业收入。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `cal_qty` | cal_qty | `DECIMAL(18,2)` | Nullable | 基于weight的数量 |
| `date` | date | `DECIMAL(18,2)` | Nullable | 一致性预测对应的日期 |
| `unit_price` | unit_price | `DECIMAL(18,2)` | Nullable | 单价，用来计算revenue  |
| `override_qty` | override_qty | `DECIMAL(18,2)` | Nullable | - |
| `customer` | customer | `VARCHAR(10)` | Nullable | - |
| `reba_adjustment_qty` | reba_adjustment_qty | `DECIMAL(18,2)` | Nullable | 在重新平衡需求计划到供应计划时，对计算出的一致预测或预测覆盖数量(如果指定)所做的调整。 |
| `reba_override_qty` | reba_override_qty | `DECIMAL(18,2)` | Nullable | - |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `region` | region | `VARCHAR(10)` | Nullable | - |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `allocation_level` | allocation_level | `VARCHAR(10)` | Nullable | - |
| `order_priority` | order_priority | `VARCHAR` | Nullable | - |
| `consensus_forecast` | consensus_forecast | `VARCHAR` | PK / NOT NULL | 共识预测销售收入 (Qty * UnitPrice) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：S&OP 跨职能共识预测汇总与财务折算
* **因果流向**：`ipc_consensus_forecast` 是 S&OP 的核心控制层，汇总了经过各职能部门（销售、市场、供应链、财务）对账共识后的多维主需求计划。该表中的预测数量直接决定了 MPS (主生产计划) 的主拉动负荷，并用于评估未来营收达成率。
* **计算逻辑编排**：
  1. 多维汇总：根据产品系列、区域或客户层级汇总详细预测明细：
     $$ Consensus\_Val(t) = \sum_{sku \in Hierarchy} Qty_{sku}(t) \times Price_{sku}(t) $$
  2. 预算对账：对比财务年度预算（Financial Budget）线，计算偏差比率。若偏差超出准入阈值，触发需求整形（Demand Shaping）或促销拉动策略调整。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存计算中，共识预测表头部字段被扁平化存储在连续的 `ConsensusForecastHeader` 数组中，便于在时序汇总循环中快速检索并减少缓存未命中：
```cpp
// 对应 ipc_consensus_forecast 的 C++ DOD 结构体
struct ConsensusForecastHeader {
    uint32_t consensus_id;      // 共识计划ID (对应 consensus_forecast)
    uint32_t hierarchy_node_id;  // 聚合节点逻辑编码 (如产品族)
    int start_day;              // 计划期起始相对天数
    int end_day;                // 计划期结束相对天数
    double target_revenue;      // 目标营业额
    double approved_qty;        // 审核通过的计划总量
};
```

###### 3. 边界与异常处理
* **跨时区日历转换偏差**：各销售大区的日历时区若有不一致，引擎在加载时会将日期统一转换为 UTC 的绝对天数偏移，避免由于跨时区引起的需求在桶边界处重复计算或遗漏。