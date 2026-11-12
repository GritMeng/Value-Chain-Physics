---
table_name: "ipc_assignment_policy"
alias: "assignment_policy"
module: "7_Control_Data_Model_CDM"
cpp_struct: "AssignmentPolicyRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_assignment_policy` (assignment_policy)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `assignment_policy_num` | assignment_policy_num | `VARCHAR(10)` | Nullable | Policy唯一标识 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `sub_assemble_alternate` | sub_assemble_alternate | `VARCHAR(10)` | Nullable | Y- 组件的工单可以用替换料
N- 组件的工单不可以用替换料
默认值：Y |
| `final_assemble_alternate` | final_assemble_alternate | `VARCHAR(10)` | Nullable | Y- 最终产品的工单可以用替换料
N- 最终产品的工单不可以用替换料
默认值：Y |
| `components` | components | `VARCHAR(40)` | Nullable | 选中的组件不可以混料
Reference Table: [[ipc_bom_route|BOM]] |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table: ControlGroup |
| `soft_assignment_rule` | soft_assignment_rule | `VARCHAR(10)` | Nullable | HardReservation - 尽管OnHand和SR都不能满足其需求也要把资源占住。
PartialSquareReservation - OnHand和SR齐套部分占料
SquareReservation - 完全齐套占料
 |
| `demand_type` | demand_type | `VARCHAR(10)` | Nullable | 被DemandType多条引用 |
| `demand_line_item` | demand_line_item | `VARCHAR(10)` | Nullable | 被DemandLineItem多条引用 |
| `re_assignment_rule` | re_assignment_rule | `VARCHAR(10)` | Nullable | Unrelease - 不能释放Supply给其他订单.
IfNoLater - 只要影响CTP交期就释放Supply给其他订单

 |
| `splitting_rule` | splitting_rule | `VARCHAR(10)` | Nullable | ByAlternate - 替换料分开
ByDate - 不同的齐套日期分开
ByAlternateDate - 按照替换料及齐套日期分开 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多渠道稀缺资源比例与均分分配（Fair-Share）算法
* **因果流向**：在 IOP 交付优化中，当发生严重产能或原料短缺、且多个需求订单处于同一优先级时，引擎读取 `ipc_assignment_policy` 来执行分配决策。
* **分配算法编排**：
  1. 按比例分摊（Proportional Allocation）：
     $$ Allocated\_Qty_i = \min\left( Demand_i, Total\_Supply \times \frac{Demand_i}{\sum Demand} \right) $$
  2. 均等分摊（Equal Share）：每个需求均分供给量直至达到其本身需求上限。
  3. 分摊尾数取整：因为分配可能产生小数，而物料可能只支持整件交付，分配器自动依据物料 lot_size 向下取整，将溢出的残余数量（Residual Qty）按优先级高低进行二次尾数补偿。

###### 2. 物理内存结构设计 (C++ DOD Layout)
分配规则被编译为底层求解器的分支指令参数，直接在多对多 Pegging 图上运行：
```cpp
// 分配策略类型
enum class AllocationPolicyType : uint8_t {
    STRICT_PRIORITY = 0,
    FAIR_SHARE_PROPORTIONAL = 1,
    EQUAL_SHARE = 2
};

// 对应 ipc_assignment_policy 的 C++ DOD 结构体
struct AssignmentPolicyRecord {
    uint32_t policy_id;                 // 策略 ID (对应 assignment_policy)
    AllocationPolicyType policy_type;   // 分配策略枚举
    double min_allocation_threshold;    // 起分阈值，低于该比例不予发货
};
```

###### 3. 边界与异常处理
* **除零异常防护**：当所有竞争需求的 $Demand_i$ 之和为 0 时，比例分摊公式的分母为零。引擎检测到该状态后，会自动跳过比例分配循环，避免 CPU 硬件除零异常导致进程崩溃。