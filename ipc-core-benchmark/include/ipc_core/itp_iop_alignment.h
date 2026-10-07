#pragma once
#include "types.h"
#include <vector>
#include <unordered_map>

namespace ipc_core {

// 主计划 (ITP) 生成阶段：设定宏观产能防波堤与防漂移配额
std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> generate_itp_master_allotments(
    const std::vector<IndependentDemand>& master_demands,
    const std::vector<PartSiteRecord>& parts,
    double capacity_buffer_factor = 1.1 // 10% 产能缓冲防波堤
);

// 执行计划 (IOP) 派程阶段：在刚性配额阻断与优先顺序约束下求解订单分配
struct IOPExecutionResult {
    uint32_t total_orders = 0;
    uint32_t scheduled_orders = 0;
    uint32_t blocked_orders = 0;
    double total_fulfilled_qty = 0.0;
    double quota_utilization = 0.0;
};

IOPExecutionResult run_iop_execution_alignment(
    const std::vector<IndependentDemand>& execution_demands,
    const std::vector<PartSiteRecord>& parts,
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>& allotment_constraints
);

} // namespace ipc_core
