---
table_name: "ipc_resource_capacity"
alias: "capacity"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ConstraintRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_resource_capacity` (capacity)

> **业务说明**: Capacity表标识工作中心在特定时间点的可用能力，用于调度操作的持续时间。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `work_center` | work_center | `VARCHAR(10)` | Nullable | Reference Table: WorkCenter |
| `std_labor` | std_labor | `DECIMAL(18,2)` | Nullable | 人工一天的工时 |
| `std_mechine` | std_mechine | `DECIMAL(18,2)` | Nullable | 机器一天的工时 |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 生效日期 |
| `day_hour` | day_hour | `DECIMAL(18,2)` | Nullable | 此工作中心的一天可用的工时 |
| `labor_run_ratio` | labor_run_ratio | `DECIMAL(18,2)` | Nullable | Labor工时/Standard工时 |
| `labor_setup_ratio` | labor_setup_ratio | `DECIMAL(18,2)` | Nullable | Labor工时/Standard工时 |
| `machine_run_ratio` | machine_run_ratio | `DECIMAL(18,2)` | Nullable | Machine工时/Standar工时 |
| `machine_setup_ratio` | machine_setup_ratio | `DECIMAL(18,2)` | Nullable | Machine工时/Standar工时 |
| `queue_time` | queue_time | `DECIMAL(18,2)` | Nullable | 用于调度此工作中心的所有操作的预操作队列时间，除非被覆盖(请参阅
CRPOperation.QueueTimeOverride)。此值是安装程序可以开始之前等待的小时数，必须大于或等于零。 |
| `wait_time` | wait_time | `DECIMAL(18,2)` | Nullable | - |
| `efficiency` | efficiency | `DECIMAL(18,2)` | Nullable | 用于缩放此工作中心的操作设置和运行时间的因子。换句话说，就是衡量你的假设产出和实际产出。当与Utilization因子相乘时，得到的值就是使用因子。这用于将运行时间和设置时间从“标准小时”转换为实际经过的小时。 |
| `hours_per_day` | hours_per_day | `DECIMAL(18,2)` | Nullable | 每天工作时长 |
| `max_machine_capacity` | max_machine_capacity | `DECIMAL(18,2)` | Nullable | 最大工时 |
| `max_labor_capacity` | max_labor_capacity | `DECIMAL(18,2)` | Nullable | 最大工时 |
| `number_of_resources` | number_of_resources | `DECIMAL(18,2)` | Nullable | 此工作中心可用的资源数量。
使用此值调整工作中心的可用容量，该值必须是非负的。
如果有多个可用资源，并且允许在给定操作上进行批分割(由
Operation.MaxResource决定)，那么操作将比只使用一个资源时更快地完成。 |
| `utilization` | utilization | `DECIMAL(18,2)` | Nullable | 该因素用于扩展此工作中心的操作设置和运行时间，以允许一天中的非生产时段(例如，休息时间)。当与
效率因子，结果值是使用因子。这用于将运行时间和设置时间从
“标准小时”到实际经过的小时 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：有限能力工时预算冲减与拉平
* **因果流向**：`ipc_resource_capacity` 定义了每个工作中心在各个时段的物理可用产能。
* **算法编排**：
  1. CTP 检查：当工单在某天 $S$ 排产时，工序（Operation）计算其所需的总负荷：
     $$ Load = SetupTime + RunTime \times Qty $$
  2. 刚性产能约束：系统校验当前分配负荷是否超过可用上限 $Capacity_{total} = hours\_per\_day \times number\_of\_resources \times efficiency \times utilization$。
  3. 替代路径回溯：若当前工作中心负荷溢出，算法原位回滚，在 $O(1)$ 时间内释放临时占用的库存和下层产能，转而尝试替代工艺路线。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存计算中，产能负荷以连续的双精度浮点数数组记录在 `ConstraintRecord` 中，保证多核 CPU 并行评估时的内存读取效率：

```cpp
// 关联 ipc_resource_capacity 表的 C++ DOD 物理数据结构
struct ConstraintRecord {
    uint32_t constraint_id;              // 约束资源逻辑 ID (全局一维索引)
    std::string constraint_code;         // 资源物理编码 (对应 work_center)
    std::string constraint_type;         // 约束类型 (Constrained, LoadOnly, Unconstrained)
    std::vector<double> rates;           // 天级可用工时上限数组 (对应 hours_per_day)
    std::vector<double> allocated_rates; // 天级已占用工时负荷数组
};
```