---
table_name: "ipc_commercial_bonus_plan_by_interval"
alias: "bonus_plan_by_interval"
module: "4_IBP_Consolidated"
cpp_struct: "BonusInterval"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_commercial_bonus_plan_by_interval` (bonus_plan_by_interval)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `plan` | plan | `VARCHAR` | PK / NOT NULL | Reference:BonusPlan |
| `interval` | interval | `VARCHAR` | Nullable | 要从项目或任务的BonusDate中减去的奖金日历间隔数，以确定在此记录上定义的奖金值的最后生效日期。奖金在该日期和前一记录的最后生效日期之间有效(按间隔)。
如果时间间隔在项目或任务的完成日期和奖励日期之间定义了多个有效记录，那么它们的有效奖励值将按照ProjectType或TaskType表。 |
| `id` | id | `VARCHAR` | PK / NOT NULL | - |
| `on_time_bonus` | on_time_bonus | `DOUBLE` | Nullable | 应用于CalcFinishDate早于其CalcFinishDate的项目或任务的一次性奖金
PenaltyDate。 |
| `interval_bonus` | interval_bonus | `VARCHAR(40)` | Nullable | 经常性的奖金。应用于项目或任务的PenaltyDate和CalcFinishDate之间的每个有效日期，该日期属于与项目类型或任务类型关联的惩罚日历。例如，罚金费用可能适用于每个工作日或每个星期的间隔。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：阶梯区间提成核算与渠道达标率分析
* **因果流向**：`ipc_commercial_bonus_plan_by_interval` 定义了按业绩区间波动的提成结算规则。当销售代表大促实际完成率（Compliance Rate）落入不同区间（如 80%-100% 或 100%-120%）时，引擎读取此表以匹配相应的阶梯奖金系数，更新财务损益账目。
* **分摊算法编排**：
  - 检索变动区间列表。对完成率 $CR$ 匹配满足条件的区间段：
     $$ CR \in [min\_achievement, max\_achievement) $$
  - 取出对应的 `payout_multiplier` 乘数，折算最终可支配销售费用。

###### 2. 物理内存结构设计 (C++ DOD Layout)
阶梯区间在内存中以密集数组形式存放，作为 plan 的下属明细：
```cpp
// 单个阶梯区间
struct BonusInterval {
    double min_achievement_ratio;     // 业绩完成率下限
    double max_achievement_ratio;     // 业绩完成率上限
    double payout_multiplier;         // 提成乘数比例
};

// 对应 ipc_commercial_bonus_plan_by_interval 的内存结构
struct CommercialBonusPlanByIntervalRecord {
    uint32_t plan_id;
    std::vector<BonusInterval> intervals; // 阶梯变动区间向量
};
```

###### 3. 边界与异常处理
* **区间重叠与真空自动插值**：若配置人员漏配了部分区间（如 90%~95% 缺失），引擎会自动以最近的低级区间进行插值平填，防止达标率落入真空期时佣金核算为零。