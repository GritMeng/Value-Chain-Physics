#pragma once
#include "ipc_types.h"
#include "duckdb.hpp"
#include <vector>
#include <unordered_map>
#include <string>

namespace ipc {
    double get_double_value(const duckdb::Value& val);
    int32_t get_int_value(const duckdb::Value& val);

    void load_operations_from_db(
        duckdb::Connection &con,
        std::vector<std::vector<OperationRecord>> &part_routings,
        std::unordered_map<std::string, std::vector<double>> &wc_daily_capacity,
        size_t num_parts
    );

    class HierarchyResolver;

    class DbAdapter {
    public:
        explicit DbAdapter(duckdb::Connection& con);
        void load_calendars(std::unordered_map<std::string, CalendarRecord>& calendars);
        void load_parts(std::vector<PartSiteRecord>& parts);
        void load_boms(std::vector<FlatBomItem>& boms);
        void load_demands(std::vector<IndependentDemand>& demands);
        void load_scheduled_receipts(std::vector<ScheduledReceiptRecord>& srs);
        void load_operations(std::vector<OperationRecord>& operations);
        void load_product_hierarchy(HierarchyResolver& product_resolver);
        void load_customer_hierarchy(HierarchyResolver& customer_resolver);
        void load_region_hierarchy(HierarchyResolver& region_resolver);
        void load_customer_comb_hierarchy(std::vector<CustomerCombRecord>& comb_records);
    private:
        duckdb::Connection& connection;
    };

    void generate_pegging_records(
        const std::vector<PartSiteRecord>& parts,
        const std::vector<IndependentDemand>& demands,
        const std::vector<PlannedOrder>& ipc_planned_orders,
        const std::vector<AlternateAllocationRecord>& alt_records,
        std::vector<SupplyAssignmentRecord>& pegging,
        const std::vector<ScheduledReceiptRecord>& srs = std::vector<ScheduledReceiptRecord>()
    );
}

