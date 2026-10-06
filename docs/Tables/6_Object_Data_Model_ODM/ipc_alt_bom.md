---
table_name: "ipc_alt_bom"
alias: "alt_bom"
module: "6_Object_Data_Model_ODM"
cpp_struct: "AltBomRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_alt_bom` (alt_bom)

> **业务说明**: 替换的BOM值

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `alt_bom` | alt_bom | `INTEGER` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：工程变更（ECO）下的替代 BOM 结构生效判定
* **因果流向**：`ipc_alt_bom` 存储了除主 BOM 之外的备选/替代组件结构版本清单。在计划排程遇到主物料长期缺货或发生工程设计变更时，系统查询该表，在特定日期区间内切换为替代的物料结构进行需求分解。
* **生效时间轴编排**：
  - 检查工程变更日期（ECO Date）：当计划排产日期 $t$ 落入特定备选 BOM 版本的生效时间区间内，引擎自动切换 BOM 关系。
  - 清单爆破：按备选 BOM 结构爆破出子件需求，写入依赖需求账本。

###### 2. 物理内存结构设计 (C++ DOD Layout)
替代 BOM 配置在编译拓扑图时被直接整合进 CSR (Compressed Sparse Row) 结构的备选分支中，用绝对天数偏移表示生效周期：
```cpp
// 对应 ipc_alt_bom 的 C++ DOD 数据结构
struct AltBomRecord {
    uint32_t alt_bom_id;        // 替代BOM标识
    uint32_t part_id;           // 所属物料 ID
    int version_code;           // 版本序列号
    int active_start_day;       // 生效开始计划天 (对应相对天数)
    int active_end_day;         // 有效结束天
};
```

###### 3. 边界与异常处理
* **版本真空期 fallback**：若某物料在历史版本交替期间出现“时间真空期”（即该天没有任何 BOM 版本生效），引擎会自动沿用最近的历史版本作为备选，并抛出警告日志，防止物料需求漏分解导致产线停开。