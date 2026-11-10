#pragma once
#include "ipc_types.h"
#include <vector>
#include <cstdint>

namespace ipc {

void expand_lsc_tree_node(
    uint32_t root_part_id,
    double root_dimension_val,
    uint32_t current_node_id,
    uint32_t parent_node_id,
    double current_per_qty,
    double current_root_per_qty,
    int current_lt,
    uint32_t current_level,
    const std::vector<FlatBomItem>& boms,
    const std::vector<PartSiteRecord>& parts,
    std::vector<LscTreeNode>& out_nodes,
    std::vector<uint32_t> path = {}
);

std::vector<LscTreeNode> compile_all_lsc_trees(
    const std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms
);

int compile_low_level_codes(
    std::vector<PartSiteRecord>& parts,
    const std::vector<FlatBomItem>& boms
);

void explode_planned_order_to_children(
    uint32_t part_id,
    double p_qty,
    int start_day,
    int demand_day,
    double dimension_val,
    const std::vector<FlatBomItem>& boms,
    const std::vector<PartSiteRecord>& parts,
    std::vector<std::vector<std::vector<double>>>& gross_demand,
    std::vector<std::vector<std::vector<uint64_t>>>& gross_demand_priority,
    const std::vector<std::vector<size_t>>& parent_to_bom_indices,
    std::vector<double>& current_on_hand,
    std::vector<std::vector<AlternateAllocationRecord>>& thread_alt_records,
    int tid,
    uint64_t parent_priority
);

} // namespace ipc
