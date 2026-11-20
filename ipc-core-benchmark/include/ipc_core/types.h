#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace ipc_core {

// 物料-站点广义节点 (ODM: odm_sku_node)
struct SkuNodeRecord {
    uint32_t part_id = 0;
    std::string sku_code;
    std::string site_code;
    double safety_stock = 0.0;
    double initial_on_hand = 0.0;
    double lot_size = 1.0;
    int lead_time = 0;
    double cum_unit_cost = 0.0;
    bool physically_storable = true;
    bool untagged_generic = true;
};

// 有向 BOM 拓扑边与替代规则 (ODM: odm_bom_edge)
struct BomEdgeRecord {
    uint32_t parent_id = 0;
    uint32_t child_id = 0;
    double unit_usage = 1.0;
    uint8_t alt_class = 0;          // 0: 主料, 1: 一类历史比例平衡, 2: 二类稳定评级, 3: 三类跨组动态归一化
    uint32_t sub_group_code = 0;    // 替代组编码
    double target_ratio = 1.0;      // 目标配额份额
    double cum_dispatched_qty = 0.0;// 历史累计已分配量
    double lot_size = 1.0;          // 最小包装倍数 (MPQ)
};

// 独立需求单 (ODM: odm_demand_order)
struct DemandOrderRecord {
    uint32_t demand_id = 0;
    uint32_t part_id = 0;
    int due_day = 0;
    double qty = 0.0;
    uint64_t priority = 1;          // 数值越小优先级越高
    std::string customer_group;
    std::string region;
};

// 工作中心日产能记录 (ODM: odm_workcenter_capacity)
struct CapacityRecord {
    std::string work_center;
    int day = 0;
    double capacity_hours = 0.0;
    double allocated_hours = 0.0;
};

// 供给水位节点 (ODM: odm_stock_onhand / odm_intransit_supply)
struct ATPSupplyNode {
    std::string supply_code;
    std::string supply_type;        // "On-Hand", "Intransit_PO", "Planned_Order"
    uint32_t part_id = 0;
    int available_day = 0;
    double qty = 0.0;
    double allocated_qty = 0.0;
    uint64_t priority = 99999ULL;
};

// 配额防波堤键 (CDM: cdm_quota_firewall)
struct QuotaFirewallKey {
    int day = 0;
    uint32_t family_id = 0;
    uint32_t cust_group_id = 0;
    uint32_t region_id = 0;

    bool operator==(const QuotaFirewallKey& o) const {
        return day == o.day && family_id == o.family_id &&
               cust_group_id == o.cust_group_id && region_id == o.region_id;
    }
};

struct QuotaFirewallKeyHash {
    size_t operator()(const QuotaFirewallKey& k) const {
        size_t h1 = std::hash<int>()(k.day);
        size_t h2 = std::hash<uint32_t>()(k.family_id);
        size_t h3 = std::hash<uint32_t>()(k.cust_group_id);
        size_t h4 = std::hash<uint32_t>()(k.region_id);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

// 配额防波堤状态 (DDM: ddm_quota_valve_ledger)
struct QuotaFirewallState {
    double total_quota = 0.0;
    double consumed_qty = 0.0;
};

// 替代分派账本记录 (DDM: ddm_sub_dispatch_ledger)
struct SubDispatchRecord {
    uint32_t day = 0;
    uint32_t parent_part_id = 0;
    uint32_t alt_part_id = 0;
    double allocated_qty = 0.0;
    int day_allocated = 0;
    uint8_t alt_class = 0;
};

// 计划补货单 (DDM: ddm_replenish_plan)
struct ReplenishPlanRecord {
    uint32_t order_id = 0;
    uint32_t part_id = 0;
    int start_day = 0;
    int due_day = 0;
    double qty = 0.0;
    bool is_scheduled = false;
    double allocated_capacity = 0.0;
};

} // namespace ipc_core
