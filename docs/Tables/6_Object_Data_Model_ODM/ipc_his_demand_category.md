---
table_name: "ipc_his_demand_category"
alias: "his_demand_category"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisDemandCategoryRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_category` (his_demand_category)

> **业务说明**: 销售历史类别

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `weight` | weight | `INTEGER` | Nullable | - |
| `category` | category | `VARCHAR(1)` | PK / NOT NULL | 该表用于对HisDemandActual和HisDemandSeries表中的值进行分类。它既确定预测需求的不同类别(例如，客户、销售、统计等)，也确定实际需求的不同类别(例如，装运、消费、销售点等)。 |
| `threshold` | threshold | `INTEGER` | Nullable | 阈值（百分比），配合着Desired. 如果Desired设置为High, 则低于High应该被提示。 反之，如果
Desired设置为Low，则高于此值会提示。并且Type.OperationRule设置为Target |
| `desired` | desired | `VARCHAR(1)` | Nullable | - |
| `type` | type | `VARCHAR(1)` | Nullable | Category类别，关联HisDemandCategory |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史需求分类表。区分销售实绩（Shipment）、消费实绩（Usage）、渠道订单，控制在预测时的调用权重。
* **计算逻辑编排**：
  1. 数据筛选：指示统计预测引擎加载哪些历史类别参与模型拟合；2. 阈值校准：根据 threshold 与 desired 设定历史波动的合理区间，超出则发出质量警告。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_category 的 C++ DOD 物理对齐结构体
struct IpcHisDemandCategoryRecord {
    int weight = 0; // weight 整型数值 (-)
    std::string category; // category 字符串 (该表用于对HisDemandActual和HisDemandSeries表中的值进行分类。它既确定预测需求的不同类别(例如，客户、销售、统计等)，也确定实际需求的不同类别(例如，装运、消费、销售点等)。)
    std::string desired; // desired 字符串 (-)
    std::string type; // type 字符串 (Category类别，关联HisDemandCategory)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。