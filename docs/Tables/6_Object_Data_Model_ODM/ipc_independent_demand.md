---
table_name: "ipc_independent_demand"
alias: "independent_demand"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_independent_demand` (independent_demand)

> **业务说明**: 独立需求预测与订单表。存储客户订单及共识预测需求，包含请求交期、数量、优先级、客户层级、维度组（dimension_grp）等，是整个计划拉动消纳的源头需求。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `demand` | demand | `VARCHAR(10)` | 🔑 **PK / Required** | 需求编号，唯一标识
Reference Table: DemandHeader |
| `item` | item | `DOUBLE` | PK / NOT NULL | 与具有相同订单id的每个独立需求记录相关联的唯一标识符 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `par_site` | par_site | `VARCHAR` | Nullable | - |
| `customer` | customer | `VARCHAR(10)` | Nullable | 唯一标识符 |
| `request_delivery_date` | request_delivery_date | `DATE` | Nullable | 客户要求交付到客户处的日期。
可为空，报表中会提示有多少需求不能获得客户的需求日期

 |
| `request_due_date` | request_due_date | `DATE` | Nullable | 根据客户需求，产品需要在仓库准备好的日期。可以输入，如果没有输入可以由RequestDeliveryDate-DeliveryRoute.TransitLT-DockToStockLT-PickPackLT-PreShipLT-ShipCalendar-PickPackCalendar-TransitCalendar得出.
 |
| `standard_delivery_date` | standard_delivery_date | `DATE` | Nullable | OrderReleaseDate+PreBuildLT+BuidLT |
| `standard_due_date` | standard_due_date | `DATE` | Nullable | OrderReleaseDate+PreBuildLT+BuidLT+PreShipLT |
| `delivery_route` | delivery_route | `VARCHAR(10)` | Nullable | 用于将此订单运输到客户的交货路线的引用。相关细节，如运输时间和承运人负责交付可从此参考。
Reference Table:DeliveryRoute |
| `open_qty` | open_qty | `DECIMAL(18,2)` | Nullable | 未交货数量, 由RequesQty-ShippedQty得出，其参与Netting。+ShippedQty消耗预测 |
| `shipped_qty` | shipped_qty | `DECIMAL(18,2)` | Nullable | 已发货数量 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `status` | status | `VARCHAR(10)` | Nullable | 用来确定参与MRP，是否参与Forecast consumption，用什么日期来consumption。
Reference Table: DemandStatus |
| `shipment_rule` | shipment_rule | `VARCHAR(10)` | Nullable | 用来确定是否可以分开发货。
Reference Table:ShipmentRule |
| `shipment_group` | shipment_group | `VARCHAR(10)` | Nullable | 引用发运组，同一发运组的需求应该统一发货。
Reference Table:ShipGroup |
| `configuration` | configuration | `DOUBLE` | Nullable | 需求行项目引用的Configuration, 用来指定特殊产品结构需求.
OrderBOM, CTO BOM，以及Offspec等场景应用。
Reference Table: SpecificRequirment
 |
| `commited_date` | commited_date | `DATE` | Nullable | 提供PromisedDate的日期 |
| `actual_ship_date` | actual_ship_date | `DATE` | Nullable | 实际发货日期
Reference Table:Shipment |
| `actual_delivery_date` | actual_delivery_date | `DATE` | Nullable | 客户实际收货日期, 手工受输入货系统集成。 |
| `block_code` | block_code | `VARCHAR(10)` | Nullable | BlockCode表包含分配给客户和/或单个行项目的持有代码，目的是防止订单超出订单履行过程中的特定阶段。例如，可以为客户的信用问题创建hold代码，在问题解决之前不允许发送他们的订单.
Reference Table: BlockCode |
| `OrderCreateDateTime` | creation_date | `DATE` | Nullable | 订单的创建日期 |
| `release_date` | release_date | `DATE` | Nullable | 订单可以开始执行的日期 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `order_priority` | order_priority | `INTEGER` | Nullable | 优先级 |
| `site` | site | `VARCHAR(10)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `par_customer` | par_customer | `VARCHAR(10)` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：独立需求预测与订单表。合并了销售订单与共识预测，是 MRP 净需求展开发运的核心拉动源头。
* **计算逻辑编排**：
  1. 需求整合：汇总 sales_order 与 forecast_detail 的有效部分；2. ATP 计算：在 RunDate 时刻对各独立需求节点计算累计 ATP 水位；3. 滚动冲销：在 Before/After Window 内进行时序滑动冲销；4. 缺口展开：对于未满足的独立需求，调用 LBL-MRP 展开 BOM 生成依赖需求。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_independent_demand 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcIndependentDemandRecord {
    double item = 0.0; // item 数量/金额精度值 (与具有相同订单id的每个独立需求记录相关联的唯一标识符)
    uint32_t part; // part 逻辑ID/映射 (物料唯一编码 (Part Code))
    uint32_t par_site; // par_site 逻辑ID/映射 (-)
    uint32_t customer; // customer 逻辑ID/映射 (唯一标识符)
    int standard_delivery_date = 0; // standard_delivery_date 相对计划天数 (OrderReleaseDate+PreBuildLT+BuidLT)
    int standard_due_date = -1; // standard_due_date 相对计划天数 (OrderReleaseDate+PreBuildLT+BuidLT+PreShipLT)
    double open_qty = 0.0; // open_qty 数量/金额精度值 (未交货数量, 由RequesQty-ShippedQty得出，其参与Netting。+ShippedQty消耗预测)
    double shipped_qty = 0.0; // shipped_qty 数量/金额精度值 (已发货数量)
    double request_qty = 0.0; // request_qty 数量/金额精度值 (原始请求的订货/预测数量)
    int commited_date = 0; // commited_date 相对计划天数 (提供PromisedDate的日期)
    int actual_delivery_date = 0; // actual_delivery_date 相对计划天数 (客户实际收货日期, 手工受输入货系统集成。)
    int OrderCreateDateTime = 0; // OrderCreateDateTime 相对计划天数 (订单的创建日期)
    int release_date = 0; // release_date 相对计划天数 (订单可以开始执行的日期)
    std::string dimension_grp; // dimension_grp 字符串 (消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配)
    int order_priority = 0; // order_priority 整型数值 (优先级)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
    uint32_t par_customer; // par_customer 逻辑ID/映射 (-)
    uint64_t composite_priority; // 优先级位权
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。