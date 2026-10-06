---
table_name: "ipc_curve_parameters"
alias: "curve_parameters"
module: "7_Control_Data_Model_CDM"
cpp_struct: "CurveParametersRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_curve_parameters` (curve_parameters)

> **业务说明**: 事件管理算法在计算OperationRule设置为“Curve”的事件阶段的调整时使用此表。在该表的type字段中指定的曲线类型决定了用于定义曲线形状的公式。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `exponential_rate` | exponential_rate | `VARCHAR` | Nullable | InitialValue * ExponentialRate |
| `curve_parameters` | curve_parameters | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `imitation_rate` | imitation_rate | `VARCHAR` | Nullable | - |
| `initial_value` | initial_value | `VARCHAR` | Nullable | - |
| `innovation_rate` | innovation_rate | `VARCHAR` | Nullable | - |
| `linear_rate` | linear_rate | `VARCHAR` | Nullable | - |
| `maximum_value` | maximum_value | `VARCHAR` | Nullable | - |
| `type` | type | `VARCHAR` | Nullable | LinearCurve
ExponentialCurve
DiffusionCurve |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：基于价格弹性的非线性需求整形（Demand Shaping）
* **因果流向**：在 IBP 财务优化中，当面临产能严重过剩时，销售部门可能会通过“降价”来拉动销量。`ipc_curve_parameters` 存储了反映非线性价格弹性需求（Price Elasticity of Demand）的参数。引擎读取此参数，计算降价幅度对应的需求增量。
* **弹性计算公式**：
  $$ Q(P) = Base\_Volume \times P^{-\epsilon} $$
  其中 $\epsilon$ 为弹性系数（`elasticity_coefficient`），通过计算得出的销量增量会自动注入销售预测，拉动 MPS 生产。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应 ipc_curve_parameters 的内存结构
struct CurveParametersRecord {
    uint32_t curve_id;                // 曲线 ID (对应 curve)
    double base_coefficient;          // 基准销量系数 (对应 base)
    double elasticity_coefficient;    // 价格弹性系数 (对应 elasticity)
};
```