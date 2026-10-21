---
table_name: "ipc_forecast_detail"
alias: "forecast_detail"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastDetailRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_detail` (forecast_detail)

> **业务说明**: 此表保存详细级别的当前预测数据(在执行Disaggreation计算之后)。每条记录都属于给定的零件、客户和类别组合，并显示诸如预测数量和日期之类的详细信息。不同类型的预测可以存储在这个表中，比如统计预测、销售预测、市场预测等等。
ForecastDetail表支持销售和运营计划。表中显示的值是基于通过各种资源输入或维护的汇总值.例如，统计预测的详细信息是基于由
在S&OP统计预测工作簿中保存预测命令。其他类型预测的详细信息基于在属于相关组成组的工作簿中输入的值(例如，值可能通过S&OP市场预测工作簿、S&OP销售预测工作簿.该表中的值还反映了CausalFactorDetail表中报告的任何因果数量。


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `forecast` | forecast | `VARCHAR(10)` | PK / NOT NULL | Forecast唯一标识 |
| `category` | category | `VARCHAR(1)` | PK / NOT NULL | - |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `value` | value | `DOUBLE` | Nullable | 当CategoryType.UnitType = 'Money'时 |
| `unit_price` | unit_price | `DOUBLE` | Nullable | 允许为此预测订单指定唯一的单价。例如，这可能用于反映促销期间的有效价格。
如果这里提供了一个非负值，它总是在effecveunitprice字段中报告(通常用于收入计算)。如果这里提供了负值，则根据CustomerPrice或Part表中的匹配记录计算单价 |
| `eff_unit_price` | eff_unit_price | `DOUBLE` | Nullable | 此预测订单的有效单价。此值基于此记录中提供的输入单价，或者基于此记录日期预测部分和客户(通过Header字段引用定义)的有效单价.
该值在计算收入时很有用，计算方法如下:
 1. 如果在UnitPrice字段中提供了一个非负值，则总是使用它.
 2. 否则，IPC将检查CustomerPrice表中具有匹配的Part和Customer值的记录，这些值的有效日期小于或等于该记录的日期。如果找到任何匹配的记录，那么此处将报告具有最新生效日期的记录中的单价。注意，如果具有相同生效日期的给定部件和客户组合存在多个记录，则在CustomerPrice中使用较大的值进行记录。
3. 但是，如果没有找到匹配的Part和Customer值，RapidResponse接下来将检查CustomerPrice表，查找具有匹配的Part和空白的记录
有效日期小于或等于记录日期的客户值(即“”)。如果找到任何匹配的记录，那么此处将报告具有最新生效日期的记录中的单价.
4. 最后，如果在上述任何条件下都找不到单价，或者在
CustomerPrice。单价为负，则值为
部分。平均售价报告在这里。但是，如果Part。AverageSellingPrice也是负的，然后是ForecastDetail。EffectiveUnit
价格设为零 |
| `date` | date | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：分销网络底层时序预测分解明细生成
* **因果流向**：`ipc_forecast_detail` 存储了共识预测向底层具体物料 SKU-Site-Customer 级别进行 Top-Down 分解后的明细记录。该表的有效单价和数量是计算未来预期收入及物料齐套需求的最基础输入。
* **分解编排算法**：
  1. 比例分解：将上一级产品族的共识数量 $Q_{parent}$ 按照历史出货比例（Disaggregation Rate）拆分至子 SKU $i$：
     $$ Q_i(t) = Q_{parent}(t) \times Rate_i(t) $$
  2. 有效价格检索：单价采用定制价（`CustomerPrice`）优先级链条检索，乘分解后的数量获取该明细的有效收入（Revenue），并用于后续财务分类账（Financial Ledger）的自动对账。

###### 2. 物理内存结构设计 (C++ DOD Layout)
预测细节表在内存中以密集数组分布，优化了时序物料需求爆炸（BOM Explosion）时的缓存局部性：
```cpp
// 对应 ipc_forecast_detail 的 C++ DOD 结构体
struct ForecastDetailRecord {
    uint32_t forecast_id;          // 预测明细 ID
    uint8_t category_type;         // 预测类别枚举 (0=Statistical, 1=Sales, 2=Override)
    double qty;                    // 分开分解后的数量 (对应 qty)
    double unit_price;             // 录入的单价
    double effective_unit_price;   // 检索后的有效价格 (对应 eff_unit_price)
    int day_bucket;                // 相对计划天数 (对应 date)
};
```

###### 3. 边界与异常处理
* **无销量冷启动分解分配**：若新物料历史销量为 0，导致分解速率计算出 $Rate_i = 0$，引擎会自动将默认分解速率降级为“等比例分摊”（Fair-Share），防止新产品无法分解出预测需求。