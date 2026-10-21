---
table_name: "ipc_demand_status"
alias: "demand_status"
module: "7_Control_Data_Model_CDM"
cpp_struct: "DemandStatusRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_demand_status` (demand_status)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 与此需求状态记录相关联的控制集 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 需求状态描述 |
| `forecast_against_date_rule` | forecast_against_date_rule | `VARCHAR(10)` | Nullable | 指定具有此状态的任何需求的预测消费间隔的日期
有效值为:
DataDate - 数据需求预测的消费区间是根据时间较晚的DueDate或Calendar中DataDate部分来设定的。DataDate(通常对应于今天的日期)。因此，准时的独立需求将使用基于其DueDate的预测，而过期需求将使用基于数据DataDate的预测(通常比它们在其到期日消耗更多的预测)。

DueDate - 根据DueDate值(DueDate为根据客户的需求交付到客户的日期减去运输LT和Buffer推算出来的需要准备好产品的日期，或者是企业自己定义的产品准备的LT+当前日期)，设定DueDate -需求预测的消耗区间。这意味着所有处于这种状态的独立需求，无论是准时的还是延迟的，都将根据它们的时间消耗预测。

MaterialType-需求预测中，根据物料的类型确定消耗区间。ForecastConsumptionDateRule设置。此设置仅包含“DueDate”和“DataDate”选项，不支持使用RequestDate来确定预测消费。 |
| `consumption` | consumption | `VARCHAR(10)` | Nullable | 设置此需求行项目是否参与Forecast Consumption
有效值为:
N-不参与
Y-参与 |
| `model` | model | `VARCHAR(10)` | Nullable | 指示如何确定与独立需求相关联的模型。该模型在分析计算中的实际应用则由物料的MUEPoolNetting.Type决定。ModelRule价值。有效值为:
N - model设置为默认模型;这是加载到模型表中的第一个值，通常值为“None”。在独立需求中提供的输入值。模型忽略了。
Y-正规模型设置为独立需求模型中提供的输入值 |
| `netting_rule` | netting_rule | `VARCHAR(10)` | Nullable | 指定是否在Netting计算中使用该需求。有效值为:
N-需求被网络忽略
Y-当前需求被网络使用(至少有一个需求状态值必须使用此设置) |
| `demand_line_item` | demand_line_item | `VARCHAR(10)` | Nullable | Set DemandOrderLine |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：销售订单生命周期状态机控制与预分配释放
* **因果流向**：`ipc_demand_status` 控制销售订单在系统中的当前物理状态（如：Open、Hold、Allocated、Shipped）。若订单状态跃迁为 `CANCELLED` (取消) 或 `CREDIT_HOLD` (信用冻结)，计划引擎在下一次 FIFO 分配时，会自动将其已预占的库存 Pegging 记录（`ipc_supply_assignment`）释放，让渡给其他合格订单。
* **状态机转换逻辑**：
  - `OPEN` ➔ `ALLOCATED`：锁定库存或产能预占，生成 Pegging。
  - `CREDIT_HOLD`：释放 Pegging，不拉动 MRP。
  - `SHIPPED`：永久消退在手物理库存，清除时序 Pegging。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 需求生命周期状态
enum class DemandState : uint8_t {
    OPEN = 0,
    CREDIT_HOLD = 1,
    ALLOCATED = 2,
    SHIPPED = 3,
    CANCELLED = 4
};

// 对应 ipc_demand_status 的内存物理结构
struct DemandStatusRecord {
    uint32_t demand_id;          // 需求 ID (对应 demand)
    DemandState current_state;   // 当前状态 (对应 demand_status)
    int state_change_day;        // 状态变更相对天数
};
```