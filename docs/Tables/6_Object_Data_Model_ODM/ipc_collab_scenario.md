---
table_name: "ipc_collab_scenario"
alias: "沙盘模拟场景记录表"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_collab_scenario` (沙盘模拟场景记录表)

> **业务说明**: 记录计划员发起的所有多维模拟推演沙盘场景及其审批流转状态。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `scenario_code` | 场景唯一编码 | `VARCHAR(100)` | 🔑 **PK / Required** | 主键，场景唯一编码 |
| `scenario_name` | 场景名称 | `VARCHAR(200)` | Nullable | 场景名称与描述 |
| `created_by` | 创建人 | `VARCHAR(100)` | Nullable | 发起规划重算的账号ID |
| `created_at` | 创建时间 | `TIMESTAMP` | Nullable | 沙盘创建的时间戳 |
| `status` | 审批状态 | `VARCHAR(50)` | Nullable | Draft, Approved, Rolled-Back等 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多沙箱克隆与 3-Way Diff 合并
* **因果流向**：`ipc_collab_scenario` 记录并追踪计划员开启的 What-If 虚拟分支。
* **沙箱隔离与合并算法编排**：
  1. 零拷贝物理克隆：当计划员新建沙箱场景时，后端利用系统的写时克隆（Copy-on-Write）技术，在百微秒内将物理主库 `ipc.db` 复制为独立的 `sandbox_scenario_x.db`，实现彻底的数据隔离与多轨并行写。
  2. 并发读写路由：后端 FastAPI 维护场景连接池（Connection Pool），根据请求的场景参数，将其动态重定向至对应的沙箱物理文件上，主生产库不产生写锁。
  3. 3-Way Diff 冲突对比：当计划员申请将沙箱推送到 Master 主库时，系统执行 3-Way Diff 对比算法（比较父场景 P、子场景 C、主库最新状态 M），分类出“新增、删除、修改、编辑冲突”四种状态：
     - 若 $P = M$ 且 $C \neq P$，代表单向修改，允许推送。
     - 若 $P \neq M$ 且 $C \neq P$，代表其他计划员已更新主库，判定为编辑写冲突（Conflict!），系统熔断并调起冲突裁决面板。

###### 2. 数据库连接池路由物理结构 (Connection Routing)
```python
# 连接池路由映射逻辑说明
class DuckDBConnectionPool:
    def get_connection(self, scenario_code: str):
        # Master 只读，沙箱独占读写，彻底隔离
        if scenario_code == "MASTER":
            return duckdb.connect("ipc.db", read_only=True)
        else:
            return duckdb.connect(f"sandbox_{scenario_code}.db", read_only=False)
```