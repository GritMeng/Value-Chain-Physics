---
table_name: "ipc_work_center_capacity"
alias: "work_center_capacity"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CalendarRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_work_center_capacity` (work_center_capacity)

> **业务说明**: 给定工作中心和日期的有效Capacity。这包括该日期的工作时数、可用于工作时数的资源数量，以及任何效率或利用率因素。因为工作中心容量的详细信息可以作为输入提供.Capacity、CapacityOverride和WorkCenterType表，还指出了每组有效容量值的来源。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `work_center` | work_center | `VARCHAR(10)` | Nullable | Reference Table: WorkCenter |
| `date` | date | `DATE` | Nullable | 本记录中报告的工作中心容量详细信息适用的日期。
如果Source是“Capacity”或“WorkCenterType”，那么在给定的工作日中每个工作中心应该只报告一条记录。但是，如果Source是" CapacityOverride "，如果工作中心在白天有不同的班次，那么每个日期可能会报告多个记录。 |
| `working_hour` | working_hour | `DECIMAL(18,2)` | Nullable | 当日工作中心可用的总工作时数。
注意:如果Source是"WorkCenterType"，则返回WorkCenterType的值.HoursPerDay(返回1 当 HoursPerDay值小于1) |
| `efficiency` | efficiency | `DECIMAL(18,2)` | Nullable | 用于缩放此工作中心的操作设置和运行时间的因子。换句话说，就是衡量你的假设产出和实际产出。当与Utilization因子相乘时，得到的值就是使用因子。这用于将运行时间和设置时间从“标准小时”转换为实际经过的小时。 |
| `number_of_resources` | number_of_resources | `DECIMAL(18,2)` | Nullable | 此工作中心可用的资源数量。
使用此值调整工作中心的可用容量，该值必须是非负的。
如果有多个可用资源，并且允许在给定操作上进行批分割(由
Operation.MaxResource决定)，那么操作将比只使用一个资源时更快地完成。 |
| `utilization` | utilization | `DECIMAL(18,2)` | Nullable | 该因素用于扩展此工作中心的操作设置和运行时间，以允许一天中的非生产时段(例如，休息时间)。当与
效率因子，结果值是使用因子。这用于将运行时间和设置时间从
“标准小时”到实际经过的小时 |
| `capacity` | capacity | `VARCHAR` | Nullable | Reference: Capacity， 对应Source |
| `capacity_override` | capacity_override | `VARCHAR` | Nullable | Reference: CapacityOverride， 对应Source |
| `source` | source | `VARCHAR` | Nullable | WorkCenterType
Capacity
CapacityOverride
 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：天级动态日历合并
* **因果流向**：`ipc_work_center_capacity` 归集了由于工厂日历、加班排班或停机维护产生的有效天级可用能力数值。
* **日历合并编排**：求解器读取此表中的 `date` 和 `working_hour`。若某天为非工作日（`working_hour = 0`），引擎在内存日历中将其标记为不可用。工单交期排定回推时，自动跳过不可用日期，将任务前推或后延到最近的工作日上。

###### 2. 物理内存结构设计 (C++ DOD Layout)
工作中心日历被编译为天级 bool 向量，供 CTP 计算时快速过滤：

```cpp
// 关联工厂日历的内存对齐结构
struct CalendarRecord {
    std::string calendar_name;
    std::vector<bool> working_days; // 索引为天数偏移量，值为 true 表示开班，false 表示假期
};
```