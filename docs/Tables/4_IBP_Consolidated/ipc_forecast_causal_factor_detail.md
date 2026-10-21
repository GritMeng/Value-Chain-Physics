---
table_name: "ipc_forecast_causal_factor_detail"
alias: "causal_factor_details"
module: "4_IBP_Consolidated"
cpp_struct: "CausalFactorDetailRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_causal_factor_detail` (causal_factor_details)

> **业务说明**: 该表通过对CausalFactor表的引用包括属于给定因果因素的日期和数量详细信息。此外，本表中报告的数量反映在分类的预测细节和分类率中。如果报告了因果关系的细节，则会在以下表格中考虑这些细节:
ForecastDetail
StatisticalForecastDetail
DisaggregationRateByPartCustomer
StatisticalForecastDisaggregationRate

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `operation_rule` | operation_rule | `VARCHAR` | Nullable | 
SOP - 表明因果因素的细节只应包括在S&OP计算中。例如，在计算统计预测或分解预测时将使用它们.
SafetyStock - 指示因果因素的详细信息应仅包括在安全库存计算中。
当在部件级别创建离群值调整时使用此值。
All - 都用. |
| `date` | date | `DATE` | Nullable | 应用日期 |
| `id` | id | `VARCHAR` | PK / NOT NULL | 唯一标识符 |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `unit_price` | unit_price | `VARCHAR` | Nullable | 一个可选字段，允许将单价应用于因果因素。如果在此字段中没有提供正值，则计算因果因素的单价，并在effecveunitprice字段中报告。缺省情况下，该字段的值为-1 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：因果事件时序影响量分配与单价调整
* **因果流向**：`ipc_forecast_causal_factor_detail` 记录了因果事件在具体日期上的波动数量和单价微调值。除了在预测前清洗历史数据，在生成未来预测时，引擎还会将这些计划中的因果事件数量叠加回基准预测上，形成最终包含促销拉动的实际预测。
* **叠加算法编排**：
  - 运行常规统计预测，得到基准时序 $Baseline(t)$。
  - 检索未来计划发生的因果明细，获取对应的调整数量 $Qty_{adjust}$ 和单价 $Price_{adjust}$。
  - 叠加计算最终预测数量与有效单价：
     $$ Forecast_{final}(t) = Baseline(t) + Qty_{adjust}(t) $$

###### 2. 物理内存结构设计 (C++ DOD Layout)
因果明细在内存中以时序有序向量形式存放，以便与常规预测的时间轴进行双指针顺序合并：
```cpp
// 对应 ipc_forecast_causal_factor_detail 的 C++ DOD 结构体
struct CausalFactorDetailRecord {
    uint32_t causal_factor_id;    // 因果因素主键 ID
    int day_bucket;               // 事件相对计划天数 (对应 date)
    double adjust_qty;            // 波动数量 (对应 qty)
    double adjust_unit_price;     // 调整单价 (对应 unit_price)
    uint8_t operation_rule;       // 应用策略枚举 (0=SOP, 1=SafetyStock, 2=All)
};
```

###### 3. 边界与异常处理
* **大促价格促销毛利倒挂拦截**：如果录入的 `unit_price` 促销价低于物料在 `ipc_part` 中的标准制造成本，系统会在保存时触发低毛利（Low Margin）预警，提示业务人员检查促销定价策略是否合规。