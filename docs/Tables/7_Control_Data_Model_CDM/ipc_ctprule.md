---
table_name: "ipc_ctprule"
alias: "ctprule"
module: "7_Control_Data_Model_CDM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_ctprule` (ctprule)

> **业务说明**: 可用性检查

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 |
| `id` | id | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | 是否需要做可用性检查
Type-参考物料类型
N- 需求数量参与运算，但认为可以满足。例如VMI, Bulk物料
Y-参与CTP运算 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：CTP 承诺准入控制规则
* **因果流向**：`ipc_ctprule` 决定了物料在进行交期承诺时是采用“无限能力粗略估计”还是“有限能力 DFS 精确预占”。
* **物理内存结构**：在 C++ 中映射为 `PartSiteRecord.ctp_rule` 的策略枚举与布尔标志位，决定 CTP 递归预占函数是否激活产能与替代料检查的分支逻辑。