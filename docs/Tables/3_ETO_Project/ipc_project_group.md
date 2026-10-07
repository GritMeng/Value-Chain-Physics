---
table_name: "ipc_project_group"
alias: "project_group"
module: "3_ETO_Project"
cpp_struct: "ProjectGroupRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 3_eto_project
---

# 🏷️ `ipc_project_group` (project_group)

> **业务说明**: 项目组

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `grp` | grp | `VARCHAR` | PK / NOT NULL | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：ETO 项目群多组织资源统筹与预算控制
* **因果流向**：`ipc_project_group` 实现了对多个关联工程项目（如大型设备交钥匙工程的不同子模块）的聚合管理。项目群经理可以通过该配置对下属子项目的 WBS 任务进行统一的能力拉平（Capacity Leveling）与合并采购，并在项目群层面设定整体迟交惩罚上限。
* **计算逻辑编排**：
  1. 产能统筹：引擎在进行关键路径法（CPM）迭代时，计算项目群内共享瓶颈资源（如调试车间、资深工程师工时）的累计负荷，若超载则在群内部执行优先级排序并平移任务。
  2. 惩罚上限约束（Cap Control）：
     $$ Group\_Penalty_{actual} = \min\left( Total\_Penalty\_Cap, \sum_{p \in Group} Project\_Penalty_p \right) $$

###### 2. 物理内存结构设计 (C++ DOD Layout)
项目群在内存中由紧凑的配置结构表示，供 CPM 分析引擎在图遍历时进行快速汇总：
```cpp
// 对应 ipc_project_group 的 C++ DOD 结构体
struct ProjectGroupRecord {
    uint32_t project_group_id;  // 项目群逻辑ID (对应 project_group)
    double group_budget;        // 项目群总预算
    double total_penalty_cap;   // 累计惩罚金额上限 (对应罚金上限)
    bool is_priority_group;     // 是否具有优先调度权
};
```

###### 3. 边界与异常处理
* **群成员循环依赖拦截**：如果项目群内子项目 A 的任务依赖子项目 B，而 B 又反向依赖 A，会造成 CPM 求解死锁。引擎在编译 WBS 有向无环图（DAG）时，会自动扫描跨项目的外部依赖链条，一旦检测到有向环，强制阻断并抛出拓扑死锁报警。