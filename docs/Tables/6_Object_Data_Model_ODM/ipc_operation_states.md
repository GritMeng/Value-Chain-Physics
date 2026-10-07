---
table_name: "ipc_operation_states"
alias: "operation_states"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcOperationStatesRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_operation_states` (operation_states)

> **业务说明**: OperationState表可用于标识计划到货MO，该MO已由给定的工作中心操作在其Routing中部分完成。该表中的每条记录都引用了SR
和Operation记录，并且还包含一个CompletedQty字段，用于指示被引用的操作已经处理了多少MO。这允许
Capacity需求计划计算，以开始处理正在进行的计划收据，而不是在其Routing中的第一个操作和/或已经完成的操作/工艺.

此表还包含用于指示正在进行的操作开始特定阶段的日期和时间的字段。通常，应该提供正在进行的进度操作的当前阶段的开始日期。然而，如果定义了多个阶段的开始日期，那么使用这些阶段中最近的阶段来表示操作的正在进行的状态，并且假定较早的阶段已经完成

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `complete_qty` | complete_qty | `DECIMAL(18,2)` | Nullable | 已完成的数量 |
| `actual_finish_date` | actual_finish_date | `DATE` | Nullable | 实际完成日期 |
| `actual_finish_offset` | actual_finish_offset | `VARCHAR` | Nullable | 用的时间 |
| `actual_start_date` | actual_start_date | `DATE` | Nullable | 开始日期 |
| `actual_start_offset` | actual_start_offset | `VARCHAR` | Nullable | 开始的动作已经用的时间 |
| `actual_setup_date` | actual_setup_date | `DATE` | Nullable | Setup的日期 |
| `actual_setup_offset` | actual_setup_offset | `VARCHAR` | Nullable | Setup用的时间 |
| `actual_run_date` | actual_run_date | `DATE` | Nullable | 实际运行日期 |
| `actual_run_offset` | actual_run_offset | `VARCHAR` | Nullable | 运行用的时间 |
| `actual_tear_down_date` | actual_tear_down_date | `DATE` | Nullable | 实际日期 |
| `actual_tear_down_offset` | actual_tear_down_offset | `VARCHAR` | Nullable | 实际时间 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：工序完工状态跟踪表。实时同步车间 MES 系统的实绩完工或暂停状态，是每日重新排产（Rolling Run）的执行层物理截面快照。
* **计算逻辑编排**：
  1. 在制（WIP）清算：读取已完工工步，将已完工的产能负荷从未来排程中剔除；2. 残余工期重算：针对正在进行（Run）的工序，按剩余数量重新折算残余工期，作为滚动排程的绝对起点约束。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_operation_states 的 C++ DOD 物理对齐结构体
struct IpcOperationStatesRecord {
    double complete_qty = 0.0; // complete_qty 数量/金额精度值 (已完成的数量)
    int actual_finish_date = 0; // actual_finish_date 相对计划天数 (实际完成日期)
    std::string actual_finish_offset; // actual_finish_offset 字符串 (用的时间)
    int actual_start_date = -1; // actual_start_date 相对计划天数 (开始日期)
    std::string actual_start_offset; // actual_start_offset 字符串 (开始的动作已经用的时间)
    int actual_setup_date = 0; // actual_setup_date 相对计划天数 (Setup的日期)
    std::string actual_setup_offset; // actual_setup_offset 字符串 (Setup用的时间)
    int actual_run_date = 0; // actual_run_date 相对计划天数 (实际运行日期)
    std::string actual_run_offset; // actual_run_offset 字符串 (运行用的时间)
    int actual_tear_down_date = 0; // actual_tear_down_date 相对计划天数 (实际日期)
    std::string actual_tear_down_offset; // actual_tear_down_offset 字符串 (实际时间)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。