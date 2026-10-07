---
table_name: "ipc_supply_sort_policy"
alias: "supply_sort_policy"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSupplySortPolicyRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_supply_sort_policy` (supply_sort_policy)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `supply_policy_num` | supply_policy_num | `VARCHAR(10)` | Nullable | SupplyPolicy的唯一标识符 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `sequence` | sequence | `INTEGER` | Nullable | 排序编号，编号越小优先级越高 |
| `material` | material | `VARCHAR(40)` | Nullable | Set |
| `material_type` | material_type | `VARCHAR(10)` | Nullable | Set |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table: ControlGroup |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：供应消纳排序策略表。配置在 MRP 冲抵和 CTP 匹配过程中，供应资源（在手、在途、新工单）的消耗先后次序规则。
* **计算逻辑编排**：
  1. FIFO 消纳：默认按 available_date 升序消耗供应，保证交期最早者优先被分配；2. 货龄/呆滞优化：对在手库存进行 ABC 等级及货龄排序，优先消纳呆滞件，防止产生库龄失效。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_supply_sort_policy 的 C++ DOD 物理对齐结构体
struct IpcSupplySortPolicyRecord {
    std::string supply_policy_num; // supply_policy_num 字符串 (SupplyPolicy的唯一标识符)
    std::string descriotion; // descriotion 字符串 (描述)
    int sequence = 0; // sequence 整型数值 (排序编号，编号越小优先级越高)
    std::string material; // material 字符串 (Set)
    std::string material_type; // material_type 字符串 (Set)
    std::string control_class; // control_class 字符串 (Reference Table: ControlGroup)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。