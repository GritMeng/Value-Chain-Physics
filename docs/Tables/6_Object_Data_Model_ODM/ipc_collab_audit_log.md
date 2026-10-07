---
table_name: "ipc_collab_audit_log"
alias: "计划协同与数据重载审计表"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CollabAuditLogRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_collab_audit_log` (计划协同与数据重载审计表)

> **业务说明**: 完整追溯控制塔中发生的每一个 override 手工决策覆盖操作及其前后的变化数据。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `log_id` | 日志ID | `INTEGER` | PK / NOT NULL | 主键，自增日志编号 |
| `scenario_code` | 场景编码 | `VARCHAR(100)` | 🔑 **PK / Required** | 改动生效的场景ID |
| `table_name` | 表名 | `VARCHAR(100)` | Nullable | 被改动的物理表名 |
| `record_key` | 记录键 | `VARCHAR(200)` | Nullable | 改动行的关键主键标识 |
| `field_name` | 字段名 | `VARCHAR(100)` | Nullable | 修改的属性名 |
| `old_value` | 修改前原值 | `VARCHAR(1000)` | Nullable | 原数据值备份 |
| `new_value` | 修改后新值 | `VARCHAR(1000)` | Nullable | 新录入的值 |
| `modified_by` | 改动人 | `VARCHAR(100)` | Nullable | 操作账号 |
| `modified_at` | 改动时间 | `TIMESTAMP` | Nullable | 修改生效时间 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多用户协同沙箱修改追踪与冲突审查日志
* **因果流向**：在协同排产中，多名计划员并发在隔离场景（Scenarios）中修改数据。`ipc_collab_audit_log` 实时记录了每一次手工覆盖修改的历史。这在执行场景向主干 Baseline 合并（Merge）冲突时，提供逻辑时间戳对账和变更审核。
* **物理落库设计**：
  - 事务持久化：采用 DuckDB 追加写接口 `duckdb::Appender`，在事务提交（Commit）时零拷贝追加写入，最大化吞吐。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应 ipc_collab_audit_log 的 C++ 内存结构
struct CollabAuditLogRecord {
    uint64_t transaction_id;     // 事务唯一流水 ID
    uint32_t planner_id;         // 修改者 ID
    uint32_t scenario_id;        // 沙箱场景 ID
    uint32_t table_id;           // 被修改表的逻辑 ID
    uint32_t record_key_id;      // 修改记录的键 ID
    char change_type;            // 修改类型 ('I'=Insert, 'U'=Update, 'D'=Delete)
    double old_value;            // 修改前数值
    double new_value;            // 修改后数值
};
```