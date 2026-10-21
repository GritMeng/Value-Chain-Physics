---
table_name: "ipc_forecastitem_map"
alias: "forecastitem_map"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastItemMapRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecastitem_map` (forecastitem_map)

> **业务说明**: 这个表定义了预测项目之间的映射，它允许在计算另一个项目的统计预测时使用一个项目的历史需求。例如，对于引入新产品，可以使用它所取代的产品的历史记录，通过定义从新产品到旧产品的映射来计算统计预测。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `item` | item | `VARCHAR(10)` | PK / NOT NULL | 引用的ForecastItem |
| `to_item` | to_item | `VARCHAR(10)` | PK / NOT NULL | 关联的ForecastItem |
| `mutiplier` | mutiplier | `DOUBLE` | Nullable | 缩放所引用的预测项目的历史数量 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：新产品引进（NPI）历史销售关系置换与缩放（Product Lifecycle Linkage）
* **因果流向**：当企业发布新产品（New Product Introduction - NPI）时，由于没有历史销量，常规统计预测模型（ARIMA）无法运行。`ipc_forecastitem_map` 定义了新旧产品生命周期桥接关系：引擎通过此表，将已退市的旧产品（To Item）的历史销量乘以缩放乘数 `multiplier`，作为新产品（Item）的“虚拟销量历史”输入模型进行冷启动训练。
* **生命周期桥接计算**：
  1. 获取新物料映射配置。锁定对应旧物料 ID。
  2. 时序数据拷贝与缩放：将旧物料的历史销量乘以 `multiplier` 系数，导入新物料的训练数轴：
     $$ History_{new}(t) = History_{old}(t) \times multiplier $$
  3. 执行常规预测，并将结果输出在新物料的预测流中。

###### 2. 物理内存结构设计 (C++ DOD Layout)
新旧产品生命周期桥接通常在数据预处理阶段（Pre-processing）运行。在内存中，它被编译为映射关系块：
```cpp
// 对应 ipc_forecastitem_map 的 C++ 内存结构
struct ForecastItemMapRecord {
    uint32_t new_part_id;        // 新物料 ID (对应 item)
    uint32_t reference_part_id;  // 历史参照旧物料 ID (对应 to_item)
    double scale_multiplier;     // 历史销量缩放系数 (对应 mutiplier)
    int active_start_day;        // 映射生效相对天
    int active_end_day;          // 映射失效相对天
};
```

###### 3. 边界与异常处理
* **桥接循环链阻断（Circular Linkage）**：若配置人员误操作，将 A 桥接到 B，B 桥接到 C，C 又桥接回 A 形成循环链，引擎在静态拓扑编译时会自动追踪继承链条深度。一旦检测到深度超过 5 层或形成环路，立即强行切断，回退到无历史销量冷启动，防止 DFS 栈溢出崩溃。