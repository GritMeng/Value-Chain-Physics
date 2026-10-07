---
table_name: "ipc_his_demand_actual"
alias: "his_demand_actual"
module: "6_Object_Data_Model_ODM"
cpp_struct: "HistoricalDemandActualRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_actual` (his_demand_actual)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `reciept_date` | reciept_date | `DATE` | Nullable | 客户收到货的日期 |
| `ship_date` | ship_date | `DATE` | Nullable | 发货日期 |
| `commit_date` | commit_date | `DATE` | Nullable | 承诺日期 |
| `request_date` | request_date | `DATE` | Nullable | 希望接收货物的日期 |
| `category` | category | `VARCHAR(1)` | PK / NOT NULL | - |
| `item` | item | `DOUBLE` | Nullable | 行项目 |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `order` | order | `VARCHAR(10)` | Nullable | 需求订单号 |
| `ship_group` | ship_group | `VARCHAR(1)` | Nullable | 发货组 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：历史实际销量数据导入与时序数据对齐
* **因果流向**：`ipc_his_demand_actual` 存储了过去销售出货的真实记录（Shipped Actuals）。它是统计预测模型（ARIMA, Holt-Winters）的最核心输入数据源。其精度直接决定了未来的需求基准线预测的好坏。
* **数据对齐算法编排**：
  1. 历史窗口对齐：根据 `PredictionParameters.HistoricalIntervalCount` 截取指定时长（如过去 24 个月）的销量。
  2. 时序重分桶（Re-bucketing）：由于原始出货日期是不连续的，引擎在加载时将具体的 Gregorian 日期转换为连续的时间桶相对天数，对漏配的空白日期自动补 0，形成一条等间距的时序向量，作为预测模型的标准化输入。

###### 2. 物理内存结构设计 (C++ DOD Layout)
历史出货明细数据量巨大。为了提高统计分析（ACF/PACF 计算）时的内存扫描速度，该表在内存中采用面向列的扁平化紧凑 SoA 结构：
```cpp
// 对应 ipc_his_demand_actual 的内存物理结构体
struct HistoricalDemandActualRecord {
    uint32_t part_id;             // 物料 ID (对应 part)
    uint32_t customer_id;         // 客户 ID (对应 customer)
    int day_bucket;               // 发货日期相对相对天数 (对应 date)
    double actual_qty;            // 发货数量 (对应 qty)
    double actual_unit_price;     // 历史实际销售单价
    uint32_t sales_channel_id;    // 销售渠道 ID
};
```

###### 3. 边界与异常处理
* **历史异常负销量数据过滤**：在某些情况下，ERP 系统会因为退货或入账红字录入负数的出货记录。计划引擎加载时，会自动过滤掉所有销量 $\le 0$ 的记录，避免模型训练参数出现奇异值报错崩溃。