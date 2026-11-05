#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace ipc {

// Relation operators
enum class RelationOp : uint8_t {
    PASS = 0,
    EQ = 1,
    LT = 2,
    LE = 3,
    GE = 4,
    GT = 5,
    NE = 6
};

// 1. PartSiteRecord
struct PartSiteRecord {
    uint32_t part_id;
    std::string part_code;
    double on_hand;
    double ipc_scheduled_receipt;
    uint32_t low_level_code;
    double lead_time;
    std::string mrp_rule;
    std::string part_type;
    bool round_to_integer = false;
    bool is_phantom = false;
    double cost = 1.0; // Effective cost (C_eff) for MCDM
    std::string site = "SITE_001"; // Site code
    double transshipment_cost = 0.0; // Multi-site logistics cost
    int transshipment_lead_time = 0; // Multi-site transportation time offset
    double run_rate = 0.0; // Dynamic lead-time stretching factor
    
    // CDM Configuration Rules
    std::string on_hand_type = "Standard";
    int time_fence_days = 0;
    std::string sourcing_policy = "Standard";
    std::string planning_calendar = "DEFAULT";
    std::string ss_rule = "None";
    std::string dos_policy = "None";
    double dos_intervals = 0.0;
    double safety_stock = 0.0;
};

// ScheduledReceiptRecord for detailed scheduled receipts
struct ScheduledReceiptRecord {
    std::string sr_id;
    uint32_t part_id;
    double qty;
    int due_day;
    std::string sr_type = "In-process"; // "In-process", "Ignore", "ExplodedOnly", "Reschedulable", "RescheduleRecommend"
    double certainty_level = 0.70;
};

// OperationRecord for routing operations
struct OperationRecord {
    std::string operation;
    uint32_t sequence;
    std::string work_center;
    double setup_time = 0.0;
    double run_time = 0.0;
    std::string routing;
};

// 2. FlatBomItem
struct FlatBomItem {
    uint32_t parent_id;
    uint32_t child_id;
    double per_qty;
    double scrap;
    int alt_group_id = -1;
    int alt_priority = 0;
    double target_ratio = 1.0;
    double historical_qty = 0.0;
    double lot_size = 0.0;
    uint8_t relation_op = static_cast<uint8_t>(RelationOp::PASS);
    double target_dim_val = 0.0;
    int eff_start_day = -1;
    int eff_end_day = -1;
    double ltb_limit = -1.0;
    int mix_group_id = -1;
    std::string relationship_type = "alt";
};

struct PartBomInfo {
    std::vector<size_t> standard_bom_indices;
    std::vector<std::pair<int, std::vector<size_t>>> alt_groups;
};

// 3. IndependentDemand
struct IndependentDemand {
    uint32_t demand_id;
    std::string demand_code;
    std::string customer;
    uint32_t part_id;
    double qty;
    int due_day;
    int priority;
    double dimension_val;
    std::string preference_mode = "N";
    std::string status = "OPEN";
    int customer_tier = 3;
    double revenue = 0.0;
    uint64_t composite_priority = 0;
    uint32_t family_id = uint32_t(-1);
    uint32_t cust_group_id = uint32_t(-1);
    uint32_t region_id = uint32_t(-1);
};

// Allotment Structures
struct AllotmentConstraintKey {
    uint32_t part_id;
    int day;
    uint32_t family_id;
    uint32_t cust_group_id;
    uint32_t region_id;
    
    bool operator==(const AllotmentConstraintKey& o) const {
        return part_id == o.part_id && day == o.day && family_id == o.family_id &&
               cust_group_id == o.cust_group_id && region_id == o.region_id;
    }
};

struct AllotmentConstraintKeyHash {
    size_t operator()(const AllotmentConstraintKey& k) const {
        size_t h1 = std::hash<uint32_t>()(k.part_id);
        size_t h2 = std::hash<int>()(k.day);
        size_t h3 = std::hash<uint32_t>()(k.family_id);
        size_t h4 = std::hash<uint32_t>()(k.cust_group_id);
        size_t h5 = std::hash<uint32_t>()(k.region_id);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
    }
};

struct AllotmentState {
    double limit = -1.0; // -1 means unconstrained
    double consumed = 0.0;
    double blocked = 0.0;
    bool is_locked = false;
    
    std::string part_code;
    std::string site_code;
    std::string region_code;
    std::string cust_group_code;
    std::string family_code;
};

inline AllotmentState* find_matching_allotment(
    uint32_t part_id,
    int day,
    uint32_t family_id,
    uint32_t cust_group_id,
    uint32_t region_id,
    uint32_t wildcard_id,
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints
) {
    uint32_t families[2] = {family_id, wildcard_id};
    uint32_t groups[2] = {cust_group_id, wildcard_id};
    uint32_t regions[2] = {region_id, wildcard_id};
    
    // 1. Try exact day match first
    for (int f = 0; f < 2; ++f) {
        for (int g = 0; g < 2; ++g) {
            for (int r = 0; r < 2; ++r) {
                AllotmentConstraintKey key = {part_id, day, families[f], groups[g], regions[r]};
                auto it = allotment_constraints.find(key);
                if (it != allotment_constraints.end()) {
                    return &(it->second);
                }
            }
        }
    }
    
    // 2. Try week start day match fallback (Monday aligned: Monday is Day 3, 10, 17...)
    int wk_start = (day < 3) ? 0 : 3 + ((day - 3) / 7) * 7;
    if (wk_start != day) {
        for (int f = 0; f < 2; ++f) {
            for (int g = 0; g < 2; ++g) {
                for (int r = 0; r < 2; ++r) {
                    AllotmentConstraintKey key = {part_id, wk_start, families[f], groups[g], regions[r]};
                    auto it = allotment_constraints.find(key);
                    if (it != allotment_constraints.end()) {
                        return &(it->second);
                    }
                }
            }
        }
    }
    return nullptr;
}

struct AllotmentRollbackGuard {
    std::vector<std::pair<AllotmentState*, double>>& allocs;
    std::vector<std::pair<AllotmentState*, double>>& blocked;
    size_t alloc_start;
    size_t blocked_start;
    bool committed;
    
    AllotmentRollbackGuard(std::vector<std::pair<AllotmentState*, double>>& a,
                           std::vector<std::pair<AllotmentState*, double>>& b,
                           size_t as, size_t bs)
        : allocs(a), blocked(b), alloc_start(as), blocked_start(bs), committed(false) {}
        
    void commit() { committed = true; }
    
    ~AllotmentRollbackGuard() {
        if (!committed) {
            allocs.resize(alloc_start);
            blocked.resize(blocked_start);
        }
    }
};



inline uint64_t encode_composite_priority(bool is_committed, int customer_tier, int due_day, int original_priority, double revenue) {
    uint64_t committed_bit = is_committed ? 0ULL : 1ULL;
    uint64_t tier_val = 3ULL;
    if (customer_tier == 1) tier_val = 0ULL;
    else if (customer_tier == 2) tier_val = 1ULL;
    else if (customer_tier == 3) tier_val = 2ULL;
    uint64_t due_val = static_cast<uint64_t>(std::max(0, std::min(65535, due_day)));
    uint64_t pri_val = static_cast<uint64_t>(std::max(0, std::min(65535, original_priority)));
    uint64_t max_rev = 268435455ULL;
    uint64_t rev_val = max_rev - std::min(max_rev, static_cast<uint64_t>(revenue));
    return (committed_bit << 62) | (tier_val << 60) | (due_val << 44) | (pri_val << 28) | rev_val;
}

// 4. LscTreeNode
struct LscTreeNode {
    uint32_t root_part_id;
    double root_dimension_val;
    uint32_t node_part_id;
    uint32_t parent_part_id;
    double per_qty;
    double root_per_qty;
    int cumulative_lt;
    uint32_t node_level;
    bool is_leaf;
};

// 5. PlannedOrder
struct PlannedOrder {
    uint32_t part_id;
    double qty;
    int start_day;
    int finish_day;
    double dimension_val;
    int original_lbl_start = -1;
    int original_lbl_finish = -1;
};

// PlannedOrderSplit for cascading DBD scheduling transactions
struct PlannedOrderSplit {
    size_t original_order_idx;
    double qty;
    int finish_day;
    int start_day;
    double capacity;
    double routing_cost;
};

// 6. AlternateAllocationRecord
struct AlternateAllocationRecord {
    uint32_t demand_id;
    uint32_t main_part_id;
    uint32_t alt_part_id;
    double allocated_qty;
    int day;
    int alt_class;
};

// 7. PeggingRecord (if needed elsewhere)
struct PeggingRecord {
    uint32_t demand_id;
    uint32_t part_id;
    double qty;
    int day;
};

// 8. SwapRecord (for incomplete substitutions)
struct SwapRecord {
    std::string demand_code;
    std::string from_part;
    std::string to_part;
    double swapped_qty;
    int day;
    std::string alt_group;
    std::string swap_reason;
};

// 9. Shared Global Constraint Resource
struct ConstraintRecord {
    uint32_t constraint_id;
    std::string constraint_code;
    std::string constraint_type; // "Constrained", "LoadOnly", "Unconstrained"
    std::vector<double> rates;    // Time-phased daily available capacity
    std::vector<double> allocated_rates;
};

// 10. Many-to-Many Routing Mapping (SourceConstraint)
struct ConstraintConsumption {
    uint32_t constraint_id;
    double factor;
};

struct AlternativeRouting {
    uint32_t routing_id;
    std::vector<ConstraintConsumption> constraints;
    double routing_cost = 0.0;
    int priority = 0;
};

struct SourceConstraintRecord {
    uint32_t part_id;
    uint32_t constraint_id;
    double constraint_factor;
    double before_fixed_factor; // Setup overhead
    double after_fixed_factor;  // Clean-up overhead
    
    // Extensions for Phase 2: Multi-Constraint & Alternative Routing
    std::vector<ConstraintConsumption> extra_constraints;
    std::vector<AlternativeRouting> alternative_routings;
};

// 11. Project WBS Task Network Definitions for ETO
enum class DependencyType : uint8_t {
    FS = 0,
    FF = 1,
    SS = 2,
    SF = 3
};

struct TaskDependency {
    uint32_t predecessor_task_id;
    DependencyType dep_type;
    int lag_days = 0;
};

struct ProjectTaskRecord {
    uint32_t task_id;
    uint32_t project_id;
    std::string task_name;
    double duration = 0.0;
    
    // CPM variables
    int early_start = 0;
    int early_finish = 0;
    int late_start = 0;
    int late_finish = 0;
    bool is_critical_path = false;
    
    uint32_t output_part_id = uint32_t(-1); // Maps to a SKU if task produces/requires stock
    std::vector<TaskDependency> dependencies;
};

// 12. CalendarRecord for SCM Factory Calendars
struct CalendarRecord {
    std::string calendar_name;
    std::vector<bool> working_days; // index = planning day, value = true (working), false (holiday)
};

// 13. ProcurementGroupRecord for incomplete substitution
struct ProcurementGroupRecord {
    std::string pg_id;
    std::string part_code;
    uint32_t part_id = uint32_t(-1);
    std::string site_code;
    double target_ratio = 0.0;
};

// 14. SupplyAssignmentRecord for database pegging storage
struct SupplyAssignmentRecord {
    std::string demand_code;
    std::string ind_part;
    std::string part;
    double assigned_qty;
    std::string supply_code;
    std::string supply_type; // "On-Hand", "In-Transit", "Planned-Order"
    int due_day;
    double dimension_val;
};

// 15. CustomerCombRecord for planning BOM combination overrides
struct CustomerCombRecord {
    std::string part;
    std::string customer;
    std::string region;
    double ratio = 0.0;
    double ratio_override = -1.0;
};

} // namespace ipc


