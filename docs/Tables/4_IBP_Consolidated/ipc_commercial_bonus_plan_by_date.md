---
table_name: "ipc_commercial_bonus_plan_by_date"
alias: "bonus_plan_by_date"
module: "4_IBP_Consolidated"
cpp_struct: "CommissionWindow"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_commercial_bonus_plan_by_date` (bonus_plan_by_date)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `plan` | plan | `VARCHAR` | PK / NOT NULL | Reference:BonusPlan |
| `eff_start_date` | eff_start_date | `VARCHAR` | Nullable | - |
| `id` | id | `VARCHAR` | PK / NOT NULL | - |
| `on_time_bonus` | on_time_bonus | `DOUBLE` | Nullable | 应用于CalcFinishDate早于其CalcFinishDate的项目或任务的一次性奖金
PenaltyDate。 |
| `interval_bonus` | interval_bonus | `VARCHAR(40)` | Nullable | 经常性的奖金。应用于项目或任务的PenaltyDate和CalcFinishDate之间的每个有效日期，该日期属于与项目类型或任务类型关联的惩罚日历。例如，罚金费用可能适用于每个工作日或每个星期的间隔。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：特殊销售档期提成变动核算
* **因果流向**：`ipc_commercial_bonus_plan_by_date` 用于记录在特定促销期或新品首发活动中，佣金比例的动态变动。引擎在核算未来现金流预算时，根据发货日期落入的时段自动乘以权重。
* **计算逻辑**：
  - 时段检索：若发货日期落入指定起止区间内，自动调用变动比例。
  - 动态计提：最终计提金额计入 `ipc_financial_ledger` 的变动销售费用。

###### 2. 物理内存结构设计 (C++ DOD Layout)
时间区间变动提成在内存中以时序有序向量形式存放，支持二分法检索：
```cpp
// 单个提成变动周期
struct CommissionWindow {
    int start_day;
    int end_day;
    double special_multiplier;
};

// 对应 ipc_commercial_bonus_plan_by_date 的内存结构
struct CommercialBonusPlanByDateRecord {
    uint32_t plan_id;
    std::vector<CommissionWindow> windows; // 时间有序的佣金膨胀窗口
};
```

###### 3. 边界与异常处理
* **重叠区间覆盖逻辑**：若同一计划配置了重叠区间，引擎默认采用乘数高者，优先保证前线销售人员的激励达成。