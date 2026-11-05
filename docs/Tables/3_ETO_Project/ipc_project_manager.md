---
table_name: "ipc_project_manager"
alias: "project_manager"
module: "3_ETO_Project"
cpp_struct: "ProjectManagerRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 3_eto_project
---

# 🏷️ `ipc_project_manager` (project_manager)

> **业务说明**: 项目经理

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `manager` | manager | `VARCHAR` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：ETO 项目经理工时负荷与效能核算
* **因果流向**：`ipc_project_manager` 记录了 ETO 模式下各工程项目经理（Project Manager）的主数据和当前项目负荷。计划引擎在平拉 WBS 任务网络时，读取该配置以评估项目经理在复杂工艺路线设计和首样确认阶段的瓶颈约束。
* **负荷分配逻辑**：
  1. 负荷累加：统计当前经理名下所有 Active 状态项目的 WBS 任务分配总工时。
  2. 预警提示：若经理的 `active_projects_load` 超过额定极限（例如同时带 5 个大项目），系统会向协同看板（App Cockpit）抛出超载警告，提示在计划订单指派时重新分配协调人。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 中，项目经理负荷以简单的扁平记录数组存放，用于多项目动态平衡调度：
```cpp
// 对应 ipc_project_manager 的 C++ DOD 结构体
struct ProjectManagerRecord {
    uint32_t manager_id;         // 项目经理 ID (对应 manager)
    uint32_t department_id;      // 所属工程部门 ID
    double active_projects_load; // 当前活跃项目总负荷百分比
};
```

###### 3. 边界与异常处理
* **离职或休假代理机制**：若项目经理被设为不可用状态，引擎在重新计算项目关键路径（CPM）时，会自动将未开工项目的指派路由至其指定的代理人，防止技术签批环节卡死计划流。