---
table_name: "ipc_logistics_customer_destination"
alias: "cusomer_destination"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcLogisticsCustomerDestinationRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_customer_destination` (cusomer_destination)

> **业务说明**: 用于标识客户接收订单货物的具体地点。例如，该表中的某条记录可能会指明某个特定的客户仓库或门店，并附有该地点的联系信息。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `customer` | customer | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `distination` | distination | `VARCHAR` | PK / NOT NULL | 位置的唯一标识符 |
| `description` | description | `VARCHAR` | Nullable | 位置描述 |
| `address` | address | `VARCHAR` | Nullable | 地址, 我们在Control Tower中可以显示Map供应网络，所以需要非常具体准确。 |
| `contact_name` | contact_name | `VARCHAR(10)` | Nullable | 联系人 |
| `phone` | phone | `VARCHAR(10)` | Nullable | 电话 |
| `email` | email | `VARCHAR(10)` | Nullable | 电邮 |
| `we_chart` | we_chart | `VARCHAR(10)` | Nullable | 微信 |
| `delivery_route` | delivery_route | `VARCHAR(10)` | Nullable | 跟DeliveryRoute一对多关联 |
| `acception_calendar` | acception_calendar | `VARCHAR(10)` | Nullable | 客户的收货地工作日历
Reference Table : Calendar |
| `dock_to_stock_lt` | dock_to_stock_lt | `DECIMAL(18,2)` | Nullable | 接收货物的处理时间 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：客户收货目的地配置表。存储客户仓库及门店的物理位置、收货日历、以及物理到货至入库的处理时间（Dock-to-Stock）。
* **计算逻辑编排**：
  1. 拼单组发：在独立需求确认时，根据 customer 关联 destination，将发运订单绑定至同一交付日历；2. 交期推导：计算要求到货期（Due Day）对应的开车发运期（Ship Day）：\n      $$ Ship\_Day = Due\_Day - Transit\_LT - dock\_to\_stock\_lt $$\n

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_logistics_customer_destination 的 C++ DOD 物理对齐结构体
struct IpcLogisticsCustomerDestinationRecord {
    uint32_t customer; // customer 逻辑ID/映射 (唯一标识符)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
    std::string distination; // distination 字符串 (位置的唯一标识符)
    std::string description; // description 字符串 (位置描述)
    std::string address; // address 字符串 (地址, 我们在Control Tower中可以显示Map供应网络，所以需要非常具体准确。)
    std::string contact_name; // contact_name 字符串 (联系人)
    std::string phone; // phone 字符串 (电话)
    std::string email; // email 字符串 (电邮)
    std::string we_chart; // we_chart 字符串 (微信)
    std::string delivery_route; // delivery_route 字符串 (跟DeliveryRoute一对多关联)
    double dock_to_stock_lt = 0.0; // dock_to_stock_lt 数量/金额精度值 (接收货物的处理时间)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。