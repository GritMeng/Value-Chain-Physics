---
table_name: "ipc_wheel"
alias: "wheel"
module: "6_Object_Data_Model_ODM"
cpp_struct: "SetupTransitionMatrix"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_wheel` (wheel)

> **业务说明**: wheel表定义了生产轮的属性，如轮持续时间、周期长度和订单优先级。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `calendar` | calendar | `VARCHAR` | Nullable | Reference:Calendar |
| `interval` | interval | `DECIMAL(18,2)` | Nullable | Cycle的周期。例如，如果一个生产轮的calendar为week， CalendarInterval为2，则该轮的周期大小为2周。 |
| `eff_start_date` | eff_start_date | `VARCHAR` | Nullable | - |
| `eff_end_date` | eff_end_date | `VARCHAR` | Nullable | - |
| `first_start_date` | first_start_date | `VARCHAR` | Nullable | 计算字段。例如，如果EffectiveStartDate日期是2019年1月11日，并且循环使用周日历（周一到周日），那么轮子可以启动的第一个日期是2019年1月7日（周一）。 |
| `last_end_date` | last_end_date | `VARCHAR` | Nullable | - |
| `wheel` | wheel | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `priority` | priority | `VARCHAR` | Nullable | 优先级，数值越小越优先 |
| `wheel_type` | wheel_type | `VARCHAR` | Nullable | Reference:WheelType, Wheel的属性，规则 |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `description` | description | `VARCHAR` | Nullable | - |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：轮式生产与换型准备时间豁免
* **因果流向**：`ipc_wheel` 和相关表定义了瓶颈资源的“排产轮次（Wheel）”或“换型准备矩阵（Setup Matrix）”，用以降低由于频繁切换产品属性带来的产能消耗。
* **算法编排**：
  1. 维度特征比对：在 CTP 预占工段产能时，引擎读取前一个工单的维度特征值 $Dim_{prev}$ 与当前工单维度值 $Dim_{curr}$。
  2. 准备时间豁免：若两个工单维度特征一致，或者符合 `ipc_wheel` 中约定的环形轮转顺序，系统自动免除换型准备时间（Setup Time = 0），仅扣减净生产工时。这在离散制造中是保证“交付又准又快”的强约束优化算子。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，换型时间切换矩阵被扁平展开为一个一维的稠密矩阵（Setup Transition Matrix），支持 CPU 进行 $O(1)$ 复杂度的换型时间判定，完全摒弃了查表及指针路由的开销：

```cpp
// 换型矩阵内存物理布局 (Row-Major Contiguous Array)
struct SetupTransitionMatrix {
    uint32_t num_dimensions;         // 维度属性特征总数 (如 256 种颜色/产品规格)
    std::vector<double> setup_times; // 一维连续存放的换型切换时间矩阵
    
    // O(1) 瞬时查询从 from_dim 切换到 to_dim 的物理换型时间
    double get_setup_time(uint32_t from_dim, uint32_t to_dim) const {
        return setup_times[from_dim * num_dimensions + to_dim];
    }
};
```