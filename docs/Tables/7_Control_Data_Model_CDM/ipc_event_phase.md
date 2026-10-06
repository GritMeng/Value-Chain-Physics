---
table_name: "ipc_event_phase"
alias: "event_phase"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcEventPhaseRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_event_phase` (event_phase)

> **业务说明**: 该表用于事件管理算法。EventPhase记录包含的信息包括该阶段开始和结束的时间、使用的日历以及在该阶段应如何调整预测


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `adjustment_type` | adjustment_type | `VARCHAR` | PK / NOT NULL | Qty
Money |
| `operation_rule` | operation_rule | `VARCHAR` | Nullable | Qty - 直接增加Qty在预测上
MultiQty - Qty*MutiQty
Percentage - 增加与测试数量为 预测数量*Percentage
MutiPer - 增加预测数量为预测数量*Percentage*MutiPer
Constant - 替换预测数量
Curve - 用曲线来增加预测数量， CurveParameter 决定调整 |
| `calendar` | calendar | `VARCHAR` | Nullable | - |
| `category` | category | `VARCHAR` | Nullable | Reference: HisDemandCategory |
| `multi_calendar` | multi_calendar | `VARCHAR` | Nullable | Calendar |
| `multi_interval_count` | multi_interval_count | `VARCHAR` | Nullable | 复利时间间隔数 |
| `creation_date` | creation_date | `VARCHAR` | Nullable | 创建日期 |
| `curve_parameters` | curve_parameters | `VARCHAR` | Nullable | Reference: [[ipc_curve_parameters|CurveParameters]] |
| `Description` | end_date | `VARCHAR` | Nullable | - |
| `start_date` | start_date | `VARCHAR` | Nullable | - |
| `event` | event | `VARCHAR` | PK / NOT NULL | Reference: [[ipc_event|Event]] |
| `event_phase` | event_phase | `VARCHAR` | PK / NOT NULL | - |
| `percent` | percent | `VARCHAR` | Nullable | - |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `usage` | usage | `VARCHAR` | Nullable | Use/Ignore |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：营销事件阶段控制表。定义具体促销活动在时间轴上的起点、终点，以及对预测量及单价的调整规则。
* **计算逻辑编排**：
  1. 活动时窗定义：读取 start_date 与 end_date 限制调整范围；2. 调整逻辑解析：根据 operation_rule（如 Percentage 比例、Constant 替换、Curve 曲线）计算增量；3. 曲线拟合：若为 Curve，调用对应曲线参数（curve_parameters）对销量趋势进行时序平滑叠加。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_event_phase 的 C++ DOD 物理对齐结构体
struct IpcEventPhaseRecord {
    std::string calendar; // calendar 字符串 (-)
    std::string category; // category 字符串 (Reference: HisDemandCategory)
    std::string multi_calendar; // multi_calendar 字符串 (Calendar)
    std::string multi_interval_count; // multi_interval_count 字符串 (复利时间间隔数)
    std::string creation_date; // creation_date 字符串 (创建日期)
    std::string curve_parameters; // curve_parameters 字符串 (Reference:CurveParameters)
    std::string Description; // Description 字符串 (-)
    std::string start_date; // start_date 字符串 (-)
    std::string event; // event 字符串 (Reference:Event)
    std::string event_phase; // event_phase 字符串 (-)
    std::string percent; // percent 字符串 (-)
    std::string qty; // qty 字符串 (数量 (Quantity))
    std::string usage; // usage 字符串 (Use/Ignore)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。