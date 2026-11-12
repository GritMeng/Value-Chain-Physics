---
table_name: "ipc_logistics_transit_status"
alias: "transit_status"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcLogisticsTransitStatusRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_logistics_transit_status` (transit_status)

> **业务说明**: 转储状态

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 |
| `id` | id | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | 控制是否参与运算 
Y-参与
N不参与 |
| `status` | status | `VARCHAR(10)` | PK / NOT NULL | 转储状态。
Intransit -在途
Recieved - 已收货
NotIssued - 还未发货 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：物流在途状态定义表。定义物流节点的状态（如装车、海关清关、转运中），控制供应在求解器中的可用置信度。
* **计算逻辑编排**：
  1. 供应置信度修正：如果状态为 'In-Customs' (清关中)，系统自动在 due_day 上额外追加 2 天的安全偏差时间，防止清关滞纳导致缺料；2. 控制塔呈现：在看板端关联物流追踪节点。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_logistics_transit_status 的 C++ DOD 物理对齐结构体
struct IpcLogisticsTransitStatusRecord {
    std::string control_class; // control_class 字符串 (控制组)
    uint32_t id; // id 逻辑ID/映射 (唯一标识)
    std::string descriotion; // descriotion 字符串 (描述)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。