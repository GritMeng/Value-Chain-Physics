#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace ipc_core {

// 物料主数据与水文基础信息
struct PartSiteRecord {
    uint32_t part_id;
    std::string part_code;
    std::string site;
    double safety_stock = 0.0;
    double initial_on_hand = 0.0;
    double lot_size = 1.0;
    int lead_time = 0;
};

// 单层扁平 BOM 展开结构体（用于替代料与需求向下传导）
struct FlatBomItem {
    uint32_t parent_id;
    uint32_t child_id;
    double usage_qty = 1.0;
    uint8_t alt_class = 0;       // 0: 主料, 1: 一类替代料(按比例平摊), 2: 二类替代料(组内优先级), 3: 三类替代料(跨组动态消纳)
    uint32_t alt_group = 0;      // 替代料组 ID
    double target_ratio = 1.0;   // 目标配额比例
    double historical_qty = 0.0; // 历史已累计分配量
    double lot_size = 1.0;       // 批量倍数
};

// 独立需求 (客户订单/预测需求)
struct IndependentDemand {
    uint32_t demand_id;
    uint32_t part_id;
    int due_day;
    double qty;
    uint64_t priority;           // 位域编码优先级
    std::string customer_group;  // 客户组
    std::string region;          // 销售大区
};

// 资源/工作中心能力约束记录
struct CapacityRecord {
    std::string work_center;
    int day;
    double capacity_hours;
    double allocated_hours = 0.0;
};

// 可承诺量 (ATP) 节点
struct ATPSupplyNode {
    std::string supply_code;
    std::string supply_type;     // "On-Hand", "SR", "Planned-Order"
    uint32_t part_id;
    int available_day;
    double qty;
    double allocated_qty = 0.0;
    uint64_t priority = 99999ULL;
};

// 主计划配额键 (用于 ITP 防波堤 vs IOP 刚性阻断)
struct AllotmentConstraintKey {
    int day;
    uint32_t family_id;
    uint32_t cust_group_id;
    uint32_t region_id;

    bool operator==(const AllotmentConstraintKey& o) const {
        return day == o.day && family_id == o.family_id &&
               cust_group_id == o.cust_group_id && region_id == o.region_id;
    }
};

struct AllotmentConstraintKeyHash {
    size_t operator()(const AllotmentConstraintKey& k) const {
        size_t h1 = std::hash<int>()(k.day);
        size_t h2 = std::hash<uint32_t>()(k.family_id);
        size_t h3 = std::hash<uint32_t>()(k.cust_group_id);
        size_t h4 = std::hash<uint32_t>()(k.region_id);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

// 配额状态（ITP 分配下沉至 IOP 执行）
struct AllotmentState {
    double total_quota = 0.0;    // ITP 主计划给出的最大防波堤配额
    double consumed_qty = 0.0;   // IOP 执行计划已消耗配额
};

// 替代料分配结果记录
struct AlternateAllocationRecord {
    uint32_t day;
    uint32_t parent_part_id;
    uint32_t alt_part_id;
    double allocated_qty;
    int day_allocated;
    uint8_t alt_class;
};

// 计划订单
struct PlannedOrder {
    uint32_t order_id;
    uint32_t part_id;
    int start_day;
    int due_day;
    double qty;
    bool is_scheduled = false;
    double allocated_capacity = 0.0;
};

} // namespace ipc_core
