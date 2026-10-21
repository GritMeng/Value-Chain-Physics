---
table_name: "ipc_material_uom_relation"
alias: "part_uom"
module: "6_Object_Data_Model_ODM"
cpp_struct: "MaterialUomRelationRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_material_uom_relation` (part_uom)

> **业务说明**: 该表中存储的记录用于在生成统计预测时计算预测项目数量。当对定义了“度量单元项路径”的数量或货币字段执行工作簿级别的转换时也会使用它们。因此，该表允许使用特定于该部件的因子来计算部件，而不是使用UnitOfMeasure。为UnitofMeasure指定的BaseConversion。

ForecastItemParametersActual
StatisticalForecast
StatisticalForecastDetail
StatisticalForecastDisaggregationRate


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `factor` | factor | `DECIMAL(18,2)` | Nullable | >=1 |
| `uom` | uom | `VARCHAR(10)` | PK / NOT NULL | Reference:UnitOfMeasure |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级计量单位（UoM）转换与批量计算
* **因果流向**：`ipc_material_uom_relation` 是保障物料多级运算精度和数据一致性的基石。采购通常以“箱”或“吨”为计量单位，而在生产中以“只”或“克”消耗。计划引擎在进行 MRP 爆破和采购拉动时，必须通过此表将所有单位折算为物料在库的基准计量单位（Base UoM）。
* **单位折算算法编排**：
  - 需求接收：接收销售订单或依赖需求数量 $Q_{tr}$，获取其交易单位 $UoM_{tr}$。
  - 倍数折算：若 $UoM_{tr}$ 与 Base UoM 不同，检索转换因子 $Conversion\_Factor$。进行乘除折算：
     $$ Q_{base} = Q_{tr} \times Conversion\_Factor $$
  - 精度舍入：根据 `ipc_part_site` 中的 `round_to_integer` 控制，对折算后的基准数量决定是否向上取整，以防止开出小数数量的生产工单。

###### 2. 物理内存结构设计 (C++ DOD Layout)
单位转换率在内存中与物料 SKU 节点记录形成一维哈希关联结构，以实现超高速的转换运算：
```cpp
// 对应 ipc_material_uom_relation 的 C++ DOD 物理数据结构
struct MaterialUomRelationRecord {
    uint32_t part_id;             // 物料 ID (对应 part_id)
    uint16_t from_uom_id;         // 原始单位逻辑编码 (对应 from_uom_id)
    uint16_t to_uom_id;           // 目标单位逻辑编码 (对应 to_uom_id)
    double conversion_factor;     // 单位折算系数 (对应 conversion_factor)
};
```

###### 3. 边界与异常处理
* **除零与负折算系数拦截**：如果录入的折算系数 $\le 0$，引擎初始化时会将其强制覆盖为 $1.0$，并在加载阶段记录高优先级错误日志，防止转换时发生除零溢出或物料需求量被乘以负数导致系统逻辑混乱。