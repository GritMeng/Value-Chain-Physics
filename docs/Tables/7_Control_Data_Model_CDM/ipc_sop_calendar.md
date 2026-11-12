---
table_name: "ipc_sop_calendar"
alias: "calendar"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSopCalendarRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_sop_calendar` (calendar)

> **业务说明**: 日历用于日期计算。制造业中常用的日历包括车间日历（工作日）和月历（定义每月报告期的起始）。每个日历在“日历”表中都有一个定义好的名称。日历由与日历名称相关联的日期列表calendar_date来定义。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `description` | description | `VARCHAR` | Nullable | 日历描述 |
| `first_date` | first_date | `DATE` | Nullable | 最早CalendarDate.Vaue日历的值。这是一个计算字段。如计算Past |
| `last_date` | last_date | `DATE` | Nullable | 最晚CalendarDate.Vaue日历的值。这是一个计算字段.如计算Future |
| `planning_or_reporting` | planning_or_reporting | `VARCHAR(10)` | Nullable | 标记是仅planning计算用，还是也可以用做UI/Reporting:
planning - 仅planning计算应用
both - planning和UI/Reporting都可以应用 |
| `calendar` | calendar | `VARCHAR(10)` | PK / NOT NULL | 日历编码，唯一标识 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：S&OP 会计周期与计划日历表。定义滚动计划的大周期（如 Monthly, Quarterly），用于财务指标汇算和宏观预测对账。
* **计算逻辑编排**：
  1. 时间桶划分：根据配置，将连续的天级天数划分进周/月/季度时间桶（Buckets）；2. 统计周期汇聚：在 S&OP 重算时，限制仅在定义的 Calendar 边界处进行销量与财务指标加总呈现。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_sop_calendar 的 C++ DOD 物理对齐结构体
struct IpcSopCalendarRecord {
    std::string description; // description 字符串 (日历描述)
    int first_date = 0; // first_date 相对计划天数 (最早CalendarDate.Vaue日历的值。这是一个计算字段。如计算Past)
    int last_date = 0; // last_date 相对计划天数 (最晚CalendarDate.Vaue日历的值。这是一个计算字段.如计算Future)
    std::string calendar; // calendar 字符串 (日历编码，唯一标识)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。