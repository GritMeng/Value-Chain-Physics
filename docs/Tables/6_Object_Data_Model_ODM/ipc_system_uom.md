---
table_name: "ipc_system_uom"
alias: "uom"
module: "6_Object_Data_Model_ODM"
cpp_struct: "SystemUomRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_system_uom` (uom)

> **业务说明**: uom表确定所有有效的度量单位代码。通过在该表中输入记录，您可以定义两个单位之间的相对比率。
在计算统计预测时，或者在执行工作簿级别的度量单位转换时，使用uom, UnitOfMeasure计算数量.
但是，部件可以具有定义的特定度量单位转换速率
Part_UOM表.如果部件在PartUOMConversion表中有度量单位转换记录，则忽略base_conversion。

uom:
Ton
LB
EA


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `base_conversion` | base_conversion | `DECIMAL(18,2)` | Nullable | 因子将此单位中的数量转换为基本单位。
如果BaseConversion小于或等于零(<= 0)
在转换数量时使用UnitOfMeasure，不执行转换。这避免了除以零的问题。

 |
| `uom_grp` | uom_grp | `VARCHAR(10)` | Nullable | Reference:uom_grp |
| `description` | description | `VARCHAR` | Nullable | - |
| `rouding_down` | rouding_down | `INTEGER` | Nullable | 指示SR和计划订单上的有效数量四舍五入的程度(此字段仅适用于
OrderPolicy.YieldUsage设置为“忽略”和之外的选项
OrderPolicy.YieldRoundingUsage设置为“Use”)。
在应用良率因子之后，针对给定零件的计划收货和计划订单计算的EffQuantity字段将舍入到该字段中指定的小数点后位数。如果只需要整个订单数量，则该值应设置为0。
任何小于0的值都被解释为0，任何大于9的都认为是9。 |
| `uom` | uom | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **多计量单位折算基础**：`ipc_system_uom` 是系统计量单位（EA、KG、Ton、Pallet）的主维表。在进行全球物料平衡和物流配载报表计算时，引擎读取该表关联折算系数，将不同交易单位折算为统一的重量/体积指标，防止发货量超出承运商吨位上限。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应系统单位
struct SystemUomRecord {
    uint16_t uom_id;             // 单位ID逻辑编码 (对应 uom)
    uint8_t uom_group_id;        // 关联的单位组 ID
    char uom_code[8];            // 外部单位字符串 (如 "KG", "EA")
};
```