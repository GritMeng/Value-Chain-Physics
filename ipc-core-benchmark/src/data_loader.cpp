#include "ipc_core/data_loader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace ipc_core {

std::vector<std::vector<std::string>> read_csv(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开数据文件: " + path);
    }
    std::vector<std::vector<std::string>> rows;
    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // 兼容 CRLF
        if (line.empty()) continue;
        if (first) { first = false; continue; }                     // 跳过表头
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) fields.push_back(cell);
        rows.push_back(std::move(fields));
    }
    return rows;
}

std::vector<PartSiteRecord> load_parts(const std::string& path) {
    auto rows = read_csv(path);
    std::vector<PartSiteRecord> out;
    out.reserve(rows.size());
    for (const auto& r : rows) {
        PartSiteRecord p;
        p.part_id        = static_cast<uint32_t>(std::stoul(r[0]));
        p.part_code      = r[1];
        p.site           = r[2];
        p.safety_stock   = std::stod(r[3]);
        p.initial_on_hand= std::stod(r[4]);
        p.lot_size       = std::stod(r[5]);
        p.lead_time      = std::stoi(r[6]);
        out.push_back(std::move(p));
    }
    return out;
}

std::vector<IndependentDemand> load_demands(const std::string& path) {
    auto rows = read_csv(path);
    std::vector<IndependentDemand> out;
    out.reserve(rows.size());
    for (const auto& r : rows) {
        IndependentDemand d;
        d.demand_id      = static_cast<uint32_t>(std::stoul(r[0]));
        d.part_id        = static_cast<uint32_t>(std::stoul(r[1]));
        d.due_day        = std::stoi(r[2]);
        d.qty            = std::stod(r[3]);
        d.priority       = std::stoull(r[4]);
        d.customer_group = r[5];
        d.region         = r[6];
        out.push_back(std::move(d));
    }
    return out;
}

std::vector<FlatBomItem> load_bom(const std::string& path) {
    auto rows = read_csv(path);
    std::vector<FlatBomItem> out;
    out.reserve(rows.size());
    for (const auto& r : rows) {
        FlatBomItem b;
        b.parent_id      = static_cast<uint32_t>(std::stoul(r[0]));
        b.child_id       = static_cast<uint32_t>(std::stoul(r[1]));
        b.usage_qty      = std::stod(r[2]);
        b.alt_class      = static_cast<uint8_t>(std::stoul(r[3]));
        b.alt_group      = static_cast<uint32_t>(std::stoul(r[4]));
        b.target_ratio   = std::stod(r[5]);
        b.historical_qty = std::stod(r[6]);
        b.lot_size       = std::stod(r[7]);
        out.push_back(std::move(b));
    }
    return out;
}

std::vector<CapacityRecord> load_capacity(const std::string& path) {
    auto rows = read_csv(path);
    std::vector<CapacityRecord> out;
    out.reserve(rows.size());
    for (const auto& r : rows) {
        CapacityRecord c;
        c.work_center     = r[0];
        c.day             = std::stoi(r[1]);
        c.capacity_hours  = std::stod(r[2]);
        c.allocated_hours = 0.0;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::vector<ATPSupplyNode>> load_atp_supply(
    const std::string& path, size_t min_size) {
    auto rows = read_csv(path);
    // 先扫描最大 part_id 以确定分组规模
    size_t max_pid = min_size;
    for (const auto& r : rows) {
        max_pid = std::max(max_pid, static_cast<size_t>(std::stoul(r[2])) + 1);
    }
    std::vector<std::vector<ATPSupplyNode>> out(max_pid);
    for (const auto& r : rows) {
        ATPSupplyNode n;
        n.supply_code  = r[0];
        n.supply_type  = r[1];
        n.part_id      = static_cast<uint32_t>(std::stoul(r[2]));
        n.available_day= std::stoi(r[3]);
        n.qty          = std::stod(r[4]);
        n.allocated_qty= 0.0;
        n.priority     = static_cast<uint64_t>(std::stoull(r[5]));
        out[n.part_id].push_back(std::move(n));
    }
    return out;
}

BenchDataset load_bench_dataset(const std::string& data_dir) {
    BenchDataset ds;
    std::string dir = data_dir;
    if (!dir.empty() && dir.back() != '/') dir += '/';

    ds.parts             = load_parts(dir + "parts.csv");
    ds.master_demands    = load_demands(dir + "demands_master.csv");
    ds.execution_demands = load_demands(dir + "demands_execution.csv");
    ds.boms              = load_bom(dir + "bom.csv");
    ds.capacity_records  = load_capacity(dir + "capacity.csv");
    ds.atp_supplies      = load_atp_supply(dir + "atp_supply.csv", ds.parts.size());
    return ds;
}

} // namespace ipc_core
