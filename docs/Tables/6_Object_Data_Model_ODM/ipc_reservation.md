---
table_name: "ipc_reservation"
alias: "reservation"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_reservation` (reservation)

> **业务说明**: 工单的相关需求以及手工单独的预留

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `mo_id` | mo_id | `VARCHAR(18)` | PK / NOT NULL | 工单号 |
| `Reservation` | document | `VARCHAR(18)` | PK / NOT NULL | 预留单号 |
| `Material` | part | `VARCHAR(40)` | PK / NOT NULL | 物料号 |
| `description` | description | `VARCHAR` | Nullable | 物料描述 |
| `due_date` | due_date | `DATE` | Nullable | 期望交付或就绪日期 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `issued_qty` | issued_qty | `DECIMAL(18,2)` | Nullable | 已满足数量 |
| `eff_qty` | eff_qty | `DECIMAL(18,2)` | Nullable | 还需要的数量 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `item` | item | `DOUBLE` | Nullable | - |
| `bom_id` | bom_id | `VARCHAR(10)` | Nullable | - |
| `alg` | alg | `VARCHAR(10)` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：物理预留与锁单分配表。记录高等级客户或特定项目对在库、在途供应的物理刚性强锁定（Hard Allocation），防止被后续常规 MRP 计算抢占。
* **计算逻辑编排**：
  1. 强确权预占：在 RunDate 时刻，首先扫描此表，将对应的 OnHand 或 SR 数量扣减，并与指定的销售订单 ID 建立刚性绑定；2. 隔离计算：被锁定的供应量不计入常规 MRP 的通用可用水位 $CS_t$，普通独立需求在 CTP 探路时自动对该部分锁定资源不可见。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_reservation 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcReservationRecord {
    uint32_t mo_id; // mo_id 逻辑ID/映射 (工单号)
    std::string Reservation; // Reservation 字符串 (预留单号)
    std::string Material; // Material 字符串 (物料号)
    std::string description; // description 字符串 (物料描述)
    int due_date = -1; // due_date 相对计划天数 (期望交付或就绪日期)
    std::string dimension_grp; // dimension_grp 字符串 (消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配)
    double request_qty = 0.0; // request_qty 数量/金额精度值 (原始请求的订货/预测数量)
    double issued_qty = 0.0; // issued_qty 数量/金额精度值 (已满足数量)
    double eff_qty = 0.0; // eff_qty 数量/金额精度值 (还需要的数量)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
    double item = 0.0; // item 数量/金额精度值 (-)
    uint32_t bom_id; // bom_id 逻辑ID/映射 (-)
    std::string alg; // alg 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。