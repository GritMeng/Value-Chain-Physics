---
table_name: "ipc_site"
alias: "site"
module: "6_Object_Data_Model_ODM"
cpp_struct: "SiteRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_site` (site)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 |
| `calendar` | calendar | `VARCHAR(10)` | Nullable | 日历 |
| `address` | address | `VARCHAR` | Nullable | 地址 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `Country` | country_id | `VARCHAR(10)` | Nullable | 国家编码 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | Site描述 |
| `currency` | currency | `DECIMAL(18,2)` | Nullable | 货币 |
| `site_type` | site_type | `VARCHAR(10)` | Nullable | 报表显示Site的用途 |
| `time_zone` | time_zone | `VARCHAR(10)` | Nullable | 时区 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多站点供应链网络的汇率转换与时区一致性引擎
* **因果流向**：`ipc_site` 是全局网络流模型的基础网格节点。由于企业集团通常跨越多个国家和地区，每个工厂/仓库的本币、默认生产日历和时区不同。引擎在加载主数据时，通过该表将全球站点的输入数据归一化为单一时间与财务基准。
* **归一化算法编排**：
  1. 时区校准：获取各站点 `time_zone` 偏移，将所有在途物流单的离合交期、排产天数统一平移至 UTC+0 儒略日。
  2. 汇率折算：读取 `currency` 并与全局汇率表关联，在计算 Consensus Revenue 时将欧元、日元等折算为集团本币（如 USD）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
站点数据在内存中为全局可达的密集网格节点配置，支持快速索引：
```cpp
// 对应 ipc_site 的 C++ DOD 物理数据结构
struct SiteRecord {
    uint32_t site_id;                // 站点ID逻辑编码
    uint32_t calendar_id;            // 关联的日历 ID (指向 CalendarRecord)
    uint16_t currency_id;            // 本币货币 ID
    int timezone_offset_minutes;     // 时区偏差分钟数
    bool is_manufacturing_node;      // 是否为制造工厂节点 (0=仓库/DC, 1=工厂)
};
```

###### 3. 边界与异常处理
* **未知时区默认兜底**：若某仓储站点的 `time_zone` 为空或无效，引擎自动采用系统基准时区（Run Timezone，如 UTC+8），防止时间戳平移时产生空值或负数溢出。