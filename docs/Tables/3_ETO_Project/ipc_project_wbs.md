---
table_name: "ipc_project_wbs"
alias: "ETO项目WBS元素任务表"
module: "3_ETO_Project"
cpp_struct: "TaskDependency"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 3_eto_project
---

# 🏷️ `ipc_project_wbs` (ETO项目WBS元素任务表)

> **业务说明**: ETO WBS 任务分解表。层级化管理工程项目任务网络拓扑（SMT、组装、测试等），支持状态 override 状态切换并与 C++ 引擎联动重新排程。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `wbs_code` | WBS编码 | `VARCHAR(100)` | 🔑 **PK / Required** | WBS任务节点的唯一层级编码 (如 WBS_001_SMT) |
| `project_code` | 项目编码 | `VARCHAR(100)` | PK / NOT NULL | 项目编码，关联的 ETO 项目ID |
| `parent_wbs_code` | 父WBS编码 | `VARCHAR(100)` | Nullable | 父 WBS 任务编码，用于建立层级任务树 |
| `wbs_level` | WBS层级 | `INTEGER` | Nullable | 任务在树中的层级深度 |
| `wbs_status` | 任务状态 | `VARCHAR(50)` | Nullable | WBS任务完成状态: ACTIVE (执行中), COMPLETED (已完工), PENDING (已挂起) |
| `description` | 描述 | `VARCHAR(200)` | Nullable | 任务节点详情说明 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：WBS 任务网络拓扑与关键路径法（CPM）
* **因果流向**：`ipc_project_wbs` 定义了项目内部复杂的任务依赖网络（SMT ➔ 组装 ➔ 测试 ➔ 发运），并且这些任务与具体的物料供应（Part）与设备产能（Work Center）强耦合。
* **CPM 排程算法编排**：
  1. 前向计算（Forward Pass）：从起点任务开始，根据工序周期 `duration` 和前置任务，计算出每个任务的最早开工时间（Early Start, ES）和最早完工时间（Early Finish, EF）：
     $$ EF = ES + duration $$
  2. 后向计算（Backward Pass）：从截止日期开始，反向推导最晚开工时间（Late Start, LS）和最晚完工时间（Late Finish, LF）：
     $$ LS = LF - duration $$
  3. 计算时浮时（Total Float）：对每个任务，计算总时差 $TF = LS - ES$。若 $TF = 0$，则该任务位于**“项目关键路径”**上，任何延迟都会导致项目完工期整体漂移。
  4. 物料齐套驱动：若任务的 `output_part_id` 对应的原材料缺料，其 `duration` 会因等待物料齐套（`part_ready_date`）而动态拉伸，直接推动关键路径变动。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎内，任务依赖网络被拉平为连续的 `ProjectTaskRecord` 数组，任务间的 Finish-to-Start (FS) 等关系被记录在紧凑的 `TaskDependency` 向量中，消除树状多叉引用的高昂指针开销：

```cpp
// 依赖关系枚举 (FS - 结束到开始, FF - 结束到结束, SS - 开始到开始, SF - 开始到结束)
enum class DependencyType : uint8_t { FS = 0, FF = 1, SS = 2, SF = 3 };

struct TaskDependency {
    uint32_t predecessor_task_id; // 前置任务的连续数组索引
    DependencyType dep_type;      // 依赖关系类型
    int lag_days = 0;             // 时间偏置/时延 (Lag Days)
};

// 关联 ipc_project_wbs 表的 C++ DOD 物理对齐结构体
struct ProjectTaskRecord {
    uint32_t task_id;                         // 任务 ID (全局逻辑 Offset)
    uint32_t project_id;                      // 所属项目 ID
    std::string task_name;                    // 任务描述
    double duration = 0.0;                    // 工期 (对应 duration)
    
    // CPM 关键路径法临时计算变量 (天数)
    int early_start = 0;
    int early_finish = 0;
    int late_start = 0;
    int late_finish = 0;
    bool is_critical_path = false;            // 是否处于关键路径上
    
    uint32_t output_part_id = uint32_t(-1);   // 本任务产出的物料 ID (关联到 Part 数组)
    std::vector<TaskDependency> dependencies; // 紧凑存储的前置任务列表
};
```