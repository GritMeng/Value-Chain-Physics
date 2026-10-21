---
table_name: "ipc_his_supply_actual"
alias: "his_supply_actual"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_supply_actual` (his_supply_actual)

> **业务说明**: 包含历史供应订单的实际详细信息，例如下订单的日期和实际收到订单的日期，以便在计算安全库存项目的交货时间变化时使用。该表中使用的记录是与项目匹配的记录HistoricalSupplyCategory参考

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `actual_dock_date` | actual_dock_date | `DATE` | Nullable | - |
| `actual_ship_date` | actual_ship_date | `DATE` | Nullable | - |
| `date` | date | `DATE` | Nullable | Ship单据的创建日期 |
| `handing_cost` | handing_cost | `DOUBLE` | Nullable | - |
| `his_supply_header` | his_supply_header | `VARCHAR` | PK / NOT NULL | Reference |
| `lead_time` | lead_time | `VARCHAR` | Nullable | 与此历史供应相关的已知交货时间。
通常，在定义OrderDate和Date字段时，假定供应可变性的安全库存计算中使用的历史提前期值是基于这两个字段计算的。然而，如果未提供OrderDate字段，则将安全库存计算中使用的历史交货期值设置为该字段中提供的值. |
| `line_due_date` | line_due_date | `VARCHAR` | Nullable | 期望的发货日期 |
| `order` | order | `VARCHAR` | Nullable | Reference : HisSupplyOrder |
| `order_date` | order_date | `VARCHAR` | Nullable | 与此历史供应相关的已知交货时间。
通常，在定义OrderDate和Date字段时，假定供应可变性的安全库存计算中使用的历史提前期值是基于这两个字段计算的。然而，如果
未提供OrderDate字段，则将安全库存计算中使用的历史交货期值设置为该字段中提供的值 |
| `order_due_date` | order_due_date | `VARCHAR` | Nullable | 指示与历史供应订单关联的DueDate。
如果使用MPS应用程序，则应该填充此字段，并且某些报告详细信息(如进度实现)的资源需要此字段。 |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `receipt_qty` | receipt_qty | `VARCHAR` | Nullable | 接收到的数量 |
| `shipment` | shipment | `VARCHAR` | Nullable | Reference:HisShipment |
| `shipping_cost` | shipping_cost | `DOUBLE` | Nullable | - |
| `source` | source | `VARCHAR` | Nullable | Reference:Source |
| `unit_price` | unit_price | `DOUBLE` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史供应交付实绩表。记录历史采购订单（PO）和生产工单（WO）的实际发运、到货日期和数量，用于计算提前期变动（LT Deviation）。
* **计算逻辑编排**：
  1. 偏差度量：计算实际提前期 $LT_{actual} = actual\_dock\_date - order\_date$；2. 方差传播：基于多条交付实绩计算提前期均值 $\mu_{LT}$ 与标准差 $\sigma_{LT}$，作为多级安全库存优化（MEIO）中供应侧不确定性的直接输入；3. 准时交付率（OTIF）核算：\n      $$ OTIF = \\frac{\\sum I(actual\_dock\_date \\le line\_due\_date \\text{{ and }} receipt\_qty \\ge qty)}{{Total\_Orders}} $$\n

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_supply_actual 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcHisSupplyActualRecord {
    int actual_dock_date = 0; // actual_dock_date 相对计划天数 (-)
    int actual_ship_date = 0; // actual_ship_date 相对计划天数 (-)
    int date = 0; // date 相对计划天数 (Ship单据的创建日期)
    double handing_cost = 0.0; // handing_cost 数量/金额精度值 (-)
    std::string his_supply_header; // his_supply_header 字符串 (Reference)
    std::string line_due_date; // line_due_date 字符串 (期望的发货日期)
    std::string order; // order 字符串 (Reference : HisSupplyOrder)
    std::string qty; // qty 字符串 (数量 (Quantity))
    std::string receipt_qty; // receipt_qty 字符串 (接收到的数量)
    std::string shipment; // shipment 字符串 (Reference:HisShipment)
    double shipping_cost = 0.0; // shipping_cost 数量/金额精度值 (-)
    std::string source; // source 字符串 (Reference:Source)
    double unit_price = 0.0; // unit_price 数量/金额精度值 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。