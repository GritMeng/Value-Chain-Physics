---
table_name: "ipc_alt_grp"
alias: "alt_grp"
module: "6_Object_Data_Model_ODM"
cpp_struct: "AltGroupRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_alt_grp` (alt_grp)

> **业务说明**: 同一组的物料为替换关系

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `alt_grp` | alt_grp | `VARCHAR(10)` | PK / NOT NULL | 替代组编码，相同替代组内的组件物料属于可替换物料 |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组
Reference Table: ControlGroup |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `alt_grp_type` | alt_grp_type | `VARCHAR(10)` | Nullable | 定义与此替换关联的处理规则。例如：是否允许混料等。
Reference Table: alt_grp_type |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多站点零部件替代组网格配置
* **因果流向**：`ipc_alt_grp` 定义了哪些组件在一个替代组中，是物料替代逻辑的主索引表。BOM 分解器在解析主要物料（Primary Component）时，若检测到缺料，会通过该表索引找到对应的替代组及包含的子物料列表，进入替代抉择逻辑。
* **匹配算法编排**：
  - 站点隔离：替代规则绑定到具体 `site`。工厂 A 的替代关系在工厂 B 不生效。
  - 规则链接：通过 `alt_grp_type` 字段关联具体的混料与分配优先级策略。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，为了避免树状跳表检索导致的 Cache 命中率下降，替代组在内存中以紧凑的逻辑 ID 分布：
```cpp
// 对应 ipc_alt_grp 表的 C++ 物理结构体
struct AltGroupRecord {
    uint32_t alt_group_id;    // 替代组逻辑逻辑ID (由字符串哈希化)
    uint32_t site_id;         // 站点ID (对应 site)
    uint32_t grp_type_id;     // 规则类型ID (指向 AltGrpTypeRecord)
};
```

###### 3. 边界与异常处理
* **空替代组拦截**：若配置了替代组但组内没有包含任何备选物料，预编译器会进行静态校验拦截，并在启动时抛出报错，防止引擎在运行时浪费 CPU 进行无意义的替代料循环扫描。