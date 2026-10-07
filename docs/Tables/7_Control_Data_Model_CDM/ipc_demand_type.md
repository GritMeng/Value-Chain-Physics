---
table_name: "ipc_demand_type"
alias: "demand_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "DemandTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_demand_type` (demand_type)

> **业务说明**: DemandType表定义了需求类型的值。在independent_demand_item表中使用它来识别不同类型的需求及其处理规则。它还与其他需求表和控制表一起使用，以指定需求处理行为。必须定义至少一个DemandType。通常情况下，对于以下每种需求至少有一个DemandType:客户或销售订单、预测、发货和相关需求。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 |
| `demand_type` | demand_type | `VARCHAR(10)` | PK / NOT NULL | 需求状态类型的字符串值 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | 决定在MRP Netting 如何处理需求。
有效值是:
DependentDemand - 用于逐个组件的预测冲减。每个组件的依赖需求即由其上层物料的需求通过BOM展开驱动的需求。被视为SalesActual(为预测冲销处理)或Regular(为预测冲销未处理)，由PartType.DependentDemandForecastConsumption中的设置确定是否可以冲减组件的预测。这个值不应该用在独立需求记录引用的需求类型记录中。如果是，那么这些独立的需求记录将被视为“常规”需求。

Forecast——预期会有实际销售发生的预测。它会被实际销售消耗(减少)了。在MRP Netting中，只使用需求时间围栏(DTF)上或之后的未消耗预测作为需求。销售需求使用物料计划日历上预测间隔范围内的销售预测。

SalesOrder -实际需求预测的销售预测。需求的LineItem的Qty是还需要的数量。加上装运数量字段用于消费销售预测;然而，在MRP Netting中，只有数量字段是实际使用的需求。定义的需求类型中至少有一个必须将ProcessingRule设置为SalesOrder。

Regular - 不参与Consumption，参与Netting。 |
| `speard_setting` | speard_setting | `VARCHAR(10)` | Nullable | 指定该demand是否采用平铺技术
Y - 采用
N- 不采用 |
| `spread` | spread | `VARCHAR(10)` | Nullable | Reference Table: SpreadProfile |
| `safety_stock_usage` | safety_stock_usage | `BOOLEAN` | Nullable | 当SafetyStock用PercentOfDemand的时候，指定该类型的需求是否参与计算。
Y - 参与
N - 不参与 |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 被SupplyType多条记录引用。 SupplyType是相关需求时，其对应的DemandType为此DemandTyoe |
| `due_date_rule` | due_date_rule | `VARCHAR(10)` | Nullable | 1. RequesDueDate
2. StandardDueDate
3. LaterRequestStandard
4. PromisedDueDate(业务模式有可能是在询价，合同，下订单阶段就会给一个AvailableDate) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：需求性质分类与分配引擎路由分配
* **因果流向**：`ipc_demand_type` 维护了所有需求记录的性质分类（如：实际销售订单 SO、销售预测 FC、站点调拨 STO、安全库存补货 SS）。分配引擎根据该表配置，决定该需求在进行供需匹配时的默认优先级排队位权，以及它是否需要执行实时 CTP 有限能力预占。
* **路由控制编排**：
  - 提取需求的 `demand_type`。
  - 若为 `SO`：`requires_ctp = true`，触发有限能力 CTP 正排承诺。
  - 若为 `SS`：`requires_ctp = false`，不进行 CTP 实期预占，直接进入 MRP 倒排计算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应 ipc_demand_type 的 C++ 内存物理结构
struct DemandTypeRecord {
    uint8_t demand_type_id;          // 需求类型 ID (对应 demand_type)
    uint16_t default_priority_rank;  // 默认排序优先级
    bool consumes_forecast;          // 是否冲销消纳预测
    bool requires_ctp;               // 是否需要执行实时 CTP 承诺
};
```