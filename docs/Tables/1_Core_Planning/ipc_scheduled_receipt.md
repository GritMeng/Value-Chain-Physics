---
table_name: "ipc_scheduled_receipt"
alias: "scheduled_reciept"
module: "1_Core_Planning"
cpp_struct: "ScheduledReceiptRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_scheduled_receipt` (scheduled_reciept)

> **业务说明**: 在途供应订单表（未来到料）。记录采购订单和工单的在途未交付明细，是 LBL-MRP 计算时已存在且有确定交付期的可供消纳供应资源。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `sr_id` | sr_id | `VARCHAR(18)` | Nullable | 供应订单编号，如采购订单和工单。
Reference Table: SupplyOrder |
| `item` | item | `DOUBLE` | Nullable | LineNum,供应订单的schedulelineItem编号
Reference Table: SupplyOrder |
| `sup_sr_id` | sup_sr_id | `VARCHAR(18)` | Nullable | 供应订单编号，如采购订单和工单。
Reference Table: SupplyOrder |
| `to_part` | to_part | `VARCHAR(40)` | Nullable | 物料编号 |
| `from_part` | from_part | `VARCHAR(40)` | Nullable | 发货物料号 |
| `from_site` | from_site | `VARCHAR(8)` | Nullable | 从哪个Site发出 |
| `to_site` | to_site | `VARCHAR(8)` | Nullable | 接收Site |
| `transfer_order` | transfer_order | `VARCHAR(18)` | Nullable | 转储单编号 |
| `pur_to_part` | pur_to_part | `VARCHAR(40)` | Nullable | 物料编号 |
| `pur_from_part` | pur_from_part | `VARCHAR(40)` | Nullable | 发货物料号 |
| `pur_from_site` | pur_from_site | `VARCHAR(8)` | Nullable | 从哪个Site发出 |
| `pur_to_site` | pur_to_site | `VARCHAR(8)` | Nullable | 接收Site |
| `pur_Item` | pur_item | `DOUBLE` | Nullable | 转储行项目 |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `shipped_qty` | shipped_qty | `DECIMAL(18,2)` | Nullable | 已发货数量 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `recieved_qty` | recieved_qty | `DECIMAL(18,2)` | Nullable | 已收货数量 |
| `request_stock_date` | request_stock_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的StockDate. Or RequestDockDate+MaterialSource.DockToStockLT。 |
| `request_dock_date` | request_dock_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的DockDate,RequestStockDate-DockToStockLT-Canlendar. 这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequestShipDate+Source.TransitCalendar+Source.TransitLT+Destination.Site.Calendar. 如果是工单，DockDate为完工日期。 |
| `request_ship_date` | request_ship_date | `DATE` | Nullable | 根据客户需求推导由MRP出来的ShipDate,RequesStockDate-MaterialSource.TransitLT-Canlendar. 这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequesDueDate+Source.ShipCalendar+MaterialSource.PreShipLT。
注意：ShipDate如果已经超过RunDate，意味着供应商几乎不能完成交付。应该在MRP中考虑以何种策略应对这种情况。 |
| `request_due_date` | request_due_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的DueDate,RequestShipDate-MaterialSource.PreShipLT-Source.ShipCalendar-TransitCalendar.这里的LT如果启用了ConstrainPlanning,用Constrain来计算。Or RequestBultStartDate+MaterialSource.BuiltLT(Fixed,Ad,Va).
 |
| `request_built_date` | request_built_date | `DATE` | Nullable | 根据客户需求由MRP推导出来的BuiltDate,由RequestDueDate- MaterialSource.Built(AD,Va,Fixed)LT-Calendar.Or OrderStartDate+MaterialSource.PreBuiltLT |
| `request_order_start_date` | request_order_start_date | `DATE` | Nullable | 根据客户需求由MRP推导出出来的SupplyOrder开始处理的准备执行的日期。RequestBuiltDate-MaterialSource.PreBuiltLT-Calendar-FreezeDate. Or RunDate+Calendar+FreezeDate |
| `ship_to` | ship_to | `VARCHAR(8)` | Nullable | 接收货物的Site
Reference Table: Site |
| `to_location` | to_location | `VARCHAR(10)` | Nullable | 接收货物的Location
Reference Table: Location |
| `unit` | unit | `VARCHAR(10)` | Nullable | 采购单位，Reference Table: [[ipc_part|Material]]Source.SupplierUOM |
| `confirmed_date_by_supplier` | confirmed_date_by_supplier | `DATE` | Nullable | 供应商确认的日期,这里是第一次确认的日期 |
| `confirmed_date_by_buyer` | confirmed_date_by_buyer | `DATE` | Nullable | Buyer已经确认此订单的日期确认的日期 |
| `confirmed_by_supplier` | confirmed_by_supplier | `BOOLEAN` | Nullable | 供应商是否确认
Y - 已经确认
N - 没有全部确认 |
| `supply_status` | supply_status | `VARCHAR(10)` | Nullable | 处理规则。 Reference Table: SupplyStatus |
| `sche_stock_date` | sche_stock_date | `DATE` | Nullable | 反馈的日期 |
| `sche_dock_date` | sche_dock_date | `DATE` | Nullable | 反馈的日期 |
| `state` | state | `VARCHAR(10)` | Nullable | 供给所处的状态：
Shipped - 已发货
Built - 已经开始生产
CTB - 物料和资源已经准备好（这里才是订单真正可以开始的日期）
 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：在途供应消纳与 CTB 可承诺交期确权
* **因果流向**：`ipc_scheduled_receipt` 是在计划期 RunDate 之后、在未来不同日期 $t$ 到货的在途采购订单（PO）或车间工单（WO）。
* **计算逻辑编排**：
  1. 未来到货流：不同于在手库存，在途供应具有时间属性，只能消纳由于其到货日期（`request_dock_date` / `due_day`）之后的需求。
  2. 可靠性概率缩放：结合 `certainty_level`（供应可信度），对数量进行概率折算：
     $$ Supply_{effective} = qty \times certainty\_level $$
     用以防范供应商延期风险，计算出概率分位数的安全水平。
  3. CTB 确权：只有当在途的 `state = 'CTB'`（物料与产能均已就绪）时，该供应才能被视为刚性可用现有量。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，在途到货流被编译为 `ScheduledReceiptRecord` 数组，严格按到货天数 `due_day` 进行升序排序。当 MRP 引擎消纳时，直接使用 **二分查找 (Binary Search)** 在一维数组中进行 $O(\log N)$ 定位，找到符合交期偏置的最早可用在途供应，避免了逐日循环遍历：

```cpp
// 关联 ipc_scheduled_receipt 数据的 C++ DOD 物理对齐结构体
struct ScheduledReceiptRecord {
    std::string sr_id;             // 供应订单行唯一主键 (对应 sr_id)
    uint32_t part_id;              // 物料逻辑 ID
    double qty;                    // 供应数量 (对应 qty)
    int due_day;                   // 预计到货天数 (以 RunDate 为基准的相对天数，对应 request_dock_date)
    std::string sr_type = "In-process"; // 供应状态控制
    double certainty_level = 0.70; // 供应可信度系数 (对应 certainty_level)
};
```