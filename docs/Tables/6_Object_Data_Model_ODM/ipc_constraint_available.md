---
table_name: "ipc_constraint_available"
alias: "constraint_available"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ConstraintAvailableRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint_available` (constraint_available)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `constraint` | constraint | `VARCHAR(10)` | PK / NOT NULL | Reference Table: Constrain |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 生效日期 |
| `rate` | rate | `DECIMAL(18,2)` | Nullable | 在此记录的有效时间内，每个时间单位(constraint . calendar)可用的约束数量。 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `eff_end_date` | eff_end_date | `VARCHAR` | Nullable | - |
| `ot_rate` | ot_rate | `DECIMAL(18,2)` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时序可用产能额度计算与动态日历合并
* **因果流向**：`ipc_constraint_available` 记录了各个机器约束、人工工时约束在各计划天数下的绝对可用产能额度。它是排产引擎在进行有限能力拉平（Capacity Leveling）时计算天级剩余负荷的基准线。
* **可用产能计算编排**：
  1. 日历班次折算：根据该约束关联的工作中心日历，获取额定开班时长。
  2. 效率与负荷折减：
     $$ Capacity_{available}(c, t) = Work\_Hours(c, t) \times Efficiency\_Rate_c \times Count\_of\_Machines_c $$
  3. 叠加维护停机计划（Downtime Override）：如果有临时大修，直接扣减对应的额度，并在此表记录。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 内存中，天级可用产能以时序密集双精度浮点数向量存放，以支持高频的产能冲减判定：
```cpp
// 对应 ipc_constraint_available 的 C++ DOD 结构体
struct ConstraintAvailableRecord {
    uint32_t constraint_id;      // 约束 ID (对应 constraint)
    int day_bucket;              // 计划相对天数 (对应 date)
    double available_hours;      // 可用工时数量 (对应 available)
    double cost_per_hour;        // 产能使用的单位小时成本 (对应 cost)
};
```

###### 3. 边界与异常处理
* **维护重叠冲突**：若同一日期录入了多个重叠的停机计划，引擎自动执行“并集”扣减，最大程度保护可用产能数据，防止虚高产能导致工单排产后车间超载。