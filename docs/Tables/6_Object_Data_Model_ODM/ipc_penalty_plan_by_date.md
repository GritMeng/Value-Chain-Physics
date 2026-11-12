---
table_name: "ipc_penalty_plan_by_date"
alias: "penalty_plan_by_date"
module: "6_Object_Data_Model_ODM"
cpp_struct: "PenaltyWindow"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_penalty_plan_by_date` (penalty_plan_by_date)

> **业务说明**: 保存有效的日期记录，用于定义指定惩罚计划和使用该惩罚计划的项目和/或任务的一次性和经常性惩罚成本。指定的惩罚成本可以同时应用于两个项目ProjectType.“Date”的PenaltyRule值以及具有TaskType的任务的PenaltyRule值
“日期”。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `plan` | plan | `VARCHAR` | PK / NOT NULL | Reference:PenaltyPlan |
| `eff_start_date` | eff_start_date | `VARCHAR` | Nullable | - |
| `id` | id | `VARCHAR` | PK / NOT NULL | - |
| `on_time_cost` | on_time_cost | `DOUBLE` | Nullable | 应用于CalcFinishDate晚于其CalcFinishDate的项目或任务的一次性惩罚成本
PenaltyDate。 |
| `interval_cost` | interval_cost | `VARCHAR(40)` | Nullable | 经常性的罚款成本。应用于项目或任务的PenaltyDate和CalcFinishDate之间的每个有效日期，该日期属于与项目类型或任务类型关联的惩罚日历。例如，罚金费用可能适用于每个工作日或每个星期的间隔。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：促销旺季与特殊协议动态延迟惩罚率核算
* **因果流向**：`ipc_penalty_plan_by_date` 记录了特殊促销档期（如 618, 双 11）或大客户合同保障期内，临时性膨胀的迟交惩罚率。当排产器在此时间范围内调整工单交期时，引擎会以该表中的动态惩罚因子替代常规迟交费用。
* **调整编排逻辑**：
  - 时段检索：当发生延迟交货的日期落入本表的指定日期区间内，调取相应的惩罚系数乘数 $Multiplier$。
  - 膨胀核算：最终惩罚以基准惩罚乘以上述乘数，以倒逼引擎将宝贵的稀缺产能优先向该促销档期的订单倾斜。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 内存中，该日期阶段配置被编译为时间段检索节点，存储于按时间升序排列的连续向量中：
```cpp
// 对应单个惩罚波动时间窗口
struct PenaltyWindow {
    int start_day;
    int end_day;
    double rate_multiplier;
};

// 对应 ipc_penalty_plan_by_date 的内存物理结构
struct PenaltyPlanByDateRecord {
    uint32_t penalty_plan_id;
    std::vector<PenaltyWindow> windows; // 时间有序的膨胀窗口向量
};
```

###### 3. 边界与异常处理
* **重叠区间覆盖逻辑**：若对同一惩罚计划在相同日期内配置了多个重叠的膨胀窗口，引擎默认选取 $Multiplier$ 最大的一条，以最严苛的财务惩罚来强制保证大促期间的交付率。