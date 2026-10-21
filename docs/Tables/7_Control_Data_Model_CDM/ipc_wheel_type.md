---
table_name: "ipc_wheel_type"
alias: "wheel_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcWheelTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_wheel_type` (wheel_type)

> **业务说明**: 定义Wheel的属性规则

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `description` | description | `VARCHAR` | Nullable | - |
| `demand_rule` | demand_rule | `VARCHAR` | Nullable | 此设置确定分配给此cycle但由于cycle已超出约束而无法构建的未满足的需求会发生什么情况.
Defer - 下个Cycle处理
Ignore - 不处理 |
| `supply_rule` | supply_rule | `VARCHAR` | Nullable | 此设置决定如何处理未使用的约束。
未用约束是指在Wheel周期内，在没有达到最小约束消耗目标的情况下，剩余约束怎么处理:
Future - 可以满足未来的需求，也就是说未来需求先来填约束
Ignore -  如果mini有剩余空闲在那 |
| `wheel_type` | wheel_type | `VARCHAR` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：排产轮盘类别定义表。定义特定的循环生产排程模式（如固定周期、柔性循环轮盘），用于高频换型的石化或半导体大宗制造场景。
* **计算逻辑编排**：
  1. 固定循环约束建立：在 DBD 调度时，限制特定的 Product Family 只能在轮盘（Wheel）指定的固定时间周期（如每周二生产 A 类，周四生产 B 类）进行排程，强制约束工单开工期在轮盘槽位（Slot）对齐，以最大化减少大类换型损失。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_wheel_type 的 C++ DOD 物理对齐结构体
struct IpcWheelTypeRecord {
    std::string description; // description 字符串 (-)
    std::string wheel_type; // wheel_type 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。