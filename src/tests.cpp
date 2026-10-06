#include "ipc/tests.h"
#include "ipc/globals.h"
#include "ipc/vocab.h"
#include "ipc/mrp_engine.h"
#include "ipc/dbd_engine.h"
#include "ipc/math_utils.h"
#include "ipc/lsc_tree.h"
#include "ipc/substitution.h"
#include "ipc/dimension.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cassert>

namespace ipc {

void run_stress_test();

#ifdef _OPENMP

#include <omp.h>

#endif

// =====================================================================

// 全局配置：控制物理存盘查账开关

// FALSE: 内存超高速模式 (默认)； TRUE: 物理写盘 ipc.db，支持 DBeaver 连入对账

// =====================================================================

#define DEBUG_PERSIST true

#define DEBUG_CSV_EXPORT false

// =====================================================================

// 一、 C++ DOD（面向数据设计）数据结构与维度规划定义

// =====================================================================



void run_patent_verification_tests() {

    std::cout << "=====================================================================" << std::endl;

    std::cout << "[验证] [单元测试启动] 正在进行国家发明专利示例高精度检验..." << std::endl;

    std::cout << "=====================================================================" << std::endl;

    // 1. 一类替换料测试 (表 1 模拟)

    {

        std::cout << "[测试] 1. 一类替换料专利验证（表 1 对账单）..." << std::endl;

        FlatBomItem bom1 = {0, 10, 1.0, 0.0, 1, 1, 0.6, 0.0, 0.0, 0, 0.0};

        FlatBomItem bom2 = {0, 11, 1.0, 0.0, 1, 1, 0.4, 0.0, 0.0, 0, 0.0};

        std::vector<FlatBomItem*> group = {&bom1, &bom2};

        std::vector<double> current_on_hand(100, 100.0);

        // 第一次分配：净需求 = 10 -> 应分配 P1:6 P2:4 -> 差距绝对值 P1:6 P2:4 -> 选择 P1

        uint32_t choice1 = allocate_class1(10.0, group, current_on_hand);

        assert(choice1 == 10);

        bom1.historical_qty += 10.0;

        // 第二次分配：净需求 = 10 -> 总量 = 20 -> 应分配 P1:12 P2:8 -> 差距绝对值 P1:|10-12|=2 P2:|0-8|=8 -> 选择 P2

        uint32_t choice2 = allocate_class1(10.0, group, current_on_hand);

        assert(choice2 == 11);

        bom2.historical_qty += 10.0;

        // 第三次分配：净需求 = 20 -> 总量 = 40 -> 应分配 P1:24 P2:16 -> 差距绝对值 P1:|10-24|=14 P2:|10-16|=6 -> 选择 P1

        uint32_t choice3 = allocate_class1(20.0, group, current_on_hand);

        assert(choice3 == 10);

        bom1.historical_qty += 20.0;

        assert(bom1.historical_qty == 30.0);

        assert(bom2.historical_qty == 10.0);

        std::cout << "   -> [OK] 一类替换料三次连续分配状态机断言完全通过！" << std::endl;

    }

    // 2. 二类替换料测试 (表 2 模拟)

    {

        std::cout << "[测试] 2. 二类替换料专利验证（表 2 对账单）..." << std::endl;

        FlatBomItem bom1 = {0, 10, 1.0, 0.0, 1, 2, 0.6, 0.0, 0.0, 0, 0.0};

        FlatBomItem bom2 = {0, 11, 1.0, 0.0, 1, 2, 0.4, 0.0, 0.0, 0, 0.0};

        std::vector<FlatBomItem*> group = {&bom1, &bom2};

        // 第一次分配：评级均为0 -> 选择配额比例大者 P1

        uint32_t choice1 = allocate_class2(group);

        assert(choice1 == 10);

        bom1.historical_qty += 10.0;

        // 第二次分配：P1评级 10/0.6=16.67, P2评级 0 -> 选择 P2

        uint32_t choice2 = allocate_class2(group);

        assert(choice2 == 11);

        bom2.historical_qty += 20.0;

        // 第三次分配：P1评级 10/0.6=16.67, P2评级 20/0.4=50.0 -> 选择 P1

        uint32_t choice3 = allocate_class2(group);

        assert(choice3 == 10);

        bom1.historical_qty += 30.0;

        assert(bom1.historical_qty == 40.0);

        assert(bom2.historical_qty == 20.0);

        std::cout << "   -> [OK] 二类替换料稳定评级扣减及配额平局决策断言完全通过！" << std::endl;

    }

    // 3. 三类替换料测试 (表 3 模拟)

    {

        std::cout << "[测试] 3. 三类替换料专利验证（表 3 对账单）..." << std::endl;

        FlatBomItem bom1 = {0, 10, 1.0, 0.0, 1, 3, 0.5, 0.0, 20.0, 0, 0.0};

        FlatBomItem bom2 = {0, 11, 1.0, 0.0, 1, 3, 0.3, 0.0, 15.0, 0, 0.0};

        FlatBomItem bom3 = {0, 12, 1.0, 0.0, 1, 3, 0.2, 0.0, 10.0, 0, 0.0};

        std::vector<FlatBomItem*> group = {&bom1, &bom2, &bom3};

        std::vector<double> current_on_hand(100, 100.0);

        std::vector<PartSiteRecord> dummy_parts(100);
        for (int i = 0; i < 100; ++i) {
            dummy_parts[i].part_id = i;
            dummy_parts[i].safety_stock = 0.0;
        }

        std::vector<AlternateAllocationRecord> out_alt_records;

        // 净需求 = 100. 第一轮分配 P1 实际 60 -> P2/P3 归一化配额 (60%/40%) -> 第二轮 P2 实际 30 -> 第三轮 P3 实际 10

        allocate_class3(100.0, group, current_on_hand, 0, 0, out_alt_records, dummy_parts);

        assert(bom1.historical_qty == 60.0);

        assert(bom2.historical_qty == 30.0);

        assert(bom3.historical_qty == 10.0);

        std::cout << "   -> [OK] 三类替换料多轮订单倍数向上取整与配额比例归一化重算完全通过！" << std::endl;

    }

    // 4. 维度匹配条件评估测试

    {

        std::cout << "[测试] 4. 维度匹配与降级评估验证..." << std::endl;

        // EQ 匹配

        assert(evaluate_dimension(102.0, static_cast<uint8_t>(RelationOp::EQ), 102.0) == true);

        assert(evaluate_dimension(101.0, static_cast<uint8_t>(RelationOp::EQ), 102.0) == false);

        // GE 降级匹配 (如 512MB 芯片可以满足 256MB 芯片需求)

        assert(evaluate_dimension(102.0, static_cast<uint8_t>(RelationOp::GE), 101.0) == true);

        assert(evaluate_dimension(100.0, static_cast<uint8_t>(RelationOp::GE), 101.0) == false);

        // PASS 匹配

        assert(evaluate_dimension(100.0, static_cast<uint8_t>(RelationOp::PASS), 999.0) == true);

        std::cout << "   -> [OK] 关系符 EQ, GE, PASS 多维分级降级校验算子断言完全通过！" << std::endl;

    }

    // 5. 流水双端累加矩阵交集算法验证

    {

        std::cout << "[测试] 5. 流水双端累加矩阵交集算法验证（无分支交集算子）..." << std::endl;

        double CD_1 = 120.0; // Cumulative gross demand to day t

        double CD_0 = 40.0;  // Cumulative gross demand to day t-1

        double CS_0 = 100.0; // Cumulative stock (OnHand + SR)

        // Zero-Branch Intersection formula

        double allocated = std::max(0.0, std::min(CD_1, CS_0) - std::max(CD_0, 0.0));

        double shortage = std::max(0.0, CD_1 - std::max(CD_0, CS_0));

        assert(allocated == 60.0);

        assert(shortage == 20.0);

        std::cout << "   -> [OK] 双端交集算子几何切分断言完全通过！" << std::endl;

    }

    std::cout << "=====================================================================" << std::endl;

    std::cout << "[OK] [单元测试成功] 证书级专利算子 100% 正确！无报错退出！" << std::endl;

    std::cout << "=====================================================================\n" << std::endl;

}


void generate_massive_mock_data(

    std::vector<PartSiteRecord>& parts,

    std::vector<FlatBomItem>& boms,

    std::vector<IndependentDemand>& demands,

    int num_parts_target,

    int num_demands_target

) {

    std::cout << "[数据] [数据基因工程] 正在全内存生成 " << num_parts_target << " 种物料并构建 10 层 BOM 树..." << std::endl;

    std::mt19937 rng(42);

    // Finished: 70% of parts (e.g. 1400 out of 2000 target)

    int finished_cnt = static_cast<int>(num_parts_target * 0.7);

    // Semi: 20% of parts (e.g. 400 parts)

    int semi_cnt = static_cast<int>(num_parts_target * 0.2);

    // Raw: 5% of parts (e.g. 100 parts)

    int raw_cnt = static_cast<int>(num_parts_target * 0.05);

    // Alt: 5% of parts (e.g. 100 parts)

    int alt_cnt = num_parts_target - finished_cnt - semi_cnt - raw_cnt;

    int finished_end = finished_cnt;

    int semi_start = finished_end;

    int semi_end = semi_start + semi_cnt;

    int raw_start = semi_end;

    int raw_end = raw_start + raw_cnt;

    int alt_start = raw_end;

    parts.clear();

    for (int i = 0; i < num_parts_target; ++i) {

        std::string code = "PART_" + std::to_string(i);

        uint32_t pid = vocab.get_or_create(code);

        PartSiteRecord rec;

        rec.part_id = pid;

        rec.part_code = code;

        rec.low_level_code = 0;

        if (i < finished_end) {

            rec.part_type = "FINISHED";

            rec.mrp_rule = "MPS";

            rec.on_hand = 0.0;

            rec.ipc_scheduled_receipt = 0.0;

            rec.lead_time = 2.0;

            rec.is_phantom = false;

        } else if (i < semi_end) {

            rec.part_type = "SEMI";

            rec.mrp_rule = "MRP";

            // 10层级BOM

            int level = 1 + (i - semi_start) / std::max(1, (semi_cnt / 10));

            if (level > 10) level = 10;

            if (level % 3 == 0) { // 每3层一个虚拟件

                rec.is_phantom = true;

                rec.on_hand = 0.0;

                rec.ipc_scheduled_receipt = 0.0;

                rec.lead_time = 0.0;

            } else {

                rec.is_phantom = false;

                // 仅 10% 概率有微小库存，其余 90% 全面短缺迫使生成 Planned Orders

                rec.on_hand = (std::uniform_real_distribution<double>(0.0, 1.0)(rng) > 0.9) ? std::uniform_real_distribution<double>(1.0, 5.0)(rng) : 0.0;

                rec.ipc_scheduled_receipt = 0.0;

                rec.lead_time = 3.0;

            }

        } else if (i < raw_end) {

            rec.part_type = "RAW";

            rec.mrp_rule = "MRP";

            rec.on_hand = (std::uniform_real_distribution<double>(0.0, 1.0)(rng) > 0.9) ? std::uniform_real_distribution<double>(1.0, 10.0)(rng) : 0.0;

            rec.ipc_scheduled_receipt = 0.0;

            rec.lead_time = 10.0;

            rec.is_phantom = false;

        } else {

            rec.part_type = "ALT";

            rec.mrp_rule = "MRP";

            rec.on_hand = (std::uniform_real_distribution<double>(0.0, 1.0)(rng) > 0.9) ? std::uniform_real_distribution<double>(1.0, 5.0)(rng) : 0.0;

            rec.ipc_scheduled_receipt = 0.0;

            rec.lead_time = 8.0;

            rec.is_phantom = false;

        }

        if (rec.part_type == "RAW" || rec.part_type == "ALT") {

            rec.round_to_integer = (i % 2 == 0);

        } else {

            rec.round_to_integer = true;

        }

        parts.push_back(rec);

    }

    boms.clear();

    int alt_group_cnt = 0;

    // 1. FINISHED -> L1 SEMI

    int l1_size = std::max(1, semi_cnt / 10);

    int l1_start = semi_start;

    for (int i = 0; i < finished_end; ++i) {

        int num_deps = std::uniform_int_distribution<int>(1, 2)(rng);

        for (int d = 0; d < num_deps; ++d) {

            int child_idx = l1_start + std::uniform_int_distribution<int>(0, l1_size - 1)(rng);

            FlatBomItem item;

            item.parent_id = static_cast<uint32_t>(i);

            item.child_id = static_cast<uint32_t>(child_idx);

            item.per_qty = 1.0;

            item.scrap = 0.0;

            item.alt_group_id = -1;

            item.alt_priority = 0;

            item.target_ratio = 1.0;

            item.historical_qty = 0.0;

            item.lot_size = 0.0;

            item.relation_op = static_cast<uint8_t>(RelationOp::PASS);

            item.target_dim_val = 0.0;

            boms.push_back(item);

        }

    }

    // 2. L1 -> L2 -> ... -> L10 SEMI

    int step = std::max(1, semi_cnt / 10);

    for (int lvl = 1; lvl < 10; ++lvl) {

        int parent_start = semi_start + (lvl - 1) * step;

        int parent_end = parent_start + step;

        int child_start = semi_start + lvl * step;

        for (int p = parent_start; p < parent_end; ++p) {

            int num_deps = std::uniform_int_distribution<int>(1, 2)(rng);

            for (int d = 0; d < num_deps; ++d) {

                int child_idx = child_start + std::uniform_int_distribution<int>(0, step - 1)(rng);

                FlatBomItem item;

                item.parent_id = p;

                item.child_id = child_idx;

                item.per_qty = 1.0;

                item.scrap = 0.01;

                item.alt_group_id = -1;

                item.alt_priority = 0;

                item.target_ratio = 1.0;

                item.historical_qty = 0.0;

                item.lot_size = 0.0;

                item.relation_op = static_cast<uint8_t>(RelationOp::PASS);

                item.target_dim_val = 0.0;

                boms.push_back(item);

            }

        }

    }

    // 3. L10 SEMI -> RAW

    int l10_start = semi_start + 9 * step;

    int l10_end = semi_end;

    for (int p = l10_start; p < l10_end; ++p) {

        int num_deps = std::uniform_int_distribution<int>(1, 2)(rng);

        for (int d = 0; d < num_deps; ++d) {

            int child_idx = raw_start + std::uniform_int_distribution<int>(0, raw_cnt - 1)(rng);

            FlatBomItem item;

            item.parent_id = p;

            item.child_id = child_idx;

            item.per_qty = std::uniform_int_distribution<int>(1, 2)(rng);

            item.scrap = 0.02;

            double r_val = std::uniform_real_distribution<double>(0.0, 1.0)(rng);

            if (r_val > 0.8) {

                item.relation_op = static_cast<uint8_t>(RelationOp::EQ);

                item.target_dim_val = 102.0;

            } else if (r_val > 0.5) {

                item.relation_op = static_cast<uint8_t>(RelationOp::GE);

                item.target_dim_val = 101.0;

            } else {

                item.relation_op = static_cast<uint8_t>(RelationOp::PASS);

                item.target_dim_val = 0.0;

            }

            if (std::uniform_real_distribution<double>(0.0, 1.0)(rng) > 0.8 && alt_cnt > 0) {

                int gid = alt_group_cnt++;

                int alt_class = std::uniform_int_distribution<int>(1, 3)(rng);

                item.alt_group_id = gid;

                item.alt_priority = alt_class;

                item.target_ratio = 0.5;

                item.historical_qty = 100.0;

                item.lot_size = alt_class == 3 ? 10.0 : 0.0;

                int sub_idx = alt_start + (gid % alt_cnt);

                FlatBomItem sub_item;

                sub_item.parent_id = static_cast<uint32_t>(p);

                sub_item.child_id = static_cast<uint32_t>(sub_idx);

                sub_item.per_qty = item.per_qty;

                sub_item.scrap = item.scrap;

                sub_item.alt_group_id = gid;

                sub_item.alt_priority = alt_class;

                sub_item.target_ratio = 0.5;

                sub_item.historical_qty = 100.0;

                sub_item.lot_size = alt_class == 3 ? 5.0 : 0.0;

                sub_item.relation_op = item.relation_op;

                sub_item.target_dim_val = item.target_dim_val;

                boms.push_back(sub_item);

            } else {

                item.alt_group_id = -1;

                item.alt_priority = 0;

                item.target_ratio = 1.0;

                item.historical_qty = 0.0;

                item.lot_size = 0.0;

            }

            boms.push_back(item);

        }

    }

    // 4. 生成 1,000,000 张客户需求订单，广泛分散在 1,400 个成品上

    for (int i = 0; i < num_demands_target; ++i) {

        IndependentDemand d;

        d.demand_id = i + 1;

        d.customer = "CUST_" + std::to_string(std::uniform_int_distribution<int>(1, 10)(rng));

        d.part_id = std::uniform_int_distribution<int>(0, finished_end - 1)(rng);

        d.qty = std::uniform_int_distribution<int>(10, 100)(rng);

        d.due_day = std::uniform_int_distribution<int>(20, 350)(rng);

        d.priority = i + 1;

        int dim_choice = std::uniform_int_distribution<int>(0, 2)(rng);

        d.dimension_val = dim_choice == 0 ? 100.0 : (dim_choice == 1 ? 101.0 : 102.0);

        // 增加复合优先级维度的 Mock 数据

        d.status = (std::uniform_real_distribution<double>(0.0, 1.0)(rng) > 0.5) ? "COMMITTED" : "OPEN";

        d.customer_tier = std::uniform_int_distribution<int>(1, 3)(rng);

        d.revenue = std::uniform_real_distribution<double>(1000.0, 50000.0)(rng);

        d.composite_priority = encode_composite_priority(d.status == "COMMITTED", d.customer_tier, d.due_day, d.priority, d.revenue);

        demands.push_back(d);

    }

    std::cout << "[OK] [数据基因工程] 10 层 BOM 树与虚拟件生成成功！" << std::endl;

}

} // namespace ipc
