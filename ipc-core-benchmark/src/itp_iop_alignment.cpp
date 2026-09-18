#include "ipc_core/itp_iop_alignment.h"
#include <algorithm>

namespace ipc_core {

std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> generate_itp_master_allotments(
    const std::vector<IndependentDemand>& master_demands,
    const std::vector<PartSiteRecord>& parts,
    double capacity_buffer_factor
) {
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> allotments;

    for (const auto& demand : master_demands) {
        AllotmentConstraintKey key;
        key.day = demand.due_day;
        key.family_id = demand.part_id / 10; // 假定族 ID 抽象映射
        key.cust_group_id = std::hash<std::string>()(demand.customer_group) % 100;
        key.region_id = std::hash<std::string>()(demand.region) % 10;

        // ITP 主计划根据预设的软约束缓冲区生成防波堤配额
        allotments[key].total_quota += demand.qty * capacity_buffer_factor;
    }

    return allotments;
}

IOPExecutionResult run_iop_execution_alignment(
    const std::vector<IndependentDemand>& execution_demands,
    const std::vector<PartSiteRecord>& parts,
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints
) {
    IOPExecutionResult result;
    result.total_orders = static_cast<uint32_t>(execution_demands.size());

    // 1. 按优先级从高到低对执行需求排序
    std::vector<IndependentDemand> sorted_demands = execution_demands;
    std::sort(sorted_demands.begin(), sorted_demands.end(), [](const IndependentDemand& a, const IndependentDemand& b) {
        return a.priority < b.priority;
    });

    double total_quota_allocated = 0.0;
    double total_quota_consumed = 0.0;
    for (const auto& kv : allotment_constraints) {
        total_quota_allocated += kv.second.total_quota;
    }

    // 2. IOP 严格在 ITP 给出的刚性配额阻断下进行校验扣减
    for (const auto& demand : sorted_demands) {
        AllotmentConstraintKey key;
        key.day = demand.due_day;
        key.family_id = demand.part_id / 10;
        key.cust_group_id = std::hash<std::string>()(demand.customer_group) % 100;
        key.region_id = std::hash<std::string>()(demand.region) % 10;

        auto it = allotment_constraints.find(key);
        if (it != allotment_constraints.end()) {
            double remaining_quota = it->second.total_quota - it->second.consumed_qty;
            if (remaining_quota >= demand.qty) {
                // 配额满足，予以排产
                it->second.consumed_qty += demand.qty;
                result.scheduled_orders++;
                result.total_fulfilled_qty += demand.qty;
            } else {
                // 超出 ITP 配额阻断边界，强制拦截阻断（防止越权抢料与计划漂移）
                result.blocked_orders++;
            }
        } else {
            // 无 ITP 主计划配额授权的需求直接阻断
            result.blocked_orders++;
        }
    }

    for (const auto& kv : allotment_constraints) {
        total_quota_consumed += kv.second.consumed_qty;
    }

    result.quota_utilization = total_quota_allocated > 0.0 ? (total_quota_consumed / total_quota_allocated) : 0.0;
    return result;
}

} // namespace ipc_core
