#include <chrono>
#include <iostream>
#include <vector>
#include "ipc_types.h"

#include "csv_export.h"

int main() {
    const size_t N = 1000000; // 1 million orders
    std::vector<PlannedOrder> orders;
    orders.reserve(N);
    // Generate dummy data
    for (size_t i = 0; i < N; ++i) {
        PlannedOrder o;
        o.part_id = static_cast<uint32_t>(i % 1000);
        o.qty = static_cast<double>(i % 500 + 1);
        o.start_day = static_cast<int>(i % 365);
        o.finish_day = o.start_day + static_cast<int>(o.qty / 10);
        o.dimension_val = static_cast<double>(i % 100) / 10.0;
        orders.push_back(o);
    }

    auto start = std::chrono::high_resolution_clock::now();
    csv_export_ipc_planned_orders(orders);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Exported " << N << " planned orders in " << elapsed.count() << " seconds." << std::endl;
    return 0;
}
