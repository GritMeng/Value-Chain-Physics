---
table_name: "ipc_predict_outlier_parameters"
alias: "predict_outlier_parameters"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcPredictOutlierParametersRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_predict_outlier_parameters` (predict_outlier_parameters)

> **业务说明**: 它在预测项目级别存储异常值调整，这意味着属于多个预测类别的项目
PredictionParameters可以对每个类别使用不同的异常值调整。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `DATE` | Nullable | - |
| `forecast_item` | forecast_item | `VARCHAR(10)` | Nullable | - |
| `value` | value | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `operation_rule` | operation_rule | `VARCHAR` | Nullable | 指定何时应使用在此记录上定义的离群值调整。
All - Outlier和Statistical Forecast都用
Statistical Forecast - 只统计预测 |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：离群值过滤参数表。配置统计预测引擎在清洗历史数据时，识别和剔除异常销量波动的控制阈值。
* **计算逻辑编排**：
  1. 离群判定：设定滑动窗口大小及标准差倍数（如 3-Sigma 原则）；2. 替换计算：对判定为 Outlier 的销量，根据参数选择归零、用滑动中位数替换，或保留原始值；3. 数据清洗链：将清洗后数据输出为干净的历史销量流（Cleaned Sales Series）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_predict_outlier_parameters 的 C++ DOD 物理对齐结构体
struct IpcPredictOutlierParametersRecord {
    int date = 0; // date 相对计划天数 (-)
    std::string forecast_item; // forecast_item 字符串 (-)
    std::string value; // value 字符串 (唯一标识)
    std::string qty; // qty 字符串 (数量 (Quantity))
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。