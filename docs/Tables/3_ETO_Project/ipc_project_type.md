---
table_name: "ipc_project_type"
alias: "project_type"
module: "3_ETO_Project"
cpp_struct: "ProjectTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 3_eto_project
---

# 🏷️ `ipc_project_type` (project_type)

> **业务说明**: ProjectType表包含在项目级别定义处理和计算的可配置设置。例如，此表设置项目的工作日历，并指定项目日期是从给定的开始日期还是完成日期计算的。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `add_duration` | add_duration | `VARCHAR` | Nullable | 当从完成开始调度项目时，此设置指示正在进行的任务是StartDate还是StartDate +
ActualDuration，在约束具有“StartToStart”或“FinishToStart”关系的前身时使用.
Y-
N |
| `allow_non_working_days` | allow_non_working_days | `VARCHAR` | Nullable | 非工作日是否考虑安排工作 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：ETO 项目类别模板与关键工序提前期拉伸
* **因果流向**：`ipc_project_type` 定义了 ETO 项目的分类策略（如新研发定制类、标准改型类、常规定制类）。不同类别的项目具有截然不同的技术调试提前期和缓冲天数。
* **计划平移算法**：
  1. 模版应用：根据项目类型自动套用 WBS 任务网络模版。
  2. 提前期拉伸：针对新研发定制类（High-Risk Customization），自动应用 `default_lead_time_buffer_days` 对总工期向右延展平移，预留研发变更容错空间：
     $$ Task\_EF = Task\_ES + Duration + Buffer_{type} $$

###### 2. 物理内存结构设计 (C++ DOD Layout)
项目类型配置在内存中作为静态常数存储：
```cpp
// 对应 ipc_project_type 的 C++ 内存物理对齐结构体
struct ProjectTypeRecord {
    uint32_t project_type_id;            // 项目类型 ID (对应 project_type)
    double default_lead_time_buffer_days;// 默认提前期安全缓冲天数
    double resource_priority_weight;     // 项目群内资源抢占权重
};
```

###### 3. 边界与异常处理
* **未知类型兜底模板**：若新建项目录入了未定义的类型，引擎会自动降级应用“标准改型类”模板进行 WBS 爆破，以最小提前期余量进行倒排计算，并发出通知。