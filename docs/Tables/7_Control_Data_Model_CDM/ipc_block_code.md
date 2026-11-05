---
table_name: "ipc_block_code"
alias: "block_code"
module: "7_Control_Data_Model_CDM"
cpp_struct: "BlockCodeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_block_code` (block_code)

> **业务说明**: 记录了分配给客户及/或个别订单项目的“暂停”代码，其目的是防止订单在订单履行流程中的某个阶段之后继续推进。例如，可能会为客户的信用问题创建一个暂停代码，这样他们的订单在问题解决之前是不允许发货的。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `BlockCode` | block_code | `VARCHAR(10)` | PK / NOT NULL | 唯一编码标识 |
| `description` | description | `VARCHAR` | Nullable | 目的的描述。（例如，信用额度检查没有通过） |
| `allow_ctp` | allow_ctp | `BOOLEAN` | Nullable | 是否驱动ctp |
| `allow_shipment` | allow_shipment | `BOOLEAN` | Nullable | 是否可以发货 |
| `allow_mrp` | allow_mrp | `VARCHAR` | Nullable | 是否驱动补货 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：物料/站点状态冻结与供需准入控制
* **因果流向**：`ipc_block_code` 维护了计划冻结码（如：品质异常冻结、出口管制冻结）。当物料或站点被指派了某个冻结码时，计划引擎在进行 MRP 时序缺口扣减时，需要决定是直接忽略该批库存，还是阻止开出生产订单。
* **准入拦截逻辑**：
  - 检索 `block_production`：若为 True，MRP 引擎无法为该物料生成 `ipc_planned_order` (计划生产单)，只能从其他工厂调拨或报错。
  - 检索 `block_shipping`：阻断调拨和发货计划。

###### 2. 物理内存结构设计 (C++ DOD Layout)
冻结码在内存中以紧凑的掩码结构存储：
```cpp
// 对应 ipc_block_code 的 C++ 内存物理结构
struct BlockCodeRecord {
    uint32_t block_code_id;      // 冻结码ID (对应 block_code)
    bool block_production;       // 是否禁止生产
    bool block_shipping;         // 是否禁止发运
    bool block_procurement;      // 是否禁止采购
    int lift_day_offset;         // 自动解冻的相对天数
};
```