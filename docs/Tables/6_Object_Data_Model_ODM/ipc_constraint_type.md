---
table_name: "ipc_constraint_type"
alias: "constraint_type"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ConstraintTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint_type` (constraint_type)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `constraint_type` | constraint_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组
Reference Table: ControlGroup |
| `eff_rule` | eff_rule | `VARCHAR(10)` | Nullable | 生失效Date的规则，用来自动转换跟企业应用之间的数据集成。例如，企业数据库中Start 2020.7.1意味着7.1之后生效，我们应该自动转化2020.7.1是否在生效期内。
inclusive_exclusive - 开始日期在生效期间，失效日期不在。
always - 一直生效 
never - 不生效
exlusive - 开始日期和结束日期都不在。
inlusive - 都在 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | 指定Constrain如何应用。 
constrainted - 考虑约束条件并且考虑最大限制。 
load_only - 只体现报表，不做可用性检查。
planning_only - Planning应用考虑即Netting产生计划订单，CTP不考虑。
ctp_only - Netting时不考虑，CTP考虑. |
| `fill_sche` | fill_sche | `BOOLEAN` | Nullable | 是否尽早占用 Y - 今早占用  N - 适时 |
| `ScheduleRule` | sche_rule | `VARCHAR(10)` | Nullable | BackwardOnly, Normal |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：约束运行属性判定与超载熔断机制
* **因果流向**：`ipc_constraint_type` 规定了约束是属于“硬约束（Constrained，必须刚性拦截）”、“软约束（LoadOnly，只记录负荷不拦截排产）”还是“无约束（Unconstrained，无限能力）”。这直接影响了 CTP 引擎在遇到超载时是报错重排还是照常通过。
* **决策算法编排**：
  - 读取约束的类别标志 `capacity_type`：
    - `HARD_CONSTRAINED`：启动 CTP 有限产能平拉。若某天剩余负荷不足，强制将工单向前或向后平移。
    - `SOFT_LOAD_ONLY`：直接将负荷计入 `ipc_constraint_load`，但工单照常开出，并在前台抛出超载百分比红色预警。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 约束容量控制模式
enum class CapacityType : uint8_t {
    HARD_CONSTRAINED = 0,
    SOFT_LOAD_ONLY = 1,
    UNCONSTRAINED = 2
};

// 对应 ipc_constraint_type 的 C++ 内存物理对齐结构体
struct ConstraintTypeRecord {
    uint32_t constraint_type_id;         // 约束类型 ID (对应 constraint_type)
    CapacityType capacity_type;          // 容量控制模式
    double utilization_warning_threshold;// 超载预警触发水位 (如 0.85)
};
```