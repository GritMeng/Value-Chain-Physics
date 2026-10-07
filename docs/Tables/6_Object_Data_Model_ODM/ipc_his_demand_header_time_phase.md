---
table_name: "ipc_his_demand_header_time_phase"
alias: "his_demand_header_time_phase"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisDemandHeaderTimePhaseRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_header_time_phase` (his_demand_header_time_phase)

> **业务说明**: 此表标识每个唯一的部件、客户和历史需求类别组合，并可用于在为特定部件客户生成一致预测时指定与预测类别关联的权重。通常，它由多组历史实际需求、历史预测和其他项目引用.
如果HisDemandHeader表包含给定预测类别和部分客户组合的记录，并且在
HisDemandHeaderTimePhasedAttributes表或HisDemandHeaderRollingWeight表，则从该表中获取类别权重

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `category` | category | `VARCHAR(1)` | PK / NOT NULL | 需求类别 |
| `weight` | weight | `INTEGER` | Nullable | 可覆盖HisDemandCatetory中的Weight |
| `eff_unit_price` | eff_unit_price | `DATE` | Nullable | - |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史需求时限权重定义表。支持设定在特定日期区间内，该物料客户的权重和价格变动细节。
* **计算逻辑编排**：
  1. 区间拦截：检测 RunDate 是否落在生效窗口内；2. 动态调整：若生效，系统将 weight 注入时序计算，重新计算该大客户的历史需求折算值。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_header_time_phase 的 C++ DOD 物理对齐结构体
struct IpcHisDemandHeaderTimePhaseRecord {
    std::string category; // category 字符串 (需求类别)
    int weight = 0; // weight 整型数值 (可覆盖HisDemandCatetory中的Weight)
    int eff_unit_price = 0; // eff_unit_price 相对计划天数 (-)
    int eff_end_date = -1; // eff_end_date 相对计划天数 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。