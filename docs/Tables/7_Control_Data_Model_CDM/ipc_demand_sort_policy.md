---
table_name: "ipc_demand_sort_policy"
alias: "demand_sort_policy"
module: "7_Control_Data_Model_CDM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_demand_sort_policy` (demand_sort_policy)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `demand_policy_num` | demand_policy_num | `VARCHAR(10)` | Nullable | DemandPolicy的唯一标识符 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `sequence_num` | sequence_num | `INTEGER` | Nullable | 排序编号，编号越小优先级越高 |
| `material` | material | `VARCHAR(40)` | Nullable | Set |
| `material_type` | material_type | `VARCHAR(10)` | Nullable | Set |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table: ControlGroup |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：需求分排队策略
* **因果流向**：`ipc_demand_sort_policy` 定义了多级订单到达后的排序分配优先级。在 IOP 级消纳前，引擎加载此表中的 `sequence_num`，对各物料的需求进行洗牌排序。
* **物理内存结构**：引擎读取排序规则，直接作用于 `IndependentDemand` 数组的 `std::sort` 排序条件中，保证后续 CTP 预占资源时，高优先级的需求先抢占库存。