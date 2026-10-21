---
table_name: "ipc_procurement_schedule_line"
alias: "procurement_schedule_line"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcProcurementScheduleLineRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_procurement_schedule_line` (procurement_schedule_line)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `id` | id | `VARCHAR(18)` | Nullable | 供应订单编号，如采购订单和工单。
Reference Table: SupplyOrder |
| `item` | item | `DOUBLE` | Nullable | LineNum,供应订单的schedulelineItem编号
Reference Table: SupplyOrder |
| `quantity` | quantity | `DECIMAL(18,2)` | Nullable | 还未收货的数量 |
| `shipped_qty` | shipped_qty | `DECIMAL(18,2)` | Nullable | 已发货数量 |
| `needed_qty` | needed_qty | `DECIMAL(18,2)` | Nullable | 需求数量 |
| `recieved_qty` | recieved_qty | `DECIMAL(18,2)` | Nullable | 已收货数量 |
| `request_stock_date` | request_stock_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的StockDate. Or RequestDockDate+MaterialSource.DockToStockLT。 |
| `request_dock_date` | request_dock_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的DockDate,RequestStockDate-DockToStockLT-Canlendar. 这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequestShipDate+Source.TransitCalendar+Source.TransitLT+Destination.Site.Calendar. 如果是工单，DockDate为完工日期。 |
| `request_ship_date` | request_ship_date | `DATE` | Nullable | 根据客户需求推导由MRP出来的ShipDate,RequesStockDate-MaterialSource.TransitLT-Canlendar. 这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequesDueDate+Source.ShipCalendar+MaterialSource.PreShipLT。
注意：ShipDate如果已经超过RunDate，意味着供应商几乎不能完成交付。应该在MRP中考虑以何种策略应对这种情况。 |
| `request_due_date` | request_due_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的DueDate,RequestShipDate-MaterialSource.PreShipLT-Source.ShipCalendar-TransitCalendar.这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequestBultStartDate+MaterialSource.BuiltLT(Fixed,Ad,Va).
 |
| `request_built_date` | request_built_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的BuiltDate,由RequestDueDate- MaterialSource.Built(AD,Va,Fixed)LT-Calendar.Or OrderStartDate+MaterialSource.PreBuiltLT |
| `request_order_start_date` | request_order_start_date | `DATE` | Nullable | 根据客户需求由MRP推导出出来的SupplyOrder开始处理的准备执行的日期。RequestBuiltDate-MaterialSource.PreBuiltLT-Calendar-FreezeDate. Or RunDate+Calendar+FreezeDate |
| `request_ship_to_site` | request_ship_to_site | `VARCHAR(8)` | Nullable | 接收货物的Site
Reference Table: Site |
| `request_ship_to_location` | request_ship_to_location | `VARCHAR(10)` | Nullable | 接收货物的Location
Reference Table: Location |
| `unit` | unit | `VARCHAR(10)` | Nullable | 采购单位，Reference Table: [[ipc_part|Material]]Source.SupplierUOM |
| `schedule_line` | schedule_line | `DOUBLE` | PK / NOT NULL | 计划交货行，采购订单的一个Item,供应商可能有多次发货，对应不同的交付计划。 |
| `processed` | processed | `BOOLEAN` | Nullable | 订单是否已经经过全部确认，包括供应商和Buyer
Y - 已经确认
N - 没有全部确认 |
| `confirmed_date_by_supplier` | confirmed_date_by_supplier | `DATE` | Nullable | 供应商确认的日期,这里是第一次确认的日期 |
| `confirmed_date_by_buyer` | confirmed_date_by_buyer | `DATE` | Nullable | Buyer已经确认此订单的日期确认的日期 |
| `confirmed_by_supplier` | confirmed_by_supplier | `BOOLEAN` | Nullable | 供应商是否确认
Y - 已经确认
N - 没有全部确认 |
| `supply_status` | supply_status | `VARCHAR(10)` | Nullable | 处理规则。 Reference Table: SupplyStatus |
| `confirmend_qty` | confirmend_qty | `DECIMAL(18,2)` | Nullable | 供应商确认的数量 |
| `resch_stock_date` | resch_stock_date | `DATE` | Nullable | 经过CTP重新规划的日期 |
| `resch_dock_date` | resch_dock_date | `DATE` | Nullable | 经过CTP重新规划的日期 |
| `resch_ship_date` | resch_ship_date | `DATE` | Nullable | 经过CTP重新规划的日期 |
| `resch_due_date` | resch_due_date | `DATE` | Nullable | 经过CTP重新规划的日期 |
| `resch_built_date` | resch_built_date | `DATE` | Nullable | 经过CTP重新规划的日期 |
| `resch_order_start_date` | resch_order_start_date | `DATE` | Nullable | 经过CTP重新规划的日期 |
| `resch_quantity` | resch_quantity | `DECIMAL(18,2)` | Nullable | 经过CTP重新规划的数量 |
| `state` | state | `VARCHAR(10)` | Nullable | 供给所处的状态：
Shipped - 已发货
Built - 已经开始生产
CTB - 物料和资源已经准备好（这里才是订单真正可以开始的日期）
 |
| `actual_ship_date` | actual_ship_date | `DATE` | Nullable | 实际发货日期, Reference Table: Shipment  |
| `actual_stock_date` | actual_stock_date | `DATE` | Nullable | 实际收货日期 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：主数据层级拓扑映射与沙盘数据逻辑隔离算法
* **因果流向**：系统主数据及控制配置表。
* **计算逻辑编排**：
  1. 数据加载与主数据校验：引擎启动时从物理数据库中读取该表记录，通过 Hash 映射机制将字符串主键转译为 $O(1)$ 的内存索引 ID，建立缓存友好的 SoA 内存块；
2. 时序对齐与时空平移：结合计划日历（CalendarRecord），对记录中的日期字段进行天级偏移（Day Offset）计算，在时间数轴上与独立需求或在手水位对齐；
3. 供需消纳与抵扣：依据 Composite Priority 优先级位权或 FIFO 滑动窗口，对数量字段进行原子扣减或比例分摊计算：\n      $$ Qty_{{effective}}(t) = \\max\\left(0, Qty_{{request}}(t) - Qty_{{allocated}}(t)\\right) $$\n
4. 指标同步与回写：计算结果暂存在线程局部的事务栈中，确认齐套后批量落库，并级联更新上层财务账本或控制塔指标看板。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_procurement_schedule_line 的 C++ DOD 物理对齐结构体
struct IpcProcurementScheduleLineRecord {
    double quantity = 0.0; // quantity 数量/金额精度值 (还未收货的数量)
    double shipped_qty = 0.0; // shipped_qty 数量/金额精度值 (已发货数量)
    double needed_qty = 0.0; // needed_qty 数量/金额精度值 (需求数量)
    double recieved_qty = 0.0; // recieved_qty 数量/金额精度值 (已收货数量)
    int request_stock_date = 0; // request_stock_date 相对计划天数 (根据客户需求由MRP推导出来的StockDate. Or RequestDockDate+MaterialSource.DockToStockLT。)
    int request_dock_date = 0; // request_dock_date 相对计划天数 (根据客户需求由MRP推导出来的DockDate,RequestStockDate-DockToStockLT-Canlendar. 这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequestShipDate+Source.TransitCalendar+Source.TransitLT+Destination.Site.Calendar. 如果是工单，DockDate为完工日期。)
    int request_built_date = 0; // request_built_date 相对计划天数 (根据客户需求由MRP推导出来的BuiltDate,由RequestDueDate- MaterialSource.Built(AD,Va,Fixed)LT-Calendar.Or OrderStartDate+MaterialSource.PreBuiltLT)
    int request_order_start_date = -1; // request_order_start_date 相对计划天数 (根据客户需求由MRP推导出出来的SupplyOrder开始处理的准备执行的日期。RequestBuiltDate-MaterialSource.PreBuiltLT-Calendar-FreezeDate. Or RunDate+Calendar+FreezeDate)
    std::string unit; // unit 字符串 (采购单位，Reference Table: MaterialSource.SupplierUOM)
    double schedule_line = 0.0; // schedule_line 数量/金额精度值 (计划交货行，采购订单的一个Item,供应商可能有多次发货，对应不同的交付计划。)
    int confirmed_date_by_supplier = 0; // confirmed_date_by_supplier 相对计划天数 (供应商确认的日期,这里是第一次确认的日期)
    int confirmed_date_by_buyer = 0; // confirmed_date_by_buyer 相对计划天数 (Buyer已经确认此订单的日期确认的日期)
    std::string supply_status; // supply_status 字符串 (处理规则。 Reference Table: SupplyStatus)
    double confirmend_qty = 0.0; // confirmend_qty 数量/金额精度值 (供应商确认的数量)
    int resch_stock_date = 0; // resch_stock_date 相对计划天数 (经过CTP重新规划的日期)
    int resch_dock_date = 0; // resch_dock_date 相对计划天数 (经过CTP重新规划的日期)
    int resch_ship_date = 0; // resch_ship_date 相对计划天数 (经过CTP重新规划的日期)
    int resch_due_date = -1; // resch_due_date 相对计划天数 (经过CTP重新规划的日期)
    int resch_built_date = 0; // resch_built_date 相对计划天数 (经过CTP重新规划的日期)
    int resch_order_start_date = -1; // resch_order_start_date 相对计划天数 (经过CTP重新规划的日期)
    double resch_quantity = 0.0; // resch_quantity 数量/金额精度值 (经过CTP重新规划的数量)
    int actual_ship_date = 0; // actual_ship_date 相对计划天数 (实际发货日期, Reference Table: Shipment)
    int actual_stock_date = 0; // actual_stock_date 相对计划天数 (实际收货日期)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。