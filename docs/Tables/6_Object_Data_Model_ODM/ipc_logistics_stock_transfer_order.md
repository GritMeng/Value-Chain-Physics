---
table_name: "ipc_logistics_stock_transfer_order"
alias: "stock_transfer_order"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_stock_transfer_order` (stock_transfer_order)

> **业务说明**: 订单行项目

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `id` | id | `VARCHAR(10)` | PK / NOT NULL | 需求编号，唯一标识
Reference Table: DemandHeader |
| `item` | item | `DOUBLE` | PK / NOT NULL | 与具有相同订单id的每个独立需求记录相关联的唯一标识符 |
| `material` | material | `VARCHAR(40)` | PK / NOT NULL | 物料编号
Reference Table:Material |
| `customer` | customer | `VARCHAR(10)` | Nullable | 客户编号
Reference Table:Customer |
| `request_delivery_date` | request_delivery_date | `DATE` | Nullable | 客户要求交付到客户处的日期。
可为空，报表中会提示有多少需求不能获得客户的需求日期

 |
| `request_due_date` | request_due_date | `DATE` | Nullable | 根据客户需求，产品需要在仓库准备好的日期。可以输入，如果没有输入可以由RequestDeliveryDate-DeliveryRoute.TransitLT-DockToStockLT-PickPackLT-PreShipLT-ShipCalendar-PickPackCalendar-TransitCalendar得出.
 |
| `standard_due_date` | standard_due_date | `DATE` | Nullable | OrderReleaseDate+PreBuildLT+BuidLT |
| `standard_ship_date` | standard_ship_date | `DATE` | Nullable | OrderReleaseDate+PreBuildLT+BuidLT+PreShipLT |
| `due_date` | due_date | `DATE` | Nullable | 期望交付或就绪日期 |
| `promised_delivery_date` | promised_delivery_date | `DATE` | Nullable | 由CTP计算得出可交付到客户处的日期，用以衡量交付的可靠性,ControlTower会引用。 |
| `available_date` | available_date | `DATE` | Nullable | 实际供应可用或可承诺交付日期 (ATP Date) |
| `promised_ship_date` | promised_ship_date | `DATE` | Nullable | 由AvailableDate（历史的）+PickPackLT+TransitLT+PickPackCalendar+TransitCalendar决定，也可输入。 |
| `promised_due_date` | promised_due_date | `DATE` | Nullable | 由AvailableDate（历史的）决定 |
| `available_delivery_date` | available_delivery_date | `DATE` | Nullable | 由AvailableDate+PickPackLT+TransitLT+PickPackCalendar+TransitCalendar决定+DockToStockLT+ReceptCalendar决定,实时更新 |
| `available_ship_date` | available_ship_date | `DATE` | Nullable | 由AvailableDate+PickPackLT+PickPackCalendar决定,实时更新 |
| `delivery_route` | delivery_route | `VARCHAR(10)` | Nullable | 用于将此订单运输到客户的交货路线的引用。相关细节，如运输时间和承运人负责交付可从此参考。
Reference Table:DeliveryRoute |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `shipped_qty` | shipped_qty | `DECIMAL(18,2)` | Nullable | 已发货数量 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `priority` | priority | `INTEGER` | Nullable | 优先级，数值越小越优先 |
| `status` | status | `VARCHAR(10)` | Nullable | 用来确定参与MRP，是否参与Forecast consumption，用什么日期来consumption。
Reference Table: DemandStatus |
| `shipment_rule` | shipment_rule | `VARCHAR(10)` | Nullable | 用来确定是否可以分开发货。
Reference Table:ShipmentRule |
| `ship_group` | ship_group | `VARCHAR(10)` | Nullable | 引用发运组，同一发运组的需求应该统一发货。
Reference Table:ShipGroup |
| `configuration` | configuration | `DOUBLE` | Nullable | 需求行项目引用的Configuration, 用来指定特殊产品结构需求.
OrderBOM, CTO BOM，以及Offspec等场景应用。
Reference Table: SpecificRequirment
 |
| `pool` | pool | `VARCHAR(10)` | Nullable | 需求行项目引用的Configuration, 用来指定特殊的Supply需求。相当于头等舱。
Reference Table: SpecificRequirment |
| `confirmed_date` | confirmed_date | `DATE` | Nullable | 提供PromisedDate的日期 |
| `actual_ship_date` | actual_ship_date | `DATE` | Nullable | 实际发货日期
Reference Table:Shipment |
| `actual_delivery_date` | actual_delivery_date | `DATE` | Nullable | 客户实际收货日期, 手工受输入货系统集成。 |
| `block_code` | block_code | `VARCHAR(10)` | Nullable | BlockCode表包含分配给客户和/或单个行项目的持有代码，目的是防止订单超出订单履行过程中的特定阶段。例如，可以为客户的信用问题创建hold代码，在问题解决之前不允许发送他们的订单.
Reference Table: BlockCode |
| `order_create_date` | order_create_date | `DATE` | Nullable | 订单的创建日期 |
| `order_release_date` | order_release_date | `DATE` | Nullable | 订单可以开始执行的日期 |
| `standard_release_date` | standard_release_date | `DATE` | Nullable | 根据创建日期+Calendar+OrderPreExecuteLT得到 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `demand_source_type` | demand_source_type | `VARCHAR(10)` | Nullable | STO |
| `planning_priority` | planning_priority | `INTEGER` | Nullable | - |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：集团内部转储双向供需传导
* **因果流向**：库存转储订单 `ipc_logistics_stock_transfer_order` (STO) 定义了集团内各工厂站点间的内部需求拉动。
* **双向消纳编排**：
  1. 双重角色映射：一个 STO 订单在发货站点（From Site）表现为“依赖需求（Dependent Demand）”，在接收站点（To Site）表现为“在途确认供给（Scheduled Receipt）”。
  2. 交期偏差控制：引擎根据转储路由的物流提前期自动平移发货与到货时间，并通过发/收站点的双重日历校验可用性，实时更新 `available_delivery_date` 和 `available_ship_date`。