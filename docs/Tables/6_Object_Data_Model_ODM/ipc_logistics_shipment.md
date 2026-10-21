---
table_name: "ipc_logistics_shipment"
alias: "shipment"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcLogisticsShipmentRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_shipment` (shipment)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `supplier` | supplier | `VARCHAR(40)` | Nullable | 供应商编码
Reference Table: Supplier |
| `carrier` | carrier | `VARCHAR(10)` | Nullable | 承运商
Reference Table: Carrier |
| `shipment` | shipment | `VARCHAR(10)` | Nullable | Shipment单号 |
| `ship_date` | ship_date | `DATE` | Nullable | 发货日期, 其对应SR中的ActralShipDate |
| `airport` | airport | `VARCHAR` | Nullable | Reference Table: LogisticLocation |
| `port` | port | `VARCHAR(10)` | Nullable | Reference Table: LogisticLocation |
| `location` | location | `VARCHAR(10)` | Nullable | 具体的物理库位编码 |
| `delivery_date` | delivery_date | `DATE` | Nullable | 预计到达日期 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：物理发运记录表。记录实际运输载具的起运、在途与预计到达状态，控制 ATP 水位在发运时刻的正式消纳扣减。
* **计算逻辑编排**：
  1. 实绩扣减：起运状态触发后，系统扣减对应站点的 physical inventory，并将 assigned supply 状态由 'CTB' (在制) 跃迁为 'Shipped' (已发运)；2. 在途时间修正：利用在途位置和实际路线，动态更新 CTP 中的预计到货期（ETA）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_logistics_shipment 的 C++ DOD 物理对齐结构体
struct IpcLogisticsShipmentRecord {
    std::string shipment; // shipment 字符串 (Shipment单号)
    int ship_date = 0; // ship_date 相对计划天数 (发货日期, 其对应SR中的ActralShipDate)
    std::string airport; // airport 字符串 (Reference Table: LogisticLocation)
    std::string port; // port 字符串 (Reference Table: LogisticLocation)
    std::string location; // location 字符串 (具体的物理库位编码)
    int delivery_date = 0; // delivery_date 相对计划天数 (预计到达日期)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。