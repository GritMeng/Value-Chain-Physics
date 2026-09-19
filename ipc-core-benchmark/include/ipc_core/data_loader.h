#pragma once
// IPC Core 测试数据加载器 (CSV -> 内存结构)
// 将 data/*.csv 加载为引擎所需的输入结构，替代运行时内存合成。
#include "types.h"
#include <string>
#include <vector>

namespace ipc_core {

// 加载结果聚合结构：承载全部引擎输入
struct BenchDataset {
    std::vector<PartSiteRecord> parts;                              // parts.csv
    std::vector<IndependentDemand> master_demands;                  // demands_master.csv
    std::vector<IndependentDemand> execution_demands;               // demands_execution.csv
    std::vector<FlatBomItem> boms;                                  // bom.csv
    std::vector<std::vector<ATPSupplyNode>> atp_supplies;           // atp_supply.csv (按 part_id 索引)
    std::vector<CapacityRecord> capacity_records;                   // capacity.csv
};

// 通用 CSV 读取（跳过表头，按 ',' 切分）
std::vector<std::vector<std::string>> read_csv(const std::string& path);

// 单项加载器
std::vector<PartSiteRecord> load_parts(const std::string& path);
std::vector<IndependentDemand> load_demands(const std::string& path);
std::vector<FlatBomItem> load_bom(const std::string& path);
std::vector<CapacityRecord> load_capacity(const std::string& path);

// atp_supply.csv -> 按 part_id 归组的二维向量（会自动扩容到 max_part_id+1）
std::vector<std::vector<ATPSupplyNode>> load_atp_supply(
    const std::string& path, size_t min_size = 0);

// 一次性加载基准测试全部数据
BenchDataset load_bench_dataset(const std::string& data_dir);

} // namespace ipc_core
