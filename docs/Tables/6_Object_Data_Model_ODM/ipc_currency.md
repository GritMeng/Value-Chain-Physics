---
table_name: "ipc_currency"
alias: "currency"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CurrencyExchangeRate"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_currency` (currency)

> **业务说明**: 货币相关信息

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `currency` | currency | `DECIMAL(18,2)` | PK / NOT NULL | 货币代码 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 货币描述 |
| `default` | default | `VARCHAR(10)` | Nullable | 默认货币 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多国本币汇率对账与 IBP 财务利润核算
* **因果流向**：`ipc_currency` 存储了全球各站点的汇率折算规则。在 IBP 财务大账本（Financial Ledger）合并多国分公司的营收与采购支出时，求解器读取本表将所有不同货币单位统一折算为集团本币（如 USD），进行财务损益（P&L）核算。
* **汇率折算算法编排**：
  - 加载源货币金额 $Amt_{local}$。
  - 折算计算：$Amt_{reporting} = Amt_{local} \times exchange\_rate\_to\_base$。
  - 汇总合并：累加各区域利润贡献。

###### 2. 物理内存结构设计 (C++ DOD Layout)
汇率表在内存中被编译为一个连续的只读转换数组，支持使用 `uint16_t` 货币 ID 进行 O(1) 的向量化乘法折算：
```cpp
// 对应 ipc_currency 的内存物理结构体
struct CurrencyExchangeRate {
    uint16_t currency_id;            // 货币 ID (对应 currency 编码)
    double exchange_rate_to_base;     // 折算至集团本币的汇率
};
```

###### 3. 边界与异常处理
* **汇率缺失默认兜底**：若在财务折算时遇到未配置汇率的非常用货币，引擎强制使用汇率 $1.0$（即按等额折算），并在 IBP 报表显著位置抛出“汇率配置缺失异常”警报，防止汇总中断。