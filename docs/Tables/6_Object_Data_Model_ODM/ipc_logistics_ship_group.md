---
table_name: "ipc_logistics_ship_group"
alias: "ship_grp"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcLogisticsShipGroupRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_ship_group` (ship_grp)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `ship_group_number` | ship_group_number | `VARCHAR(10)` | Nullable | 此发运组的唯一标识符 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `demand_line_item` | demand_line_item | `VARCHAR(10)` | Nullable | 关联独立需求 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：发运组配置表。将同一客户或区域的多笔独立需求划入发运组，以实现满载运输（FTL）拼单，控制集拼周期。
* **计算逻辑编排**：
  1. 订单聚合：在计划期内，扫描同一 ship_group_number 下的未交货行项目；2. FTL/LTL 判定：计算聚合后的总体积与总重量，匹配满载规格，决定物流费率折算系数，计入物流成本总账。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_logistics_ship_group 的 C++ DOD 物理对齐结构体
struct IpcLogisticsShipGroupRecord {
    std::string ship_group_number; // ship_group_number 字符串 (此发运组的唯一标识符)
    std::string descriotion; // descriotion 字符串 (描述)
    std::string demand_line_item; // demand_line_item 字符串 (关联独立需求)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。