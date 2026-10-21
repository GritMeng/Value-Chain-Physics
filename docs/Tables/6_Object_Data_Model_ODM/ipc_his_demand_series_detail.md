---
table_name: "ipc_his_demand_series_detail"
alias: "his_demand_series_detail"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisDemandSeriesDetailRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_series_detail` (his_demand_series_detail)

> **业务说明**: 该表包含历史需求序列中每个唯一数量和日期间隔组合的条目。它为历史需求序列中的每个非零点包含一个条目.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | - |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | - |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `series` | series | `VARCHAR` | Nullable | - |
| `unit_price` | unit_price | `DOUBLE` | Nullable | 与历史需求相关的单价。此字段用于计算与历史需求相关的收入。 |
| `value` | value | `DOUBLE` | Nullable | 与这一历史需求相关的货币价值。
此字段的值用于
HistoricalDemandCategory。UnitType = 'Value |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史需求序列明细表。存储具体历史时期内的销量及价格明细，是 Holt-Winters/ARIMA 等模型的直接输入源。
* **计算逻辑编排**：
  1. 时序对齐：按 eff_start_date 升序排列，生成连续的时序数组；2. 收入核算：对历史各时段销量计算 $Qty \\times UnitPrice$ 折算为历史销售额，为 IBP 提供历史 ROI 分析基础。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_series_detail 的 C++ DOD 物理对齐结构体
struct IpcHisDemandSeriesDetailRecord {
    int eff_start_date = -1; // eff_start_date 相对计划天数 (-)
    int eff_end_date = -1; // eff_end_date 相对计划天数 (-)
    double qty = 0.0; // qty 数量/金额精度值 (数量 (Quantity))
    std::string series; // series 字符串 (-)
    double unit_price = 0.0; // unit_price 数量/金额精度值 (与历史需求相关的单价。此字段用于计算与历史需求相关的收入。)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。