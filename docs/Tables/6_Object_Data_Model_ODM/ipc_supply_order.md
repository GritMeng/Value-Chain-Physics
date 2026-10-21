---
table_name: "ipc_supply_order"
alias: "supply_order"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcSupplyOrderRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_supply_order` (supply_order)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `sr_id` | sr_id | `VARCHAR(18)` | PK / NOT NULL | 供应订单编号，如采购订单和工单。
Reference Table: SupplyOrder |
| `to_site` | to_site | `VARCHAR(8)` | Nullable | 接收Site |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 标识其是make, purchase, transfer |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：供应订单定义主表。存储所有在途采购订单（PO）、生产工单（WO）及调拨单的抬头和状态，是 MRP 算账的资产凭证根表。
* **计算逻辑编排**：
  1. 数据同步与状态校验：从 ERP 批量加载订单抬头，验证 order_status；2. 履约链溯源：作为 `ipc_supply_assignment` 的父项，连接底层采购实绩与顶层客户订单，支撑供应控制塔进行订单交付可靠性评级。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_supply_order 的 C++ DOD 物理对齐结构体
struct IpcSupplyOrderRecord {
    uint32_t to_site; // to_site 逻辑ID/映射 (接收Site)
    std::string supply_type; // supply_type 字符串 (标识其是make, purchase, transfer)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。