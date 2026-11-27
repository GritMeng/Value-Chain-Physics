#pragma once
#include "types.h"
#include <cstdint>
#include <string>
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

// 连续平坦开放寻址防波堤表（零逐节点堆分配，支持 500,000+ 并发需求毫秒级协同）
struct FlatQuotaFirewallTable {
  struct Slot {
    uint64_t packed_key = UINT64_MAX;
    double total_quota = 0.0;
    double consumed_qty = 0.0;
  };
  std::vector<Slot> slots;
  size_t active_buckets = 0;
  double total_quota_sum = 0.0;

  void init_pow2(size_t expected_keys);
  Slot &upsert(uint64_t key);
  Slot *find(uint64_t key);
};

FlatQuotaFirewallTable generate_itp_flat_allotments(
    const std::vector<DemandOrderRecord> &master_demands,
    double capacity_buffer_factor = 1.1);

IOPExecutionResult run_iop_flat_alignment(
    const std::vector<DemandOrderRecord> &execution_demands,
    FlatQuotaFirewallTable &flat_firewalls);

// 通用 N 区域 × 多子件齐套 -> 最紧瓶颈识别 -> 战略保底 + 触顶循环注水公平切分 -> 逆向重展 -> 独立桶 IOP
struct RegionalDemandStream {
  std::string region_code;
  uint32_t strategic_rank = 1;           // 数值越小战略优先级越高
  double forecast_qty = 0.0;             // 中期预测毛需求
  double strategic_floor_qty = 0.0;      // 战略保底锁定量
  std::vector<double> dedicated_caps;    // 各专用受限子件可支撑的最大成套上限
  double non_bottleneck_usage = 1.0;     // 非瓶颈外购件单耗
  double exec_arriving_qty = 0.0;        // 执行期实际到达需求
};

struct SharedComponentPool {
  std::string part_code;
  double available_supply = 0.0;
  double unit_usage = 1.0;               // 每单位父件消耗量
};

struct RegionalStreamAllotment {
  std::string region_code;
  double kitted_ceiling = 0.0;
  double granted_strategic_floor = 0.0;
  double granted_fair_share = 0.0;
  double final_allotment = 0.0;
  double renetted_non_bottleneck_buy = 0.0;
  double ipc_fulfilled_qty = 0.0;
  double ipc_blocked_qty = 0.0;
  double legacy_unprotected_fulfilled_qty = 0.0;
  double legacy_starved_shortage_qty = 0.0;
};

struct MultiRegionAllotmentResult {
  bool has_bottleneck = false;
  std::string most_gating_part_code;
  double most_gating_scarcity_ratio = 1.0;
  double total_kitted_demand = 0.0;
  double total_allotment = 0.0;
  double legacy_total_non_bottleneck_buy = 0.0;
  double ipc_total_renetted_buy = 0.0;
  double total_excess_buy_eliminated = 0.0;
  std::vector<RegionalStreamAllotment> region_outcomes;
};

MultiRegionAllotmentResult evaluate_multi_region_most_gating_allotment(
    const std::vector<RegionalDemandStream> &streams,
    const std::vector<SharedComponentPool> &shared_pools);

// 双区域标准无菌算例便捷入口（内部直接调用通用多区域注水切分引擎）
struct RegionalAllotmentInput {
  double forecast_a = 6000.0;
  double dedicated_supply_a = 5000.0;
  double strategic_floor_a = 2000.0;
  double exec_order_a = 4500.0;
  double forecast_d = 3000.0;
  double dedicated_supply_d = 3000.0;
  double strategic_floor_d = 0.0;
  double exec_order_d = 2000.0;
  double shared_chip_supply = 6000.0;
};

struct ClosedLoopAllotmentResult {
  double kitted_a = 0.0;
  double kitted_d = 0.0;
  bool is_most_gating = false;
  double allotment_a = 0.0;
  double allotment_d = 0.0;
  double legacy_non_bottleneck_buy = 0.0;
  double ipc_renetted_buy = 0.0;
  double excess_buy_eliminated = 0.0;
  double legacy_fulfilled_d = 0.0;
  double legacy_shortage_d = 0.0;
  double ipc_fulfilled_a = 0.0;
  double ipc_blocked_a = 0.0;
  double ipc_fulfilled_d = 0.0;
  double ipc_shortage_d = 0.0;
};

ClosedLoopAllotmentResult evaluate_itp_kitting_most_gating_closed_loop(
    const RegionalAllotmentInput &in);

} // namespace ipc_core
