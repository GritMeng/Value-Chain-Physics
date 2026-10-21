---
table_name: "ipc_his_demand_series"
alias: "his_demand_series"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisDemandSeriesRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_series` (his_demand_series)

> **业务说明**: 该表包含每个HistoricalDemandHeader和特定的AsOfDate(需求记录生成的日期)和Sequence(在同一日期生成多个系列的情况下)的一条记录。每个历史需求系列条目对应于组成一组历史需求的点的集合(对于给定的部件、客户、类别和截止日期)。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `series` | series | `DOUBLE` | Nullable | 一个标识符，用于确保报头的唯一性。
CreationDate和Sequence的组合。 |
| `creation_date` | creation_date | `VARCHAR` | Nullable | 生成此历史需求系列的日期. |
| `header` | header | `VARCHAR` | Nullable | Reference HisDemandHeader |
| `sequence` | sequence | `VARCHAR` | Nullable | 如果在给定的CreationDate上存在多个零件、客户和需求类别组合的历史需求系列，则该字段用于标识每个系列。给定CreationDate上的第一个序列应该具有最低的Sequence值，而给定CreationDate上的最后一个序列应该具有最高的Sequence值. |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史需求序列版本控制表。支持对同一物料客户在不同 As-Of 历史截面上保存多版本历史数据以做对比。
* **计算逻辑编排**：
  1. 多版本隔离：根据 AsOfDate 和 Sequence 标识历史版本；2. 回溯对比：在评估预测模型准确性时，通过 series ID 抓取特定历史版本与今日实际进行拟合残差分析。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_series 的 C++ DOD 物理对齐结构体
struct IpcHisDemandSeriesRecord {
    std::string creation_date; // creation_date 字符串 (生成此历史需求系列的日期.)
    std::string header; // header 字符串 (Reference HisDemandHeader)
    std::string sequence; // sequence 字符串 (如果在给定的CreationDate上存在多个零件、客户和需求类别组合的历史需求系列，则该字段用于标识每个系列。给定CreationDate上的第一个序列应该具有最低的Sequence值，而给定CreationDate上的最后一个序列应该具有最高的Sequence值.)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。