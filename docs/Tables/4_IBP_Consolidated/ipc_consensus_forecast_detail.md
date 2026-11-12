---
table_name: "ipc_consensus_forecast_detail"
alias: "consensus_forecast_detail"
module: "4_IBP_Consolidated"
cpp_struct: "ConsensusForecastDetailRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_consensus_forecast_detail` (consensus_forecast_detail)

> **业务说明**: ConsensusForecastDetail表识别并报告ForecastDetail记录的详细信息，这些记录用于在销售和运营计划过程中为特定部件和客户生成一致预测值，并在ConsensusForecast表中报告。例如，它可能会报告多个加权预测类别的细节，这些类别对给定日期的共识预测有贡献，同时还会报告每个类别对共识预测值的贡献量

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `consensus_forecast` | consensus_forecast | `VARCHAR` | Nullable | 共识预测销售收入 (Qty * UnitPrice) |
| `eff_qty` | eff_qty | `VARCHAR` | Nullable | ConsensusForecastDetail表识别并报告ForecastDetail记录的详细信息，这些记录用于在销售和运营计划过程中为特定部件和客户生成一致预测值，并在ConsensusForecast表中报告。例如，它可能会报告多个加权预测类别的细节，这些类别对给定日期的共识预测有贡献，同时还会报告每个类别对共识预测值的贡献量.在某些情况下，该字段可能返回零。例如，如果引用的ForecastDetail记录表示一个负面的预报调整，或者一个预报流，其数量通过调整减少为零。 |
| `forecast_detail` | forecast_detail | `VARCHAR` | Nullable | Reference
对ForecastDetail记录的引用。这将返回形成一致预测数量的特定部件、客户和预测类别组合的预测数量和日期.在使用预测覆盖或预测再平衡覆盖的情况下，只有与该覆盖相关的ForecastDetail记录在此表中被引用(即，任何其他有助于在
ConsensusForecast。CalculatedQuantity在本表中被忽略)。 |
| `part` | part | `VARCHAR` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多渠道加权共识预测与多维分解
* **因果流向**：`ipc_consensus_forecast_detail` 记录了 S&OP (销售与运营计划) 决策中，由销售预测 (Sales Forecast)、市场预测 (Marketing Forecast)、财务目标 (Financial Budget) 以及系统统计预测 (Statistical Forecast) 经过加权平均或人工覆盖调整后的最终共识预测 (Consensus Forecast) 的分解明细。
* **算法编排逻辑**：
  1. 加权合并：根据各渠道的权重矩阵，计算初始时间桶共识数量：
     $$ Q_{consensus}(t) = \sum_{c \in Channels} Weight_c \times Q_{forecast\_detail}(c, t) $$
  2. 差异对账：对比销售数量与财务营收目标，生成差异 (Gap) 报警，支持决策者输入 override（覆盖量）。
  3. 比例分解 (Disaggregation)：若共识预测在产品系列/客户组层面输入，引擎调用分解速率（Disaggregation Rate），按历史销售比例或物料-客户分配比例，自顶向下（Top-Down）将需求拆分至具体 SKU-Site 级别的天级明细，并记录此表。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存计算中，共识预测细节被表示为一维连续的 SoA (Structure of Arrays) 数组，以提高 CPU 对时序预测指标进行加权累加计算时的向量化 (AVX-512) 效率：
```cpp
// 对应 ipc_consensus_forecast_detail 的内存对齐物理结构体
struct ConsensusForecastDetailRecord {
    uint32_t part_id;            // 物料 SKU ID (对应 part)
    uint32_t customer_id;        // 客户 ID
    int day_bucket;              // 计划相对天数 (基于 RunDate)
    double stat_qty;             // 统计预测贡献量
    double sales_qty;            // 销售预测贡献量
    double marketing_qty;        // 市场预测贡献量
    double final_qty;            // 最终共识预测量 (对应 eff_qty)
    double unit_price;           // 预测对应的有效单价
    uint32_t scenario_id;        // 多沙箱隔离场景 ID
};
```

###### 3. 边界与异常处理
* **权重不归一化处理**：若录入的渠道权重之和 $\sum Weight \neq 1.0$，引擎执行重归一化：
  $$ Normalized\_Weight_i = \frac{Weight_i}{\sum Weight} $$
* **无效或超限 Override 拦截**：如果录入的手工覆盖量为负数，系统自动将其截断为零，并产生警告日志。