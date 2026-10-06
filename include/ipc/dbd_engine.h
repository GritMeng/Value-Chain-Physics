#pragma once
#include "ipc_types.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace ipc {

    struct ATPSupplyNode {
        std::string supply_code;
        std::string supply_type; // "On-Hand", "SR", "Planned-Order"
        uint32_t part_id;
        int available_day;
        double qty;
        double allocated_qty = 0.0;
        uint64_t priority = 99999ULL;
        int planned_order_index = -1;
    };

    struct WcAllocation {
        std::string work_center;
        int day;
        double hours;
    };

    bool reserve_atp_and_capacity_recursive(
        uint32_t part_id,
        int due_day,
        double qty,
        uint64_t priority,
        double dimension_val,
        std::vector<std::vector<ATPSupplyNode>>& atp_supplies,
        std::vector<ConstraintRecord>& shared_constraints,
        const std::vector<PartSiteRecord>& parts,
        const std::vector<FlatBomItem>& boms,
        const std::vector<PartBomInfo>& part_bom_info,
        const std::vector<SourceConstraintRecord>& source_constraints,
        std::vector<ATPSupplyNode*>& temp_allocations,
        std::vector<double>& temp_alloc_qty,
        std::vector<std::pair<size_t, std::pair<int, double>>>& temp_capacity_allocations,
        const std::string& preference_mode,
        std::vector<double>& bom_ltb_consumed,
        std::vector<std::pair<size_t, double>>& temp_ltb_allocations,
        std::unordered_map<uint32_t, int>& active_mix_groups,
        const std::vector<std::vector<double>>& last_dim_val,
        double& chosen_routing_cost,
        std::vector<bool>& active_parts,
        std::vector<PlannedOrderSplit>& temp_po_splits = *reinterpret_cast<std::vector<PlannedOrderSplit>*>(0),
        bool is_recursive_child = false,
        std::vector<WcAllocation>& temp_wc_allocations = *reinterpret_cast<std::vector<WcAllocation>*>(0),
        int root_due_day = -1,
        uint32_t root_family_id = uint32_t(-1),
        uint32_t root_cust_group_id = uint32_t(-1),
        uint32_t root_region_id = uint32_t(-1),
        uint32_t wildcard_id = uint32_t(-1),
        std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints = *reinterpret_cast<std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>*>(0),
        std::vector<std::pair<AllotmentState*, double>>& temp_allotment_allocations = *reinterpret_cast<std::vector<std::pair<AllotmentState*, double>>*>(0),
        std::vector<std::pair<AllotmentState*, double>>& temp_allotment_blocked = *reinterpret_cast<std::vector<std::pair<AllotmentState*, double>>*>(0),
        bool skip_po_quota = false
    );

    void run_dbd_dispatch_engine(
        const std::vector<PartSiteRecord>& parts,
        const std::vector<FlatBomItem>& boms,
        const std::vector<PlannedOrder>& ipc_planned_orders,
        const std::vector<IndependentDemand>& demands,
        std::vector<PlannedOrder>& out_scheduled_orders,
        std::vector<double>& out_allocated_rates,
        std::vector<double>& out_order_capacities,
        std::vector<double>& out_order_routing_costs,
        const std::string& solver_mode = "iop",
        uint32_t wildcard_id = uint32_t(-1),
        std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints = *reinterpret_cast<std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>*>(0)
    );
}
