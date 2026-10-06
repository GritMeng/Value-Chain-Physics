---
table_name: "ipc_event_phase_header"
alias: "event_phase_header"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcEventPhaseHeaderRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_event_phase_header` (event_phase_header)

> **业务说明**: 支持运算，关联EventPhase和HisDemandHeader

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `event_phase` | event_phase | `VARCHAR` | PK / NOT NULL | Reference : EventPhase |
| `Header` | descriotion | `VARCHAR` | PK / NOT NULL | Reference: [[ipc_demand_header|HisDemandHeader]] |
| `ID` | event | `VARCHAR` | PK / NOT NULL | 唯一标识 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：事件阶段关系关联头表。将促销阶段与历史需求分类账头相绑定，确定营销活动的辐射范围。
* **计算逻辑编排**：
  1. 范围映射：将 event_phase 映射至具体的物料-客户-渠道头；2. 增量过滤器：在 S&OP 重算时，限定仅有匹配此头表的预测行项目会被施加促销增量，执行局部重计算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_event_phase_header 的 C++ DOD 物理对齐结构体
struct IpcEventPhaseHeaderRecord {
    std::string event_phase; // event_phase 字符串 (Reference : EventPhase)
    std::string Header; // Header 字符串 (Reference: HisDemandHeader)
    uint32_t ID; // ID 逻辑ID/映射 (唯一标识)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。