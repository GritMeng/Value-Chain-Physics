#pragma once
#include "ipc_types.h"
#include <vector>

namespace ipc {
    void run_patent_verification_tests();
    
    void generate_massive_mock_data(
        std::vector<PartSiteRecord>& parts,
        std::vector<FlatBomItem>& boms,
        std::vector<IndependentDemand>& demands,
        int num_parts_target = 10000,
        int num_demands_target = 3000000
    );

    void run_stress_test();
}
