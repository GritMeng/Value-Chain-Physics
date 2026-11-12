---
table_name: "ipc_abc_class"
alias: "abc_class"
module: "6_Object_Data_Model_ODM"
cpp_struct: "AbcClassRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_abc_class` (abc_class)

> **业务说明**: 此表格列出了有效的 ABC 代码，这些代码根据年度销售额或其他标准将零部件进行分类。这些代码用于识别那些影响最大的零部件，并应予以重点关注。“受控制的”
此表的“site”字段是可选的，系统或数据管理员可以决定该字段是用于唯一标识表中的记录，还是在查询中被忽略、不在插入定义、对话框或“数据源和映射”窗口中显示。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `abc` | abc | `BOOLEAN` | PK / NOT NULL | - |
| `description` | description | `VARCHAR` | Nullable | - |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：帕累托（Pareto）ABC 价值分类与安全库存服务矩阵自动映射
* **因果流向**：`ipc_abc_class` 定义了企业的物料 ABC 价值分类矩阵。计划引擎根据物料的年度销售总额（或历史出货总值）按 Pareto 原理进行排序划分，并以此表映射出其默认的安全库存服务水平目标（如 A 类高价物料维持 98% 交付率以保核心营收，C 类低价辅料维持 90% 交付率以防爆仓）。
* **分类编排逻辑**：
  1. 计算年度消费总额：对各物料 $i$ 统计 $Value_i = \sum Qty_i \times Price_i$。
  2. 帕累托降序排列：将所有物料按 $Value_i$ 降序排序，累计总额比例。
  3. 划分归类：前 80% 累计金额的 SKU 归为 A 类，80%-95% 归为 B 类，其余 5% 归为 C 类，写入本表。
  4. 规则绑定：将对应的服务目标注入 `ipc_service_level_target`，作为 MEIO 计算的初始系数。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 内存中，ABC 分类信息直接作为 `PartSiteRecord` 的属性字段，使用极窄的 `uint8_t` 进行存储，以节约内存带宽并加速分类检索：
```cpp
// 对应 ipc_abc_class 的 C++ 内存物理结构体
struct AbcClassRecord {
    uint8_t abc_class_code;           // ABC 编码 (0='A', 1='B', 2='C') (对应 abc_class)
    double revenue_threshold_lower;   // 分类金额占比下限
    double revenue_threshold_upper;   // 分类金额占比上限
    double default_service_level_target; // 该分类物料的默认交付目标 (e.g. 0.95)
};
```

###### 3. 边界与异常处理
* **战略物料优先级覆盖**：部分物料虽然年度消费金额极低（属于 C 类），但由于其是核心卡脖子关键元器件（如定制芯片），一旦缺料会导致整机停产。引擎支持在 `Part` 级别设置 `override_abc_class = 'A'`，强制提升其服务水准。