#pragma once
#include "ipc_types.h"
#include <vector>
#include <cstdint>

namespace ipc {

uint32_t allocate_class1(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand
);

uint32_t allocate_class2(
    std::vector<FlatBomItem*>& group_items
);

void allocate_class3(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand,
    int day,
    uint32_t part_id,
    std::vector<AlternateAllocationRecord>& out_alt_records,
    const std::vector<PartSiteRecord>& parts
);

} // namespace ipc
