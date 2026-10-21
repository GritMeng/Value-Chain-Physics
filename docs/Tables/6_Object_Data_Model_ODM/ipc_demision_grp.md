---
table_name: "ipc_demision_grp"
alias: "demision_grp"
module: "6_Object_Data_Model_ODM"
cpp_struct: "DimensionGroupRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_demision_grp` (demision_grp)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `demision_grp` | demision_grp | `VARCHAR(10)` | Nullable | 维度组 |
| `demision` | demision | `VARCHAR(10)` | Nullable | 维度Reference Table: Demision |
| `value` | value | `INTEGER` | Nullable | Demmision具体的数值，如果选定的Demision是个范围，那么此值为起始值.  |
| `relation_ship` | relation_ship | `VARCHAR(10)` | Nullable | EQ（等于）, LT（小于）, GT（大于）, LE（小于等于）, GE（大于等于）, NE（不等于） |
| `value2` | value2 | `INTEGER` | Nullable | EQ（等于）, LT（小于）, GT（大于）, LE（小于等于）, GE（大于等于）, NE（不等于） |
| `relation_ship2` | relation_ship2 | `VARCHAR(10)` | Nullable | Demmision具体的数值，如果选定的Demision是个范围，那么此值为终值 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多维配额特征组合分级机制
* **因果流向**：`ipc_demision_grp` 将多个单独的属性维度（如芯片的速度等级、温度范围、批次）绑定为一个逻辑维度组（Dimension Group）。半导体分级和钢铁降级消纳引擎读取此表，以此执行多特征条件的综合相似度计算，寻找最合规的降级料替代。
* **物理内存结构**：
```cpp
// 对应 ipc_demision_grp 的 C++ 内存物理结构
struct DimensionGroupRecord {
    uint32_t dimension_group_id;          // 维度组 ID (对应 demision_grp)
    uint8_t attribute_count;              // 包含的物理特征维度数量
    uint16_t primary_sorting_dimension_id;// 用于主排序的维度特征 ID
};
```