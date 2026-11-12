---
table_name: "ipc_penalty_plan_by_interval"
alias: "penalty_plan_by_interval"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcPenaltyPlanByIntervalRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_penalty_plan_by_interval` (penalty_plan_by_interval)

> **业务说明**: 保存基于间隔的记录，以惩罚日历间隔表示，用于定义指定惩罚计划和使用该惩罚计划的项目和/或任务的一次性和经常性惩罚成本。指定的惩罚成本可以同时应用于具有ProjectType的项目。“Interval”的PenaltyRule值，以及具有TaskType的任务。PenaltyRule“Interval”的值。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `plan` | plan | `VARCHAR` | PK / NOT NULL | Reference:PenaltyPlan |
| `interval` | interval | `VARCHAR` | Nullable | 在项目或任务的“惩罚日期”之后，此记录中的成本生效的惩罚日历期间的数目。例如，如果使用每周罚款日历，此字段中的值2表示此记录中定义的成本从罚款日期后的第二周开始生效，并一直适用到下一个有效记录。如果在项目或任务的惩罚日期和完成日期之间存在多个有效间隔值，则使用有效惩罚成本的AccumulationRule字段 |
| `id` | id | `VARCHAR` | PK / NOT NULL | - |
| `on_time_cost` | on_time_cost | `DOUBLE` | Nullable | 应用于CalcFinishDate晚于其CalcFinishDate的项目或任务的一次性惩罚成本
PenaltyDate。 |
| `interval_cost` | interval_cost | `VARCHAR(40)` | Nullable | 经常性的罚款成本。应用于项目或任务的PenaltyDate和CalcFinishDate之间的每个有效日期，该日期属于与项目类型或任务类型关联的惩罚日历。例如，罚金费用可能适用于每个工作日或每个星期的间隔。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：主数据层级拓扑映射与沙盘数据逻辑隔离算法
* **因果流向**：系统主数据及控制配置表。
* **计算逻辑编排**：
  1. 数据加载与主数据校验：引擎启动时从物理数据库中读取该表记录，通过 Hash 映射机制将字符串主键转译为 $O(1)$ 的内存索引 ID，建立缓存友好的 SoA 内存块；
2. 时序对齐与时空平移：结合计划日历（CalendarRecord），对记录中的日期字段进行天级偏移（Day Offset）计算，在时间数轴上与独立需求或在手水位对齐；
3. 供需消纳与抵扣：依据 Composite Priority 优先级位权或 FIFO 滑动窗口，对数量字段进行原子扣减或比例分摊计算：\n      $$ Qty_{{effective}}(t) = \\max\\left(0, Qty_{{request}}(t) - Qty_{{allocated}}(t)\\right) $$\n
4. 指标同步与回写：计算结果暂存在线程局部的事务栈中，确认齐套后批量落库，并级联更新上层财务账本或控制塔指标看板。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_penalty_plan_by_interval 的 C++ DOD 物理对齐结构体
struct IpcPenaltyPlanByIntervalRecord {
    std::string plan; // plan 字符串 (Reference:PenaltyPlan)
    std::string interval; // interval 字符串 (在项目或任务的“惩罚日期”之后，此记录中的成本生效的惩罚日历期间的数目。例如，如果使用每周罚款日历，此字段中的值2表示此记录中定义的成本从罚款日期后的第二周开始生效，并一直适用到下一个有效记录。如果在项目或任务的惩罚日期和完成日期之间存在多个有效间隔值，则使用有效惩罚成本的AccumulationRule字段)
    uint32_t id; // id 逻辑ID/映射 (-)
    std::string interval_cost; // interval_cost 字符串 (经常性的罚款成本。应用于项目或任务的PenaltyDate和CalcFinishDate之间的每个有效日期，该日期属于与项目类型或任务类型关联的惩罚日历。例如，罚金费用可能适用于每个工作日或每个星期的间隔。)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。