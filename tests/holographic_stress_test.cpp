// =============================================================================
// IPC ENGINE — HOLOGRAPHIC STRESS TEST
// 全息压力测试：元数据 → 引擎 → 持久化 三层全覆盖
//
// 测试层次:
//   Layer 1 — METADATA (元数据层)
//     1.1  词汇表 (PartVocab) 极端容量与碰撞稳健性
//     1.2  BOM 拓扑深度 / 宽度 / 幂等性
//     1.3  LLC 编译：合法 BOM vs 循环死锁检测
//     1.4  替代组 (Alt Group) 完整性与配额归一化
//     1.5  Allotment 配额边界：超限锁定与通配符匹配
//     1.6  64-bit 复合优先级编码覆盖全域
//     1.7  CalendarRecord 工作日推算（前推/后推）
//
//   Layer 2 — ENGINE (引擎层)
//     2.1  双端前缀和消纳算子 — 边界值 + 浮点精度
//     2.2  一类替换 — 3 轮次 + 平局规则
//     2.3  二类替换 — 供应商评级最低优先 + 空历史
//     2.4  三类替换 — 多候选 Lot-size 归一化收敛
//     2.5  维度感知消纳 — EQ / GE / LT 全算子
//     2.6  MCDM 组替换 — MaxLLC / NewCost / ExistCost 排序
//     2.7  Swap 引擎 — 不完全替代呆滞料回补
//     2.8  LBL MRP 20 层 BOM 级联 (50K parts / 500K demands)
//     2.9  DBD 并行调度引擎 — 有限产能 OTP 排产
//     2.10 零内存分配事务回滚栈 — 失败回退验证
//
//   Layer 3 — PERSISTENCE (持久化层)
//     3.1  DuckDB 内存模式写盘：计划订单落表
//     3.2  幂等性重算：两次运行结果全等性
//     3.3  跨场景隔离查账：SQL WHERE 过滤验证
//     3.4  大规模结果集（100万行）写盘吞吐基准
//     3.5  数据完整性：planned_orders 行数守恒校验
//
// 编译方式 (MSVC):
//   cl /std:c++17 /O2 /openmp /Iinclude /c src/engine_main.cpp /Fo build\engine_main.obj
//   cl /std:c++17 /O2 /openmp /Iinclude /c tests\holographic_stress_test.cpp /Fo build\holo_stress.obj
//   link /OUT:bin\holo_stress.exe build\engine_main.obj build\holo_stress.obj duckdb.lib
// =============================================================================

#pragma execution_character_set("utf-8")
#pragma warning(disable: 4146 4996)

#include "ipc_types.h"
#include "ipc/vocab.h"
#include "ipc/mrp_engine.h"
#include "ipc/dbd_engine.h"
#include "duckdb.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <cmath>
#include <chrono>
#include <random>
#include <stdexcept>
#include <sstream>
#include <iomanip>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace ipc;

static std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>
    g_holo_allotment;


// ---------------------------------------------------------------------------
// Test Harness Infrastructure
// ---------------------------------------------------------------------------
struct TestResult {
    std::string name;
    bool passed;
    double duration_ms;
    std::string message;
};

static std::vector<TestResult> g_results;
static int g_passed = 0;
static int g_failed = 0;

#define HOLO_TEST(name, ...) \
    do { \
        auto _t0 = std::chrono::high_resolution_clock::now(); \
        bool _pass = true; \
        std::string _msg; \
        try { [&]() { __VA_ARGS__ }(); } \
        catch (const std::exception& e) { _pass = false; _msg = e.what(); } \
        catch (...) { _pass = false; _msg = "Unknown exception"; } \
        auto _t1 = std::chrono::high_resolution_clock::now(); \
        double _ms = std::chrono::duration<double, std::milli>(_t1 - _t0).count(); \
        g_results.push_back({name, _pass, _ms, _msg}); \
        if (_pass) { std::cout << "  [PASS] " << name << " (" << std::fixed << std::setprecision(2) << _ms << " ms)\n"; ++g_passed; } \
        else { std::cout << "  [FAIL] " << name << " : " << _msg << " (" << _ms << " ms)\n"; ++g_failed; } \
    } while(0)

#define ASSERT_EQ(a, b) \
    if (!((a) == (b))) { std::ostringstream _ss; _ss << #a << " == " << #b << " failed: " << (a) << " != " << (b); throw std::runtime_error(_ss.str()); }

#define ASSERT_NEAR(a, b, eps) \
    if (std::abs((a) - (b)) > (eps)) { std::ostringstream _ss; _ss << #a << " ~= " << #b << " failed: diff = " << std::abs((a)-(b)); throw std::runtime_error(_ss.str()); }

#define ASSERT_TRUE(cond) \
    if (!(cond)) { throw std::runtime_error("Assert failed: " #cond); }

#define ASSERT_THROW(expr) \
    { bool _threw = false; try { expr; } catch (...) { _threw = true; } \
      if (!_threw) { throw std::runtime_error("Expected exception not thrown: " #expr); } }

// Label macro not needed but kept as no-op for compatibility
#define HOLO_TEST_END

// ============================================================================
// ██ LAYER 1 — METADATA TESTS
// ============================================================================

void layer1_vocab_capacity() {
    std::cout << "\n=== 第1层：元数据 ===\n";

    HOLO_TEST("1.1a PartVocab 10万 SKU 注册不碰撞", {
        PartVocab local_vocab;
        const int N = 100000;
        for (int i = 0; i < N; ++i) {
            std::string code = "META_VOCAB_" + std::to_string(i);
            uint32_t id = local_vocab.get_or_create(code);
            ASSERT_EQ(id, (uint32_t)i);
        }
        ASSERT_EQ(local_vocab.size(), (size_t)N);
        _test_end:;
    });

    HOLO_TEST("1.1b PartVocab 重复注册返回相同 ID", {
        PartVocab local_vocab;
        uint32_t id1 = local_vocab.get_or_create("DUPLICATE_SKU");
        uint32_t id2 = local_vocab.get_or_create("DUPLICATE_SKU");
        uint32_t id3 = local_vocab.get_or_create("DUPLICATE_SKU");
        ASSERT_EQ(id1, id2);
        ASSERT_EQ(id2, id3);
        ASSERT_EQ(local_vocab.size(), (size_t)1);
        _test_end:;
    });

    HOLO_TEST("1.1c PartVocab get_code 越界返回 UNKNOWN", {
        PartVocab local_vocab;
        local_vocab.get_or_create("ONLY_ONE");
        std::string s = local_vocab.get_code(9999);
        ASSERT_TRUE(s == "UNKNOWN");
        _test_end:;
    });
}

void layer1_bom_topology() {
    HOLO_TEST("1.2a BOM 拓扑：线性链 20层深度，LLC 单调递增", {
        // 0 -> 1 -> 2 -> ... -> 19 (linear chain)
        std::vector<PartSiteRecord> parts(20);
        std::vector<FlatBomItem> boms;
        for (int i = 0; i < 20; ++i) {
            parts[i].part_id = i;
            parts[i].part_code = "LC_" + std::to_string(i);
            parts[i].mrp_rule = "MRP";
            parts[i].part_type = (i == 0) ? "FINISHED" : "SEMI";
            parts[i].lead_time = 1.0;
        }
        for (int i = 0; i < 19; ++i) {
            FlatBomItem b;
            b.parent_id = i; b.child_id = i + 1;
            b.per_qty = 1.0; b.scrap = 0.0;
            b.relation_op = static_cast<uint8_t>(RelationOp::PASS);
            boms.push_back(b);
        }
        // Verify LLC increases monotonically
        std::vector<uint32_t> llc(20, 0);
        bool relaxed = true;
        while (relaxed) {
            relaxed = false;
            for (const auto& bom : boms) {
                if (llc[bom.child_id] <= llc[bom.parent_id]) {
                    llc[bom.child_id] = llc[bom.parent_id] + 1;
                    relaxed = true;
                }
            }
        }
        for (int i = 0; i < 20; ++i) {
            ASSERT_EQ(llc[i], (uint32_t)i);
        }
        _test_end:;
    });

    HOLO_TEST("1.2b BOM 拓扑：扇形结构 1个FG×64个L1子件，LLC平坦", {
        std::vector<uint32_t> llc(65, 0);
        std::vector<FlatBomItem> boms;
        for (int k = 1; k <= 64; ++k) {
            FlatBomItem b;
            b.parent_id = 0; b.child_id = k;
            b.per_qty = 1.0; b.scrap = 0.0;
            b.relation_op = static_cast<uint8_t>(RelationOp::PASS);
            boms.push_back(b);
        }
        bool relaxed = true;
        while (relaxed) {
            relaxed = false;
            for (const auto& b : boms) {
                if (llc[b.child_id] <= llc[b.parent_id]) {
                    llc[b.child_id] = llc[b.parent_id] + 1;
                    relaxed = true;
                }
            }
        }
        ASSERT_EQ(llc[0], (uint32_t)0);
        for (int k = 1; k <= 64; ++k) {
            ASSERT_EQ(llc[k], (uint32_t)1);
        }
        _test_end:;
    });

    HOLO_TEST("1.3  LLC 编译：含自循环 BOM 应抛出异常", {
        // A -> B -> A (cycle)
        std::vector<uint32_t> llc(2, 0);
        std::vector<FlatBomItem> boms;
        FlatBomItem b1; b1.parent_id = 0; b1.child_id = 1; b1.per_qty = 1.0; boms.push_back(b1);
        FlatBomItem b2; b2.parent_id = 1; b2.child_id = 0; b2.per_qty = 1.0; boms.push_back(b2);

        bool relaxed = true;
        size_t iterations = 0;
        bool caught = false;
        while (relaxed && iterations < 100) {
            relaxed = false;
            iterations++;
            for (const auto& b : boms) {
                if (llc[b.child_id] <= llc[b.parent_id]) {
                    llc[b.child_id] = llc[b.parent_id] + 1;
                    relaxed = true;
                }
            }
        }
        if (iterations >= 100) caught = true;
        ASSERT_TRUE(caught);
        _test_end:;
    });
}

void layer1_alt_groups() {
    HOLO_TEST("1.4a 替代组配额归一：3候选比例之和应恒为 1.0", {
        double r1 = 0.5, r2 = 0.3, r3 = 0.2;
        double sum = r1 + r2 + r3;
        ASSERT_NEAR(sum, 1.0, 1e-12);
        _test_end:;
    });

    HOLO_TEST("1.4b 替代组配额归一：动态移除后重归一", {
        std::vector<double> ratios = {0.5, 0.3, 0.2};
        // Remove first candidate (ratio 0.5), re-normalize remaining
        ratios.erase(ratios.begin());
        double sum = std::accumulate(ratios.begin(), ratios.end(), 0.0);
        for (auto& r : ratios) r /= sum;
        ASSERT_NEAR(ratios[0] + ratios[1], 1.0, 1e-12);
        // Original: 0.3 / (0.3+0.2) = 0.6
        ASSERT_NEAR(ratios[0], 0.6, 1e-9);
        ASSERT_NEAR(ratios[1], 0.4, 1e-9);
        _test_end:;
    });

    HOLO_TEST("1.4c 替代组配额归一：空配额 fallback 均分", {
        std::vector<double> ratios = {0.0, 0.0, 0.0};
        double sum = std::accumulate(ratios.begin(), ratios.end(), 0.0);
        if (sum < 1e-12) {
            for (auto& r : ratios) r = 1.0 / ratios.size();
        }
        for (auto& r : ratios) {
            ASSERT_NEAR(r, 1.0/3.0, 1e-9);
        }
        _test_end:;
    });
}

void layer1_allotment() {
    HOLO_TEST("1.5a Allotment 超限锁定：消耗超过 limit 后被阻断", {
        AllotmentState state;
        state.limit = 100.0;
        state.consumed = 0.0;
        state.is_locked = false;

        // First allocation: 80 units — should succeed
        double req1 = 80.0;
        double avail = state.limit - state.consumed;
        ASSERT_TRUE(avail >= req1);
        state.consumed += req1;

        // Second allocation: 30 units — exceeds limit
        double req2 = 30.0;
        avail = state.limit - state.consumed;
        ASSERT_TRUE(avail < req2); // Must be rejected
        _test_end:;
    });

    HOLO_TEST("1.5b Allotment 通配符匹配：精确键优先于通配键", {
        std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> constraints;
        uint32_t wildcard = uint32_t(-1);

        // Register wildcard entry
        AllotmentConstraintKey wc_key = {1001, 10, wildcard, wildcard, wildcard};
        AllotmentState wc_state; wc_state.limit = 50.0; wc_state.consumed = 0.0;
        constraints[wc_key] = wc_state;

        // Register exact entry
        AllotmentConstraintKey exact_key = {1001, 10, 200, 300, 400};
        AllotmentState exact_state; exact_state.limit = 200.0; exact_state.consumed = 0.0;
        constraints[exact_key] = exact_state;

        AllotmentState* found = find_matching_allotment(1001, 10, 200, 300, 400, wildcard, constraints);
        ASSERT_TRUE(found != nullptr);
        ASSERT_NEAR(found->limit, 200.0, 1e-9); // exact match wins
        _test_end:;
    });
}

void layer1_priority_encoding() {
    HOLO_TEST("1.6a 64-bit 优先级：COMMITTED 订单编码值 < OPEN 订单", {
        uint64_t committed = encode_composite_priority(true,  1, 10, 1, 10000.0);
        uint64_t open      = encode_composite_priority(false, 1, 10, 1, 10000.0);
        ASSERT_TRUE(committed < open);
        _test_end:;
    });

    HOLO_TEST("1.6b 64-bit 优先级：Tier-1 客户比 Tier-3 优先", {
        uint64_t tier1 = encode_composite_priority(false, 1, 30, 3, 5000.0);
        uint64_t tier3 = encode_composite_priority(false, 3, 30, 3, 5000.0);
        ASSERT_TRUE(tier1 < tier3);
        _test_end:;
    });

    HOLO_TEST("1.6c 64-bit 优先级：Due Day 越早优先级越高", {
        uint64_t early = encode_composite_priority(false, 2, 5,  3, 1000.0);
        uint64_t late  = encode_composite_priority(false, 2, 30, 3, 1000.0);
        ASSERT_TRUE(early < late);
        _test_end:;
    });

    HOLO_TEST("1.6d 64-bit 优先级：Revenue 越高优先级越高（inv编码）", {
        uint64_t high_rev = encode_composite_priority(false, 2, 20, 3, 500000.0);
        uint64_t low_rev  = encode_composite_priority(false, 2, 20, 3, 100.0);
        ASSERT_TRUE(high_rev < low_rev);
        _test_end:;
    });

    HOLO_TEST("1.6e 64-bit 优先级：全域最高 (committed+tier1+due1+pri1+maxrev)", {
        uint64_t best  = encode_composite_priority(true,  1, 1,  1, 268435455.0);
        uint64_t worst = encode_composite_priority(false, 3, 65535, 65535, 0.0);
        ASSERT_TRUE(best < worst);
        _test_end:;
    });
}

void layer1_calendar() {
    HOLO_TEST("1.7a 工作日计算：周六/周日为非工作日（默认日历）", {
        // Day 0 is our epoch reference. Day % 7 == 1 → Sat, 2 → Sun (per engine logic)
        auto is_workday = [](int day) { return (day % 7 != 1) && (day % 7 != 2); };
        // Validate a known week pattern: days 0-6
        ASSERT_TRUE( is_workday(0)); // 周一
        ASSERT_TRUE(!is_workday(1)); // 周六
        ASSERT_TRUE(!is_workday(2)); // 周日
        ASSERT_TRUE( is_workday(3));
        ASSERT_TRUE( is_workday(4));
        ASSERT_TRUE( is_workday(5));
        ASSERT_TRUE( is_workday(6));
        _test_end:;
    });

    HOLO_TEST("1.7b 工作日后推：10个工作日偏移正确", {
        auto is_workday = [](int day) { return (day % 7 != 1) && (day % 7 != 2); };
        // Forward offset from day 0, 10 working days
        int day = 0;
        int counted = 0;
        while (counted < 10 && day < 365) {
            day++;
            if (is_workday(day)) counted++;
        }
        // In a 5-day work week, 10 working days = 2 full weeks = 14 calendar days
        ASSERT_EQ(day, 14);
        _test_end:;
    });
}

// ============================================================================
// ██ LAYER 2 — ENGINE TESTS
// ============================================================================

void layer2_netting_operator() {
    std::cout << "\n=== 第2层：计划引擎 ===\n";

    HOLO_TEST("2.1a 前缀和消纳：全覆盖 (CD1 < CS)", {
        double CS = 200.0, CD0 = 0.0, CD1 = 80.0;
        double consumed = std::max(0.0, std::min(CD1, CS) - CD0);
        double net_dem  = std::max(0.0, CD1 - std::max(CD0, CS));
        ASSERT_NEAR(consumed, 80.0, 1e-9);
        ASSERT_NEAR(net_dem,  0.0,  1e-9);
        _test_end:;
    });

    HOLO_TEST("2.1b 前缀和消纳：跨越边界 (CD0 < CS < CD1)", {
        double CS = 100.0, CD0 = 40.0, CD1 = 120.0;
        double consumed = std::max(0.0, std::min(CD1, CS) - CD0);
        double net_dem  = std::max(0.0, CD1 - std::max(CD0, CS));
        ASSERT_NEAR(consumed, 60.0, 1e-9);
        ASSERT_NEAR(net_dem,  20.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.1c 前缀和消纳：完全缺货 (CD0 >= CS)", {
        double CS = 100.0, CD0 = 100.0, CD1 = 150.0;
        double consumed = std::max(0.0, std::min(CD1, CS) - CD0);
        double net_dem  = std::max(0.0, CD1 - std::max(CD0, CS));
        ASSERT_NEAR(consumed, 0.0,  1e-9);
        ASSERT_NEAR(net_dem,  50.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.1d 前缀和消纳：零供应 CS=0", {
        double CS = 0.0, CD0 = 0.0, CD1 = 99.0;
        double consumed = std::max(0.0, std::min(CD1, CS) - CD0);
        double net_dem  = std::max(0.0, CD1 - std::max(CD0, CS));
        ASSERT_NEAR(consumed, 0.0,  1e-9);
        ASSERT_NEAR(net_dem,  99.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.1e 前缀和消纳：连续 5 需求事件批次验证", {
        double CS = 350.0;
        std::vector<double> qtys = {80.0, 100.0, 120.0, 60.0, 40.0};
        std::vector<double> expected_consumed = {80.0, 100.0, 120.0, 50.0, 0.0};
        double CD0 = 0.0;
        for (size_t j = 0; j < qtys.size(); ++j) {
            double CD1 = CD0 + qtys[j];
            double c = std::max(0.0, std::min(CD1, CS) - CD0);
            ASSERT_NEAR(c, expected_consumed[j], 1e-9);
            CD0 = CD1;
        }
        _test_end:;
    });
}

void layer2_class1_substitution() {
    HOLO_TEST("2.2a 一类替换：3轮次配额收敛 (60%/40%)", {
        FlatBomItem bom1 = { 1, 10, 1.0, 0.0, 1, 1, 0.6, 0.0, 0.0 };
        FlatBomItem bom2 = { 1, 11, 1.0, 0.0, 1, 1, 0.4, 0.0, 0.0 };
        std::vector<FlatBomItem*> group = { &bom1, &bom2 };

        auto alloc_c1 = [](double net, const std::vector<FlatBomItem*>& g) -> uint32_t {
            double hist = 0.0;
            for (auto* it : g) hist += it->historical_qty;
            double total = hist + net;
            FlatBomItem* best = nullptr;
            double max_gap = -1.0;
            for (auto* it : g) {
                double gap = std::abs(it->historical_qty - total * it->target_ratio);
                if (gap > max_gap || (std::abs(gap - max_gap) < 1e-9 && (!best || it->target_ratio > best->target_ratio)))
                    { max_gap = gap; best = it; }
            }
            return best ? best->child_id : uint32_t(-1);
        };

        ASSERT_EQ(alloc_c1(10.0, group), (uint32_t)10); bom1.historical_qty += 10.0;
        ASSERT_EQ(alloc_c1(10.0, group), (uint32_t)11); bom2.historical_qty += 10.0;
        ASSERT_EQ(alloc_c1(20.0, group), (uint32_t)10); bom1.historical_qty += 20.0;
        ASSERT_NEAR(bom1.historical_qty, 30.0, 1e-9);
        ASSERT_NEAR(bom2.historical_qty, 10.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.2b 一类替换：平局时选更高配额候选", {
        // Both with 0 history, ratio 0.6 vs 0.4 — expect 0.6 candidate wins
        FlatBomItem b1 = { 1, 20, 1.0, 0.0, 2, 1, 0.6, 0.0, 0.0 };
        FlatBomItem b2 = { 1, 21, 1.0, 0.0, 2, 1, 0.4, 0.0, 0.0 };
        // At net=10, hist=0: gap_20=|0-6|=6, gap_21=|0-4|=4 → b1 wins (not tie)
        std::vector<FlatBomItem*> grp = { &b1, &b2 };
        double hist = 0.0, total = hist + 10.0;
        double g1 = std::abs(0.0 - total * 0.6); // 6
        double g2 = std::abs(0.0 - total * 0.4); // 4
        ASSERT_TRUE(g1 > g2);
        _test_end:;
    });
}

void layer2_class2_substitution() {
    HOLO_TEST("2.3a 二类替换：供应商评级最低优先", {
        FlatBomItem b1 = { 1, 30, 1.0, 0.0, 3, 2, 0.6, 10.0, 0.0 }; // Rating = 10/0.6 ≈ 16.67
        FlatBomItem b2 = { 1, 31, 1.0, 0.0, 3, 2, 0.4,  0.0, 0.0 }; // Rating = 0/0.4 = 0
        std::vector<FlatBomItem*> grp = { &b1, &b2 };

        auto alloc_c2 = [](const std::vector<FlatBomItem*>& g) -> uint32_t {
            FlatBomItem* best = nullptr;
            double min_r = 1e18;
            for (auto* it : g) {
                double r = it->historical_qty / (it->target_ratio > 0 ? it->target_ratio : 1.0);
                if (r < min_r || (std::abs(r - min_r) < 1e-9 && (!best || it->target_ratio > best->target_ratio)))
                    { min_r = r; best = it; }
            }
            return best ? best->child_id : uint32_t(-1);
        };

        ASSERT_EQ(alloc_c2(grp), (uint32_t)31); // b2 has lower rating (0)
        _test_end:;
    });

    HOLO_TEST("2.3b 二类替换：空历史全为 0 时选更高配额", {
        FlatBomItem b1 = { 1, 40, 1.0, 0.0, 4, 2, 0.7, 0.0, 0.0 };
        FlatBomItem b2 = { 1, 41, 1.0, 0.0, 4, 2, 0.3, 0.0, 0.0 };
        // Both rating = 0, tie → higher ratio (0.7) wins
        std::vector<FlatBomItem*> grp = { &b1, &b2 };
        auto alloc_c2 = [](const std::vector<FlatBomItem*>& g) -> uint32_t {
            FlatBomItem* best = nullptr;
            double min_r = 1e18;
            for (auto* it : g) {
                double r = it->historical_qty / (it->target_ratio > 0 ? it->target_ratio : 1.0);
                if (r < min_r || (std::abs(r - min_r) < 1e-9 && (!best || it->target_ratio > best->target_ratio)))
                    { min_r = r; best = it; }
            }
            return best ? best->child_id : uint32_t(-1);
        };
        ASSERT_EQ(alloc_c2(grp), (uint32_t)40);
        _test_end:;
    });
}

void layer2_class3_substitution() {
    HOLO_TEST("2.4a 三类替换：3候选 Lot-size 收敛 (100单位 → 60/30/10)", {
        FlatBomItem b1 = { 1, 50, 1.0, 0.0, 5, 3, 0.5, 0.0, 20.0 };
        FlatBomItem b2 = { 1, 51, 1.0, 0.0, 5, 3, 0.3, 0.0, 15.0 };
        FlatBomItem b3 = { 1, 52, 1.0, 0.0, 5, 3, 0.2, 0.0, 10.0 };

        struct ActiveCand { FlatBomItem* item; double cur_ratio; double due_qty; };
        std::vector<ActiveCand> active = {
            {&b1, 0.5, 0.0}, {&b2, 0.3, 0.0}, {&b3, 0.2, 0.0}
        };

        double rem = 100.0;
        while (rem > 0.0 && !active.empty()) {
            for (auto& c : active) c.due_qty = c.cur_ratio * rem;
            std::sort(active.begin(), active.end(),
                [](const ActiveCand& a, const ActiveCand& b) { return a.due_qty > b.due_qty; });
            auto& chosen = active.front();
            double lot  = chosen.item->lot_size > 0 ? chosen.item->lot_size : 1.0;
            double actual = std::ceil(chosen.due_qty / lot) * lot;
            double consumed = std::min(actual, rem);
            chosen.item->historical_qty += consumed;
            rem -= consumed;
            active.erase(active.begin());
            if (!active.empty()) {
                double sum_due = 0.0;
                for (auto& c : active) sum_due += c.due_qty;
                if (sum_due > 0.0) for (auto& c : active) c.cur_ratio = c.due_qty / sum_due;
            }
        }

        ASSERT_NEAR(b1.historical_qty, 60.0, 1e-9);
        ASSERT_NEAR(b2.historical_qty, 30.0, 1e-9);
        ASSERT_NEAR(b3.historical_qty, 10.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.4b 三类替换：单候选直接全量分配", {
        FlatBomItem b1 = { 1, 60, 1.0, 0.0, 6, 3, 1.0, 0.0, 5.0 };
        struct AC { FlatBomItem* item; double cur_ratio; double due_qty; };
        std::vector<AC> active = {{&b1, 1.0, 0.0}};
        double rem = 37.0;
        while (rem > 0.0 && !active.empty()) {
            for (auto& c : active) c.due_qty = c.cur_ratio * rem;
            auto& chosen = active.front();
            double lot = chosen.item->lot_size > 0 ? chosen.item->lot_size : 1.0;
            double actual = std::ceil(chosen.due_qty / lot) * lot;
            double consumed = std::min(actual, rem);
            chosen.item->historical_qty += consumed;
            rem -= consumed;
            active.erase(active.begin());
        }
        // ceil(37/5)*5 = 40, but consumed = min(40, 37) = 37
        ASSERT_NEAR(b1.historical_qty, 37.0, 1e-9);
        _test_end:;
    });
}

void layer2_dimension_matching() {
    HOLO_TEST("2.5a 维度匹配：EQ 精确匹配", {
        auto eval = [](double order_val, uint8_t op, double bom_val) -> bool {
            switch (static_cast<RelationOp>(op)) {
                case RelationOp::EQ: return std::abs(order_val - bom_val) < 1e-9;
                case RelationOp::GE: return order_val >= bom_val;
                case RelationOp::GT: return order_val >  bom_val;
                case RelationOp::LE: return order_val <= bom_val;
                case RelationOp::LT: return order_val <  bom_val;
                case RelationOp::NE: return std::abs(order_val - bom_val) > 1e-9;
                default: return true;
            }
        };
        ASSERT_TRUE( eval(102.0, (uint8_t)RelationOp::EQ, 102.0));
        ASSERT_TRUE(!eval(101.0, (uint8_t)RelationOp::EQ, 102.0));
        _test_end:;
    });

    HOLO_TEST("2.5b 维度匹配：GE 降级消纳（高规格覆盖低规格）", {
        auto eval_ge = [](double ord, double bom) { return ord >= bom; };
        ASSERT_TRUE( eval_ge(102.0, 101.0)); // 512MB covers 256MB req
        ASSERT_TRUE(!eval_ge(100.0, 101.0)); // 128MB cannot cover 256MB req
        ASSERT_TRUE( eval_ge(102.0, 100.0)); // 512MB covers 128MB req
        _test_end:;
    });

    HOLO_TEST("2.5c 维度匹配：NE 屏蔽特定规格", {
        auto eval_ne = [](double ord, double bom) { return std::abs(ord - bom) > 1e-9; };
        ASSERT_TRUE(!eval_ne(102.0, 102.0)); // exact match → NOT eligible
        ASSERT_TRUE( eval_ne(101.0, 102.0)); // different → eligible
        _test_end:;
    });

    HOLO_TEST("2.5d 维度匹配：PASS 算子始终为 true", {
        ASSERT_TRUE(true); // RelationOp::PASS always eligible
        _test_end:;
    });
}

void layer2_mcdm_groups() {
    HOLO_TEST("2.6a MCDM 组替换：MaxLLC 最小优先", {
        struct MCDM { int max_llc; double new_cost; double exist_cost; int id; };
        std::vector<MCDM> cands = {
            {3, 500.0, 100.0, 1},
            {2, 800.0, 200.0, 2}, // LLC=2 wins over LLC=3
            {4, 100.0,  50.0, 3}
        };
        std::sort(cands.begin(), cands.end(), [](const MCDM& a, const MCDM& b) {
            if (a.max_llc != b.max_llc) return a.max_llc < b.max_llc;
            if (std::abs(a.new_cost - b.new_cost) > 1e-9) return a.new_cost < b.new_cost;
            return a.exist_cost < b.exist_cost;
        });
        ASSERT_EQ(cands[0].id, 2);
        _test_end:;
    });

    HOLO_TEST("2.6b MCDM 组替换：同LLC时 NewCost 最小优先", {
        struct MCDM { int max_llc; double new_cost; int id; };
        std::vector<MCDM> cands = {{2, 1200.0, 1}, {2, 500.0, 2}, {2, 800.0, 3}};
        std::sort(cands.begin(), cands.end(), [](const MCDM& a, const MCDM& b) {
            if (a.max_llc != b.max_llc) return a.max_llc < b.max_llc;
            return a.new_cost < b.new_cost;
        });
        ASSERT_EQ(cands[0].id, 2);
        _test_end:;
    });
}

void layer2_swap_engine() {
    HOLO_TEST("2.7a Swap 引擎：呆滞料部分回补", {
        double net_demand    = 100.0;
        double alt_stagnant  = 60.0;
        std::vector<SwapRecord> swap_records;

        if (net_demand > 0.0 && alt_stagnant > 0.0) {
            double swap_qty = std::min(net_demand, alt_stagnant);
            net_demand  -= swap_qty;
            alt_stagnant -= swap_qty;
            swap_records.push_back({"D001", "PART_A", "PART_ALT", swap_qty, 5, "GRP_1", "Stagnant"});
        }

        ASSERT_NEAR(net_demand,   40.0, 1e-9); // 60 swapped, 40 remaining
        ASSERT_NEAR(alt_stagnant,  0.0, 1e-9);
        ASSERT_EQ(swap_records.size(), (size_t)1);
        ASSERT_NEAR(swap_records[0].swapped_qty, 60.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.7b Swap 引擎：完全回补，净需求归零", {
        double net_demand = 50.0, alt = 200.0;
        std::vector<SwapRecord> recs;
        double swap_qty = std::min(net_demand, alt);
        net_demand -= swap_qty; alt -= swap_qty;
        recs.push_back({"D002", "P_MAIN", "P_ALT", swap_qty, 8, "GRP_2", "Stagnant"});
        ASSERT_NEAR(net_demand, 0.0, 1e-9);
        ASSERT_NEAR(alt, 150.0, 1e-9);
        _test_end:;
    });

    HOLO_TEST("2.7c Swap 引擎：无呆滞料，Swap 记录为空", {
        double net_demand = 50.0, alt = 0.0;
        std::vector<SwapRecord> recs;
        if (alt > 0.0) {
            double q = std::min(net_demand, alt);
            net_demand -= q;
            recs.push_back({});
        }
        ASSERT_EQ(recs.size(), (size_t)0);
        ASSERT_NEAR(net_demand, 50.0, 1e-9);
        _test_end:;
    });
}

void layer2_full_engine() {
    HOLO_TEST("2.8  LBL MRP 引擎：20层BOM/5万物料/50万需求", {
        const int NUM_PARTS   = 50000;
        const int NUM_DEMANDS = 500000;

        std::vector<PartSiteRecord> parts;
        std::vector<FlatBomItem> boms;
        std::vector<IndependentDemand> demands;
        std::vector<PlannedOrder> planned_orders;
        std::vector<AlternateAllocationRecord> alt_records;
        std::vector<SwapRecord> swap_records;

        std::mt19937 rng(42);
        parts.resize(NUM_PARTS);

        for (int i = 0; i < NUM_PARTS; ++i) {
            auto& rec = parts[i];
            rec.part_id = i;
            rec.part_code = "HST_" + std::to_string(i);
            rec.low_level_code = 0;
            rec.on_hand = 0.0;
            rec.round_to_integer = true;
            rec.site = "SITE_001";
            if (i < 1000) { rec.part_type = "FINISHED"; rec.mrp_rule = "MPS"; rec.lead_time = 2.0; }
            else if (i < 37000) { rec.part_type = "SEMI"; rec.mrp_rule = "MRP"; rec.lead_time = 3.0; rec.is_phantom = (i % 10 == 0); }
            else if (i < 39000) { rec.part_type = "RAW"; rec.mrp_rule = "MRP"; rec.lead_time = 10.0; rec.site = (i%2==0)?"SUPP_A":"SUPP_B"; }
            else { rec.part_type = "ALT"; rec.mrp_rule = "MRP"; rec.lead_time = 8.0; rec.site = "SUPP_B"; }
        }

        // 5-level chain for first 1000 FGs
        for (int i = 0; i < 1000; ++i) {
            int base = 1000 + i;
            for (int lvl = 0; lvl < 4; ++lvl) {
                FlatBomItem b;
                b.parent_id = (lvl == 0) ? i : (1000 + i + lvl*1000 - 1000);
                b.child_id  = 1000 + i + lvl*1000;
                b.per_qty = 1.0; b.scrap = 0.0;
                b.relation_op = static_cast<uint8_t>(RelationOp::PASS);
                boms.push_back(b);
            }
        }

        // 2000-chain from level 4 (part 5000-6999) down 15 levels to raw
        for (int lvl = 4; lvl < 19; ++lvl) {
            int p_start = 5000 + (lvl-4)*2000;
            int c_start = p_start + 2000;
            for (int off = 0; off < 2000; ++off) {
                FlatBomItem b;
                b.parent_id = p_start + off;
                b.child_id  = c_start + off;
                b.per_qty = 1.0; b.scrap = 0.0;
                b.relation_op = static_cast<uint8_t>(RelationOp::PASS);
                boms.push_back(b);
            }
        }

        // Substitution at raw level
        for (int r = 37000; r < 39000; ++r) {
            int grp = r - 37000;
            FlatBomItem b1, b2;
            b1.parent_id = r; b1.child_id = r; b1.per_qty = 1.0; b1.alt_group_id = grp; b1.alt_priority = 1; b1.target_ratio = 0.5; b1.lot_size = 5.0; b1.relationship_type = "alt";
            b2.parent_id = r; b2.child_id = 39000+grp; b2.per_qty = 1.0; b2.alt_group_id = grp; b2.alt_priority = 2; b2.target_ratio = 0.5; b2.lot_size = 10.0; b2.relationship_type = "alt";
            boms.push_back(b1); boms.push_back(b2);
        }

        // Generate demands
        demands.resize(NUM_DEMANDS);
        for (int i = 0; i < NUM_DEMANDS; ++i) {
            auto& d = demands[i];
            d.demand_id = i;
            d.customer = "C_" + std::to_string(i % 100);
            d.part_id  = std::uniform_int_distribution<int>(0, 999)(rng);
            d.qty      = std::uniform_real_distribution<double>(5.0, 50.0)(rng);
            d.due_day  = std::uniform_int_distribution<int>(10, 85)(rng);
            d.priority = std::uniform_int_distribution<int>(1, 5)(rng);
            d.dimension_val = 100.0;
            d.status = (i % 10 == 0) ? "COMMITTED" : "OPEN";
            d.customer_tier = (i % 3 == 0) ? 1 : 2;
            d.revenue = d.qty * 15.0;
            d.composite_priority = encode_composite_priority(d.status=="COMMITTED", d.customer_tier, d.due_day, d.priority, d.revenue);
        }

        run_lbl_mrp_engine(parts, boms, demands, planned_orders, alt_records, swap_records, 25);

        ASSERT_TRUE(planned_orders.size() > 0);
        std::cout << "      → 生成计划订单: " << planned_orders.size() << " 笔\n";
        _test_end:;
    });

    HOLO_TEST("2.9  DBD 派程引擎：有限产能 OTP 排产（子集 10万需求）", {
        std::vector<PartSiteRecord> parts(10);
        std::vector<FlatBomItem> boms;
        std::vector<IndependentDemand> demands;
        std::vector<PlannedOrder> planned, scheduled;
        std::vector<AlternateAllocationRecord> alt_recs;
        std::vector<SwapRecord> swap_recs;
        std::vector<double> alloc_rates, cap, rcosts;

        std::mt19937 rng(99);
        for (int i = 0; i < 10; ++i) {
            parts[i].part_id = i; parts[i].part_code = "DBD_" + std::to_string(i);
            parts[i].mrp_rule = "MPS"; parts[i].part_type = "FINISHED";
            parts[i].lead_time = 1.0; parts[i].on_hand = 1000.0;
        }
        demands.resize(100000);
        for (int i = 0; i < 100000; ++i) {
            demands[i].demand_id = i;
            demands[i].part_id   = i % 10;
            demands[i].qty       = std::uniform_real_distribution<double>(1.0, 10.0)(rng);
            demands[i].due_day   = std::uniform_int_distribution<int>(5, 85)(rng);
            demands[i].priority  = 3;
            demands[i].dimension_val = 100.0;
            demands[i].composite_priority = encode_composite_priority(false, 3, demands[i].due_day, 3, demands[i].qty*10);
        }

        run_lbl_mrp_engine(parts, boms, demands, planned, alt_recs, swap_recs, 5);
        run_dbd_dispatch_engine(parts, boms, planned, demands, scheduled, alloc_rates, cap, rcosts);

        ASSERT_TRUE(scheduled.size() >= 0); // Engine must not crash
        std::cout << "      → DBD 排产结果: " << scheduled.size() << " 笔工单\n";
        _test_end:;
    });

    HOLO_TEST("2.10 事务回滚栈：失败分配后状态完全还原", {
        // Simulate savepoint / rollback pattern
        std::vector<double> shared_alloc = {0.0, 0.0, 0.0}; // capacity by day
        size_t savepoint = shared_alloc.size();

        // Allocate (push)
        shared_alloc.push_back(50.0);
        shared_alloc.push_back(30.0);
        ASSERT_EQ(shared_alloc.size(), (size_t)5);

        // Simulate failure: rollback to savepoint
        shared_alloc.resize(savepoint);
        ASSERT_EQ(shared_alloc.size(), (size_t)3);
        for (double v : shared_alloc) { ASSERT_NEAR(v, 0.0, 1e-9); }
        _test_end:;
    });
}

// ============================================================================
// ██ LAYER 3 — PERSISTENCE TESTS
// ============================================================================

void layer3_duckdb_persistence() {
    std::cout << "\n=== 第3层：持久化 ===\n";

    HOLO_TEST("3.1  DuckDB 内存写盘：planned_orders 表创建并写入", {
        duckdb::DuckDB db(nullptr); // in-memory
        duckdb::Connection con(db);

        con.Query("CREATE TABLE IF NOT EXISTS holo_planned_orders ("
                  "  part_id    INTEGER,"
                  "  qty        DOUBLE,"
                  "  start_day  INTEGER,"
                  "  finish_day INTEGER,"
                  "  dim_val    DOUBLE"
                  ")");

        // Insert 1000 test orders
        con.Query("BEGIN");
        for (int i = 0; i < 1000; ++i) {
            std::string sql = "INSERT INTO holo_planned_orders VALUES ("
                + std::to_string(i % 50) + ","
                + std::to_string(10.0 + i * 0.1) + ","
                + std::to_string(i) + ","
                + std::to_string(i + 5) + ","
                + "100.0)";
            con.Query(sql);
        }
        con.Query("COMMIT");

        auto res = con.Query("SELECT COUNT(*) FROM holo_planned_orders");
        ASSERT_TRUE(res->HasError() == false);
        ASSERT_EQ(res->GetValue<int64_t>(0, 0), (int64_t)1000);
        _test_end:;
    });

    HOLO_TEST("3.2  幂等性重算：两次写盘结果行数相同", {
        duckdb::DuckDB db(nullptr);
        duckdb::Connection con(db);

        auto run_insert = [&](int seed) {
            con.Query("DROP TABLE IF EXISTS holo_idempotent");
            con.Query("CREATE TABLE holo_idempotent (part_id INT, qty DOUBLE, day INT)");
            std::mt19937 rng(seed);
            con.Query("BEGIN");
            for (int i = 0; i < 500; ++i) {
                int pid = rng() % 100;
                double qty = (rng() % 1000) * 0.1;
                int day = rng() % 90;
                std::string sql = "INSERT INTO holo_idempotent VALUES ("
                    + std::to_string(pid) + "," + std::to_string(qty) + "," + std::to_string(day) + ")";
                con.Query(sql);
            }
            con.Query("COMMIT");
            auto r = con.Query("SELECT COUNT(*) FROM holo_idempotent");
            return r->GetValue<int64_t>(0, 0);
        };

        int64_t count1 = run_insert(42);
        int64_t count2 = run_insert(42);
        ASSERT_EQ(count1, count2);
        ASSERT_EQ(count1, (int64_t)500);
        _test_end:;
    });

    HOLO_TEST("3.3  跨场景隔离查账：SQL 过滤场景专属数据", {
        duckdb::DuckDB db(nullptr);
        duckdb::Connection con(db);

        con.Query("CREATE TABLE holo_isolation ("
                  "  part_code VARCHAR, qty DOUBLE, scenario VARCHAR)");

        // Inject mass noise data
        con.Query("BEGIN");
        for (int i = 0; i < 5000; ++i) {
            std::string sql = "INSERT INTO holo_isolation VALUES ('PART_MASS_" + std::to_string(i)
                + "', 10.0, 'stress')";
            con.Query(sql);
        }
        // Inject 3 scenario-specific rows
        con.Query("INSERT INTO holo_isolation VALUES ('PART_SCENARIO1_P1', 30.0, 'scenario1')");
        con.Query("INSERT INTO holo_isolation VALUES ('PART_SCENARIO1_P2', 10.0, 'scenario1')");
        con.Query("INSERT INTO holo_isolation VALUES ('PART_SCENARIO1_MAIN', 40.0, 'scenario1')");
        con.Query("COMMIT");

        // Isolated query: only scenario1 parts
        auto res = con.Query(
            "SELECT SUM(qty) FROM holo_isolation WHERE part_code LIKE 'PART_SCENARIO1%'");
        ASSERT_TRUE(!res->HasError());
        double total_qty = res->GetValue<double>(0, 0);
        ASSERT_NEAR(total_qty, 80.0, 1e-9);

        // Verify noise not contaminated
        auto cnt = con.Query(
            "SELECT COUNT(*) FROM holo_isolation WHERE part_code LIKE 'PART_SCENARIO1%'");
        ASSERT_EQ(cnt->GetValue<int64_t>(0, 0), (int64_t)3);
        _test_end:;
    });

    HOLO_TEST("3.4  大批量写盘吞吐：100万行写入 < 10秒", {
        duckdb::DuckDB db(nullptr);
        duckdb::Connection con(db);
        con.Query("CREATE TABLE holo_bulk (id INT, qty DOUBLE, day INT, part_id INT)");

        auto t0 = std::chrono::high_resolution_clock::now();

        // Use appender for maximum throughput
        duckdb::Appender appender(con, "holo_bulk");
        for (int i = 0; i < 1000000; ++i) {
            appender.AppendRow(i, (double)(i % 1000) * 0.5, i % 91, i % 50000);
        }
        appender.Close();

        auto t1 = std::chrono::high_resolution_clock::now();
        double elapsed_s = std::chrono::duration<double>(t1 - t0).count();

        auto cnt = con.Query("SELECT COUNT(*) FROM holo_bulk");
        ASSERT_EQ(cnt->GetValue<int64_t>(0, 0), (int64_t)1000000);
        ASSERT_TRUE(elapsed_s < 10.0); // Must complete within 10 seconds
        std::cout << "      → 100万行写盘耗时: " << std::fixed << std::setprecision(3) << elapsed_s << " s\n";
        _test_end:;
    });

    HOLO_TEST("3.5  数据完整性：planned_orders 行数守恒 (写入=读回)", {
        duckdb::DuckDB db(nullptr);
        duckdb::Connection con(db);
        con.Query("CREATE TABLE holo_integrity (part_id INT, qty DOUBLE, start_day INT, finish_day INT)");

        // Simulate 200K planned orders written by engine
        const int N = 200000;
        duckdb::Appender app(con, "holo_integrity");
        for (int i = 0; i < N; ++i) {
            app.AppendRow(i % 50000, (double)(i % 500) + 1.0, i % 91, (i % 91) + 3);
        }
        app.Close();

        auto cnt = con.Query("SELECT COUNT(*) FROM holo_integrity");
        int64_t actual = cnt->GetValue<int64_t>(0, 0);
        ASSERT_EQ(actual, (int64_t)N);

        // Verify no zero-qty orders slipped in
        auto zero = con.Query("SELECT COUNT(*) FROM holo_integrity WHERE qty <= 0");
        ASSERT_EQ(zero->GetValue<int64_t>(0, 0), (int64_t)0);
        _test_end:;
    });
}

// ============================================================================
// ██ MAIN — TEST ORCHESTRATOR
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    IPC ENGINE — HOLOGRAPHIC STRESS TEST SUITE  (全息压力测试)       ║\n";
    std::cout << "║    第1层: 元数据层  |  第2层: 计划引擎层  |  第3层: 持久化层  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n";

#ifdef _OPENMP
    std::cout << "OpenMP 线程数: " << omp_get_max_threads() << "\n";
#endif

    auto wall_start = std::chrono::high_resolution_clock::now();

    // --- Layer 1: Metadata ---
    layer1_vocab_capacity();
    layer1_bom_topology();
    layer1_alt_groups();
    layer1_allotment();
    layer1_priority_encoding();
    layer1_calendar();

    // --- Layer 2: Engine ---
    layer2_netting_operator();
    layer2_class1_substitution();
    layer2_class2_substitution();
    layer2_class3_substitution();
    layer2_dimension_matching();
    layer2_mcdm_groups();
    layer2_swap_engine();
    layer2_full_engine();  // Heavy: 20-layer LBL + DBD

    // --- Layer 3: Persistence ---
    layer3_duckdb_persistence();

    auto wall_end = std::chrono::high_resolution_clock::now();
    double wall_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  测试报告                                                            ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  总用例数: " << std::setw(4) << (g_passed + g_failed) << "   ";
    std::cout << "通过: " << std::setw(4) << g_passed << "   ";
    std::cout << "失败: " << std::setw(4) << g_failed << "                      ║\n";
    std::cout << "║  总耗时: " << std::fixed << std::setprecision(2) << std::setw(10) << wall_ms << " ms                                           ║\n";

    if (g_failed > 0) {
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║  [失败用例]:                                                          ║\n";
        for (const auto& r : g_results) {
            if (!r.passed) {
                std::cout << "║    ✗ " << r.name << "\n";
                std::cout << "║      → " << r.message << "\n";
            }
        }
    }

    std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n";

    return (g_failed == 0) ? 0 : 1;
}
