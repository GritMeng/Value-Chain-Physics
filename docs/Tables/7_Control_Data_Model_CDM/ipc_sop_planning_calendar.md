---
table_name: "ipc_sop_planning_calendar"
alias: "planning_canlendar"
module: "7_Control_Data_Model_CDM"
cpp_struct: "CalendarDayRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_sop_planning_calendar` (planning_canlendar)

> **业务说明**: 物料的日历提供了用于计算日期的工作安排信息。可以使用多个planning_calendar记录来使不同物料拥有不同的操作安排。采购的零部件可能按照供应商所定义的安排进行运作

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `DATE` | Nullable | 日期 |
| `calendar` | calendar | `VARCHAR(10)` | Nullable | - |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组的编码 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `work_centers` | work_centers | `VARCHAR(10)` | Nullable | 跟工作中心数据相关 |
| `spread_canlendar` | spread_canlendar | `VARCHAR(10)` | Nullable | MPS 预测spreading应用 |
| `run_date` | run_date | `DATE` | Nullable | 日历。值，用于确定从企业数据源提取数据的日期。注意:RunDate。FirstDate计算字段可用于获取用于报告目的的日期。将运行日期获取到系统中的此方法允许将日期放入提取的表中，但可以通过part属性指定该表 |
| `assignment_calendar` | assignment_calendar | `VARCHAR(10)` | Nullable | 提供执行fair-share and equal-share分配的日期。相同OrderPriority的所有需求会在此期间计算。 |
| `co_by_product_calendar` | co_by_product_calendar | `VARCHAR(10)` | Nullable | 在计算副产品时使用间隔的日历。指定的日历定义了一个时间间隔，在这个时间间隔内收集和排序对主要产品的需求，并且在这个时间间隔的开始创建对主要产品所需的计划订单。例如，如果设置为“周”日历，那么所有需要的计划订单都将在一周里开始创建。此设置可用于帮助确保所产生的副产物或副产物供应可用于满足期间内的需求，从而有助于减少副产物和副产物供应可能造成的过剩。注意，一个给定的初级产品和它所有相关的副产品和副产品应该引用相同的cobyproduct计划间隔日历。如果不需要过多的副产品和副产品，或者这种“预先计划”逻辑不是必需的，那么这个日历应该设置为“每天”日历。这个日历被与任何副产品或副产品配置无关的部分忽略 |
| `planning_interval` | planning_interval | `DECIMAL(18,2)` | Nullable | - |
| `expiry_calendar` | expiry_calendar | `VARCHAR(10)` | Nullable | 在物料到期计算中使用其间隔时间的日历 |
| `forecast_calendar` | forecast_calendar | `VARCHAR(10)` | Nullable | 给出预测间隔日期(接收预测的Buckets)。例如,月 |
| `order_point_calendar` | order_point_calendar | `VARCHAR(10)` | Nullable | 订货点使用的日历 |
| `percent_safety_calendar` | percent_safety_calendar | `VARCHAR(10)` | Nullable | 当SafetyStockQuantityRule是“PercentOfDemand”或“FranctionOfDemand”时，标识用于计算安全库存的日历。也就是说，计算所依赖的日期段是基于一个周期的，这个周期的长度是N个间隔。其中间隔由引用的日历值定义，“N”设置为Part.PercentSafetyBucketCount。例如，为了拥有一个基于四周需求的安全值，可以将该字段设置为引用“周”日历和部分。设置为“4”的PercentSafetyBucketCount. |
| `bucket` | bucket | `VARCHAR(10)` | Nullable | - |
| `supply_calendar` | supply_calendar | `VARCHAR(10)` | Nullable | 在部件类型为MaterialType.DaySupplyRule=’ByPeriod’等时给出有效的计划订单到期日历。DaysSupplyRule =“按周期”、“按周期优先”、“按周期结束”或“按周期优先”。例如，如果计划的订单应该在本周初到期，那么将其设置为周日历。或者，如果计划的订单应该在给定间隔内的第一个需求日到期，则将其设置为工作日日历(例如，Material.PlanningCalendar.Timeunits)。 |
| `customer_acception_calendar` | customer_acception_calendar | `VARCHAR(10)` | Nullable | 客户接收位置的日历 |
| `due_date_rule` | due_date_rule | `VARCHAR(10)` | Nullable | - |
| `intermedia_calendar` | intermedia_calendar | `VARCHAR` | Nullable | - |
| `planning_calendar` | planning_calendar | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `time_unit` | time_unit | `VARCHAR(10)` | Nullable | reference:calendar |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多层次日历合并与时效转换引擎
* **因果流向**：`ipc_sop_planning_calendar` 是系统进行 MRP 运算和产能负荷排程的时间坐标尺。它将实际物理日期（Gregorian Calendar）翻译为计划引擎内部的绝对天数索引（`day_bucket`），并整合工作中心日历、供应商工作日历和物流清关日历，判定各站点的产能供应可用时间窗口。
* **时效转换与合并算法**：
  1. 日历合并（Logical Calendar ANDing）：若一个制造工单既需要扣减工作中心资源，又需要供应商在途收货（Scheduled Receipt），其可用日期必须取两方日历的交集：
     $$ Is\_Workday_{combined}(t) = Is\_Workday_{wc}(t) \land Is\_Workday_{vendor}(t) $$
  2. 计划订单交期平移（Lead Time Bucketing）：在扣减提前期（Lead Time）时，如果遇到非工作日（Holiday），排产引擎会自动向前（对于 MRP 倒排）或向后（对于 CTP 正排）跳过非工作日，确保工单的 `start_date` 和 `finish_date` 落在合法的工厂开班日期内。

###### 2. 物理内存结构设计 (C++ DOD Layout)
日历在内存中通常由一个紧凑的布尔型密集向量（Dense Vector of Boolean）表示，支持 $O(1)$ 的时间桶可达性查询，最大限度节省高速缓存（Cache）空间：
```cpp
// 对应 ipc_sop_planning_calendar 的内存对齐物理结构
struct CalendarDayRecord {
    bool is_working_day : 1;      // 相对天数是否为工作日 (使用 bitfield 压缩)
    bool is_shipping_day : 1;     // 是否可以发货
    bool is_receiving_day : 1;    // 是否可以收货
    uint8_t shift_count : 5;      // 班次数量
    double available_hours;       // 该天班次名义总产能工时
};

struct SopPlanningCalendarRecord {
    uint32_t calendar_id;                  // 日历 ID (对应 planning_calendar)
    int start_julian_date;                 // 计划期第一天的儒略日 (Julian Date)
    std::vector<CalendarDayRecord> days;   // 时序密集分布数组，索引即为 Day Bucket 相对天数
};
```

###### 3. 边界与异常处理
* **日历有效期溢出（Calendar Overflow）**：若计划订单的推算日期超出该日历配置的 `days` 数组最大范围（例如推算出了 3 年后的工单，但日历只维护了 2 年），系统会自动使用默认的工作日规则（如周一至周五开班，周六日休息）进行外推兜底，同时抛出低优先级预警日志，不阻断计算。