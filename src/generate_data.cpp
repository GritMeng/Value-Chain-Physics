// generate_data.cpp
// Simple synthetic data generator for IPC engine
// Creates a DuckDB database (ipc.db) and populates a few core tables with random data.

#include <duckdb.hpp>
#include <iostream>
#include <random>
#include <vector>

int main() {
    try {
        // Open (or create) DuckDB database file
        duckdb::DuckDB db("data/ipc.db");
        duckdb::Connection con(db);

        // Example: create a minimal schema for demonstration
        con.Query("CREATE TABLE IF NOT EXISTS demands (demand_id BIGINT PRIMARY KEY, customer VARCHAR, part_id BIGINT, qty DOUBLE, due_day INTEGER, priority INTEGER);");
        con.Query("CREATE TABLE IF NOT EXISTS parts (part_id BIGINT PRIMARY KEY, part_code VARCHAR, on_hand DOUBLE, cost DOUBLE);");

        // Generate random data
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> custDist(1, 10);
        std::uniform_int_distribution<int> partDist(1000, 1100);
        std::uniform_real_distribution<double> qtyDist(1.0, 1000.0);
        std::uniform_int_distribution<int> dayDist(1, 90);
        std::uniform_int_distribution<int> prioDist(1, 5);
        std::uniform_real_distribution<double> costDist(0.5, 10.0);

        // Insert 500 synthetic demand records (adjust as needed)
        for (int i = 1; i <= 500; ++i) {
            long long demand_id = i;
            std::string customer = "CUST_" + std::to_string(custDist(rng));
            long long part_id = partDist(rng);
            double qty = qtyDist(rng);
            int due_day = dayDist(rng);
            int priority = prioDist(rng);
            std::string sql = "INSERT INTO demands VALUES (" + std::to_string(demand_id) + ", '" + customer + "', " + std::to_string(part_id) + ", " + std::to_string(qty) + ", " + std::to_string(due_day) + ", " + std::to_string(priority) + ");";
            con.Query(sql);
        }

        // Insert 200 synthetic part records
        for (int i = 0; i < 200; ++i) {
            long long part_id = partDist(rng);
            std::string part_code = "PART_" + std::to_string(part_id);
            double on_hand = qtyDist(rng);
            double cost = costDist(rng);
            std::string sql = "INSERT INTO parts VALUES (" + std::to_string(part_id) + ", '" + part_code + "', " + std::to_string(on_hand) + ", " + std::to_string(cost) + ");";
            con.Query(sql);
        }

        std::cout << "Synthetic data generation completed. Database: ipc.db" << std::endl;
    } catch (const std::exception &ex) {
        std::cerr << "Error during data generation: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
