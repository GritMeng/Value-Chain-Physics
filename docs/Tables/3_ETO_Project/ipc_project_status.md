---
table_name: "ipc_project_status"
alias: "project_status"
module: "3_ETO_Project"
cpp_struct: "ProjectStatusRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 3_eto_project
---

# 🏷️ `ipc_project_status` (project_status)

> **业务说明**: ProjectStatus表定义了可以分配给项目的状态值。这些值表示项目的完成程度，并可用于筛选到感兴趣的特定项目。
例如，项目可能被标识为计划的、开放的、延迟的或完成的

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `status` | status | `VARCHAR` | PK / NOT NULL | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：ETO 项目生命周期状态机控制与动态产能释放
* **因果流向**：`ipc_project_status` 驱动了 ETO 项目状态转换的业务流。当项目状态由 `Planned` 切换为 `Active` 时，系统将预留的虚拟产能（Soft Allocation）转化为物理工单占用（Hard Allocation）；若状态被置为 `On-Hold`（挂起），计划引擎会在下一次 LBL-MRP 循环中自动释放该项目所有未开工 WBS 任务占用的机器与人工负荷，为其他活跃项目腾出空间。
* **状态机转换编排**：
  - `Initiated` -> `Planned`：只计算关键路径与粗估 BOM 需求，不产生具体 MRP 生产订单。
  - `Active`：产生正式 CTP 承诺，下达工单。
  - `On-Hold`：冻结已开工任务，取消未开工任务的资源占用。

###### 2. 物理内存结构设计 (C++ DOD Layout)
项目状态作为核心控制标量，在内存中直接存放在 `ProjectTaskRecord` 头部，优化了状态机跃迁判定时的内存读取：
```cpp
// 项目生命周期状态枚举
enum class ProjectState : uint8_t {
    INITIATED = 0,
    PLANNED = 1,
    ACTIVE = 2,
    ON_HOLD = 3,
    COMPLETED = 4
};

// 对应 ipc_project_status 的内存物理结构
struct ProjectStatusRecord {
    uint32_t project_id;         // 项目ID (对应 project)
    ProjectState current_state;  // 当前状态 (对应 project_status)
    int state_change_day;        // 状态变更相对计划天数
};
```

###### 3. 边界与异常处理
* **已发料任务挂起保护**：若项目被置为 `On-Hold`，但其下属 WBS 任务对应的生产工单已在车间发料并开工（WIP 状态），引擎将强制保留该工单的产能与库存占用，只挂起尚未发料的未来任务，防止车间产生在制半成品积压呆滞。