---
table_name: "ipc_disaggregation_parameters_by_material_customer"
alias: "disaggregation_parameters_by_material_customer"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcDisaggregationParametersByMaterialCustomerRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_disaggregation_parameters_by_material_customer` (disaggregation_parameters_by_material_customer)

> **业务说明**: 此表保存物料客户和预测类别级别的参数值，用于在确定分解率时覆盖由SOPAnalyticsConfiguration表设置的默认参数。
对于特定的Category, 其应用于所有相关MaterialCustomer.例如，与给定预测类别相关联的所有零件客户可能具有基于相同历史需求类别并使用相同历史数据范围的分解率。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `actual_category` | actual_category | `VARCHAR` | Nullable | Reference:HisDemandCategory |
| `header` | header | `VARCHAR` | PK / NOT NULL | Reference:HisDemandHeader |
| `his_interval_count` | his_interval_count | `VARCHAR` | Nullable | 在计算此部分客户和预测类别组合的分解率时要使用的历史数据的周期数。InnerCalendar用于表示间隔时间。 |
| `inner_calendar` | inner_calendar | `VARCHAR` | Nullable | 内部分解日历，用于预测具有季节性趋势的分解，以定义一个季节的长度。例如，对于季节按月按年分解，这将被设置为每月日历。具有较高数值的月份将收到更多被分解的数据。如果预测分解不是季节性的，则应将其设置为与OuterCalendar相同的值。 |
| `outer_calendar` | outer_calendar | `VARCHAR` | Nullable | 在预测分解中使用的外部分解日历，用于定义预测分解的期间。例如，使用季节按月按年分解，这将被设置为年度日历。方法引用的日历不能表示比所引用的日历更小的时间间隔
InnerCalendar字段(但是，如果分解不是季节性的，它可以是相同的日历)。OuterCalendar标记也应该总是直接落在InnerCalendar标记上。例如，使用按月分解的方式，则显示年度日历标记 |
| `category` | category | `VARCHAR` | PK / NOT NULL | Reference:HisDemandCategory |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：物料客户级别预测分解参数表。定义 S&OP 主需求计划向底层 SKU 站点分解时的权重覆盖参数。
* **计算逻辑编排**：
  1. 特殊覆盖判定：当 S&OP 自顶向下分解时，检索该表是否存在特定 SKU-Customer 配置；2. 历史区间销量统计：读取 his_interval_count 内的历史出货，计算细分占比；3. 季节性分摊：结合内部分解日历（InnerCalendar）与外部分解日历（OuterCalendar）进行趋势平滑，计算时序分摊系数。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_disaggregation_parameters_by_material_customer 的 C++ DOD 物理对齐结构体
struct IpcDisaggregationParametersByMaterialCustomerRecord {
    std::string actual_category; // actual_category 字符串 (Reference:HisDemandCategory)
    std::string header; // header 字符串 (Reference:HisDemandHeader)
    std::string his_interval_count; // his_interval_count 字符串 (在计算此部分客户和预测类别组合的分解率时要使用的历史数据的周期数。InnerCalendar用于表示间隔时间。)
    std::string inner_calendar; // inner_calendar 字符串 (内部分解日历，用于预测具有季节性趋势的分解，以定义一个季节的长度。例如，对于季节按月按年分解，这将被设置为每月日历。具有较高数值的月份将收到更多被分解的数据。如果预测分解不是季节性的，则应将其设置为与OuterCalendar相同的值。)
    std::string category; // category 字符串 (Reference:HisDemandCategory)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。