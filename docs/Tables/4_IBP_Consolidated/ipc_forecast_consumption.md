---
table_name: "ipc_forecast_consumption"
alias: "forecast_consumption"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastConsumptionRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_consumption` (forecast_consumption)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `consensus_forecast` | consensus_forecast | `VARCHAR` | Nullable | 共识预测销售收入 (Qty * UnitPrice) |
| `consumed_qty` | consumed_qty | `VARCHAR` | Nullable | 实际需求所消耗的预测量的数量(实际用
独立需求记录与一个
“SalesActual”的Order.Type.OperationRule设置)
 |
| `material` | material | `VARCHAR` | PK / NOT NULL | - |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `actual_due_date` | actual_due_date | `VARCHAR` | Nullable | - |
| `actual_qty` | actual_qty | `VARCHAR` | Nullable | 总的实际数量 |
| `source` | source | `VARCHAR` | Nullable | - |
| `forecast` | forecast | `VARCHAR` | Nullable | - |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时序预测消纳与冲销关系度量
* **因果流向**：当实际的 `ipc_sales_order_line` (销售实际) 流入系统时，冲销引擎 (Consumption Engine) 将对 consensus forecast 进行冲减。该冲减过程生成一条 `ipc_forecast_consumption` 记录，用于精确追踪哪笔销售订单消减了哪部分预测数量，为计划员提供供需不匹配的追溯视图 (Traceability View)。
* **算法实现要点**：
  1. 冲销区间判定：提取 `ipc_part_site` 中的冲销向前/向后天数限制，形成时间轴上的滑动区间。
  2. 匹配与分摊：按照 FIFO (先进先出) 结合客户优先级，将实际销售订单数量分摊到区间内的预测事件上，计算：
     $$ Consumed\_Qty = \min(Remaining\_Forecast, Open\_Order\_Qty) $$
  3. 消耗映射：生成对冲后的剩余预测量，并在此表中产生一对一或一对多的 Pegging 记录。

###### 2. 物理内存结构设计 (C++ DOD Layout)
预测冲销关系在内存中由专门的 `ForecastConsumptionRecord` 扁平数组表示，支持高速的前向和后向关联查询：
```cpp
// 对应 ipc_forecast_consumption 的内存 DOD 结构体
struct ForecastConsumptionRecord {
    uint32_t forecast_id;          // 被消纳的预测记录逻辑ID
    uint32_t sales_order_line_id; // 触发消纳的实际销售订单行ID
    double consumed_qty;          // 被冲减消纳的数量 (对应 consumed_qty)
    int consumption_day;          // 消纳发生的计划天数
    uint32_t part_id;             // 物料ID
};
```

###### 3. 边界与异常处理
* **超期冲减豁免**：若销售订单的交期已超出预测冲销窗口的上限 (After Forecast Window) 或下限 (Before Forecast Window)，则该订单不执行冲销逻辑，直接作为额外独立需求拉动 MPS/MRP，防止在手库存水位被过度低估。