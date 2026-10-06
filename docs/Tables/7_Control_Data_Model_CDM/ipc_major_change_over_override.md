---
table_name: "ipc_major_change_over_override"
alias: "major_change_over_override"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcMajorChangeOverOverrideRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_major_change_over_override` (major_change_over_override)

> **业务说明**: 组与组之间的changeover

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `production_group` | production_group | `VARCHAR` | Nullable | - |
| `from_grp` | from_grp | `VARCHAR` | Nullable | Reference:ProductionGroup |
| `to_grp` | to_grp | `VARCHAR` | Nullable | Reference:ProductionGroup |
| `set_up` | set_up | `DECIMAL(18,2)` | Nullable | - |
| `clean_up` | clean_up | `DECIMAL(18,2)` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：大类换型时间覆盖表。当工作中心从生产 A 大类产品切换至 B 大类产品时，覆盖静态工序的 setup_time，计入大类换型惩罚时间。
* **计算逻辑编排**：
  1. 换型检测：在排产引擎对排班队列进行局部搜索（Local Search）时，检测相邻两个工单的 Product Family。若发生变化，则读取此表获取换型开销 $T_{{setup\_override}}$；2. 负荷锁死：在工作中心 capacity 上锁死对应时段，不容纳任何加工任务。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_major_change_over_override 的 C++ DOD 物理对齐结构体
struct IpcMajorChangeOverOverrideRecord {
    std::string production_group; // production_group 字符串 (-)
    std::string from_grp; // from_grp 字符串 (Reference:ProductionGroup)
    std::string to_grp; // to_grp 字符串 (Reference:ProductionGroup)
    double set_up = 0.0; // set_up 数量/金额精度值 (-)
    double clean_up = 0.0; // clean_up 数量/金额精度值 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。