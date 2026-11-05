---
table_name: "ipc_operation_type"
alias: "operation_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcOperationTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_operation_type` (operation_type)

> **业务说明**: 设定执行操作时所采用的处理规则

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 Reference Tablle: ControlGroup |
| `transit_time_sequence` | transit_time_sequence | `VARCHAR(10)` | Nullable | before_opertion - 在当前Operation前工序考虑
after_operation - 在当前Operation后工序考虑 |
| `operation_type` | operation_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `ransit_time_rule` | ransit_time_rule | `VARCHAR(10)` | Nullable | Hours - 精确到小时
Days- 将“TransitTime”解释为在操作之间移动所需的工作日数量，不进行四舍五入（例如，从操作 10 的当天两小时移动到操作 20，若有一个“TransitTime Days”，则时间将移动到下一个工作日的两小时）。此规则仅适用于整数天数，若为小数，则在计算运输时间终点前将向上取整为下一个整数。
DaysRounding - 将“TransitTime”解释为在操作之间移动并从该日开始进行下一项操作所需的工作日数量（例如，如果“TransitTime”不是在一天的开始，则会向前推进 1 天，并移动到下一个工作日的开始）。此规则仅适用于整数天数，计算运输时间终点前，小数部分将向上取整为下一个整数。 |
| `eff_rule` | eff_rule | `VARCHAR(10)` | Nullable | never - 不用
always - 一直用
in_ex - 从开始日期开始直到结束日期之前
ex- 从开始日的下一个日期开始到结束日期	
in - 从开始日期到结束日期	
ex - 不包括结束日期	 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `date_rule` | date_rule | `VARCHAR(10)` | Nullable | fixed_date - 手工或者单据上的固定日期	
cal_due_date - due date	
cal_dock_date - 到门日期
cal_start_date - 动作开始执行日期	
always - 一直可用 |
| `batch_rule` | batch_rule | `VARCHAR(10)` | Nullable | ignore_zero - 前道工序有供应就可以开始
respect_zero - 整批都到了才可以开始 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：工序类别定义表。定义工序为物理加工（Run）、机器设置（Setup）、外协加工（Subcontract）或质检（QC），控制其在产能扣减时的处理规则。
* **计算逻辑编排**：
  1. 负荷分流：外协加工工序不扣减本地工作中心的机器产能，而是触发跨站点物流调拨提前期偏置；2. QC 工序：自动应用 Yield 废品率折算，拉动前序的投料放大系数。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_operation_type 的 C++ DOD 物理对齐结构体
struct IpcOperationTypeRecord {
    std::string control_class; // control_class 字符串 (控制组 Reference Tablle: ControlGroup)
    std::string operation_type; // operation_type 字符串 (唯一标识)
    std::string description; // description 字符串 (描述)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。