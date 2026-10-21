---
table_name: "ipc_demand_header"
alias: "demand_header"
module: "7_Control_Data_Model_CDM"
cpp_struct: "DemandHeaderRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_demand_header` (demand_header)

> **业务说明**: DemandHeader表包含了DemandItem记录的所有公共(头)信息。
标题信息是关于整个订单的常见信息

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `customer` | customer | `VARCHAR` | Nullable | Reference:Customer   Customer, Site |
| `demand` | demand | `VARCHAR` | 🔑 **PK / Required** | 唯一标识 |
| `demand_type` | demand_type | `VARCHAR` | PK / NOT NULL | 确定处理的规则 |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：主需求账本头管理与场景版本标识
* **因果流向**：`ipc_demand_header` 维护了所有输入计划引擎的订单和预测的头数据，关联了对应的币种和计划员。在多场景克隆时，它携带场景版本 ID，供三路冲突 Diff 引擎判定修改源头。
* **物理内存结构**：
```cpp
// 对应 ipc_demand_header 的 C++ 内存物理对齐结构体
struct DemandHeaderRecord {
    uint32_t demand_header_id;   // 需求头 ID (对应 id)
    uint32_t planner_id;         // 关联的计划员 ID
    uint16_t currency_id;        // 结算币种 ID
    uint32_t scenario_id;        // 场景分支版本 ID
};
```