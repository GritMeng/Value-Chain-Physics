---
table_name: "ipc_aggregate_material_customer_type"
alias: "aggregate_material_customer_type"
module: "6_Object_Data_Model_ODM"
cpp_struct: "AggregateMaterialCustomerTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_aggregate_material_customer_type` (aggregate_material_customer_type)

> **业务说明**: 聚合级类型，用来判别是否需要Disaggregation 分解

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `operation_rule` | operation_rule | `VARCHAR(1)` | Nullable | Use - 需要分解Dissaggregation
Ignore - 不需要 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `type` | type | `VARCHAR(10)` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多维需求聚类分析与预测模型降维
* **因果流向**：`ipc_aggregate_material_customer_type` 定义了用于需求预测降维的物料-客户聚合组。通过将数万个极细颗粒度的 SKU 按类别与销售渠道聚合为少数几个“超级聚合大类”（Aggregated Groups），能极大平抑底层单个 SKU 的极度无规则随机波动，使统计预测结果更稳定。
* **聚类聚合计算**：
  - 汇总累加：将组内所有子 SKU 历史销量累加，得到聚合大类的出货历史。
  - 运行统计预测：在大类级别生成高质量预测曲线。
  - 自上而下分解：最后再利用 disaggregation 将大类预测拆回底层 SKU。

###### 2. 物理内存结构设计 (C++ DOD Layout)
聚合大类在内存中以密集 SoA 结构对齐存储：
```cpp
// 对应 ipc_aggregate_material_customer_type 的 C++ DOD 物理数据结构
struct AggregateMaterialCustomerTypeRecord {
    uint32_t aggregation_id;          // 聚合大类逻辑 ID (对应 id)
    uint32_t parent_product_family_id;// 挂载的产品大类 ID
    uint16_t customer_channel_id;     // 客户渠道分类 ID
};
```

###### 3. 边界与异常处理
* **孤立物料等比分摊**：若某些底层 SKU 无法关联到任何聚合大类，引擎默认将其作为独立个体进行单独预测，不予丢弃。