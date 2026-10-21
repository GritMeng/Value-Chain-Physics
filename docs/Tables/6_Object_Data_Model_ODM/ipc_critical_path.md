---
table_name: "ipc_critical_path"
alias: "critical_path"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_critical_path` (critical_path)

> **业务说明**: 报告项目关键路径上所有任务的计算表。这提供了一种方法来识别那些直接影响项目完成日期的任务，以及限制这些任务的任何相关项(例如，任务可能根据材料需求进行限制)。还显示由特定项进行门控的所有任务的详细信息，而不管门控任务是否在关键路径上

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `is_critical` | is_critical | `VARCHAR` | Nullable | Y/N |
| `gating_demand` | gating_demand | `VARCHAR` | Nullable | Reference:IndependentDemand
 |
| `gating_supply` | gating_supply | `VARCHAR` | Nullable | Reference:SR |
| `gating_task` | gating_task | `VARCHAR` | Nullable | Reference:Task |
| `project` | project | `VARCHAR` | 🔑 **PK / Required** | - |
| `task` | task | `VARCHAR` | Nullable | - |
| `source` | source | `VARCHAR` | Nullable | 指示依赖项的类型(如果有的话)，该类型限制了在此记录上报告的任务并将其置于项目的关键路径上。
Constraint
Demand
None
Task
Supply |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：项目延迟归因与门控分析
* **因果流向**：`ipc_critical_path` 收集 CPM 算出的 $TF=0$ 的任务，计算是何种具体外部依赖（Constraint-产能不足, Supply-原料在途迟到, Demand-独立需求抢占）卡住了项目，为计划员提供清晰的**门控阻碍分析（Gating Analysis）**。