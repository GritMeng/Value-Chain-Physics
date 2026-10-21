---
table_name: "ipc_outlier_type"
alias: "outlier_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcOutlierTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_outlier_type` (outlier_type)

> **业务说明**: OutlierType表包含用于检测为统计预测和库存计划及优化配置的项目的历史数据中的异常值的规则。在生成项目的统计预测或计算项目的安全库存建议之前，该表中的规则决定了如何识别历史数据中的异常值以及如何调整检测到的异常值。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `threshold_rule` | threshold_rule | `VARCHAR(10)` | Nullable | 如果检测到异常值高于上限阈值，则此规则指定为减少历史实际数量而进行的调整类型。所选选项确定用于替代离群数量的值。
有效值为:
Forecast - 仅用于统计预测，项目的计算值来自StatisticalForecastOutlier。一般使用预报。
Forecast的计算取决于该表中的datarrule设置，如下所示:
如果使用“MovingAverageError”设置，则使用离群点处的移动平均线。如果使用可选的“RstlError”设置，则使用趋势和季节分量的总和。如果使用“Historical”设置，则Forecast返回-1，而使用平均值。
Ignore - 不需要调整.
Mean - 使用平均值
Median - 使用中位值
SmoothKeepExcess - 向前然后向后平滑后保留.
SmoothKeepExcess - 向前然后向后平滑后超过阈值部分移除.
Threshold - 使用阈值. |
| `data_rule` | data_rule | `VARCHAR(10)` | Nullable | 确定用于项的离群值检测的特定数据的规则。
有效值为:
Historical - 实际和未改变的历史数据点用于计算异常值.
MovingAverrageError - 使用实际历史数据点和移动平均线之间的差异。
移动平均是根据predicatitemparameters表上的OutlierMovingAverageWindow字段(用于统计预测)和SafetyStockItem表上的MovingAverageWindow字段(用于库存计划和优化)设置的多个间隔来计算的 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `detection_rule` | detection_rule | `VARCHAR` | Nullable | IglewiczHoaglinMethod
StandardDeviation
Winsorizing  |
| `type` | type | `VARCHAR(10)` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：离群值类别定义表。定义销量异常的归因类型（如大促、断料、市场冲击），控制是否保留在安全库存计算中。
* **计算逻辑编排**：
  1. 异常归类分流：加载异常分类标签；2. 运营规则控制：如果是 'Stockout' (断料)，在安全库存计算中将其作为额外需求风险予以计入；如果是 'Promo' (大促)，在 Holt-Winters 训练中将其剥离。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_outlier_type 的 C++ DOD 物理对齐结构体
struct IpcOutlierTypeRecord {
    std::string descriotion; // descriotion 字符串 (描述)
    std::string type; // type 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。