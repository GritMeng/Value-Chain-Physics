---
table_name: "ipc_buyer"
alias: "buyer"
module: "6_Object_Data_Model_ODM"
cpp_struct: "BuyerRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_buyer` (buyer)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `buyer` | buyer | `VARCHAR(10)` | PK / NOT NULL | 采购员ID |
| `name` | name | `VARCHAR(10)` | Nullable | 采购员名字 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：采购计划单（PR/PO）流转签批与额度约束
* **因果流向**：`ipc_buyer` 维护了企业采购员（Buyer）的基本主数据和单笔采购额度限制。MRP 引擎在生成推荐采购订单（Planned Purchase Order）后，会根据物料主数据将订单的 `buyer` 字段自动指派给对应的采购员，并校验采购总金额。
* **审批额度校验**：
  1. 金额累加：计算单笔计划采购订单总金额 $Value = Qty \times Cost$。
  2. 额度比对：若 $Value > spend\_limit\_per\_po$，该订单自动进入“待审批（Pending Approval）”状态，阻断其向 ERP 下传的自动释放（Auto-Release）通路。

###### 2. 物理内存结构设计 (C++ DOD Layout)
采购员主数据在内存中以 SoA 结构对齐存储，主要用于采购拉动阶段的高速指派：
```cpp
// 对应 ipc_buyer 表的 C++ DOD 物理数据结构
struct BuyerRecord {
    uint32_t buyer_id;           // 采购员 ID (对应 buyer)
    uint32_t department_id;      // 所属部门 ID (对应 department)
    double spend_limit_per_po;   // 单笔采购工单限额上限
    bool is_active;              // 是否活跃
};
```

###### 3. 边界与异常处理
* **人员离职自动指派重定向**：若某采购员状态被设为 `is_active = false`（如离职），引擎会自动将该员名下的物料采购单重定向指派给该部门下的默认备份采购员，防止采购流程停滞。