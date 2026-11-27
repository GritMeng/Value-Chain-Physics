#pragma once
#include "types.h"
#include <vector>
#include <cstdint>

namespace ipc_core {

// Class 1: 历史累计配额比例追平
uint32_t allocate_class1(
    double net_demand,
    std::vector<BomEdgeRecord*>& group_items,
    std::vector<double>& current_on_hand
);

// Class 2: 稳定评级选优
uint32_t allocate_class2(
    std::vector<BomEdgeRecord*>& group_items
);

// Class 3: 跨组动态配额重归一化、最小包装倍数圆整与安全库存保护
void allocate_class3(
    double net_demand,
    std::vector<BomEdgeRecord*>& group_items,
    std::vector<double>& current_on_hand,
    int day,
    uint32_t part_id,
    std::vector<SubDispatchRecord>& out_alt_records,
    const std::vector<SkuNodeRecord>& parts
);

} // namespace ipc_core
