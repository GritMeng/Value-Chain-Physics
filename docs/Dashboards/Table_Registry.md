# 📋 全表物理元数据注册表

本表通过 `Dataview` 插件动态汇总了当前 IPC 计划决策引擎中全量物理表的设计状态。

```dataview
table alias as "业务别名", module as "所属业务模块", cpp_struct as "C++ DOD 对齐结构体", status as "丰富进度"
from "Tables"
where file.name != "1_Core_Planning" and file.name != "2_Co_product_Optimization" and file.name != "3_ETO_Project" and file.name != "4_IBP_Consolidated" and file.name != "5_IO_Safety_Stock" and file.name != "6_Object_Data_Model_ODM" and file.name != "7_Control_Data_Model_CDM"
sort status desc, file.name asc
```
