---
table_name: "ipc_operation_sequence_type"
alias: "operation_sequence_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcOperationSequenceTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_operation_sequence_type` (operation_sequence_type)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 Reference Tablle: ControlGroup |
| `os_type` | os_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | Ingnor - 忽略
Parallel - 并行的
Standard - 标准的 |
| `alignment_rule` | alignment_rule | `VARCHAR` | Nullable | 当并行序列中的操作持续时间与标准序列中分支点之间的间隔时间不相同时，此设置决定了这些序列应如何对齐：
earliest - 并行序列会在早期进行比对。例如，如果并行序列的持续时间短于标准序列中分支操作与返回操作之间的时间间隔，那么该并行序列将与分支操作的起始点对齐（在末端假定有一个浮动值）
lastest - 并行序列尽可能晚地对齐。例如，如果并行序列的持续时间短于标准序列中的分支和返回操作之间的时间，则并行序列与返回操作的结束保持一致
（在开始时假定为float） |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：工序顺序关系类型表。定义 FS, SS, FF, SF 四类逻辑关系的解析规则。
* **计算逻辑编排**：
  1. 求解器语法映射：在将工艺路线编译为内存 DAG 图时，将 sequence_type 映射为 C++ 求解器的松弛约束判定分支，确保顺排与倒排计算的合法性。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_operation_sequence_type 的 C++ DOD 物理对齐结构体
struct IpcOperationSequenceTypeRecord {
    std::string control_class; // control_class 字符串 (控制组 Reference Tablle: ControlGroup)
    std::string os_type; // os_type 字符串 (唯一标识)
    std::string description; // description 字符串 (描述)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。