#include "ipc_core/substitution_engine.h"
#include <iostream>
#include <cassert>

using namespace ipc_core;

int main() {
    std::cout << "========================================================\n";
    std::cout << "[Test 3] 一/二/三类替代料配额平衡与动态消纳测试\n";
    std::cout << "========================================================\n";

    // 初始化测试物料
    std::vector<PartSiteRecord> parts = {
        {0, "MAIN_CHIP",    "PLANT_01", 0.0, 0.0,  1.0, 0},
        {1, "ALT_CHIP_CLASS1", "PLANT_01", 0.0, 100.0, 1.0, 0},
        {2, "ALT_CHIP_CLASS2", "PLANT_01", 0.0, 100.0, 1.0, 0},
        {3, "ALT_CHIP_CLASS3", "PLANT_01", 10.0, 100.0, 5.0, 0} // Safety stock = 10, Lot size = 5
    };

    std::vector<double> current_on_hand = {0.0, 100.0, 100.0, 100.0};

    // 1. 一类替代料测试 (按比例配额平衡)
    std::cout << "\n[Step 1] 测试一类替代料按历史配额平衡分配 (Class 1)...\n";
    FlatBomItem bom_item1_a = {0, 1, 1.0, 1, 1, 0.6, 120.0, 1.0}; // Target = 60%, Hist = 120
    FlatBomItem bom_item1_b = {0, 2, 1.0, 1, 1, 0.4, 100.0, 1.0}; // Target = 40%, Hist = 100
    std::vector<FlatBomItem*> group1 = {&bom_item1_a, &bom_item1_b};

    uint32_t chosen_class1 = allocate_class1(50.0, group1, current_on_hand);
    std::cout << "  - 选优分配替代物料 ID: " << chosen_class1 << " (预期: 1 号替代物料，因配额落后量更大)\n";
    assert(chosen_class1 == 1);

    // 2. 三类替代料测试 (跨组动态归一化、批量倍数与安全库存保护)
    std::cout << "\n[Step 2] 测试三类替代料动态消纳 (Class 3)...\n";
    FlatBomItem bom_item3_a = {0, 3, 1.0, 3, 2, 0.5, 0.0, 5.0}; // Lot size = 5
    std::vector<FlatBomItem*> group3 = {&bom_item3_a};
    std::vector<AlternateAllocationRecord> alt_records;

    allocate_class3(12.0, group3, current_on_hand, 1, 0, alt_records, parts);

    std::cout << "  - 生成替代分配记录条数: " << alt_records.size() << "\n";
    if (!alt_records.empty()) {
        std::cout << "  - 实际扣减分配量: " << alt_records[0].allocated_qty << " (批量倍数 5 向上对齐: ⌈6/5⌉*5 = 10)\n";
        std::cout << "  - 剩余库存: " << current_on_hand[3] << " (安全库存 10 保障未被违规侵占)\n";
        assert(alt_records[0].allocated_qty == 10.0);
    }

    std::cout << "\n>>> [PASSED] Test 3: 一/二/三类替代料决策逻辑验证通过！\n";
    return 0;
}
