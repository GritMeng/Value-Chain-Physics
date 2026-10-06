---
table_name: "ipc_commercial_bonus_plan"
alias: "bonus_plan"
module: "4_IBP_Consolidated"
cpp_struct: "CommercialBonusPlanRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_commercial_bonus_plan` (bonus_plan)

> **业务说明**: BonusSchedule表包含可以使用的不同奖金计划的字符串值。Project表和Task表都引用此表来指定与给定项目或任务相关的奖金时间表(如果有的话)。奖金时间表用于确定在指定奖金日期之前完成的任务或项目的奖金收入。也就是说，奖励可以通过项目获得，其中Project.CalcFinishDate在Project.BonusDate之前按或者Task.CalcFinishDateTask.BonusDate之前。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `bonus_plan` | bonus_plan | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：S&OP 销售提成核算与渠道激励匹配
* **因果流向**：`ipc_commercial_bonus_plan` 将前端销售代表的激励机制与后端 S&OP 共识需求计划进行对账挂钩。引擎读取此表以预测未来周期内需要计提的佣金成本，并在 IBP 财务总分类账中进行列支。
* **核算编排逻辑**：
  1. 业绩完成率计算：对比销售代表名下的实际出货量（Actual Qty）与目标配额量（Quota Target）：
     $$ Compliance\_Rate = \frac{Actual\_Qty}{Quota\_Target\_Qty} $$
  2. 佣金分摊核算：若完成率落在 $[0.0, 1.0]$，按 base 提成率计提；若完成率 $> 1.0$，超出部分按超级乘数倍率计提奖金。

###### 2. 物理内存结构设计 (C++ DOD Layout)
销售佣金策略在内存中采用面向业绩核算的紧凑结构，供 IBP 财务模块在期末对账时调用：
```cpp
// 对应 ipc_commercial_bonus_plan 的 C++ DOD 结构体
struct CommercialBonusPlanRecord {
    uint32_t plan_id;                 // 提成计划 ID (对应 commercial_bonus_plan)
    uint32_t sales_rep_id;            // 销售代表 ID
    double quota_target_qty;          // 目标配额销售量 (对应 quota_target)
    double base_commission_rate;      // 基准佣金比例
    double super_bonus_multiplier;    // 超额奖金乘数
};
```

###### 3. 边界与异常处理
* **目标配额为零防护**：若大客户经理的配额目标被误设为 0，为防分母为零异常，引擎会自动将完成率重设为 1.0，仅核算基础提成，并发出警告提示。