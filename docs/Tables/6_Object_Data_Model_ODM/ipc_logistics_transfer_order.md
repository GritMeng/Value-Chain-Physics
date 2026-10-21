---
table_name: "ipc_logistics_transfer_order"
alias: "transfer_order"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_transfer_order` (transfer_order)

> **业务说明**: 转储单

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `transfer_order` | transfer_order | `VARCHAR(18)` | PK / NOT NULL | 转储单编号 |
| `to_part` | to_part | `VARCHAR(40)` | PK / NOT NULL | 物料编号 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `ship_date` | ship_date | `DATE` | Nullable | 发货日期 |
| `dock_to_stock_lt` | dock_to_stock_lt | `DECIMAL(18,2)` | Nullable | 门到仓位提前期 |
| `from_part` | from_part | `VARCHAR(40)` | PK / NOT NULL | 发货物料号 |
| `demision_grp` | demision_grp | `VARCHAR(10)` | Nullable | 维度 |
| `from_site` | from_site | `VARCHAR(8)` | PK / NOT NULL | 从哪个Site发出 |
| `dock_date` | dock_date | `DATE` | Nullable | 到门日期 |
| `TransitCanlendar` | transit_calendar | `VARCHAR(10)` | Nullable | 日历 |
| `unit` | unit | `VARCHAR(10)` | Nullable | 单位 |
| `to_site` | to_site | `VARCHAR(8)` | PK / NOT NULL | 接收Site |
| `to_location` | to_location | `VARCHAR(10)` | Nullable | 接收仓库 |
| `Item` | item | `DOUBLE` | PK / NOT NULL | 转储行项目 |
| `shipped_qty` | shipped_qty | `DECIMAL(18,2)` | Nullable | 发出数量 |
| `recievend_qty` | recievend_qty | `DECIMAL(18,2)` | Nullable | 接收数量 |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | TransferOrder |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：跨站点调拨订单明细表。记录两个物理厂区/仓库站点之间的在途物流和预计交付，是跨站点 CTP 递归的时空偏置节点。
* **计算逻辑编排**：
  1. 跨站点供应预占：当 Site B 缺料且 sourcing_policy 配置为向 Site A 调拨时，引擎在该表生成一条建议调拨记录（Suggested STO）；2. 提前期平移：根据 PartSite 的调拨提前期在时间轴上进行前推，在 Site A 处产生依赖需求，并在 Site B 处产生预计到货调拨供应。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_logistics_transfer_order 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcLogisticsTransferOrderRecord {
    std::string transfer_order; // transfer_order 字符串 (转储单编号)
    uint32_t to_part; // to_part 逻辑ID/映射 (物料编号)
    double request_qty = 0.0; // request_qty 数量/金额精度值 (原始请求的订货/预测数量)
    int ship_date = 0; // ship_date 相对计划天数 (发货日期)
    double dock_to_stock_lt = 0.0; // dock_to_stock_lt 数量/金额精度值 (门到仓位提前期)
    uint32_t from_part; // from_part 逻辑ID/映射 (发货物料号)
    std::string demision_grp; // demision_grp 字符串 (维度)
    uint32_t from_site; // from_site 逻辑ID/映射 (从哪个Site发出)
    int dock_date = 0; // dock_date 相对计划天数 (到门日期)
    std::string TransitCanlendar; // TransitCanlendar 字符串 (日历)
    std::string unit; // unit 字符串 (单位)
    uint32_t to_site; // to_site 逻辑ID/映射 (接收Site)
    std::string to_location; // to_location 字符串 (接收仓库)
    double Item = 0.0; // Item 数量/金额精度值 (转储行项目)
    double shipped_qty = 0.0; // shipped_qty 数量/金额精度值 (发出数量)
    double recievend_qty = 0.0; // recievend_qty 数量/金额精度值 (接收数量)
    std::string supply_type; // supply_type 字符串 (TransferOrder)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。