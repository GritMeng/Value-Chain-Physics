---
table_name: "ipc_alt_grp_type"
alias: "alt_grp_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "AltGrpTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_alt_grp_type` (alt_grp_type)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组
Reference Table: ControlGroup |
| `alt_grp_type` | alt_grp_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `mix_rule` | mix_rule | `VARCHAR(10)` | Nullable | 控制是否对同一订单混料。
unit_parent - 同一单位的组装件不能混。
parent - 对应的单笔供应单不能混料
demand - 整笔需求不能混料
Unrestricted - 无限制
 |
| `source_rule` | source_rule | `VARCHAR(10)` | Nullable | on_going -  如果不用就离目标更的替换料为选择物料。
to_date- 在此笔需求之前谁累计的供给数量/Ratio,谁小选谁。
propotional - 采购比例.
 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | interchangeable - 任何在同一alt_grp中的物料的现有supply均可以来满足需求。产生PlannedOrder的时候只用primary part.
substitute - 替换料可以补充Primary Part.但Primary part的supply不能供给Substitue物料的需求。 计划订单均可产生。
option - 均可替换，计划订单均可产生.

 |
| `pre_rule` | pre_rule | `VARCHAR(10)` | Nullable | Z - on-hand, SR with tolerance, excess PL with tolerance, PL 
N - on-time/at least late with tolerance by priority
C - 先现存供应，然后PL





 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多层次混料控制与替代物料匹配规则
* **因果流向**：`ipc_alt_grp_type` 定义了替代组（Alternate Group）在 MRP 需求分解与 CTP 预占计算中，底层处理规则和物理限制。它控制着是否允许在同一笔订单中混料（如半导体封装中是否允许混用不同厂家的支架）、替代料的分配优先级判定、以及计划工单的生成机制。
* **规则控制编排**：
  1. 混料控制（Mix Rule）：在生成工单时，读取 `mix_rule`：
     - `demand`：整笔需求必须由单一物料满足，不可混料。若主料不足，必须 100% 切换为替代料，否则报错。
     - `parent`：同一张 Planned Order 内的组件不可混料。
     - `unit_parent`：同一个产出单位不能混料，但多笔工单间可使用不同物料。
  2. 货源分配（Source Rule）：当 `source_rule` 为 `to_date` 时，引擎根据历史累计实际比例分摊：
     $$ Selected\_Part = \min_{p \in Group} \left( \frac{Cumulative\_Qty_p}{Target\_Ratio_p} \right) $$
     当为 `on_going` 时，则分配给在本次需求后最接近目标比例的物料，实行动态偏差纠偏。

###### 2. 物理内存结构设计 (C++ DOD Layout)
替代类型规则是 BOM 分解判定函数的最底层逻辑分支。在 C++ 引擎中，该配置被编译为紧凑的位域（Bit-field）枚举，并与 `AlternativeRouting` 结构对齐存储，避免虚函数调用开销：
```cpp
// 混料控制规则枚举
enum class MixRule : uint8_t {
    UNRESTRICTED = 0,
    DEMAND_LEVEL = 1,
    PARENT_LEVEL = 2,
    UNIT_PARENT_LEVEL = 3
};

// 货源分配规则
enum class AltSourceRule : uint8_t {
    STATIC_PRIORITY = 0,
    TO_DATE_BALANCE = 1,
    ON_GOING_TARGET = 2,
    PROPORTIONAL = 3
};

// 对应 ipc_alt_grp_type 的 C++ 内存结构体
struct AltGrpTypeRecord {
    uint32_t alt_grp_type_id;   // 类型ID (对应 alt_grp_type)
    MixRule mix_rule;           // 混料规则 (对应 mix_rule)
    AltSourceRule source_rule;  // 货源分配规则 (对应 source_rule)
    uint8_t operation_rule;     // 替换运行类型 (interchangeable, substitute...)
    uint8_t pre_rule;           // 预占优先规则 (Z, N, C...)
};
```

###### 3. 边界与异常处理
* **强非混料死锁松弛**：若 `mix_rule` 设置为 `demand` 且没有任何单一替代物料的物理在库与在途（OnHand + SR）能够 100% 满足整笔需求，引擎将产生死锁。此时引擎触发松弛降级，在产生报警（Warning Log）的同时暂时将规则降级为 `parent` 级混料，确保能开出订单，避免整条产线停工。