---
table_name: "ipc_io_dos_policy"
alias: "dos_policy"
module: "5_IO_Safety_Stock"
cpp_struct: "DosPolicyRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_dos_policy` (dos_policy)

> **业务说明**: “供应天数策略表（dos_policyTable）” 用于定义如何为启用供应天数逻辑的物料累积需求并生成计划订单。该逻辑旨在通过合并指定天数或时间段内的需求，减少计划订单总数。例如：

将两周内的所有需求合并为一个计划订单。
核心功能：
分阶段设置：

短期策略：适用于计划周期内的近期时段（如前2个月）。
长期策略：适用于计划周期的远期时段（如2个月后）。
分界点：通过 Fence 和 FenceCalendar 字段定义短期与长期的分割点。
关联方式：

部件通过 Part.dos_policy字段引用此表的策略。
简化配置场景：
单一策略需求：
若部件仅需一种供应天数设置，可使用旧有字段（Part、PartType、PlanningCalendars 表中的字段）。
或 仍使用此表，但需满足以下任一条件：
将短期范围设为覆盖整个计划周期。
将短期范围设为“0”，仅用长期范围覆盖整个周期。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table: control_class |
| `dos_policy` | dos_policy | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符 |
| `description` | description | `VARCHAR` | Nullable | DOS策略描述 |
| `short_term_dos_rule` | short_term_dos_rule | `VARCHAR(10)` | Nullable | by_period - 从period_time_unit和period_intervals取时间单位和单位数，从一个为满足的需求开始，落在PeriodCanlendar期间的需求。DueDate在这个期间的开始。
from_demand - 从period_time_unit和period_intervals取时间单位和单位数，从一个为满足的需求开始，落在PeriodCanlendar期间的需求。DueDate在第一未被满足的需求DueDate.
from_supply - 从period_time_unit和period_intervals取时间单位和单位数，从一个为满足的需求开始，落在PeriodCanlendar期间的需求。DueDate在第一个可被计划的Supply DueDate.即当Backward Request DueDate不能计划时，用StandardDueDate.
by_period_end - 从period_time_unit和period_intervals取时间单位和单位数，从一个为满足的需求开始，落在PeriodCanlendar期间的需求。DueDate在这个期间的结束。 |
| `long_term_dos_rule` | long_term_dos_rule | `VARCHAR(10)` | Nullable | 解释同ShortTerm一致。 |
| `st_intervals` | st_intervals | `DECIMAL(18,2)` | Nullable | Buckets数量, 多少个Bucket的计划订单合并。 |
| `lt_intervals` | lt_intervals | `DECIMAL(18,2)` | Nullable | Buckets数量，多少个Bucket的计划订单合并。 |
| `fence` | fence | `DECIMAL(18,2)` | Nullable | RunDate之后，多少个Bucket为Short和Long的分界线. |
| `fengce_time_unit` | fengce_time_unit | `VARCHAR(10)` | Nullable | Reference Table: Calendar |
| `st_time_unit` | st_time_unit | `DATE` | Nullable | - |
| `lt_time_unit` | lt_time_unit | `DATE` | Nullable | - |
| `lt_date_rule` | lt_date_rule | `VARCHAR(10)` | Nullable | dock_date
due_date |
| `st_date_rule` | st_date_rule | `VARCHAR(10)` | Nullable | dock_date
due_date |
| `st_dos` | st_dos | `DECIMAL(18,2)` | Nullable | time_unit = week, intervals = 2, st_dos = 4 意味着8 week的demand/supply需要合并 |
| `lt_dos` | lt_dos | `DECIMAL(18,2)` | Nullable | time_unit = week, intervals = 2, lt_dos = 4 意味着8 week的demand/supply需要合并 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时序双分界动态供应天数（DOS）合并算法
* **因果流向**：`ipc_io_dos_policy` 用于定义物料的“供应天数（Days of Supply）”计划生成规则。为了避免短期计划频繁震荡（Nervousness）并合并大批量生产的经济批量，引擎读取该表，将一段时间内的独立和依赖需求汇总，生成单个合并的计划订单（Planned Order）。
* **合并算法编排**：
  1. 时段分界划分：计算分界日期（Fence Date）：
     $$ Date_{fence} = RunDate + fence \times Unit_{fence} $$
     若排产需求时间 $t \le Date_{fence}$，启用短期 DOS 规则 `short_term_dos_rule` 及合并周期 $ST\_Dos = st\_intervals \times Unit_{st}$；若 $t > Date_{fence}$，则启用长期 DOS 规则。
  2. 需求归并消纳：从第一个未满足的需求日期 $T_{demand}$ 开始，引擎在数轴上向后滑动 $ST\_Dos$ 长度的窗口，将该窗口内所有缺口数量累加为单笔合并需求：
     $$ Q_{merged} = \sum_{\tau = T_{demand}}^{T_{demand} + ST\_Dos} Net\_Requirement(\tau) $$
  3. 交期决策（Due Date Rule）：依据 `st_date_rule`，工单交期可落于周期起点（`period_start`）、终点（`period_end`）或首笔需求发生日（`from_demand`）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，DOS 策略与物料记录在物理内存中紧密对齐，通过轻量级结构体直接嵌入或指针跳转，在 LBL-MRP 订单生成期间快速取值：
```cpp
// 对应 ipc_io_dos_policy 表的 C++ DOD 物理数据结构
struct DosPolicyRecord {
    uint32_t policy_id;            // 策略ID (对应 dos_policy 字段)
    uint8_t short_term_rule;       // 短期合并规则枚举 (0=by_period, 1=from_demand...)
    uint8_t long_term_rule;        // 长期合并规则枚举
    double short_term_days;        // 短期合并天数 (对应 st_dos 换算值)
    double long_term_days;         // 长期合并天数 (对应 lt_dos 换算值)
    int fence_offset_days;         // 分界相对天数 (对应 fence 换算值)
    uint32_t planning_calendar_id; // 关联的计划日历 ID
};
```

###### 3. 边界与异常处理
* **跨期订单交期冲突**：如果在短期向长期过渡的边界（Fence）处，合并窗口跨越了 `Date_{fence}`，引擎自动截断该窗口，在 Fence 处强制拆分订单，防止远期未明确的需求反向污染短期高精度的排产窗口。
* **合并量超出最大起订量**：若合并后的 $Q_{merged}$ 超出物料在 `ipc_part_site` 中定义的 `max_order_qty`，引擎将自动按最大值拆分成多笔订单，并为每笔拆分工单独立计算提前期置换。