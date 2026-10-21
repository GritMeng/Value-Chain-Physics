---
table_name: "ipc_hierarchy_product_family"
alias: "product_family"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ProductFamilyRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_hierarchy_product_family` (product_family)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `family_num` | family_num | `VARCHAR(10)` | Nullable | 唯一标识符 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 产品系列描述 |
| `material` | material | `VARCHAR(40)` | Nullable | Set |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：产品系列级粗能力（RCCP）瓶颈对账
* **因果流向**：`ipc_hierarchy_product_family` 记录了核心产品系列大类的属性。在 S&OP 阶段，由于详细排产订单尚未生成，计划员通过该表以产品系列大类为单位，与制造工厂的粗能力（Rough Cut Capacity Planning）进行产能平衡，判断宏观供需达成的可行性。
* **粗能力计算编排**：
  1. RCCP 资源消耗折算：产品大类消耗的负荷 = 预测大类数量 $\times$ 产品系列平均工时消耗率（Capacity Bill of Material）。
  2. 瓶颈比对：与工厂额定瓶颈工时相比，显示大类负荷比率，驱动大类计划的削峰填谷。

###### 2. 物理内存结构设计 (C++ DOD Layout)
产品系列元数据在内存中以密集 SoA 向量存放：
```cpp
// 对应 ipc_hierarchy_product_family 的 C++ DOD 结构
struct ProductFamilyRecord {
    uint32_t product_family_id;   // 产品系列逻辑 ID (对应 product_family)
    uint32_t division_id;         // 所属事业部 ID
    double target_margin_percent; // 期望财务利润率下限
};
```

###### 3. 边界与异常处理
* **新加入物料自动挂载**：一旦新 SKU 在主物料表被创建并关联了产品大类，层次引擎会在运行期间自动将其加入产品系列的 RCCP 汇总计算中，无需人工手动重新维护系列映射。