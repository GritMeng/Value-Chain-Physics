---
table_name: "ipc_customer"
alias: "customer"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CustomerRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_customer` (customer)

> **业务说明**: 客户是独立需求项目的消费者，可能是消费者、分销商、服务中心或工厂间订单的工厂标识符。
此表的“Site”字段是可选的，系统或数据管理员可以选择该字段是否能唯一标识表中的记录，或者在查询中是否将其忽略，以及在插入定义、对话框或数据源与映射窗口中是否显示该字段。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `customer` | customer | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符 |
| `region` | region | `VARCHAR(10)` | Nullable | 与该客户相关的地区名称。例如，这可能标识与该客户关联的销售区域。
Reference Table: Region |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `early_ship` | early_ship | `DECIMAL(18,2)` | Nullable | 可以提前发出的buckets数量 |
| `block_code` | block_code | `VARCHAR(10)` | Nullable | 不同的BlockCode对应不同的业务流程 |
| `name` | name | `VARCHAR(10)` | Nullable | 客户名字 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：基于客户信用额度控制与客户等级的需求准入控制
* **因果流向**：在 IBP 共识需求评审和订单排产中，`ipc_customer` 提供了客户的基本评级与信用约束。客户的信用级别和信用额度直接影响其需求是否能够被排入 MPS，大客户的优先级权重直接决定了其需求在全局优先级排序中的位权。
* **准入控制编排**：
  1. 信用额度校验：当销售订单流入时，引擎检查：
     $$ Credit\_Available = Credit\_Limit - Current\_Receivables $$
     若新单金额超出可用信用额，系统将订单状态置为 `CREDIT_HOLD`，排产器暂时不为其分配产能，防止坏账风险。
  2. 位权编码注入：客户等级（VVIP=Tier 1，VIP=Tier 2）在运行时会被注入到 `IndependentDemand.composite_priority` 的第 60-61 位，实现大客户需求的刚性插队与优先交付。

###### 2. 物理内存结构设计 (C++ DOD Layout)
客户元数据在内存中以只读密集 SoA 形式存放，以支持高并发的订单准入评估：
```cpp
// 对应 ipc_customer 表的 C++ DOD 内存结构体
struct CustomerRecord {
    uint32_t customer_id;       // 客户ID (对应 customer)
    uint8_t customer_tier;       // 客户层级 (1=VVIP, 2=Tier1, 3=Tier2, 4=Tier3)
    double revenue_weight;       // 营收权重系数
    double credit_limit;         // 信用额度上限
    double current_receivables;  // 当前应收账款金额
};
```

###### 3. 边界与异常处理
* **大客户特批授信豁免**：若订单被置为 `CREDIT_HOLD`，系统支持在 `ipc_sales_order_line` 级别设置 `override_credit_lock = true`。排产引擎检测到该标志后，会自动跳过信用校验，正常进行交期承诺。