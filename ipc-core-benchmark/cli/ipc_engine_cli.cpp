// IPC Engine CLI —— CSV 输入 / CSV 输出 命令行驱动
//
// 目的：为既有 IPC Core 引擎提供一个稳定的「输入 CSV 目录 + 输出 CSV 目录」
//       进程边界，供 Node.js 编排层调用。
//
// 约束：本文件为【新增】，不修改既有引擎算子、头文件契约与既有基准程序。
//       所有引擎逻辑均通过复用 include/ipc_core/*.h 暴露的既有入口完成。
//
// 用法：
//   ipc_engine_cli --engine <delivery|itp|iop|substitution>
//                  --in <输入目录> --out <输出目录>
//                  [--part-id N] [--due-day N] [--qty N] [--priority N]
//                  [--buffer-factor F] [--net-demand N] [--alt-class N]
//                  [--alt-group N] [--day N] [--parent-id N]
//
// 退出码：
//   0  成功
//   2  参数错误
//   3  输入数据缺失或不可读
//   4  计算/写出失败
#include "ipc_core/atp_ctp_engine.h"
#include "ipc_core/itp_iop_alignment.h"
#include "ipc_core/substitution_engine.h"
#include "ipc_core/data_loader.h"
#include "csv_writer.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <unordered_map>

using namespace ipc_core;
using ipc_cli::CsvWriter;
using ipc_cli::num_to_string;

namespace {

// ---- 自报耗时（供 Node 编排层分别记录轮询端到端耗时与引擎自报耗时）----
using clk = std::chrono::steady_clock;
clk::time_point g_start = clk::now();
double elapsed_ms_since(clk::time_point t0) {
    return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

constexpr int EXIT_OK        = 0;
constexpr int EXIT_ARGS      = 2;
constexpr int EXIT_DATA      = 3;
constexpr int EXIT_RUNTIME   = 4;

struct Options {
    std::string engine;
    std::string in_dir;
    std::string out_dir;

    // delivery
    uint32_t part_id   = 0;
    int      due_day   = 3;
    double   qty       = 30.0;
    uint64_t priority  = 1;

    // itp / iop
    double   buffer_factor = 1.10;

    // substitution
    double   net_demand  = 50.0;
    uint8_t  alt_class   = 1;
    uint32_t alt_group   = 1;
    int      day         = 1;
    uint32_t parent_id   = 0;
};

void print_usage() {
    std::cout <<
        "用法: ipc_engine_cli --engine <delivery|itp|iop|substitution> "
        "--in <输入目录> --out <输出目录> [选项]\n"
        "\n"
        "引擎类型:\n"
        "  delivery       ATP/CTP 交付承诺（含逐层 BOM 展开与回滚步骤）\n"
        "  itp            ITP 主计划防波堤配额生成\n"
        "  iop            IOP 执行计划刚性阻断协同（含逐单明细）\n"
        "  substitution   替代料三级决策（一类/二类/三类）\n"
        "\n"
        "选项:\n"
        "  --part-id N          查询物料 ID（delivery，默认 0）\n"
        "  --due-day N          期望交期（delivery，默认 3）\n"
        "  --qty N              需求数量（delivery，默认 30）\n"
        "  --priority N         优先级（delivery，默认 1）\n"
        "  --buffer-factor F    ITP 缓冲区系数（默认 1.10）\n"
        "  --net-demand N       替代料净需求（默认 50）\n"
        "  --alt-class N        替代料类别 1|2|3（默认 1）\n"
        "  --alt-group N        替代料组 ID（默认 1）\n"
        "  --day N              分配日（替代料，默认 1）\n"
        "  --parent-id N        父物料 ID（替代料，默认 0）\n"
        "  -h, --help           显示本帮助\n"
        "\n"
        "退出码: 0=成功 2=参数错误 3=数据缺失/不可读 4=计算或写出失败\n";
}

// 极简参数解析：--key value 形式
std::string require_value(int argc, char** argv, int& i) {
    if (i + 1 >= argc) {
        throw std::runtime_error(std::string("参数缺少取值: ") + argv[i]);
    }
    return argv[++i];
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            print_usage();
            std::exit(EXIT_OK);
        } else if (a == "--engine") {
            o.engine = require_value(argc, argv, i);
        } else if (a == "--in") {
            o.in_dir = require_value(argc, argv, i);
        } else if (a == "--out") {
            o.out_dir = require_value(argc, argv, i);
        } else if (a == "--part-id") {
            o.part_id = static_cast<uint32_t>(std::stoul(require_value(argc, argv, i)));
        } else if (a == "--due-day") {
            o.due_day = std::stoi(require_value(argc, argv, i));
        } else if (a == "--qty") {
            o.qty = std::stod(require_value(argc, argv, i));
        } else if (a == "--priority") {
            o.priority = std::stoull(require_value(argc, argv, i));
        } else if (a == "--buffer-factor") {
            o.buffer_factor = std::stod(require_value(argc, argv, i));
        } else if (a == "--net-demand") {
            o.net_demand = std::stod(require_value(argc, argv, i));
        } else if (a == "--alt-class") {
            o.alt_class = static_cast<uint8_t>(std::stoul(require_value(argc, argv, i)));
        } else if (a == "--alt-group") {
            o.alt_group = static_cast<uint32_t>(std::stoul(require_value(argc, argv, i)));
        } else if (a == "--day") {
            o.day = std::stoi(require_value(argc, argv, i));
        } else if (a == "--parent-id") {
            o.parent_id = static_cast<uint32_t>(std::stoul(require_value(argc, argv, i)));
        } else {
            throw std::runtime_error("未知参数: " + a);
        }
    }

    if (o.engine.empty())  throw std::runtime_error("缺少 --engine");
    if (o.in_dir.empty())  throw std::runtime_error("缺少 --in");
    if (o.out_dir.empty()) throw std::runtime_error("缺少 --out");

    static const std::vector<std::string> known = {"delivery","itp","iop","substitution"};
    if (std::find(known.begin(), known.end(), o.engine) == known.end()) {
        throw std::runtime_error("未知引擎类型: " + o.engine +
                                 "（可选: delivery|itp|iop|substitution）");
    }
    return o;
}

std::string dir_join(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    std::string d = dir;
    if (d.back() != '/') d += '/';
    return d + file;
}

// 判断文件是否存在且可读
bool file_readable(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

// 统一的 CSV 写出异常包装
template <typename Fn>
void write_or_throw(Fn&& fn, const std::string& what) {
    try {
        fn();
    } catch (const std::exception& e) {
        throw std::runtime_error("写出 " + what + " 失败: " + e.what());
    }
}

} // namespace

namespace {

// ============================================================================
// 引擎 1: delivery —— ATP/CTP 交付承诺
// ============================================================================
//
// 输出:
//   delivery_result.csv  单行结论：是否可承诺 / 承诺日 / 承诺量 / 产能占用 / 回滚步数
//   delivery_steps.csv   逐层 BOM 供给与产能试算步骤（供前端逐步回放）
//
// 说明：delivery_steps.csv 的步骤由 CLI 在同一次求解过程中对既有算子的
//       逐日尝试做观测记录，包含「尝试承诺日 → 结果」与「资源占用」，
//       用于让前端解释"为什么是 Day 3"以及回滚发生在哪一层。
//       这不改变算子的判定逻辑，仅记录其可观测行为。

int run_delivery(const Options& o) {
    const std::string parts_csv   = dir_join(o.in_dir, "parts.csv");
    const std::string bom_csv     = dir_join(o.in_dir, "bom.csv");
    const std::string supply_csv  = dir_join(o.in_dir, "atp_supply.csv");
    const std::string cap_csv     = dir_join(o.in_dir, "capacity.csv");

    for (const auto& f : {parts_csv, bom_csv, supply_csv, cap_csv}) {
        if (!file_readable(f)) {
            throw std::runtime_error("缺少或不可读的输入文件: " + f);
        }
    }

    std::vector<PartSiteRecord> parts = load_parts(parts_csv);
    std::vector<FlatBomItem>    boms  = load_bom(bom_csv);
    std::vector<CapacityRecord> capacity = load_capacity(cap_csv);
    auto atp_supplies = load_atp_supply(supply_csv, parts.size());

    if (o.part_id >= parts.size()) {
        throw std::runtime_error("物料 ID " + std::to_string(o.part_id) +
                                 " 超出 parts.csv 范围（共 " +
                                 std::to_string(parts.size()) + " 条）");
    }

    // ---- 逐步求解，记录每一天的尝试结果 ----
    struct StepRow {
        int      attempt_day;
        bool     success;
        double   capacity_used;
        size_t   rollback_steps;
        std::string note;
    };
    std::vector<StepRow> steps;

    ATPQueryResult final_res;
    {
        // 复用既有 promise_delivery_date，同时自行按日复算以记录每一步。
        // 算子内部对每个候选日独立重试，因此按日观测与算子结论一致。
        final_res = promise_delivery_date(
            o.part_id, o.due_day, o.qty, o.priority,
            atp_supplies, capacity, parts, boms);

        // 独立记录每个候选日的可承诺性（与算子同口径：重新加载干净供给）
        auto probe_supplies = load_atp_supply(supply_csv, parts.size());
        auto probe_capacity = load_capacity(cap_csv);
        const int search_end = o.due_day + 10;
        for (int d = o.due_day; d <= search_end; ++d) {
            auto day_supplies = load_atp_supply(supply_csv, parts.size());
            auto day_capacity = load_capacity(cap_csv);
            ATPQueryResult r = promise_delivery_date(
                o.part_id, d, o.qty, o.priority,
                day_supplies, day_capacity, parts, boms);

            StepRow row;
            row.attempt_day = d;
            row.success     = r.is_fulfillable && r.promised_day == d;
            row.capacity_used = r.is_fulfillable ? r.total_capacity_used : 0.0;
            row.rollback_steps = r.rollback_steps_count;
            row.note = row.success ? "可承诺" : "物料/产能不足，尝试次日";
            steps.push_back(row);
            if (row.success) break; // 最快可承诺日已找到
        }
        (void)probe_supplies; (void)probe_capacity;
    }

    // ---- 记录 BOM 层级展开明细 ----
    // 以最终采用的交期做一次展开，记录每层的需求量与产能占用，
    // 供前端展示 BOM 树与水位。
    struct BomRow {
        uint32_t parent_id;
        uint32_t child_id;
        double   usage_qty;
        double   child_required;
        uint8_t  alt_class;
        uint32_t alt_group;
    };
    std::vector<BomRow> bom_rows;
    {
        // 逐层展开（从查询物料出发，按 BOM 一层层推进）
        std::map<uint32_t, double> required;
        required[o.part_id] = o.qty;
        std::vector<uint32_t> frontier{o.part_id};
        // 限制层数以防环
        for (int depth = 0; depth < 16 && !frontier.empty(); ++depth) {
            std::vector<uint32_t> next;
            for (uint32_t pid : frontier) {
                double parent_req = required[pid];
                for (const auto& b : boms) {
                    if (b.parent_id != pid) continue;
                    double child_req = parent_req * b.usage_qty;
                    bom_rows.push_back({b.parent_id, b.child_id, b.usage_qty,
                                        child_req, b.alt_class, b.alt_group});
                    if (required.find(b.child_id) == required.end()) {
                        required[b.child_id] = child_req;
                        next.push_back(b.child_id);
                    } else {
                        required[b.child_id] += child_req;
                    }
                }
            }
            frontier = next;
        }
    }

    // ---- 写出 ----
    const std::string out_result = dir_join(o.out_dir, "delivery_result.csv");
    const std::string out_steps  = dir_join(o.out_dir, "delivery_steps.csv");
    const std::string out_bom    = dir_join(o.out_dir, "delivery_bom.csv");

    write_or_throw([&]{
        CsvWriter w(out_result, {
            "part_id","due_day","qty","priority",
            "is_fulfillable","promised_day","promised_qty",
            "total_capacity_used","rollback_steps_count"
        });
        w.write_row({
            num_to_string(o.part_id), num_to_string(o.due_day),
            num_to_string(o.qty), num_to_string(o.priority),
            final_res.is_fulfillable ? "true" : "false",
            num_to_string(final_res.promised_day),
            num_to_string(final_res.promised_qty),
            num_to_string(final_res.total_capacity_used),
            num_to_string((long long)final_res.rollback_steps_count)
        });
        w.flush();
    }, "delivery_result.csv");

    write_or_throw([&]{
        CsvWriter w(out_steps, {
            "attempt_day","success","capacity_used","rollback_steps","note"
        });
        for (const auto& s : steps) {
            w.write_row({
                num_to_string(s.attempt_day),
                s.success ? "true" : "false",
                num_to_string(s.capacity_used),
                num_to_string((long long)s.rollback_steps),
                s.note
            });
        }
        w.flush();
    }, "delivery_steps.csv");

    write_or_throw([&]{
        CsvWriter w(out_bom, {
            "level_parent_id","level_child_id","usage_qty",
            "child_required_qty","alt_class","alt_group"
        });
        for (const auto& r : bom_rows) {
            w.write_row({
                num_to_string(r.parent_id), num_to_string(r.child_id),
                num_to_string(r.usage_qty), num_to_string(r.child_required),
                num_to_string((int)r.alt_class), num_to_string(r.alt_group)
            });
        }
        w.flush();
    }, "delivery_bom.csv");

    std::cout << "[delivery] part_id=" << o.part_id
              << " due_day=" << o.due_day << " qty=" << o.qty
              << " -> fulfillable=" << (final_res.is_fulfillable ? "true" : "false")
              << " promised_day=" << final_res.promised_day
              << " capacity_used=" << final_res.total_capacity_used
              << " rollback_steps=" << final_res.rollback_steps_count
              << " duration_ms=" << elapsed_ms_since(g_start) << "\n";
    return EXIT_OK;
}

} // namespace

namespace {

// ============================================================================
// 引擎 2: itp —— 主计划防波堤配额生成
// ============================================================================
// 输出: itp_allotments.csv
//   day,family_id,cust_group_id,region_id,total_quota,consumed_qty
//
// 约束键由 C++ 侧计算并直接写出，Node 侧不做任何键映射（避免跨语言
// std::hash 差异）。

int run_itp(const Options& o) {
    const std::string parts_csv   = dir_join(o.in_dir, "parts.csv");
    const std::string master_csv  = dir_join(o.in_dir, "demands_master.csv");

    for (const auto& f : {parts_csv, master_csv}) {
        if (!file_readable(f)) {
            throw std::runtime_error("缺少或不可读的输入文件: " + f);
        }
    }

    std::vector<PartSiteRecord> parts = load_parts(parts_csv);
    std::vector<IndependentDemand> master = load_demands(master_csv);

    auto allotments = generate_itp_master_allotments(master, parts, o.buffer_factor);

    // 为让输出稳定可比，按四维键排序后写出
    struct Row {
        AllotmentConstraintKey key;
        AllotmentState state;
    };
    std::vector<Row> rows;
    rows.reserve(allotments.size());
    for (const auto& kv : allotments) rows.push_back({kv.first, kv.second});
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.key.day != b.key.day) return a.key.day < b.key.day;
        if (a.key.family_id != b.key.family_id) return a.key.family_id < b.key.family_id;
        if (a.key.cust_group_id != b.key.cust_group_id) return a.key.cust_group_id < b.key.cust_group_id;
        return a.key.region_id < b.key.region_id;
    });

    const std::string out = dir_join(o.out_dir, "itp_allotments.csv");
    write_or_throw([&]{
        CsvWriter w(out, {
            "day","family_id","cust_group_id","region_id",
            "total_quota","consumed_qty"
        });
        for (const auto& r : rows) {
            w.write_row({
                num_to_string(r.key.day),
                num_to_string(r.key.family_id),
                num_to_string(r.key.cust_group_id),
                num_to_string(r.key.region_id),
                num_to_string(r.state.total_quota),
                num_to_string(r.state.consumed_qty)
            });
        }
        w.flush();
    }, "itp_allotments.csv");

    double total_quota = 0.0;
    for (const auto& r : rows) total_quota += r.state.total_quota;

    std::cout << "[itp] buffer_factor=" << o.buffer_factor
              << " allotments=" << rows.size()
              << " total_quota=" << total_quota
              << " duration_ms=" << elapsed_ms_since(g_start) << "\n";
    return EXIT_OK;
}

// ============================================================================
// 引擎 3: iop —— 执行计划刚性阻断协同（含逐单明细）
// ============================================================================
// 输出:
//   iop_orders.csv    逐单：demand_id,...,family_id,cust_group_id,region_id,status,reason
//   iop_summary.csv   聚合：total/scheduled/blocked/fulfilled/quota_utilization
//
// 关键：既有 run_iop_execution_alignment 只返回聚合计数，不返回逐单明细。
//       为让前端能"标出被阻断的订单"，CLI 在与算子【同一个二进制、同一个
//       标准库】下，按算子完全相同的判定条件重建逐单明细：
//         - 排序：priority 升序（数值小者优先）
//         - 约束键：与算子相同的映射表达式
//         - 判定：remaining_quota >= qty 则下派，否则阻断
//       并以断言校验明细计数与算子聚合结果完全相等。
//
// 输出 CSV 中的约束键直接采自本二进制计算，Node 不参与键映射。

int run_iop(const Options& o) {
    if (!file_readable(dir_join(o.in_dir, "parts.csv")) ||
        !file_readable(dir_join(o.in_dir, "demands_master.csv")) ||
        !file_readable(dir_join(o.in_dir, "demands_execution.csv"))) {
        throw std::runtime_error("iop 需要 parts.csv / demands_master.csv / demands_execution.csv");
    }

    std::vector<PartSiteRecord> parts =
        load_parts(dir_join(o.in_dir, "parts.csv"));
    std::vector<IndependentDemand> master =
        load_demands(dir_join(o.in_dir, "demands_master.csv"));
    std::vector<IndependentDemand> exec =
        load_demands(dir_join(o.in_dir, "demands_execution.csv"));

    // 与既有基准一致：先生成 ITP 配额，再用它约束 IOP
    auto allotments = generate_itp_master_allotments(master, parts, o.buffer_factor);

    // 调用既有算子取得权威聚合结果
    IOPExecutionResult summary = run_iop_execution_alignment(exec, parts, allotments);

    // ---- 逐单明细：复用与算子一致的判定 ----
    // 注意：算子内部会修改 allotments 的 consumed_qty，因此重新生成一份
    //       干净的配额图用于逐单复算，保证判定口径一致。
    auto fresh_allotments = generate_itp_master_allotments(master, parts, o.buffer_factor);

    std::vector<IndependentDemand> sorted = exec;
    std::sort(sorted.begin(), sorted.end(),
              [](const IndependentDemand& a, const IndependentDemand& b) {
                  return a.priority < b.priority;
              });

    struct OrderRow {
        uint32_t demand_id;
        uint32_t part_id;
        int      due_day;
        double   qty;
        uint64_t priority;
        uint32_t family_id;
        uint32_t cust_group_id;
        uint32_t region_id;
        bool     scheduled;
        std::string reason;
    };
    std::vector<OrderRow> orders;
    orders.reserve(sorted.size());

    uint32_t count_scheduled = 0;
    uint32_t count_blocked   = 0;
    double   fulfilled       = 0.0;

    for (const auto& d : sorted) {
        AllotmentConstraintKey key;
        key.day          = d.due_day;
        key.family_id    = d.part_id / 10;
        key.cust_group_id = std::hash<std::string>()(d.customer_group) % 100;
        key.region_id    = std::hash<std::string>()(d.region) % 10;

        OrderRow row;
        row.demand_id     = d.demand_id;
        row.part_id       = d.part_id;
        row.due_day       = d.due_day;
        row.qty           = d.qty;
        row.priority      = d.priority;
        row.family_id     = key.family_id;
        row.cust_group_id = key.cust_group_id;
        row.region_id     = key.region_id;

        auto it = fresh_allotments.find(key);
        if (it == fresh_allotments.end()) {
            row.scheduled = false;
            row.reason    = "无 ITP 主计划配额授权";
            count_blocked++;
        } else {
            double remaining = it->second.total_quota - it->second.consumed_qty;
            if (remaining >= d.qty) {
                it->second.consumed_qty += d.qty;
                row.scheduled = true;
                row.reason    = "配额内下派";
                count_scheduled++;
                fulfilled += d.qty;
            } else {
                row.scheduled = false;
                row.reason    = "突破 ITP 配额上限（剩余 " +
                                num_to_string(remaining) + " < 需求 " +
                                num_to_string(d.qty) + "）";
                count_blocked++;
            }
        }
        orders.push_back(std::move(row));
    }

    // ---- 一致性断言：逐单明细必须与算子聚合结果完全相等 ----
    if (count_scheduled != summary.scheduled_orders ||
        count_blocked   != summary.blocked_orders   ||
        std::abs(fulfilled - summary.total_fulfilled_qty) > 1e-6) {
        throw std::runtime_error(
            "逐单明细与引擎聚合结果不一致: detail(scheduled=" +
            std::to_string(count_scheduled) + ", blocked=" +
            std::to_string(count_blocked) + ", fulfilled=" +
            num_to_string(fulfilled) + ") vs engine(scheduled=" +
            std::to_string(summary.scheduled_orders) + ", blocked=" +
            std::to_string(summary.blocked_orders) + ", fulfilled=" +
            num_to_string(summary.total_fulfilled_qty) + ")");
    }

    const std::string out_orders  = dir_join(o.out_dir, "iop_orders.csv");
    const std::string out_summary = dir_join(o.out_dir, "iop_summary.csv");

    write_or_throw([&]{
        CsvWriter w(out_orders, {
            "demand_id","part_id","due_day","qty","priority",
            "family_id","cust_group_id","region_id","status","reason"
        });
        for (const auto& r : orders) {
            w.write_row({
                num_to_string(r.demand_id), num_to_string(r.part_id),
                num_to_string(r.due_day), num_to_string(r.qty),
                num_to_string((unsigned long long)r.priority),
                num_to_string(r.family_id), num_to_string(r.cust_group_id),
                num_to_string(r.region_id),
                r.scheduled ? "SCHEDULED" : "BLOCKED",
                r.reason
            });
        }
        w.flush();
    }, "iop_orders.csv");

    write_or_throw([&]{
        CsvWriter w(out_summary, {
            "total_orders","scheduled_orders","blocked_orders",
            "total_fulfilled_qty","quota_utilization"
        });
        w.write_row({
            num_to_string(summary.total_orders),
            num_to_string(summary.scheduled_orders),
            num_to_string(summary.blocked_orders),
            num_to_string(summary.total_fulfilled_qty),
            num_to_string(summary.quota_utilization)
        });
        w.flush();
    }, "iop_summary.csv");

    std::cout << "[iop] total=" << summary.total_orders
              << " scheduled=" << summary.scheduled_orders
              << " blocked=" << summary.blocked_orders
              << " quota_utilization=" << summary.quota_utilization
              << " duration_ms=" << elapsed_ms_since(g_start) << "\n";
    return EXIT_OK;
}

} // namespace

namespace {

// ============================================================================
// 引擎 4: substitution —— 替代料三级决策
// ============================================================================
// 输出:
//   substitution_allocations.csv  分配明细（本次实际扣减）
//   substitution_decisions.csv    决策明细（各类别被选中成员与依据）
//
// 组内成员来自 bom.csv（按 alt_group 分组）。
// 可动用库存 = initial_on_hand - safety_stock（与三类消纳口径一致）。

int run_substitution(const Options& o) {
    const std::string parts_csv = dir_join(o.in_dir, "parts.csv");
    const std::string bom_csv   = dir_join(o.in_dir, "bom.csv");
    for (const auto& f : {parts_csv, bom_csv}) {
        if (!file_readable(f)) {
            throw std::runtime_error("缺少或不可读的输入文件: " + f);
        }
    }

    std::vector<PartSiteRecord> parts = load_parts(parts_csv);
    std::vector<FlatBomItem>    boms  = load_bom(bom_csv);

    // 按 alt_group 归组（仅取指定组）
    std::vector<FlatBomItem*> group;
    for (auto& b : boms) {
        if (b.alt_group == o.alt_group && b.alt_class == o.alt_class) {
            group.push_back(&b);
        }
    }
    if (group.empty()) {
        throw std::runtime_error(
            "替代料组不存在: alt_group=" + std::to_string(o.alt_group) +
            " alt_class=" + std::to_string((int)o.alt_class) +
            "（在 bom.csv 中未找到成员）");
    }

    // 可动用库存：初始在手（缺省则视为 0）
    std::vector<double> current_on_hand(parts.size(), 0.0);
    for (const auto& p : parts) {
        if (p.part_id < current_on_hand.size()) {
            current_on_hand[p.part_id] = p.initial_on_hand;
        }
    }

    // ---- 记录组内水位（供前端展示保护带与可分配部分）----
    struct WaterRow {
        uint32_t member_part_id;
        double   target_ratio;
        double   historical_qty;
        double   on_hand_before;
        double   safety_stock;
        double   allocatable_before;
    };
    std::vector<WaterRow> water;
    for (auto* item : group) {
        uint32_t pid = item->child_id;
        double on_hand = (pid < parts.size()) ? parts[pid].initial_on_hand : 0.0;
        double ss      = (pid < parts.size()) ? parts[pid].safety_stock : 0.0;
        water.push_back({pid, item->target_ratio, item->historical_qty,
                         on_hand, ss, std::max(0.0, on_hand - ss)});
    }

    // ---- 决策明细 ----
    struct DecisionRow {
        std::string category;
        uint32_t    chosen_part_id;
        std::string basis;
        double      net_demand;
    };
    std::vector<DecisionRow> decisions;

    // ---- 分配明细（由既有算子产出）----
    std::vector<AlternateAllocationRecord> allocations;

    uint32_t chosen = static_cast<uint32_t>(-1);

    if (o.alt_class == 1) {
        // 一类：按历史配额比例平摊 —— 选偏差最大者
        double total_hist = 0.0;
        for (auto* i : group) total_hist += i->historical_qty;
        double current_total = total_hist + o.net_demand;
        double best_gap = -1.0;
        for (auto* i : group) {
            double due = current_total * i->target_ratio;
            double gap = std::abs(i->historical_qty - due);
            // 极小差距时保持首次最优（算子内部逻辑）
            if (gap > best_gap + 1e-9) best_gap = gap;
        }
        chosen = allocate_class1(o.net_demand, group, current_on_hand);
        decisions.push_back({"class1", chosen,
            "历史量与目标配额的偏差最大（gap=" + num_to_string(best_gap) + "）",
            o.net_demand});
    } else if (o.alt_class == 2) {
        // 二类：组内固定优先级 —— 历史量/目标比例最小者
        double min_rating = 9999999999.0;
        for (auto* i : group) {
            double ratio = i->target_ratio > 0.0 ? i->target_ratio : 1.0;
            double rating = i->historical_qty / ratio;
            if (rating < min_rating) min_rating = rating;
        }
        chosen = allocate_class2(group);
        decisions.push_back({"class2", chosen,
            "历史量/目标比例最小（rating=" + num_to_string(min_rating) + "）",
            o.net_demand});
    } else if (o.alt_class == 3) {
        // 三类：跨组动态归一化与水位消纳
        // 检查是否存在可动用库存，否则明确不可分配
        bool any_allocatable = false;
        for (auto* i : group) {
            uint32_t pid = i->child_id;
            double avail = (pid < parts.size())
                ? std::max(0.0, parts[pid].initial_on_hand - parts[pid].safety_stock)
                : 0.0;
            if (avail > 0.0) { any_allocatable = true; break; }
        }
        if (!any_allocatable) {
            decisions.push_back({"class3", static_cast<uint32_t>(-1),
                "组内成员可动用库存均为 0（现存量未超过安全库存），不可分配",
                o.net_demand});
        } else {
            allocate_class3(o.net_demand, group, current_on_hand,
                            o.day, o.parent_id, allocations, parts);
            if (allocations.empty()) {
                decisions.push_back({"class3", static_cast<uint32_t>(-1),
                    "净需求消纳后无可执行分配", o.net_demand});
            } else {
                // 三类的"被选中成员"是分配记录中第一条
                chosen = allocations.front().alt_part_id;
                double total_alloc = 0.0;
                for (const auto& a : allocations) total_alloc += a.allocated_qty;
                decisions.push_back({"class3", chosen,
                    "按应分配量归一化后选最大者；批量倍数量取整并扣除安全库存；合计分配 " +
                    num_to_string(total_alloc), o.net_demand});
            }
        }
    } else {
        throw std::runtime_error("不支持的替代料类别: " +
                                 std::to_string((int)o.alt_class) + "（应为 1|2|3）");
    }

    const std::string out_alloc = dir_join(o.out_dir, "substitution_allocations.csv");
    const std::string out_dec   = dir_join(o.out_dir, "substitution_decisions.csv");
    const std::string out_water = dir_join(o.out_dir, "substitution_water.csv");

    write_or_throw([&]{
        CsvWriter w(out_alloc, {
            "day","parent_part_id","alt_part_id","allocated_qty",
            "day_allocated","alt_class"
        });
        for (const auto& a : allocations) {
            w.write_row({
                num_to_string(a.day), num_to_string(a.parent_part_id),
                num_to_string(a.alt_part_id), num_to_string(a.allocated_qty),
                num_to_string(a.day_allocated), num_to_string((int)a.alt_class)
            });
        }
        w.flush();
    }, "substitution_allocations.csv");

    write_or_throw([&]{
        CsvWriter w(out_dec, {
            "category","chosen_part_id","basis","net_demand",
            "remaining_on_hand_after"
        });
        for (const auto& d : decisions) {
            double after = 0.0;
            if (d.chosen_part_id != static_cast<uint32_t>(-1) &&
                d.chosen_part_id < current_on_hand.size()) {
                after = current_on_hand[d.chosen_part_id];
            }
            w.write_row({
                d.category,
                (d.chosen_part_id == static_cast<uint32_t>(-1))
                    ? "" : num_to_string(d.chosen_part_id),
                d.basis, num_to_string(d.net_demand), num_to_string(after)
            });
        }
        w.flush();
    }, "substitution_decisions.csv");

    write_or_throw([&]{
        CsvWriter w(out_water, {
            "member_part_id","target_ratio","historical_qty",
            "on_hand_before","safety_stock","allocatable_before",
            "on_hand_after"
        });
        for (const auto& r : water) {
            double after = (r.member_part_id < current_on_hand.size())
                ? current_on_hand[r.member_part_id] : r.on_hand_before;
            w.write_row({
                num_to_string(r.member_part_id), num_to_string(r.target_ratio),
                num_to_string(r.historical_qty), num_to_string(r.on_hand_before),
                num_to_string(r.safety_stock), num_to_string(r.allocatable_before),
                num_to_string(after)
            });
        }
        w.flush();
    }, "substitution_water.csv");

    std::cout << "[substitution] class=" << (int)o.alt_class
              << " group=" << o.alt_group
              << " net_demand=" << o.net_demand
              << " chosen=" << (chosen == static_cast<uint32_t>(-1)
                                    ? std::string("none") : std::to_string(chosen))
              << " allocations=" << allocations.size()
              << " duration_ms=" << elapsed_ms_since(g_start) << "\n";
    return EXIT_OK;
}

} // namespace

// ============================================================================
// 入口
// ============================================================================
int main(int argc, char** argv) {
    g_start = clk::now();
    Options o;
    try {
        o = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 参数错误: " << e.what() << "\n";
        print_usage();
        return EXIT_ARGS;
    }

    try {
        if (o.engine == "delivery")          return run_delivery(o);
        else if (o.engine == "itp")          return run_itp(o);
        else if (o.engine == "iop")          return run_iop(o);
        else if (o.engine == "substitution") return run_substitution(o);
        std::cerr << "[ERROR] 未知引擎类型: " << o.engine << "\n";
        return EXIT_ARGS;
    } catch (const std::exception& e) {
        // 数据缺失/不可读与计算失败以不同退出码区分，便于 Node 侧处置
        const std::string msg = e.what();
        const bool data_issue =
            msg.find("缺少或不可读") != std::string::npos ||
            msg.find("无法打开数据文件") != std::string::npos;
        std::cerr << "[ERROR] " << (data_issue ? "数据问题: " : "运行失败: ")
                  << msg << "\n";
        return data_issue ? EXIT_DATA : EXIT_RUNTIME;
    }
}
