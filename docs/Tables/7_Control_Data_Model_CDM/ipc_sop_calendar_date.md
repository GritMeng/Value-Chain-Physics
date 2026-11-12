---
table_name: "ipc_sop_calendar_date"
alias: "calendar_date"
module: "7_Control_Data_Model_CDM"
cpp_struct: "CalendarDateRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_sop_calendar_date` (calendar_date)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `DATE` | PK / NOT NULL | 日期 |
| `display` | display | `VARCHAR(10)` | Nullable | 显示出来的日期 |
| `calendar` | calendar | `VARCHAR(10)` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：物理日期实例化与班次开开歇判定
* **因果流向**：`ipc_sop_calendar_date` 记录了各个逻辑日历（Calendar）在具体日期（Date）上的明细属性，定义了每一天是工作日、公休日还是特殊的停机维护日（Maintenance Day）。计划引擎的排产器（Scheduler）在运行时，依据该表初始化引擎内部的全局时间轴，将连续的物理时间离散化为可供 MRP/APS 使用的时间桶。
* **判定算法编排**：
  1. 日历日实例构造：对日历内的每一天，根据其所属的 `calendar`，匹配其基准工作小时数（Base Hours）。
  2. 临时班次变更合并：叠加上游生产日历临时变更（如临时加班、台风停产），动态计算出最终的可用时长。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 内存中，该表数据用于一次性编译出所有日历的时间轴（Timeline）索引。为避免大量的 `std::string` 哈希检索，日历名称在加载时被哈希为 `uint32_t ID`，其日期被映射为与基准日的相对偏移天数，存储于极窄的扁平 SoA 数组中：
```cpp
// 对应 ipc_sop_calendar_date 的内存 DOD 结构体
struct CalendarDateRecord {
    uint32_t calendar_id; // 日历逻辑编码 (哈希值)
    int day_offset;       // 相对计划天数 (以基准日 2000-01-01 为 0 的相对偏移天数)
    bool is_working;      // 该日是否工作 (对应工作日判定)
    float base_hours;     // 额定工作时长 (用于能力折算)
};
```

###### 3. 边界与异常处理
* **日历孔洞防护（Calendar Holes）**：如果日历日期表在某段计划期内出现数据断层（如漏配了某个月 of 日期记录），引擎在进行倒排拉动时会陷入无限死循环。此时，引擎的日历预编译器在初始化时会进行“完整性断层扫描”，一旦发现日期不连续，自动复制上一年的日历结构进行“插值填平”，并发出严重警告。