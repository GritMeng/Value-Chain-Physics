---
table_name: "ipc_speard_profile"
alias: "speard_profile"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSpeardProfileRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_speard_profile` (speard_profile)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组
Reference Table: ControlGroup |
| `id` | id | `VARCHAR(10)` | Nullable | 需求扩展配置文件的名称 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | Spread描述描述 |
| `numbers_of_weight` | numbers_of_weight | `DECIMAL(18,2)` | Nullable | 用于定义扩展函数的点数(最小为0，最大值为13) |
| `weight` | weight | `DECIMAL(18,2)` | Nullable | - |
| `demand_type` | demand_type | `VARCHAR(10)` | Nullable | 引用SpreadProfile的一组DemandType记录 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：预测平铺模式配置表。定义当用户上传大颗粒度预测（如按月、按季）时，如何在天级日历上平铺分摊预测数量（如等比例平铺、工作日平铺）。
* **计算逻辑编排**：
  1. 日级拆分计算：读取平铺模式，如果为 'WorkdayOnly'，则遍历目标月内的 `ipc_calendar_date`，仅将预测数量均匀分摊在 working_days 为 true 的天数上：\n      $$ Daily\_Qty = \\frac{{Total\_Period\_Qty}}{{Number\_of\_Working\_Days}} $$\n   2. 异常平摊：非工作日分摊数量置零，落库至 forecast_detail。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_speard_profile 的 C++ DOD 物理对齐结构体
struct IpcSpeardProfileRecord {
    uint32_t id; // id 逻辑ID/映射 (需求扩展配置文件的名称)
    std::string descriotion; // descriotion 字符串 (Spread描述描述)
    double numbers_of_weight = 0.0; // numbers_of_weight 数量/金额精度值 (用于定义扩展函数的点数(最小为0，最大值为13))
    double weight = 0.0; // weight 数量/金额精度值 (-)
    std::string demand_type; // demand_type 字符串 (引用SpreadProfile的一组DemandType记录)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。