---
table_name: "ipc_mrp_rule"
alias: "mrp_rule"
module: "7_Control_Data_Model_CDM"
cpp_struct: "MrpRuleRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_mrp_rule` (mrp_rule)

> **业务说明**: 包含了与 DDMRP 相关的配置设置。它用于确定用于针对使用此规则的物料开启或关闭 DDMRP 计算的需求处理行为设置。此表支持 DDMRP 应用程序。为每个物料开启解耦交货期计算。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `mrp_rule` | mrp_rule | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 规则描述 |
| `operation_rule` | operation_rule | `BOOLEAN` | Nullable | Y - 考虑  N - 不考虑 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：MPS/MRP/CTP 混合启发式控制引擎
* **因果流向**：`ipc_mrp_rule` 是定义物料在计划引擎中采用何种运算策略的“宪法”。当求解器扫描到物料的 mrp_rule 属性时，通过此表决定是运行标准的低代码（Low Level Code）级联净需求倒排，还是只进行 Master Planning，亦或是激活实时 CTP 有限能力预占。
* **Heuristics 运行编排**：
  - `mrp` 标准模式：执行经典物料需求计划，将总需求转化为各阶 BOM 净需求。
  - `mps` 关键主物料：运行主生产计划。锁定计划时限（Demand Time Fence）内的计划，防范物料计划频繁修改。
  - `ctp` 模式：在销售订单录入时触发实时有限能力 CTP 递归校验，决定交期承诺。

###### 2. 物理内存结构设计 (C++ DOD Layout)
MRP 规则作为只读元数据，在内存中以紧凑的 SoA 位域对齐存储：
```cpp
// 启发式算法类型
enum class PlanningHeuristic : uint8_t {
    MRP_STANDARD = 0,
    MPS_CRITICAL = 1,
    CTP_REALTIME = 2
};

// 对应 ipc_mrp_rule 的 C++ 内存结构体
struct MrpRuleRecord {
    uint32_t mrp_rule_id;            // 规则ID哈希 (对应 mrp_rule)
    PlanningHeuristic heuristic_type;// 计划启发式类型
    bool allow_expedite;             // 是否允许加急
    int planning_horizon_days;       // 计划展期天数
    int demand_time_fence;           // 需求冻结期天数 (DTF)
    int planning_time_fence;         // 计划冻结期天数 (PTF)
};
```

###### 3. 边界与异常处理
* **时间时限冲突自纠错**：若配置人员将需求冻结期 $DTF$ 设为大于计划冻结期 $PTF$，引擎启动静态校验时会自动进行强截断纠偏：
  $$ DTF = \min(DTF, PTF) $$
  防止由于冻结区配置重叠产生无法产生工单的需求死区。