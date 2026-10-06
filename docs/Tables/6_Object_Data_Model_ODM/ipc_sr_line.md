---
table_name: "ipc_sr_line"
alias: "sr_line"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_sr_line` (sr_line)

> **业务说明**: 供应的schedule line

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `sr_id` | sr_id | `VARCHAR(10)` | PK / NOT NULL | 需求编号，唯一标识
Reference Table: DemandHeader |
| `item` | item | `DOUBLE` | PK / NOT NULL | 与具有相同订单id的每个独立需求记录相关联的唯一标识符 |
| `request_delivery_date` | request_delivery_date | `DATE` | Nullable | 客户要求交付到客户处的日期。
可为空，报表中会提示有多少需求不能获得客户的需求日期

 |
| `request_due_date` | request_due_date | `DATE` | Nullable | 根据客户需求，产品需要在仓库准备好的日期。可以输入，如果没有输入可以由RequestDeliveryDate-DeliveryRoute.TransitLT-DockToStockLT-PickPackLT-PreShipLT-ShipCalendar-PickPackCalendar-TransitCalendar得出.
 |
| `p_delivery_date` | p_delivery_date | `DATE` | Nullable | OrderReleaseDate+PreBuildLT+BuidLT |
| `p_due_date` | p_due_date | `DATE` | Nullable | OrderReleaseDate+PreBuildLT+BuidLT+PreShipLT |
| `open_qty` | open_qty | `DECIMAL(18,2)` | Nullable | 未交货数量, 由RequesQty-ShippedQty得出，其参与Netting。+ShippedQty消耗预测 |
| `shipped_qty` | shipped_qty | `DECIMAL(18,2)` | Nullable | 已发货数量 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `status` | status | `VARCHAR(10)` | Nullable | 用来确定参与MRP，是否参与Forecast consumption，用什么日期来consumption。
Reference Table: DemandStatus |
| `shipment_rule` | shipment_rule | `VARCHAR(10)` | Nullable | 用来确定是否可以分开发货。
Reference Table:ShipmentRule |
| `shipment_group` | shipment_group | `VARCHAR(10)` | Nullable | 引用发运组，同一发运组的需求应该统一发货。
Reference Table:ShipGroup |
| `commited_date` | commited_date | `DATE` | Nullable | 提供PromisedDate的日期 |
| `actual_ship_date` | actual_ship_date | `DATE` | Nullable | 实际发货日期
Reference Table:Shipment |
| `actual_delivery_date` | actual_delivery_date | `DATE` | Nullable | 客户实际收货日期, 手工受输入货系统集成。 |
| `line` | line | `INTEGER` | Nullable | 计划行项目号 |
| `sche_delivery_date` | sche_delivery_date | `DATE` | Nullable | - |
| `sche_due_date` | sche_due_date | `DATE` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：在途供应行项目表。记录采购和生产行项目的细分计划行，是 MRP 消纳在途资源的细分维度对账实体。
* **计算逻辑编排**：
  1. 计划行消纳：支持一单多期交货，将单笔 SR 拆分为多个可用到货事件，分别在不同 ATP 截止日期上参与需求的冲抵和 Pegging 分配。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_sr_line 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcSrLineRecord {
    double item = 0.0; // item 数量/金额精度值 (与具有相同订单id的每个独立需求记录相关联的唯一标识符)
    int p_delivery_date = 0; // p_delivery_date 相对计划天数 (OrderReleaseDate+PreBuildLT+BuidLT)
    int p_due_date = -1; // p_due_date 相对计划天数 (OrderReleaseDate+PreBuildLT+BuidLT+PreShipLT)
    double open_qty = 0.0; // open_qty 数量/金额精度值 (未交货数量, 由RequesQty-ShippedQty得出，其参与Netting。+ShippedQty消耗预测)
    double shipped_qty = 0.0; // shipped_qty 数量/金额精度值 (已发货数量)
    double request_qty = 0.0; // request_qty 数量/金额精度值 (原始请求的订货/预测数量)
    int commited_date = 0; // commited_date 相对计划天数 (提供PromisedDate的日期)
    int actual_delivery_date = 0; // actual_delivery_date 相对计划天数 (客户实际收货日期, 手工受输入货系统集成。)
    int line = 0; // line 整型数值 (计划行项目号)
    int sche_delivery_date = 0; // sche_delivery_date 相对计划天数 (-)
    int sche_due_date = -1; // sche_due_date 相对计划天数 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。