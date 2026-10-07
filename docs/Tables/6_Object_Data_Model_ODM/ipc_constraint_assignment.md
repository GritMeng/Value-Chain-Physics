---
table_name: "ipc_constraint_assignment"
alias: "constraint_assignment"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ConstraintAssignmentRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint_assignment` (constraint_assignment)

> **业务说明**: 约束匹配

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `constraint` | constraint | `VARCHAR(10)` | PK / NOT NULL | 约束 |
| `supply_order` | supply_order | `VARCHAR(18)` | PK / NOT NULL | PlannedOrder，采购订单，生产订单。 |
| `souce` | souce | `VARCHAR(10)` | Nullable | PlannedOrder,ScheduleReceipt |
| `request_date` | request_date | `DATE` | Nullable | 需求日期 |
| `material` | material | `VARCHAR(40)` | Nullable | - |
| `assigned_load` | assigned_load | `DECIMAL(18,2)` | Nullable | 分配的负载 |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 供给类型：MAKE, BUY,TRANSFER |
| `auto_or_manual` | auto_or_manual | `VARCHAR(10)` | Nullable | 当是PlannedOrder的时候看是手工输入还是系统自动生成 |
| `assigned_date` | assigned_date | `DATE` | Nullable | assigned constrain available的date |
| `overload` | overload | `BOOLEAN` | Nullable | 是否超负荷，即没有constrain可用 |
| `type` | type | `VARCHAR(10)` | Nullable | Fixed,Variable,MajorSetup,MajorCleanup,MinorChangeOver,BatchFixed |
| `production_wheel` | production_wheel | `VARCHAR(10)` | Nullable | 生产轮 |
| `cycle_number` | cycle_number | `DECIMAL(18,2)` | Nullable | Cycle号 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：工序约束能力绑定与负荷因子折算
* **因果流向**：`ipc_constraint_assignment` 建立了物料、工序与全局瓶颈设备约束之间的桥梁。当工单发生制造行为时，引擎根据此表决定该工单消耗哪些设备/人工约束的时数，是 CTP 有限能力排程中最底层的负荷折算基础。
* **负荷折算编排**：
  1. 获取工单计划数量 $Qty$。
  2. 检索当前工艺关联的约束 ID 和负荷消耗因子 `capacity_consumption_factor` (单位工时)。
  3. 折算该约束的时序负荷：
     $$ Load_{delta}(t) = Qty \times capacity\_consumption\_factor + setup\_hours $$
  4. 累加至全局约束负载表 `ipc_constraint_load`，与可用天级能力进行可用性比对（CTP 判定）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，约束消耗记录作为 `AlternativeRouting` 的子项，采用 Cache 优化的扁平结构存储：
```cpp
// 对应 ipc_constraint_assignment 的 C++ DOD 物理数据结构
struct ConstraintAssignmentRecord {
    uint32_t part_id;                    // 物料 ID
    uint32_t constraint_id;              // 关联的物理设备约束 ID
    double capacity_consumption_factor;  // 单位数量消耗约束的工时比例 (对应 factor)
    double setup_hours;                  // 换型所需的固定洗枪工时
};
```