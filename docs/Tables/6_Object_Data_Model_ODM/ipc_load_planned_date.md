---
table_name: "ipc_load_planned_date"
alias: "load_planned_date"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_load_planned_date` (load_planned_date)

> **业务说明**: 描述工作中心的所有负载，从计划的收据和计划的订单到使用重新安排的计划收据日期的操作和计划的操作。
该负荷将与计划中使用的物料的分配日期相匹配.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `VARCHAR` | Nullable | 负载日期 |
| `avlabor_capacity` | avlabor_capacity | `VARCHAR` | Nullable | 所有工人每天可用的平均标准工时数 |
| `avmachine_capacity` | avmachine_capacity | `VARCHAR` | Nullable | 所有工人每天可使用的平均标准机器小时数 |
| `labor_capacity` | labor_capacity | `VARCHAR` | Nullable | 人力负载落在这一日期的总和 |
| `labor_idle` | labor_idle | `VARCHAR` | Nullable | 在此日期在此工作中心等待前一次操作的批次到达的标准工时数。这个时间包含在LaborRun时中 |
| `labor_run` | labor_run | `VARCHAR` | Nullable | 在运行时期间生成的日期上放置在此工作中心上的标准工时数 |
| `labor_setup` | labor_setup | `VARCHAR` | Nullable | 在设置期间生成的日期上放置在此工作中心上的标准工时数。 |
| `machine_capacity` | machine_capacity | `VARCHAR` | Nullable | 今天这个工作中心的机器总装机容量 |
| `machine_idle` | machine_idle | `VARCHAR` | Nullable | 在此日期在此工作中心等待前一个操作的批次到达的标准机器小时数。这个时间包含在MachineRun时中 |
| `machine_run` | machine_run | `VARCHAR` | Nullable | 在运行期间生成的日期上放置在此工作中心上的标准机器小时数。 |
| `max_labor_capacity` | max_labor_capacity | `VARCHAR` | Nullable | 在相应的产能记录中描述的该工作中心当天的最大劳动力产能。 |
| `max_machine_capacity` | max_machine_capacity | `VARCHAR` | Nullable | 在相应的产能记录中描述的该工作中心当天的最大机器产能。 |
| `number_of_workers` | number_of_workers | `VARCHAR` | Nullable | 当天的工作人员数量 |
| `number_of_mahines` | number_of_mahines | `VARCHAR` | Nullable | 当天的机器数量 |
| `setup` | setup | `VARCHAR` | Nullable | 在此工作中心上的被放置的标准小时数。 |
| `srlabor_idle` | srlabor_idle | `VARCHAR` | Nullable | SR对应的等待上一工序的时间，这个时间包含在RunTime里 |
| `srlabor_run` | srlabor_run | `VARCHAR` | Nullable | SR相应的工作者运转时间 |
| `srlabor_setup` | srlabor_setup | `VARCHAR` | Nullable | 在安装期间根据计划收据生成的日期上放置在此工作中心上的标准工时数 |
| `srmachine_idle` | srmachine_idle | `VARCHAR` | Nullable | SR的机器等待上一工序的时间 |
| `srmachine_run` | srmachine_run | `VARCHAR` | Nullable | 在运行期间根据计划收据生成的日期上放置在此工作中心上的标准机器小时数。 |
| `srmachine_setup` | srmachine_setup | `VARCHAR` | Nullable | 在安装期间根据计划收据生成的日期上放置在此工作中心上的标准小时数 |
| `work_center` | work_center | `VARCHAR` | PK / NOT NULL | 工作中心 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：计划负载日期汇总表。在内存中对所有计划订单（Planned Order）和在途供应（SR）产生的工时负荷按工作中心及日期进行累加聚合，是 MRP 排产的日负荷热力图基础。
* **计算逻辑编排**：
  1. 日级负荷累加：遍历所有 Planned Order 和 SR 负载，按 `work_center` 和 `date` 维度将 labor_run, machine_run 进行前缀和累加；2. 超载率计算：\n      $$ Load\_Ratio(wc, t) = \\frac{{Labor\_Run(wc, t) + Setup(wc, t)}}{{Max\_Labor\_Capacity(wc, t)}} $$\n   3. 异常输出：若 Load_Ratio > 1.0，向控制塔推送“产能爆红”信号，并在 CTP 阶段触发工单拆分与平移逻辑。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_load_planned_date 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcLoadPlannedDateRecord {
    std::string date; // date 字符串 (负载日期)
    std::string avlabor_capacity; // avlabor_capacity 字符串 (所有工人每天可用的平均标准工时数)
    std::string avmachine_capacity; // avmachine_capacity 字符串 (所有工人每天可使用的平均标准机器小时数)
    std::string labor_capacity; // labor_capacity 字符串 (人力负载落在这一日期的总和)
    uint32_t labor_idle; // labor_idle 逻辑ID/映射 (在此日期在此工作中心等待前一次操作的批次到达的标准工时数。这个时间包含在LaborRun时中)
    std::string labor_run; // labor_run 字符串 (在运行时期间生成的日期上放置在此工作中心上的标准工时数)
    std::string labor_setup; // labor_setup 字符串 (在设置期间生成的日期上放置在此工作中心上的标准工时数。)
    std::string machine_capacity; // machine_capacity 字符串 (今天这个工作中心的机器总装机容量)
    uint32_t machine_idle; // machine_idle 逻辑ID/映射 (在此日期在此工作中心等待前一个操作的批次到达的标准机器小时数。这个时间包含在MachineRun时中)
    std::string machine_run; // machine_run 字符串 (在运行期间生成的日期上放置在此工作中心上的标准机器小时数。)
    std::string max_labor_capacity; // max_labor_capacity 字符串 (在相应的产能记录中描述的该工作中心当天的最大劳动力产能。)
    std::string max_machine_capacity; // max_machine_capacity 字符串 (在相应的产能记录中描述的该工作中心当天的最大机器产能。)
    std::string number_of_workers; // number_of_workers 字符串 (当天的工作人员数量)
    std::string number_of_mahines; // number_of_mahines 字符串 (当天的机器数量)
    std::string setup; // setup 字符串 (在此工作中心上的被放置的标准小时数。)
    uint32_t srlabor_idle; // srlabor_idle 逻辑ID/映射 (SR对应的等待上一工序的时间，这个时间包含在RunTime里)
    std::string srlabor_run; // srlabor_run 字符串 (SR相应的工作者运转时间)
    std::string srlabor_setup; // srlabor_setup 字符串 (在安装期间根据计划收据生成的日期上放置在此工作中心上的标准工时数)
    uint32_t srmachine_idle; // srmachine_idle 逻辑ID/映射 (SR的机器等待上一工序的时间)
    std::string srmachine_run; // srmachine_run 字符串 (在运行期间根据计划收据生成的日期上放置在此工作中心上的标准机器小时数。)
    std::string srmachine_setup; // srmachine_setup 字符串 (在安装期间根据计划收据生成的日期上放置在此工作中心上的标准小时数)
    std::string work_center; // work_center 字符串 (工作中心)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。