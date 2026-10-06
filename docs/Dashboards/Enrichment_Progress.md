# 🎯 数据字典补全进度面板

以下基于表笔记中的 `status` 标签，通过 `Dataview` 动态分类汇总。

## 🔴 待丰富物理表 (Unenriched Metadata Tables)
```dataview
table alias as "别名", module as "模块"
from "Tables"
where status = "Unenriched"
sort file.name asc
```

## 🟢 已完成世界级丰富物理表 (Enriched World-Class Specification Tables)
```dataview
table alias as "别名", cpp_struct as "C++ 结构体"
from "Tables"
where status = "Enriched"
sort file.name asc
```
