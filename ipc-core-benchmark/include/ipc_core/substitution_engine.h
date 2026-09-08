#pragma once
#include "types.h"
#include <vector>
#include <cstdint>

namespace ipc_core {

// 一类替代料：按历史配额比例平摊分配 (Quota Balance)
uint32_t allocate_class1(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand
);

// 二类替代料：组内固定优先级选优 (Priority Hierarchy)
uint32_t allocate_class2(
    std::vector<FlatBomItem*>& group_items
);

// 三类替代料：跨组动态归一化与水位消纳 (Dynamic Normalization & Bucket Consumption)
void allocate_class3(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand,
    int day,
    uint32_t part_id,
    std::vector<AlternateAllocationRecord>& out_alt_records,
    const std::vector<PartSiteRecord>& parts
);

} // namespace ipc_core
