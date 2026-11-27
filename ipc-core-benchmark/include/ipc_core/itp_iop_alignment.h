#pragma once
#include "types.h"
#include <unordered_map>
#include <vector>

namespace ipc_core {

std::unordered_map<QuotaFirewallKey, QuotaFirewallState, QuotaFirewallKeyHash>
generate_itp_master_allotments(
    const std::vector<DemandOrderRecord> &master_demands,
    const std::vector<SkuNodeRecord> &parts,
    double capacity_buffer_factor = 1.1);

struct IOPExecutionResult {
  uint32_t total_orders = 0;
  uint32_t scheduled_orders = 0;
  uint32_t blocked_orders = 0;
  double total_fulfilled_qty = 0.0;
  double quota_utilization = 0.0;
};

IOPExecutionResult run_iop_execution_alignment(
    const std::vector<DemandOrderRecord> &execution_demands,
    const std::vector<SkuNodeRecord> &parts,
    std::unordered_map<QuotaFirewallKey, QuotaFirewallState,
                       QuotaFirewallKeyHash> &quota_firewalls);

// 无菌四步闭环算例输入：齐套先行 -> 最紧瓶颈战略保底+公平切分 -> 逆向重展净算 -> 独立配额桶执行
struct RegionalAllotmentInput {
  double forecast_a = 6000.0;         // 区域 A 中期预测毛需求
  double dedicated_supply_a = 5000.0; // 区域 A 专用受限子件可用供给
  double strategic_floor_a = 2000.0;  // 区域 A 战略保底锁定量
  double exec_order_a = 4500.0;       // 区域 A 执行期实际到达需求
  double forecast_d = 3000.0;         // 区域 D 中期预测毛需求
  double dedicated_supply_d = 3000.0; // 区域 D 专用受限子件可用供给
  double strategic_floor_d = 0.0;     // 区域 D 战略保底锁定量
  double exec_order_d = 2000.0;       // 区域 D 执行期实际到达需求
  double shared_chip_supply = 6000.0; // 跨区共用核心件总可用供给
};

struct ClosedLoopAllotmentResult {
  double kitted_a = 0.0;                   // Step 1: 区域 A 专用件齐套封顶有效需求
  double kitted_d = 0.0;                   // Step 1: 区域 D 专用件齐套封顶有效需求
  bool is_most_gating = false;             // Step 2: 共用核心件是否构成最紧瓶颈
  double allotment_a = 0.0;                // Step 2: 区域 A 获批保护配额
  double allotment_d = 0.0;                // Step 2: 区域 D 获批保护配额
  double legacy_non_bottleneck_buy = 0.0;  // 现有系统按原始预测直展的非瓶颈件采购量
  double ipc_renetted_buy = 0.0;           // Step 3: IPC 按配额封顶逆向重展采购量
  double excess_buy_eliminated = 0.0;      // Step 3: 消除的非瓶颈件超买呆滞量
  double legacy_fulfilled_d = 0.0;         // 现有系统无桶隔离下区域 D 实际兑现量
  double legacy_shortage_d = 0.0;          // 现有系统被高优区挤占导致的区域 D 缺口
  double ipc_fulfilled_a = 0.0;            // Step 4: IPC 独立配额桶下区域 A 放行量
  double ipc_blocked_a = 0.0;              // Step 4: IPC 独立配额桶下区域 A 超额拦截量
  double ipc_fulfilled_d = 0.0;            // Step 4: IPC 独立配额桶下区域 D 放行量
  double ipc_shortage_d = 0.0;             // Step 4: IPC 独立配额桶下区域 D 缺口 (0)
};

ClosedLoopAllotmentResult evaluate_itp_kitting_most_gating_closed_loop(
    const RegionalAllotmentInput &in);

} // namespace ipc_core
