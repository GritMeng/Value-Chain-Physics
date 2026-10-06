---
table_name: "ipc_alternate_part"
alias: "alternate_part"
module: "6_Object_Data_Model_ODM"
cpp_struct: "AlternatePartRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_alternate_part` (alternate_part)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 生效日期 |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | 失效日期 |
| `priority` | priority | `INTEGER` | Nullable | 优先级，数值越小越优先 |
| `alt_grp` | alt_grp | `VARCHAR(10)` | PK / NOT NULL | 替代组编码，相同替代组内的组件物料属于可替换物料 |
| `target` | target | `DECIMAL(18,2)` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时效性替代料优先级抉择与有效期控制
* **因果流向**：`ipc_alternate_part` 定义了组内各替代料在具体计划日期上的优先级。BOM 替代分摊算法根据该优先级从高到低依次占用各备选物料的在手库存。
* **抉择编排逻辑**：
  1. 优先级遍历：按 `priority` 数值从小到大排序检索。
  2. 时空有效性校验：校验需求日期是否落入 `eff_start_date` 与 `eff_end_date` 区间内。若不满足，则忽略该替代项。
  3. 剩余需求递归传递：若高优先级替代料扣减后仍有缺口，递归传递给下一顺位替代物料。

###### 2. 物理内存结构设计 (C++ DOD Layout)
替代物料细节在内存中以密集排序向量形式存储于 `AlternativeRouting` 结构中：
```cpp
// 对应 ipc_alternate_part 的 C++ DOD 结构
struct AlternatePartRecord {
    uint32_t parent_part_id;     // 主物料 ID (对应 part)
    uint32_t substitute_part_id; // 替代物料 ID (对应 alt_part)
    int priority;                // 替代优先级 (对应 priority，越小越优先)
    int eff_start_day;           // 有效相对开始天数
    int eff_end_day;             // 有效相对结束天数
};
```

###### 3. 边界与异常处理
* **失效日截断与订单自动重拆分**：在工单排产跨越替代料失效边界时，引擎会自动把原本合并的工单按失效日拆分为两笔：前半段使用当前替代料，后半段切换回主料或其他可用替代料，防止订单无法齐套。