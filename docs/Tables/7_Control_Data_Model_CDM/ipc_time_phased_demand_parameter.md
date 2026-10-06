---
table_name: "ipc_time_phased_demand_parameter"
alias: "time_phased_demand_parameter"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcTimePhasedDemandParameterRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_time_phased_demand_parameter` (time_phased_demand_parameter)

> **业务说明**: TimePhasedDemandParameterSet表用于手动指定给定安全库存项目(物料)的历史需求的时间阶段平均值和标准偏差。然后，这些值可以用作确定建议的安全库存水平的参数。此表适用于某项产品的历史需求数据有限，但时间分阶段的统计参数是已知的或通过其他方式估计的情况。一般来说，在一个完整的周期中，每个周期/季节应该指定一个记录。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `average` | average | `DECIMAL(18,2)` | Nullable | - |
| `id` | id | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `index` | index | `VARCHAR` | Nullable | 一种以零为基础的指数，用于表示该记录的参数在整个季节周期内适用的时期或季节。例如，如果CycleCalendar为“Year”，则
PeriodCalendar是“Month”，然后0表示1月，1表示2月，以此类推 |
| `safety_stock_item` | safety_stock_item | `VARCHAR` | PK / NOT NULL | 对安全库存项目的引用，此记录上的参数适用于该项目。
在此记录上定义的值仅在此项目引用SafetyStockItemType时用于安全库存计算
并且TimePhasedProcessingRule设置为“Use”和
NonStationaryDemandRule设置为“Manual”。 |
| `standard_deviation` | standard_deviation | `DECIMAL(18,2)` | Nullable | 指定时期内该项目历史需求的标准差(按指数计算)。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：时变需求参数表。存储预测模型在时间数轴上变化的趋势、水平、季节性等系数，用于生成远期预测。
* **计算逻辑编排**：
  1. 拟合参数提取：读取各时间桶对应的 Level, Trend, Seasonal (L, T, S) 状态值；2. 远期外推计算：\n      $$ \\hat{Y}_{t+h} = (L_t + h \\times T_t) \\times S_{t+h-p} $$\n   3. 预测量输出：生成 forecast_detail 明细记录并落库。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_time_phased_demand_parameter 的 C++ DOD 物理对齐结构体
struct IpcTimePhasedDemandParameterRecord {
    double average = 0.0; // average 数量/金额精度值 (-)
    uint32_t id; // id 逻辑ID/映射 (唯一标识)
    double standard_deviation = 0.0; // standard_deviation 数量/金额精度值 (指定时期内该项目历史需求的标准差(按指数计算)。)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。