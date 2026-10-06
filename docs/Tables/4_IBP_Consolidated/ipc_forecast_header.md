---
table_name: "ipc_forecast_header"
alias: "forecast_header"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastHeaderRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_header` (forecast_header)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `id` | id | `VARCHAR(18)` | PK / NOT NULL | 订单编号 |
| `type` | type | `VARCHAR(10)` | PK / NOT NULL | 订单类型。
Reference Table: DemandType |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：预测需求源追踪与时序生命周期配置
* **因果流向**：`ipc_forecast_header` 是预测需求表的头部定义表，指定了某一预测版本的起始与结束日期、需求来源性质及币种。引擎根据此表的配置决定是否加载特定版本的预测数据，以进行预测消纳和净需求爆破。
* **生效期控制编排**：
  - 时序判定：在排产时，如果订单到期日超出本头定义的有效时间段 $[Start\_Day, End\_Day]$，该预测明细自动失效，不参与 MRP 缺口计算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
预测头部在内存中连续存放，作为只读的过滤配置：
```cpp
// 对应 ipc_forecast_header 的 C++ DOD 物理数据结构
struct ForecastHeaderRecord {
    uint32_t forecast_id;           // 预测流 ID (对应 id)
    uint8_t forecast_source_type;   // 预测流源类型 (0=Statistical, 1=Sales, 2=Consensus)
    int active_start_day;           // 有效起始相对计划天
    int active_end_day;             // 失效结束计划天
    uint16_t currency_id;           // 结算所用本币货币 ID
};
```

###### 3. 边界与异常处理
* **真空期默认外推**：若预测需求的到期日超出 Header 定义的上限，引擎会产生只读警告日志，并默认采用最末一期的预测值进行横向外推（Flat Extrapolation），确保需求链条不会因配置缺失发生突断。