---
table_name: "ipc_forecast_causal_factor_category"
alias: "causal_factor_category"
module: "4_IBP_Consolidated"
cpp_struct: "CausalFactorCategoryRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_causal_factor_category` (causal_factor_category)

> **业务说明**: 此表存储在您的公司中定义的用于分组因果因素的类别。例如，因果因素类别可能包括促销和天气事件等内容。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `category` | category | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `operation_rule` | operation_rule | `VARCHAR` | Nullable | 表示与因果因素类别相关的因果因素细节是否应包括在S&OP算法计算、库存计划和优化(安全库存)计算中，或两者兼而有之。有效值为:
SOP - 表明因果因素的细节只应包括在S&OP计算中。例如，在计算统计预测或分解预测时将使用它们.
SafetyStock - 指示因果因素的详细信息应仅包括在安全库存计算中。
当在部件级别创建离群值调整时使用此值。
All - 都用. |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：因果因素类别归类与双计算引擎应用域分流
* **因果流向**：`ipc_forecast_causal_factor_category` 用于定义促销、价格变动等因果因素（Causal Factors）的生效边界。此表的核心是 `operation_rule`，它决定了因果细节是只应用在 S&OP（销售预测调整）上，还是只应用在安全库存（方差计算）上，亦或是两者兼具。
* **双引擎分流编排**：
  - 若设置为 `SOP`：促销增量只叠加在 `ipc_forecast_detail`，供 MRP/MPS 排程拉动；不改变日期的方差，避免安全库存虚高。
  - 若设置为 `SafetyStock`：促销导致的异常剧烈销量被视为历史方差的一部分，增加安全库存防波堤；但在 S&OP 中不生成常规预测。
  - 若为 `All`：双引擎均加载。

###### 2. 物理内存结构设计 (C++ DOD Layout)
因果类别分流规则被编译为掩码，直接对齐存储在内存中：
```cpp
// 对应 ipc_forecast_causal_factor_category 的 C++ DOD 物理数据结构
struct CausalFactorCategoryRecord {
    uint32_t category_id;        // 类别 ID
    uint8_t scope_mask;          // 掩码: Bit 0 (SOP 生效), Bit 1 (SafetyStock 生效)
};
```

###### 3. 边界与异常处理
* **非法规则兜底**：若操作规则填写了非标字符，系统默认将其视同为 `All` 双引擎分流，以最安全、最保守的策略拉动供应。