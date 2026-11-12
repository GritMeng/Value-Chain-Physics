---
table_name: "ipc_part_customer"
alias: "part_customer"
module: "6_Object_Data_Model_ODM"
cpp_struct: "PartCustomerConfig"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_part_customer` (part_customer)

> **业务说明**: 此表标识历史数据中提供的每个唯一部件和客户组合，然后用于预测生成。它还包含某些属性(例如，优先级)，这些属性将应用于所有预测记录，这些记录是由对给定部件和客户组合的一致预测生成的。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `forecast_item` | forecast_item | `VARCHAR(1)` | Nullable | - |
| `demand_type` | demand_type | `VARCHAR` | Nullable | - |
| `customer` | customer | `VARCHAR(10)` | PK / NOT NULL | - |
| `cus_site` | cus_site | `VARCHAR(8)` | Nullable | 客户是一个需求项目的消费者，可能是消费者、分销商、服务中心或工厂间订单的工厂标识符。该表的Site字段是可选的，系统或数据管理员可以选择它是唯一标识表中的记录，还是在查询中忽略它，不显示在插入定义、对话框或数据源和映射窗口中。 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `order_priority` | order_priority | `VARCHAR` | Nullable | - |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：客户专属分配渠道匹配
* **因果流向**：`ipc_part_customer` 用于建立物料 SKU 与特定客户之间的合约绑定。
* **准入编排逻辑**：当独立需求进入 CTP 准排产链时，引擎通过读取此表确立客户特权，将 `order_priority`（订单优先级覆盖）与该客户对当前物料的平均销售价格（UnitPrice）动态绑定，重新编码为 `composite_priority`。在物料紧缺时，优先将有限库存分配给核心合约客户。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，客户关联的特权和优先度以连续数组或逻辑二分映射（Map）存在，支持快速检索：
```cpp
struct PartCustomerConfig {
    uint32_t part_id;
    uint32_t customer_id;
    uint32_t priority_override; // 覆盖优先级
    double contract_price;      // 合约单价
};
```