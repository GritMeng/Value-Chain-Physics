---
table_name: "ipc_io_ss_rule"
alias: "ss_rule"
module: "5_IO_Safety_Stock"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_ss_rule` (ss_rule)

> **业务说明**: netting的计算方式以及维护安全库存水平

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | 在何种条件下触发补货及补货的数量是多少：
always -始终保持安全库存水平。
如果需要一个订单来使库存达到那个水平，那么根据订单重新安排SR规则(SupplyOrderType.OperationRule.Rescheduleable)和订单生成规则(PartSource.Order_Rule.order_gen_Rule)在允许的情况下尽早生成计划订单或加速现有订单。(PartSource.Order_Rule.order_gen_Rule)。

Ignor - 不管其他字段的设置如何，忽略掉。

if_demand - 只有该物料有一个有效的需求（独立需求或相关需求）的时候，才能产生使库存达到安全水平的需求订单。

if_operation - 有相关运作的时候，无论是Demand还是SR.


 |
| `date_rule` | date_rule | `VARCHAR(10)` | Nullable | 指定用哪个日期作为DueDate：
first_demand - 第一个需求的DueDate
first_demand_or_lt - 有Demand取其第一个Demand的DueDate
mand取Demand Date，没有Demand取Rundate+LT.
LeadTime - Rundate+LT
first_demand_or_run_date - 有Demand取其第一个Demand的DueDate，负责取Rundate |
| `ss_qty_rule` | ss_qty_rule | `DECIMAL(18,2)` | Nullable | fixed_qty - 固定数量
perc_of_demand - 取一段时间内需求数量的百分比. part.ss_rule.percentage_time_unit指定Bucket,part.percentage_intervals指定多少Buckets之内参与运算。
frac_of_demand - 0-1
range_of_coverage
 |
| `ss_calendar` | ss_calendar | `VARCHAR(10)` | Nullable | Reference Table : Calendar |
| `percent_calendar` | percent_calendar | `VARCHAR(10)` | Nullable | 计算Demand percentage的日历。另外需要在物料上Material.PercentSatetyBucketsCount为多少buckets之内，即期间。 |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table : ControlGroup |
| `Policy` | ss_rule | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `ss_level` | ss_level | `DECIMAL(18,2)` | Nullable | average - 当库存水平低于由ss_qty_Rule字段中的设置确定的订单点阈值时触发，数量为此数量并且达到或超过安全阈值。

over - 最大值，不能超过。此值大于阈值和Average数量。

under - 最小值，不能低于。此值大于阈值和Average数量。 |
| `percen_intervals` | percen_intervals | `DECIMAL(18,2)` | Nullable | - |
| `range_of_coverage` | range_of_coverage | `VARCHAR(10)` | Nullable | referance:range_of_coverage |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：安全库存计算规则匹配与水位计算
* **因果流向**：`ipc_io_ss_rule` 定义了零部件采用何种规则计算安全水位（如固定数量、覆盖天数 DOS、或方差服务水平 $Z\sigma\sqrt{L}$）。
* **物理内存结构**：在 C++ 引擎中，该规则被编译为 `PartSiteRecord.ss_rule` 中的策略枚举，指导 MEIO 求解器调用不同的数学公式计算安全库存并更新 `Axis` 数轴。