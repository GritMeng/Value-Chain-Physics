#include "csv_export.h"
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include "ipc_types.h"

namespace ipc {

static void write_csv(const std::string& filename, const std::vector<std::vector<std::string>>& rows) {
    std::ofstream ofs(filename, std::ios::trunc);
    if (!ofs.is_open()) return;
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            ofs << row[i];
            if (i + 1 < row.size()) ofs << ",";
        }
        ofs << "\n";
    }
    ofs.close();
}

void csv_export_parts(const std::vector<PartSiteRecord>& parts) {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"part_code", "part_type", "on_hand", "low_level_code", "round_to_integer"});
    for (const auto& p : parts) {
        rows.push_back({p.part_code,
                       p.part_type,
                       std::to_string(p.on_hand),
                       std::to_string(p.low_level_code),
                       p.round_to_integer ? "true" : "false"});
    }
    write_csv("parts.csv", rows);
}

void csv_export_boms(const std::vector<FlatBomItem>& boms) {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"parent_id", "child_id", "per_qty", "scrap", "alt_group_id", "alt_priority", "target_ratio", "historical_qty", "lot_size"});
    for (const auto& b : boms) {
        rows.push_back({std::to_string(b.parent_id),
                       std::to_string(b.child_id),
                       std::to_string(b.per_qty),
                       std::to_string(b.scrap),
                       std::to_string(b.alt_group_id),
                       std::to_string(b.alt_priority),
                       std::to_string(b.target_ratio),
                       std::to_string(b.historical_qty),
                       std::to_string(b.lot_size)});
    }
    write_csv("boms.csv", rows);
}

void csv_export_demands(const std::vector<IndependentDemand>& demands) {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"demand_id", "customer", "part_id", "qty", "due_day", "priority", "dimension_val"});
    for (const auto& d : demands) {
        rows.push_back({std::to_string(d.demand_id),
                       d.customer,
                       std::to_string(d.part_id),
                       std::to_string(d.qty),
                       std::to_string(d.due_day),
                       std::to_string(d.priority),
                       std::to_string(d.dimension_val)});
    }
    write_csv("demands.csv", rows);
}

void csv_export_ipc_planned_orders(const std::vector<PlannedOrder>& orders) {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"part_id", "qty", "start_day", "finish_day", "dimension_val"});
    for (const auto& o : orders) {
        if (o.qty <= 0.0) continue;
        rows.push_back({std::to_string(o.part_id),
                       std::to_string(o.qty),
                       std::to_string(o.start_day),
                       std::to_string(o.finish_day),
                       std::to_string(o.dimension_val)});
    }
    write_csv("ipc_planned_orders.csv", rows);
}

void csv_export_alternates(const std::vector<AlternateAllocationRecord>& alts) {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"demand_id", "main_part_id", "alt_part_id", "allocated_qty", "day", "alt_class"});
    for (const auto& a : alts) {
        rows.push_back({std::to_string(a.demand_id),
                       std::to_string(a.main_part_id),
                       std::to_string(a.alt_part_id),
                       std::to_string(a.allocated_qty),
                       std::to_string(a.day),
                       std::to_string(a.alt_class)});
    }
    write_csv("alternates.csv", rows);
}

void csv_export_lsctree(const std::vector<LscTreeNode>& nodes) {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"root_part_id", "root_dimension_val", "node_part_id", "parent_part_id", "per_qty", "root_per_qty", "cumulative_lt", "node_level", "is_leaf"});
    for (const auto& n : nodes) {
        rows.push_back({std::to_string(n.root_part_id),
                       std::to_string(n.root_dimension_val),
                       std::to_string(n.node_part_id),
                       std::to_string(n.parent_part_id),
                       std::to_string(n.per_qty),
                       std::to_string(n.root_per_qty),
                       std::to_string(n.cumulative_lt),
                       std::to_string(n.node_level),
                       n.is_leaf ? "true" : "false"});
    }
    write_csv("lsctree.csv", rows);
}

} // namespace ipc

