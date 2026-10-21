---
table_name: "ipc_penalty_plan"
alias: "penalty_plan"
module: "6_Object_Data_Model_ODM"
cpp_struct: "PenaltyPlanRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_penalty_plan` (penalty_plan)

> **业务说明**: PenaltySchedule表包含标可使用的不同处罚计划的字符串值。项目表和任务表都引用此表来指定与给定项目或任务相关的惩罚时间表(如果有的话)
表中的每条记录还与两个表中的一组引用记录相关联
表PenaltyScheduleByDate和PenaltyScheduleByInterval。这些表定义了与给定惩罚计划相关的一次性和经常性成本。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `plan` | plan | `VARCHAR` | PK / NOT NULL | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：交期延迟（Tardiness）财务惩罚核算算法
* **因果流向**：`ipc_penalty_plan` 定义了订单延迟交货时对企业的财务损失惩罚结构。它是交付又准又快（OTIF）双层博弈算法中，求解器决策“是否值得追加高昂运费追资源以确保不延期”的底线判定依据。
* **惩罚计算编排**：
  1. 延迟天数计算：$Delay\_Days = Actual\_Delivery\_Date - Customer\_Due\_Date$。若 $Delay\_Days \le grace\_period\_days$（宽限期），惩罚为 0。
  2. 惩罚核算公式：
     $$ Penalty = Qty \times (Fixed\_Penalty\_Rate + Delay\_Days \times Daily\_Tardiness\_Rate) \times Price $$
  3. 收益对账：将惩罚损失计入该订单对应的 `ipc_financial_ledger`，减少 Consensus Revenue 估值。

###### 2. 物理内存结构设计 (C++ DOD Layout)
惩罚计划配置在交付博弈的成本评估函数中被频繁调用，存储于高速缓存对齐的紧凑结构体中：
```cpp
// 对应 ipc_penalty_plan 的内存物理结构
struct PenaltyPlanRecord {
    uint32_t penalty_plan_id;      // 惩罚计划ID (对应 penalty_plan)
    double fixed_penalty_rate;     // 固定迟交惩罚比例
    double daily_tardiness_rate;    // 天级滞纳惩罚比例
    int grace_period_days;         // 豁免宽限期天数
};
```

###### 3. 边界与异常处理
* **惩罚上限溢出截断（Revenue Clamping）**：若迟交天数过长导致累计惩罚金额超过订单本身总价的 $100\%$，惩罚计算会自动截断上限为订单总营收，防止系统出现负收入的荒谬财务状态。