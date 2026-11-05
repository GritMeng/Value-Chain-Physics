---
table_name: "ipc_forecast"
alias: "forecast"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastEvent"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast` (forecast)

> **业务说明**: 该表中的记录显示了类型为SalesForecast的所有需求记录的摘要。原始预测数量与销售实际消耗的总消费数量一起报告。在零件需求时间范围之外的任何剩余未消耗的预测量都是应该计划的有效需求量。
预测可从独立需求看，依赖需求可从计划订单看
(PlannedAllocation和PlannedTransferAllocation)和共识预测需求。如果SupplyType，也可以从预定的收据中生成预测。AllocationForecastRule设置为“Use”。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `consensus_forecast` | consensus_forecast | `VARCHAR` | Nullable | 共识预测销售收入 (Qty * UnitPrice) |
| `consumed_qty` | consumed_qty | `VARCHAR` | Nullable | 实际需求所消耗的预测量的数量(实际用
独立需求记录与一个
“SalesActual”的Order.Type.OperationRule设置)
 |
| `date` | date | `VARCHAR` | Nullable | - |
| `eff_demand_qty` | eff_demand_qty | `VARCHAR` | Nullable | 冻结期之外的还未被冲销的数量 |
| `eff_unit_price` | eff_unit_price | `VARCHAR` | Nullable | 单价对预测的部分和客户有效。此值在确定收入时很有用，并且仅在ForecastSource设置为时计算
“独立”或“共识预测”。对于这些预报源，该字段的计算如下:
Material.AverageSellingPrice<CustomerPrice.UnitPrice<MaterialCustomer.UnitPrice |
| `forecast_source` | forecast_source | `VARCHAR` | Nullable | ConsensusForecast
Independent |
| `independent_demand` | independent_demand | `VARCHAR` | Nullable | - |
| `material` | material | `VARCHAR` | Nullable | - |

| `material_customer` | material_customer | `VARCHAR` | Nullable | - |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：共识预测消纳与冲销
* **因果流向**：`ipc_forecast` 是 ITP 战术计划的主需求流。在滚动计划周期内，当实际客户订单（Sales Order）录入时，引擎依据消纳规则对预测进行冲销。
* **冲销编排逻辑**：
  - 动态冲销区间：读取 `ipc_part_site` 中的 `before_forecast` 和 `after_forecast` 区间窗口，沿时间轴双向检索可用预测量。
  - 预测冲减：当实际订单发生时，扣减对应时段的预测值，增加 `consumed_qty`。
  - 净需求下传：冲销后剩余的未消费预测量 `eff_demand_qty` 作为净需求，与实际未交货订单一起作为拉动补货（MRP/CTP）的输入源，防止重复备料。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，预测量在时间轴上表示为区间段的离散需求事件。冲销过程通过一维数轴上的前缀和滑动窗口进行代数消纳：
```cpp
// 预测消纳事件结构
struct ForecastEvent {
    uint32_t part_id;
    int due_day;
    double original_qty;    // 原始预测量 (对应 qty)
    double consumed_qty;    // 已冲销数量 (对应 consumed_qty)
    double effective_qty;   // 剩余净预测量 (对应 eff_demand_qty)
};
```