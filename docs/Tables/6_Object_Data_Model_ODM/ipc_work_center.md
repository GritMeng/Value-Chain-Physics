---
table_name: "ipc_work_center"
alias: "work_center"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_work_center` (work_center)

> **业务说明**: WorkCenter表描述了一个唯一的工作中心。该记录包括该工作中心的名称、描述、成本信息和一组可用能力记录。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `work_center` | work_center | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `calendar` | calendar | `VARCHAR(10)` | Nullable | Reference Table: Calendar |
| `type` | type | `VARCHAR(10)` | Nullable | Reference Table: WorkCenterType |
| `department` | department | `VARCHAR(10)` | Nullable | - |
| `std_labor_run_cost` | std_labor_run_cost | `DOUBLE` | Nullable | 每小时的成本 |
| `std_labor_set_up_cost` | std_labor_set_up_cost | `DOUBLE` | Nullable | 每小时的成本 |
| `std_machine_run_cost` | std_machine_run_cost | `DOUBLE` | Nullable | - |
| `std_machine_set_up_cost` | std_machine_set_up_cost | `DOUBLE` | Nullable | - |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：工作中心成本归集与转产决策
* **因果流向**：`ipc_work_center` 表记录了工作中心的费率主数据（人工、机器费率）。
* **财务对账编排**：当 CTP 引擎确定工单在当前工作中心加工并发生换型时，引擎计算其发生的实际换型成本 $Cost_{setup} = setup\_time \times std\_machine\_set\_up\_cost$。在多路线决策时，该费率与工艺路线的惩罚项相加，作为优先级权衡的依据，确保计划不仅在物理上可行，而且在成本上最优。