---
table_name: "ipc_project"
alias: "project"
module: "3_ETO_Project"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 3_eto_project
---

# 🏷️ `ipc_project` (project)

> **业务说明**: ETO 项目主数据定义表。支持多代基线（baseline）、奖惩计划（bonus_plan/penalty_plan）以及客户绑定的项目管理根表。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `baseline` | baseline | `VARCHAR` | Nullable | 指示此项目是否为基线。将项目设置为基线之后，这个表中的某些计算字段以及Task表总是返回它们在项目建立基线时所保存的值：
Y
N |
| `bonus_date` | bonus_date | `VARCHAR` | Nullable | 奖金可能适用于此项目的日期或之前。
奖励是根据参考值计算的
BonusSchedule和ProjectType值，并且可以应用于在此日期或之前完成的项目 |
| `bonus_plan` | bonus_plan | `VARCHAR` | Nullable | Reference:BonusPlan |
| `customer` | customer | `VARCHAR` | Nullable | Reference:Customer |
| `project` | project | `VARCHAR` | 🔑 **PK / Required** | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `finish_date` | finish_date | `VARCHAR` | Nullable | - |
| `grp` | grp | `VARCHAR` | Nullable | Reference:ProjectGroup |
| `hours_per_day` | hours_per_day | `VARCHAR` | Nullable | 每天的工作小时数 |
| `manager` | manager | `VARCHAR` | Nullable | Reference:ProjectManager |
| `penalty_date` | penalty_date | `VARCHAR` | Nullable | 处罚可能累积到该项目的日期。罚款费用是根据参考PenaltySchedule和ProjectType值计算的
，并且可以在此日期和CalcFinishDate之间应用。 |
| `penalty_plan` | penalty_plan | `VARCHAR` | Nullable | Reference:PenaltyPlan |
| `rate_adjustment` | rate_adjustment | `VARCHAR` | Nullable | 可选的百分比调整将应用于为分配给本项目任务的任何资源收取的标准和加班费。例如，分配给特定客户项目的所有资源都可以打折。如果调整表示对资源费率收取额外费用，则应指定正值。如果调整表示应用于资源价格的折扣，则应指定负值。例如，0.05表示5%的溢价，而-0.1表示打九折。 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：ETO 项目财务奖惩拉拉平
* **因果流向**：`ipc_project` 表定义了工程项目的基准（Baseline）和奖惩计划。在 ETO 排程时，系统不仅拉通物料和产能，还要优化项目的总生存周期以获得最大的净收益（奖励减去罚金）。
* **财务编排逻辑**：
  1. 计算预计完工期（CalcFinishDate）：依据下层 WBS 任务网络拓扑推演得出。
  2. 奖惩判定：若完工期早于 `bonus_date`，触发 `bonus_plan` 奖励流入；若完工期迟于 `penalty_date`，按天级累加 `penalty_plan` 中的罚金成本，计入项目财务大盘。