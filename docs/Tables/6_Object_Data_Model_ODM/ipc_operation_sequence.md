---
table_name: "ipc_operation_sequence"
alias: "operation_sequence"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcOperationSequenceRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_operation_sequence` (operation_sequence)

> **业务说明**: OperationSequence表用于标识给定工艺路线中的不同操作序列。每个Routing都应该有一个标准操作序列，以及一个或多个从标准序列中分离出来的并行操作。因此，该表中的每条记录都是由其Id和对Routing表的引用的组合唯一标识的。的
然后，OperationSequence表被Operation表引用，从而定义了该表中定义的每个操作所属的Routing和顺序

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `operation_sequence` | operation_sequence | `INTEGER` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `routing` | routing | `VARCHAR(10)` | Nullable | Reference Table: [[ipc_routing|Routing]]Header |
| `operation_type` | operation_type | `VARCHAR(10)` | Nullable | 控制其是否是并行
Reference Table:OperationSequenceType
对定义此操作序列的处理规则的控制设置的引用。
例如，Type.OperationRule的“Standard”值表示路由工艺的标准顺序，“Parallel”值表示并行顺序。
通常，Routing记录应该由一个标准操作序列和一个或多个并行序列引用。请注意，如果Routing没有定义标准序列，则忽略其所有并行序列。 |
| `branch` | branch | `VARCHAR(10)` | Nullable | 从哪个operation开始分支，如果为空，那么从Standard Sequence开始 |
| `return` | return | `INTEGER` | Nullable | 从哪个operation返回，如果为空从Standard结束返回。 |
| `eff_branch` | eff_branch | `VARCHAR(10)` | Nullable | 对于并行序列，这将返回来自的工艺
哪个并行序列从标准分支出来
序列。 |
| `eff_return` | eff_return | `VARCHAR(10)` | Nullable | - |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `os_type` | os_type | `VARCHAR` | Nullable | operation_sequence_type |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：工序顺序控制表。定义工艺路线内部各工步之间的先后依赖关系（如 FS 结束开始、SS 开始开始）及 Lag 延时天数。
* **计算逻辑编排**：
  1. 拓扑排程：在 DBD 详细排产时，遍历各工步顺序记录；2. 时序平移：前置工步 A 与后置工步 B 满足关系 $Start(B) \\ge Finish(A) + Lag$，若发现冲突，利用推移算法将 B 及其下游任务整体向右平移，锁定瓶颈段负荷。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_operation_sequence 的 C++ DOD 物理对齐结构体
struct IpcOperationSequenceRecord {
    int operation_sequence = 0; // operation_sequence 整型数值 (唯一标识)
    std::string description; // description 字符串 (描述)
    uint32_t routing; // routing 逻辑ID/映射 (Reference Table:RoutingHeader)
    std::string branch; // branch 字符串 (从哪个operation开始分支，如果为空，那么从Standard Sequence开始)
    int return = 0; // return 整型数值 (从哪个operation返回，如果为空从Standard结束返回。)
    std::string eff_return; // eff_return 字符串 (-)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
    std::string os_type; // os_type 字符串 (operation_sequence_type)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。