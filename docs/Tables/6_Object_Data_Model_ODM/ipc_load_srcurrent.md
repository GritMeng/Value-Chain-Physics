---
table_name: "ipc_load_srcurrent"
alias: "load_srcurrent"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_load_srcurrent` (load_srcurrent)

> **业务说明**: 描述工作中心上的每个SR负载

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `end_date` | end_date | `VARCHAR` | Nullable | 在相应operation结束的日期 |
| `work_center` | work_center | `VARCHAR` | PK / NOT NULL | 工作中心 |
| `operation` | operation | `VARCHAR` | Nullable | - |
| `end_offset` | end_offset | `DECIMAL(18,2)` | Nullable | 将操作结束时的小时数转换为每天的小时数。 |
| `idle` | idle | `VARCHAR` | Nullable | 在此日期在此工作中心等待前一个操作的批次到达的标准小时数。 |
| `next_work_center` | next_work_center | `VARCHAR` | Nullable | 下一个工作中心 |
| `pre_work_center` | pre_work_center | `VARCHAR` | Nullable | 前一个工作中心 |
| `queue_date` | queue_date | `VARCHAR` | Nullable | 哪天开始排队 |
| `queue_offset` | queue_offset | `VARCHAR` | Nullable | 排队开始时把小时数转化为每天的小时数 |
| `routing` | routing | `VARCHAR` | PK / NOT NULL | - |
| `run` | run | `VARCHAR` | Nullable | 运行小时数 |
| `run_date` | run_date | `VARCHAR` | Nullable | 运行开始日期 |
| `run_offset` | run_offset | `VARCHAR` | Nullable | - |
| `sr` | sr | `VARCHAR` | Nullable | Reference： SR |
| `setup` | setup | `VARCHAR` | Nullable | - |
| `setup_date` | setup_date | `VARCHAR` | Nullable | - |
| `setup_offset` | setup_offset | `VARCHAR` | Nullable | - |
| `start_date` | start_date | `VARCHAR` | Nullable | - |
| `start_offset` | start_offset | `VARCHAR` | Nullable | - |
| `start_unit` | start_unit | `VARCHAR` | Nullable | - |
| `wait_date` | wait_date | `VARCHAR` | Nullable | - |
| `wait_offset` | wait_offset | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：当前在途订单物理负载明细表。记录已下发的在途工单（Scheduled Receipt）对当前物理车间设备所占用的机器及人力时间负荷。
* **计算逻辑编排**：
  1. MES 负荷映射：导入外部 MES 实绩负荷，按工序完工比例计算剩余 setup 和 run 工时；2. 刚性锁死：作为车间已经开始的任务，其在 `ipc_load_planned_date` 中具有绝对的排他占用优先级，不可被 CTP 计划订单抢占或平移。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_load_srcurrent 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcLoadSrcurrentRecord {
    std::string end_date; // end_date 字符串 (在相应operation结束的日期)
    std::string work_center; // work_center 字符串 (工作中心)
    std::string operation; // operation 字符串 (-)
    double end_offset = 0.0; // end_offset 数量/金额精度值 (将操作结束时的小时数转换为每天的小时数。)
    uint32_t idle; // idle 逻辑ID/映射 (在此日期在此工作中心等待前一个操作的批次到达的标准小时数。)
    std::string next_work_center; // next_work_center 字符串 (下一个工作中心)
    std::string pre_work_center; // pre_work_center 字符串 (前一个工作中心)
    std::string queue_date; // queue_date 字符串 (哪天开始排队)
    std::string queue_offset; // queue_offset 字符串 (排队开始时把小时数转化为每天的小时数)
    uint32_t routing; // routing 逻辑ID/映射 (-)
    std::string run; // run 字符串 (运行小时数)
    std::string run_date; // run_date 字符串 (运行开始日期)
    std::string run_offset; // run_offset 字符串 (-)
    std::string sr; // sr 字符串 (Reference： SR)
    std::string setup; // setup 字符串 (-)
    std::string setup_date; // setup_date 字符串 (-)
    std::string setup_offset; // setup_offset 字符串 (-)
    std::string start_date; // start_date 字符串 (-)
    std::string start_offset; // start_offset 字符串 (-)
    std::string start_unit; // start_unit 字符串 (-)
    std::string wait_date; // wait_date 字符串 (-)
    std::string wait_offset; // wait_offset 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。