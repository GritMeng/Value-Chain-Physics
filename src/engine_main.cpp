#pragma execution_character_set("utf-8")
#pragma warning(disable : 4146)
#pragma warning(disable : 4996)

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <mutex>
#include "duckdb.hpp"

#include "ipc_types.h"
#include "csv_export.h"
#include "ipc/globals.h"
#include "ipc/vocab.h"
#include "ipc/mrp_engine.h"
#include "ipc/dbd_engine.h"
#include "ipc/database.h"
#include "ipc/coproduct.h"
#include "ipc/tests.h"
#include "ipc/lsc_tree.h"
#include "ipc/math_utils.h"
#include "ipc/scheduling.h"
#include "ipc/hierarchy.h"

using namespace ipc;

int main(int argc, char* argv[]) {

    // A. 首先运行高置信度国家专利实例断言测试，有一丝算力偏差则立刻阻断

    ipc::run_patent_verification_tests();

    // 检查是否需要运行压力测试

    std::string db_path = "data/ipc.db";
    std::string solver_mode = "iop";
    std::string solver_step = "all"; // "all", "lbl", "dbd"
    std::string scenario_id = "baseline";
    bool start_repl = false;
    bool mode_overridden = false;
    bool step_overridden = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--stress") {
            ipc::run_stress_test();
            return 0;
        } else if (std::string(argv[i]) == "--db" && i + 1 < argc) {
            db_path = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "--mode" && i + 1 < argc) {
            solver_mode = argv[i + 1];
            mode_overridden = true;
            i++;
        } else if (std::string(argv[i]) == "--step" && i + 1 < argc) {
            solver_step = argv[i + 1];
            step_overridden = true;
            i++;
        } else if (std::string(argv[i]) == "--scenario" && i + 1 < argc) {
            scenario_id = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "--repl") {
            start_repl = true;
        }
    }

    try {

        // B. 初始化数据库引擎 (根据开关持久化到 ipc.db 或纯内存)

        std::unique_ptr<duckdb::DuckDB> db;

        if (DEBUG_PERSIST) {
            std::cout << "[DB] 持久化查账模式开启，目标文件：" << db_path << std::endl;
            db = std::make_unique<duckdb::DuckDB>(db_path);

        } else {            db = std::make_unique<duckdb::DuckDB>(nullptr);

        }

        duckdb::Connection con(*db);

        std::cout << "=====================================================================" << std::endl;

        std::cout << "[点火] [MRP 极致完全体] C++ DOD 动态维度与专利分配引擎 安全点火..." << std::endl;

        std::cout << "=====================================================================" << std::endl;

        // C. 数据载体准备

        std::vector<PartSiteRecord> parts;

        std::vector<FlatBomItem> boms;

        std::vector<IndependentDemand> demands;

        std::vector<PlannedOrder> ipc_planned_orders;

        std::vector<AlternateAllocationRecord> alt_records;
        uint32_t wildcard_id = ipc::vocab.get_or_create("*");
        std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash> allotment_constraints;

        // D. Check for existing legacy tables and run migration

        // Drop any potential legacy indexes that cause rename dependency errors

        con.Query("DROP INDEX IF EXISTS idx_id_demand;");

        con.Query("DROP INDEX IF EXISTS idx_psa_demand;");

        con.Query("DROP INDEX IF EXISTS idx_po_part;");

        con.Query("DROP INDEX IF EXISTS idx_po_finish;");

        con.Query("DROP INDEX IF EXISTS idx_parts_part;");

        // Safe database migration for all eras (Legacy -> Advanced / Interim -> Advanced)

        std::vector<std::pair<std::string, std::string>> all_migrations = {

            {"part_site", "ipc_material_node"},

            {"oh_inventory", "ipc_onhand"},

            {"scheduled_receipt", "ipc_scheduled_receipt"},

            {"part_routing_bom", "ipc_bom_route"},

            {"bom_item", "ipc_bom_item"},

            {"independent_demand", "ipc_independent_demand"},

            {"planned_order", "ipc_planned_order"},

            {"swap_result", "ipc_swap_result"},

            {"supply_assignment", "ipc_supply_assignment"},

            {"planned_supply_assignment", "ipc_planned_supply_assignment"},

            // Legacy co-products

            {"coproduct_dimension", "ipc_coproduct_dimension"},

            {"coproduct_dimension_grp", "ipc_coproduct_grouping"},

            {"coproduct_part_routing_bom", "ipc_coproduct_recipe"},

            {"coproduct_demand", "ipc_coproduct_demand"},

            {"coproduct_grp_allocation", "ipc_coproduct_allocation"},

            {"coproduct_schedule_result", "ipc_coproduct_schedule"},

            // Interim to Advanced (string concatenation prevents rename_advanced script from self-referencing replacement)

            {"ipc_suggested_" "replenishment", "ipc_planned_" "supply"},

            {"ipc_pegging_" "ledger", "ipc_firm_" "pegging_ledger"},

            {"ipc_substitution_" "journal", "ipc_swap_" "dispatch_ledger"},

            {"ipc_suggested_" "replenishment_result", "ipc_planned_" "supply_ledger"},

            {"ipc_engine_" "schedule_result", "ipc_micro_" "dispatch_ledger"},

            {"ipc_substitution_" "result", "ipc_alternate_" "allocation"},

            {"ipc_material_" "node_result", "ipc_material_" "node_status"},

            {"ipc_bom_" "lsctree_result", "ipc_bom_" "explosion_network"},

            {"ipc_co_product_" "dimension", "ipc_coproduct_" "dimension"},

            {"ipc_co_product_" "dimension_grouping", "ipc_coproduct_" "grouping"},

            {"ipc_co_product_" "route_recipe", "ipc_coproduct_" "recipe"},

            {"ipc_co_product_" "external_demand", "ipc_coproduct_" "demand"},

            {"ipc_co_product_" "allocation_journal", "ipc_coproduct_" "allocation"},

            {"ipc_co_product_" "schedule_result", "ipc_coproduct_" "schedule"}

        };

        for (const auto& pair : all_migrations) {

            auto check = con.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = '" + pair.first + "';");

            if (check && !check->HasError() && check->RowCount() > 0 && check->GetValue(0, 0).GetValue<int64_t>() > 0) {

                std::cout << "[DB MIGRATION] Migrating table '" << pair.first << "' -> '" << pair.second << "'..." << std::endl;

                con.Query("ALTER TABLE " + pair.first + " RENAME TO " + pair.second + ";");

            }

        }

        // Correct misspelled tables and migrate to new names if they exist

        auto check_old_dim = con.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'deminsion';");

        if (check_old_dim && !check_old_dim->HasError() && check_old_dim->RowCount() > 0 && check_old_dim->GetValue(0, 0).GetValue<int64_t>() > 0) {

            con.Query("CREATE TABLE IF NOT EXISTS ipc_dimension_attribute (dimension VARCHAR, description VARCHAR, value DOUBLE, value_description VARCHAR);");

            con.Query("INSERT INTO ipc_dimension_attribute (dimension, description, value, value_description) SELECT demision, descriotion, value, value_description FROM deminsion;");

            con.Query("DROP TABLE deminsion;");

            std::cout << "[DB MIGRATION] Corrected misspelled 'deminsion' table." << std::endl;

        }

        auto check_old_dim_grp = con.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'demision_grp';");

        if (check_old_dim_grp && !check_old_dim_grp->HasError() && check_old_dim_grp->RowCount() > 0 && check_old_dim_grp->GetValue(0, 0).GetValue<int64_t>() > 0) {

            con.Query("CREATE TABLE IF NOT EXISTS ipc_dimension_grouping (dimension_grp VARCHAR, dimension VARCHAR, value DOUBLE, relation_ship VARCHAR);");

            con.Query("INSERT INTO ipc_dimension_grouping (dimension_grp, dimension, value, relation_ship) SELECT demision_grp, demision, value, relation_ship FROM demision_grp;");

            con.Query("DROP TABLE demision_grp;");

            std::cout << "[DB MIGRATION] Corrected misspelled 'demision_grp' table." << std::endl;

        }

        // Interim stock and demand name check (specific cases)

        auto check_interim_stock = con.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'ipc_physical_stock';");

        if (check_interim_stock && !check_interim_stock->HasError() && check_interim_stock->RowCount() > 0 && check_interim_stock->GetValue(0, 0).GetValue<int64_t>() > 0) {

            con.Query("ALTER TABLE ipc_physical_stock RENAME TO ipc_onhand;");

        }

        auto check_interim_demand = con.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'ipc_external_commitment';");

        if (check_interim_demand && !check_interim_demand->HasError() && check_interim_demand->RowCount() > 0 && check_interim_demand->GetValue(0, 0).GetValue<int64_t>() > 0) {

            con.Query("ALTER TABLE ipc_external_commitment RENAME TO ipc_independent_demand;");

        }

        std::cout << "[DB MIGRATION] Migration checks successfully completed!" << std::endl;
        con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS lot_size DOUBLE DEFAULT 0.0;");
        con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS eff_start_day INTEGER DEFAULT -1;");
        con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS eff_end_day INTEGER DEFAULT -1;");
        con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS ltb_limit DOUBLE DEFAULT -1.0;");
        con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS mix_group_id INTEGER DEFAULT -1;");
        con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS relationship_type VARCHAR DEFAULT 'alt';");

        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS planning_calendar VARCHAR DEFAULT 'DEFAULT';");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS ss_rule VARCHAR DEFAULT 'None';");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS dos_policy VARCHAR DEFAULT 'None';");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS dos_intervals DOUBLE DEFAULT 0.0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS safety_stock DOUBLE DEFAULT 0.0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS ss_fixed_qty DOUBLE DEFAULT 0.0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS transshipment_cost DOUBLE DEFAULT 0.0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS transshipment_lead_time INTEGER DEFAULT 0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS run_rate DOUBLE DEFAULT 0.0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS lead_time DOUBLE;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS round_to_integer BOOLEAN;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS on_hand_type VARCHAR DEFAULT 'Standard';");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS time_fence_days INTEGER DEFAULT 0;");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS sourcing_policy VARCHAR DEFAULT 'Standard';");
        con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS selling_ave_price DOUBLE DEFAULT 0.0;");

        con.Query("ALTER TABLE ipc_independent_demand ADD COLUMN IF NOT EXISTS preference_mode VARCHAR DEFAULT 'N';");
        con.Query("ALTER TABLE ipc_independent_demand ADD COLUMN IF NOT EXISTS customer_tier INTEGER DEFAULT 3;");
        con.Query("ALTER TABLE ipc_independent_demand ADD COLUMN IF NOT EXISTS revenue DOUBLE DEFAULT 0.0;");

        // Load solver config from database if not overridden by CLI
        bool has_config_table = false;
        auto check_config_tbl = con.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'ipc_solver_config';");
        if (check_config_tbl && !check_config_tbl->HasError() && check_config_tbl->RowCount() > 0) {
            if (check_config_tbl->GetValue(0, 0).GetValue<int64_t>() > 0) {
                has_config_table = true;
            }
        }
        if (has_config_table) {
            auto config_res = con.Query("SELECT param_name, param_value FROM ipc_solver_config;");
            if (config_res && !config_res->HasError()) {
                for (size_t row = 0; row < config_res->RowCount(); ++row) {
                    std::string name = config_res->GetValue(0, row).ToString();
                    std::string val = config_res->GetValue(1, row).ToString();
                    if (name == "solver_mode" && !mode_overridden) {
                        solver_mode = val;
                        std::cout << "[配置] 从数据库加载 solver_mode = " << solver_mode << std::endl;
                    } else if (name == "solver_step" && !step_overridden) {
                        solver_step = val;
                        std::cout << "[配置] 从数据库加载 solver_step = " << solver_step << std::endl;
                    } else if (name == "scenario_id") {
                        scenario_id = val;
                        std::cout << "[配置] 从数据库加载 scenario_id = " << scenario_id << std::endl;
                    }
                }
            }
        }

        auto check_res = con.Query("SELECT COUNT(*) FROM ipc_independent_demand;");

        bool db_is_empty = true;

        if (check_res && !check_res->HasError() && check_res->RowCount() > 0) {

            db_is_empty = (check_res->GetValue(0, 0).GetValue<int64_t>() == 0);

        }

        if (db_is_empty) {

            std::cout << "[DB] 数据库为空，正在创建物理表结构并初始化写入 199 逻辑模型主数据..." << std::endl;

            con.Query("DROP VIEW IF EXISTS ipc_material_node;");
            con.Query("DROP TABLE IF EXISTS ipc_material_node;");
            con.Query("CREATE TABLE ipc_material_node (part VARCHAR, part_type VARCHAR, mrp_rule VARCHAR, site VARCHAR, is_phantom BOOLEAN, selling_ave_price DOUBLE DEFAULT 0.0, transshipment_cost DOUBLE DEFAULT 0.0, transshipment_lead_time INTEGER DEFAULT 0, run_rate DOUBLE DEFAULT 0.0, lead_time DOUBLE, round_to_integer BOOLEAN, on_hand_type VARCHAR DEFAULT 'Standard', time_fence_days INTEGER DEFAULT 0, sourcing_policy VARCHAR DEFAULT 'Standard', safety_stock DOUBLE DEFAULT 0.0, ss_fixed_qty DOUBLE DEFAULT 0.0, ss_rule VARCHAR DEFAULT 'None', dos_policy VARCHAR DEFAULT 'None', dos_intervals DOUBLE DEFAULT 0.0, planning_calendar VARCHAR DEFAULT 'DEFAULT', safety_stock_value DOUBLE GENERATED ALWAYS AS (safety_stock * selling_ave_price));");

            con.Query("DROP TABLE IF EXISTS ipc_onhand;");
            con.Query("CREATE TABLE ipc_onhand (location VARCHAR, part VARCHAR, site VARCHAR, available_date DATE, qty DOUBLE, inventory_type VARCHAR);");

            con.Query("DROP TABLE IF EXISTS ipc_scheduled_receipt;");
            con.Query("CREATE TABLE ipc_scheduled_receipt (sr_id VARCHAR, to_part VARCHAR, qty DOUBLE, to_site VARCHAR, request_due_date DATE, supply_status VARCHAR);");

            con.Query("DROP TABLE IF EXISTS ipc_bom_route;");
            con.Query("CREATE TABLE ipc_bom_route (site VARCHAR, part VARCHAR, bomid VARCHAR, priority INTEGER, bom_type VARCHAR);");

            con.Query("DROP TABLE IF EXISTS ipc_bom_item;");
            con.Query("CREATE TABLE ipc_bom_item (bomid VARCHAR, site VARCHAR, component VARCHAR, perqty DOUBLE, scrap DOUBLE, alt_grp VARCHAR, priority INTEGER, target DOUBLE, alt_todate_qty DOUBLE, lot_size DOUBLE DEFAULT 0.0, eff_start_day INTEGER, eff_end_day INTEGER, ltb_limit DOUBLE, mix_group_id INTEGER, relationship_type VARCHAR);");

            con.Query("DROP TABLE IF EXISTS ipc_independent_demand;");
            con.Query("CREATE TABLE ipc_independent_demand (demand VARCHAR, item DOUBLE, part VARCHAR, par_site VARCHAR, customer VARCHAR, request_delivery_date DATE, request_due_date DATE, open_qty DOUBLE, request_qty DOUBLE, status VARCHAR, order_priority INTEGER, site VARCHAR, dimension_grp VARCHAR, preference_mode VARCHAR, customer_tier INTEGER, revenue DOUBLE);");

            con.Query("DROP TABLE IF EXISTS ipc_swap_result;");
            con.Query("CREATE TABLE ipc_swap_result (demand_code VARCHAR, from_part VARCHAR, to_part VARCHAR, swapped_qty DOUBLE, day INTEGER, alt_group VARCHAR, swap_reason VARCHAR);");

            con.Query("CREATE TABLE IF NOT EXISTS ipc_planned_order (ipc_planned_order VARCHAR, request_start_date DATE, due_date DATE, qty DOUBLE, eff_qty DOUBLE, dimension_grp VARCHAR, is_planned BOOLEAN, part VARCHAR, source VARCHAR, site VARCHAR);");

            con.Query("CREATE TABLE IF NOT EXISTS ipc_dimension_attribute (dimension VARCHAR, description VARCHAR, value DOUBLE, value_description VARCHAR);");

            con.Query("CREATE TABLE IF NOT EXISTS ipc_dimension_grouping (ipc_dimension_grouping VARCHAR, dimension VARCHAR, value DOUBLE, relation_ship VARCHAR);");

            con.Query("CREATE TABLE IF NOT EXISTS ipc_supply_assignment (demand VARCHAR, item DOUBLE, ind_part VARCHAR, location VARCHAR, part VARCHAR, site VARCHAR, due_date DATE, supply VARCHAR, supply_type VARCHAR, assigned_qty DOUBLE, dimension_grp VARCHAR, available_date DATE);");

            con.Query("CREATE TABLE IF NOT EXISTS ipc_planned_supply_assignment (demand VARCHAR, item DOUBLE, location VARCHAR, part VARCHAR, site VARCHAR, due_date DATE, supply VARCHAR, supply_type VARCHAR, assigned_qty DOUBLE, dimension_grp VARCHAR, available_date DATE, ipc_planned_order VARCHAR);");
            con.Query("CREATE TABLE IF NOT EXISTS ipc_allotment_constraint (scenario_id VARCHAR, part_code VARCHAR, site_code VARCHAR, region VARCHAR DEFAULT '*', customer_group VARCHAR DEFAULT '*', product_family VARCHAR, day INTEGER, itp_calculated_qty DOUBLE, override_qty DOUBLE, is_locked BOOLEAN, PRIMARY KEY(scenario_id, part_code, site_code, region, customer_group, product_family, day));");
            con.Query("CREATE TABLE IF NOT EXISTS ipc_allotment_ledger (scenario_id VARCHAR, part_code VARCHAR, site_code VARCHAR, region VARCHAR DEFAULT '*', customer_group VARCHAR DEFAULT '*', product_family VARCHAR, day INTEGER, allotment_limit DOUBLE, consumed_qty DOUBLE, available_qty DOUBLE, blocked_demand_qty DOUBLE, PRIMARY KEY(scenario_id, part_code, site_code, region, customer_group, product_family, day));");

            std::vector<PartSiteRecord> temp_parts;

            std::vector<FlatBomItem> temp_boms;

            std::vector<IndependentDemand> temp_demands;

            ipc::generate_massive_mock_data(temp_parts, temp_boms, temp_demands, 10000, 3000000);

            std::cout << "[Debug] vocab size after mock: " << ipc::vocab.size() << std::endl;

            if (ipc::vocab.size() > 0) {

                std::cout << "[Debug] vocab[0]: " << ipc::vocab.get_code(0) << std::endl;

                std::cout << "[Debug] temp_boms count: " << temp_boms.size() << std::endl;

                if (!temp_boms.empty()) {

                    std::cout << "[Debug] temp_boms[0] parent_id: " << temp_boms[0].parent_id << " child_id: " << temp_boms[0].child_id << std::endl;

                    std::cout << "[Debug] temp_boms[0] parent_code: " << ipc::vocab.get_code(temp_boms[0].parent_id) << " child_code: " << ipc::vocab.get_code(temp_boms[0].child_id) << std::endl;

                }

            }

            con.Query("CREATE TEMP TABLE temp_ipc_material_node (part VARCHAR, part_type VARCHAR, mrp_rule VARCHAR, is_phantom BOOLEAN, site VARCHAR, transshipment_cost DOUBLE, transshipment_lead_time INTEGER);");

            con.Query("CREATE TEMP TABLE temp_ipc_onhand (part VARCHAR, qty DOUBLE);");

            con.Query("CREATE TEMP TABLE temp_ipc_scheduled_receipt (part VARCHAR, qty DOUBLE);");

            con.Query("CREATE TEMP TABLE temp_boms (parent VARCHAR, child VARCHAR, per_qty DOUBLE, scrap DOUBLE, alt_group_id INTEGER, alt_priority INTEGER, target_ratio DOUBLE, historical_qty DOUBLE, lot_size DOUBLE, eff_start_day INTEGER, eff_end_day INTEGER, ltb_limit DOUBLE, mix_group_id INTEGER, relationship_type VARCHAR);");

            con.Query("CREATE TEMP TABLE temp_demands (demand_id INTEGER, customer VARCHAR, part VARCHAR, qty DOUBLE, due_day INTEGER, priority INTEGER, dimension_val DOUBLE, preference_mode VARCHAR, status VARCHAR, customer_tier INTEGER, revenue DOUBLE);");

            {

                duckdb::Appender app_part(con, "temp_ipc_material_node");

                duckdb::Appender app_oh(con, "temp_ipc_onhand");

                duckdb::Appender app_sr(con, "temp_ipc_scheduled_receipt");

                for (const auto& p : temp_parts) {

                    app_part.BeginRow();

                    app_part.Append(p.part_code.c_str());

                    app_part.Append(p.part_type.c_str());

                    app_part.Append(p.mrp_rule.c_str());

                    app_part.Append<bool>(p.is_phantom);

                    app_part.Append(p.site.c_str());

                    app_part.Append<double>(p.transshipment_cost);

                    app_part.Append<int32_t>(p.transshipment_lead_time);

                    app_part.EndRow();

                    app_oh.BeginRow();

                    app_oh.Append(p.part_code.c_str());

                    app_oh.Append<double>(p.on_hand);

                    app_oh.EndRow();

                    if (p.ipc_scheduled_receipt > 0.0) {

                        app_sr.BeginRow();

                        app_sr.Append(p.part_code.c_str());

                        app_sr.Append<double>(p.ipc_scheduled_receipt);

                        app_sr.EndRow();

                    }

                }

            }

            {

                duckdb::Appender app_bom(con, "temp_boms");

                for (const auto& b : temp_boms) {

                    app_bom.BeginRow();

                    app_bom.Append(get_raw_part_code(ipc::vocab.get_code(b.parent_id)).c_str());

                    app_bom.Append(get_raw_part_code(ipc::vocab.get_code(b.child_id)).c_str());

                    app_bom.Append<double>(b.per_qty);

                    app_bom.Append<double>(b.scrap);

                    app_bom.Append<int32_t>(b.alt_group_id);

                    app_bom.Append<int32_t>(b.alt_priority);

                    app_bom.Append<double>(b.target_ratio);

                    app_bom.Append<double>(b.historical_qty);

                    app_bom.Append<double>(b.lot_size);

                    app_bom.Append<int32_t>(b.eff_start_day);

                    app_bom.Append<int32_t>(b.eff_end_day);

                    app_bom.Append<double>(b.ltb_limit);

                    app_bom.Append<int32_t>(b.mix_group_id);

                    app_bom.Append(b.relationship_type.c_str());

                    app_bom.EndRow();

                }

            }

            {

                duckdb::Appender app_dem(con, "temp_demands");

                for (const auto& d : temp_demands) {

                    app_dem.BeginRow();

                    app_dem.Append<int32_t>(d.demand_id);

                    app_dem.Append(d.customer.c_str());

                    app_dem.Append(get_raw_part_code(ipc::vocab.get_code(d.part_id)).c_str());

                    app_dem.Append<double>(d.qty);

                    app_dem.Append<int32_t>(d.due_day);

                    app_dem.Append<int32_t>(d.priority);

                    app_dem.Append<double>(d.dimension_val);

                    app_dem.Append(d.preference_mode.c_str());

                    app_dem.Append(d.status.c_str());

                    app_dem.Append<int32_t>(d.customer_tier);

                    app_dem.Append<double>(d.revenue);

                    app_dem.EndRow();

                }

            }

            con.Query("INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, selling_ave_price, transshipment_cost, transshipment_lead_time) SELECT part, part_type, mrp_rule, site, is_phantom, 0.0, transshipment_cost, transshipment_lead_time FROM temp_ipc_material_node;");

            con.Query("INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) SELECT 'LOC_001', part, 'SITE_001', '2026-05-29'::DATE, qty, 'On-Hand' FROM temp_ipc_onhand;");

            con.Query("INSERT INTO ipc_scheduled_receipt (sr_id, to_part, qty, to_site, request_due_date, supply_status) SELECT 'SR_' || part, part, qty, 'SITE_001', '2026-05-29'::DATE, 'In-Transit' FROM temp_ipc_scheduled_receipt;");

            con.Query("INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) SELECT DISTINCT 'SITE_001', parent, 'BOM_' || parent, 1, 'MRP' FROM temp_boms;");

            con.Query("INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, ltb_limit, mix_group_id, relationship_type) SELECT 'BOM_' || parent, 'SITE_001', child, per_qty, scrap, case when alt_group_id = -1 then NULL else 'ALT_GRP_' || CAST(alt_group_id AS VARCHAR) end, alt_priority, target_ratio, historical_qty, eff_start_day, eff_end_day, ltb_limit, mix_group_id, relationship_type FROM temp_boms;");

            con.Query("INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, preference_mode, customer_tier, revenue) SELECT 'DEMAND_' || LPAD(CAST(demand_id AS VARCHAR), 5, '0'), 1.0, part, 'SITE_001', customer, '2026-05-29'::DATE + due_day, '2026-05-29'::DATE + due_day, qty, qty, status, priority, 'SITE_001', preference_mode, customer_tier, revenue FROM temp_demands;");

            con.Query("DROP TABLE temp_ipc_material_node;");

            con.Query("DROP TABLE temp_ipc_onhand;");

            con.Query("DROP TABLE temp_ipc_scheduled_receipt;");

            con.Query("DROP TABLE temp_boms;");

            con.Query("DROP TABLE temp_demands;");

            std::cout << "[DB] 逻辑模型主数据基因初始化写入成功。" << std::endl;

        } else {

            std::cout << "[DB] 数据库非空，正在验证并升级表结构以支持高阶替换场景控制列..." << std::endl;

            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS transshipment_cost DOUBLE DEFAULT 0.0;");

            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS transshipment_lead_time INTEGER DEFAULT 0;");
            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS run_rate DOUBLE DEFAULT 0.0;");
            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS lead_time DOUBLE;");
            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS round_to_integer BOOLEAN;");
            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS on_hand_type VARCHAR DEFAULT 'Standard';");
            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS time_fence_days INTEGER DEFAULT 0;");
            con.Query("ALTER TABLE ipc_material_node ADD COLUMN IF NOT EXISTS sourcing_policy VARCHAR DEFAULT 'Standard';");

            con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS eff_start_day INTEGER DEFAULT -1;");

            con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS eff_end_day INTEGER DEFAULT -1;");

            con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS ltb_limit DOUBLE DEFAULT -1.0;");

            con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS mix_group_id INTEGER DEFAULT -1;");

            con.Query("ALTER TABLE ipc_bom_item ADD COLUMN IF NOT EXISTS relationship_type VARCHAR DEFAULT 'alt';");

            con.Query("ALTER TABLE ipc_independent_demand ADD COLUMN IF NOT EXISTS preference_mode VARCHAR DEFAULT 'N';");

            con.Query("ALTER TABLE ipc_independent_demand ADD COLUMN IF NOT EXISTS customer_tier INTEGER DEFAULT 3;");

            con.Query("ALTER TABLE ipc_independent_demand ADD COLUMN IF NOT EXISTS revenue DOUBLE DEFAULT 0.0;");

        }

        // E. 萃取阶段：从 199 逻辑数据库中提取数据输入到 C++ DOD 进行运算

        std::cout << "\n[萃取] [ECL 提取点火] 正在从数据库的逻辑主数据表中加载输入参数..." << std::endl;

        auto t_extract_start = std::chrono::high_resolution_clock::now();

        std::vector<ScheduledReceiptRecord> srs;
    std::vector<std::vector<OperationRecord>> part_routings;
    std::unordered_map<std::string, std::vector<double>> wc_daily_capacity;
    ipc::DbAdapter adapter(con); adapter.load_calendars(ipc::global_calendars);
    adapter.load_parts(parts);
    adapter.load_scheduled_receipts(srs);
    ipc::load_operations_from_db(con, part_routings, wc_daily_capacity, parts.size());
    ipc::global_part_routings = part_routings;
    ipc::global_wc_daily_capacity = wc_daily_capacity;
    ipc::global_wc_allocated_capacity.clear();

        adapter.load_boms(boms);

        adapter.load_demands(demands);

        // -------------------------------------------------------------
        // Allotment & Hierarchy Initialization
        // -------------------------------------------------------------
        std::cout << "[配额确权] 正在初始化与加载 ITP/IOP 配额刚性防波堤约束..." << std::endl;

        // 1. Load hierarchies
        std::unordered_map<std::string, std::string> family_map;
        std::unordered_map<std::string, std::string> customer_group_map;
        std::unordered_map<std::string, std::string> region_map;

        // Product family
        {
            auto pf_res = con.Query("SELECT DISTINCT material, family_num FROM ipc_hierarchy_product_family WHERE material IS NOT NULL;");
            if (pf_res && !pf_res->HasError()) {
                for (size_t row = 0; row < pf_res->RowCount(); ++row) {
                    family_map[pf_res->GetValue(0, row).ToString()] = pf_res->GetValue(1, row).ToString();
                }
            }
        }

        // Customer group
        {
            auto cg_res = con.Query("SELECT DISTINCT customer, parent_customer FROM ipc_hierarchy_customer WHERE customer IS NOT NULL;");
            if (cg_res && !cg_res->HasError()) {
                for (size_t row = 0; row < cg_res->RowCount(); ++row) {
                    customer_group_map[cg_res->GetValue(0, row).ToString()] = cg_res->GetValue(1, row).ToString();
                }
            }
        }

        // Region
        {
            auto r_res = con.Query("SELECT DISTINCT customer, region FROM ipc_customer WHERE customer IS NOT NULL;");
            if (r_res && !r_res->HasError()) {
                for (size_t row = 0; row < r_res->RowCount(); ++row) {
                    region_map[r_res->GetValue(0, row).ToString()] = r_res->GetValue(1, row).ToString();
                }
            }
        }

        // 2. Propagate hierarchy attributes to demands
        for (auto& d : demands) {
            std::string full_part_code = ipc::vocab.get_code(d.part_id);
            size_t at_pos = full_part_code.find('@');
            std::string part_code = (at_pos != std::string::npos) ? full_part_code.substr(0, at_pos) : full_part_code;

            auto fam_it = family_map.find(part_code);
            d.family_id = (fam_it != family_map.end()) ? ipc::vocab.get_or_create(fam_it->second) : wildcard_id;

            auto cg_it = customer_group_map.find(d.customer);
            d.cust_group_id = (cg_it != customer_group_map.end()) ? ipc::vocab.get_or_create(cg_it->second) : wildcard_id;

            auto reg_it = region_map.find(d.customer);
            d.region_id = (reg_it != region_map.end()) ? ipc::vocab.get_or_create(reg_it->second) : wildcard_id;
        }

        // 3. Auto-aggregate assignments to ipc_allotment_constraint if empty for this scenario
        {
            auto check_allot = con.Query("SELECT COUNT(*) FROM ipc_allotment_constraint WHERE scenario_id = '" + scenario_id + "';");
            if (check_allot && !check_allot->HasError() && check_allot->GetValue(0, 0).GetValue<int64_t>() == 0) {
                bool inherited = false;
                std::string current_id = scenario_id;
                while (current_id != "baseline" && !current_id.empty()) {
                    auto parent_res = con.Query("SELECT parent_code FROM ipc_collab_scenario WHERE scenario_code = '" + current_id + "';");
                    if (parent_res && !parent_res->HasError() && parent_res->RowCount() > 0) {
                        std::string parent_id = parent_res->GetValue(0, 0).ToString();
                        if (parent_id == current_id || parent_id.empty()) {
                            break;
                        }
                        current_id = parent_id;
                        auto check_parent = con.Query("SELECT COUNT(*) FROM ipc_allotment_constraint WHERE scenario_id = '" + current_id + "';");
                        if (check_parent && !check_parent->HasError() && check_parent->GetValue(0, 0).GetValue<int64_t>() > 0) {
                            std::cout << "[配额继承] 从祖先场景 " << current_id << " 继承战略配额约束到当前场景 " << scenario_id << "..." << std::endl;
                            std::string copy_sql = "INSERT INTO ipc_allotment_constraint (scenario_id, part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked) "
                                                   "SELECT '" + scenario_id + "', part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked FROM ipc_allotment_constraint WHERE scenario_id = '" + current_id + "';";
                            con.Query(copy_sql);
                            inherited = true;
                            break;
                        }
                    } else {
                        break;
                    }
                }

                if (!inherited) {
                    std::cout << "[配额确权] 当前场景配额表为空，自动从现有规划供应分配汇总生成 ITP 战略配额..." << std::endl;
                    std::string agg_sql = R"(
                        INSERT INTO ipc_allotment_constraint (
                            scenario_id, part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked
                        )
                        SELECT 
                            ')" + scenario_id + R"(' AS scenario_id,
                            psa.part AS part_code,
                            psa.site AS site_code,
                            COALESCE(c.region, '*') AS region,
                            COALESCE(hc.parent_customer, '*') AS customer_group,
                            COALESCE(pf.family_num, '*') AS product_family,
                            date_diff('day', '2026-05-29'::DATE, psa.available_date) AS day,
                            SUM(psa.assigned_qty) AS itp_calculated_qty,
                            NULL AS override_qty,
                            FALSE AS is_locked
                        FROM (
                            SELECT part, site, available_date, assigned_qty, demand FROM ipc_planned_supply_assignment
                            UNION ALL
                            SELECT part, site, available_date, assigned_qty, demand FROM ipc_supply_assignment
                        ) psa
                        LEFT JOIN ipc_independent_demand d ON psa.demand = d.demand
                        LEFT JOIN ipc_customer c ON d.customer = c.customer
                        LEFT JOIN ipc_hierarchy_customer hc ON d.customer = hc.customer
                        LEFT JOIN ipc_hierarchy_product_family pf ON psa.part = pf.material
                        GROUP BY psa.part, psa.site, c.region, hc.parent_customer, pf.family_num, psa.available_date;
                    )";
                    con.Query(agg_sql);
                }
            }
        }

        // 4. Load allotment constraints from DB into memory
        {
            auto allot_res = con.Query("SELECT part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked FROM ipc_allotment_constraint WHERE scenario_id = '" + scenario_id + "';");
            if (allot_res && !allot_res->HasError()) {
                for (size_t row = 0; row < allot_res->RowCount(); ++row) {
                    std::string part_code = allot_res->GetValue(0, row).ToString();
                    std::string site_code = allot_res->GetValue(1, row).ToString();
                    std::string region = allot_res->GetValue(2, row).ToString();
                    std::string customer_group = allot_res->GetValue(3, row).ToString();
                    std::string product_family = allot_res->GetValue(4, row).ToString();
                    int day = get_int_value(allot_res->GetValue(5, row));
                    double itp_qty = ipc::get_double_value(allot_res->GetValue(6, row));
                    double override_qty = ipc::get_double_value(allot_res->GetValue(7, row));
                    bool is_locked = allot_res->GetValue(8, row).GetValue<bool>();

                    AllotmentConstraintKey key;
                    key.part_id = ipc::vocab.get_or_create(part_code.find('@') != std::string::npos ? part_code : part_code + "@" + site_code);
                    key.day = day;
                    key.family_id = ipc::vocab.get_or_create(product_family);
                    key.cust_group_id = ipc::vocab.get_or_create(customer_group);
                    key.region_id = ipc::vocab.get_or_create(region);

                    AllotmentState state;
                    state.limit = (is_locked && override_qty >= 0.0) ? override_qty : itp_qty;
                    state.consumed = 0.0;
                    state.blocked = 0.0;
                    state.is_locked = is_locked;
                    state.part_code = part_code;
                    state.site_code = site_code;
                    state.region_code = region;
                    state.cust_group_code = customer_group;
                    state.family_code = product_family;

                    allotment_constraints[key] = state;
                }
            }
            std::cout << "[配额确权] 成功从数据库载入 " << allotment_constraints.size() << " 条配额约束线。" << std::endl;
        }

        std::unordered_map<std::string, double> calculated_safety_stocks;
        std::unordered_map<std::string, double> calculated_stat_forecasts;
        std::vector<LscTreeNode> global_lsctree;
        int max_level = 0;
        std::vector<SwapRecord> swap_records;
        std::vector<SupplyAssignmentRecord> pegging_records;
        double ms_mrp = 0.0;

        if (solver_step == "dbd") {
            std::cout << "[DBD 模式] 跳过 LBL 计算，直接从数据库加载 Planned Orders 和 Alternates..." << std::endl;
            auto po_res = con.Query("SELECT part_code, order_qty, start_day, finish_day, dimension_val FROM ipc_planned_order_ledger;");
            if (po_res && !po_res->HasError()) {
                for (size_t row = 0; row < po_res->RowCount(); ++row) {
                    PlannedOrder po;
                    std::string part_code = po_res->GetValue(0, row).ToString();
                    po.part_id = ipc::vocab.get_or_create(part_code + "@SITE_001");
                    po.qty = ipc::get_double_value(po_res->GetValue(1, row));
                    po.start_day = get_int_value(po_res->GetValue(2, row));
                    po.finish_day = get_int_value(po_res->GetValue(3, row));
                    po.dimension_val = ipc::get_double_value(po_res->GetValue(4, row));
                    po.original_lbl_start = po.start_day;
                    po.original_lbl_finish = po.finish_day;
                    ipc_planned_orders.push_back(po);
                }
            } else {
                std::cout << "[Warning] 加载 ipc_planned_order_ledger 失败，表不存在或为空。" << std::endl;
            }
            auto alt_res = con.Query("SELECT main_part, alt_part, allocated_qty, day, alt_class FROM ipc_alternate_allocation;");
            if (alt_res && !alt_res->HasError()) {
                for (size_t row = 0; row < alt_res->RowCount(); ++row) {
                    AlternateAllocationRecord ar;
                    ar.main_part_id = ipc::vocab.get_or_create(alt_res->GetValue(0, row).ToString() + "@SITE_001");
                    ar.alt_part_id = ipc::vocab.get_or_create(alt_res->GetValue(1, row).ToString() + "@SITE_001");
                    ar.allocated_qty = ipc::get_double_value(alt_res->GetValue(2, row));
                    ar.day = get_int_value(alt_res->GetValue(3, row));
                    ar.alt_class = get_int_value(alt_res->GetValue(4, row));
                    alt_records.push_back(ar);
                }
            }
            max_level = compile_low_level_codes(parts, boms);
        } else {
            // -------------------------------------------------------------
            // IBP: In-memory Holt-Winters Safety Stock & Proportional Disaggregation
            // -------------------------------------------------------------
            std::cout << "[IBP] Running C++ in-memory Consensus Disaggregation and Holt-Winters..." << std::endl;

            struct ConsensusForecast {
                std::string part_code;
                std::string customer;
                double qty;
                double unit_price;
                double sales_qty;
                double marketing_qty;
                double statistical_qty;
            };
            std::vector<ConsensusForecast> consensus_list;

        // Load hierarchical resolvers
        HierarchyResolver product_resolver("PRODUCT");
        HierarchyResolver customer_resolver("CUSTOMER");
        HierarchyResolver region_resolver("REGION");
        std::vector<CustomerCombRecord> comb_records;

        std::unordered_set<uint32_t> active_skus;
        std::unordered_set<uint32_t> active_customers;
        for (const auto& p : parts) {
            if (!p.part_code.empty()) {
                active_skus.insert(p.part_id);
            }
        }
        for (const auto& d : demands) {
            if (!d.customer.empty()) {
                active_customers.insert(ipc::vocab.get_or_create(d.customer));
            }
        }

        // Build product hierarchy dynamically using active parts
        std::unordered_map<std::string, std::vector<std::string>> raw_family_to_mat;
        auto pf_res = con.Query("SELECT DISTINCT family_num, material FROM ipc_hierarchy_product_family WHERE family_num IS NOT NULL AND material IS NOT NULL;");
        if (pf_res && !pf_res->HasError()) {
            for (size_t row = 0; row < pf_res->RowCount(); ++row) {
                raw_family_to_mat[pf_res->GetValue(0, row).ToString()].push_back(pf_res->GetValue(1, row).ToString());
            }
        }
        for (const auto& pair : raw_family_to_mat) {
            std::string family = pair.first;
            for (const auto& mat : pair.second) {
                product_resolver.add_relation_by_code(family, mat);
                for (const auto& p : parts) {
                    if (p.part_code.empty()) continue;
                    if (get_raw_part_code(ipc::vocab.get_code(p.part_id)) == mat) {
                        product_resolver.add_relation_by_code(family, ipc::vocab.get_code(p.part_id));
                    }
                }
            }
        }

        // Build raw code to full code relations to support raw code query resolution
        for (const auto& p : parts) {
            if (p.part_code.empty()) continue;
            std::string full_code = ipc::vocab.get_code(p.part_id);
            std::string raw_code = get_raw_part_code(full_code);
            if (raw_code != full_code) {
                product_resolver.add_relation_by_code(raw_code, full_code);
            }
        }

        adapter.load_customer_hierarchy(customer_resolver);
        adapter.load_region_hierarchy(region_resolver);
        adapter.load_customer_comb_hierarchy(comb_records);

        std::unordered_map<std::string, std::string> customer_to_region;
        auto cust_reg_res = con.Query("SELECT customer, region FROM ipc_customer WHERE customer IS NOT NULL AND region IS NOT NULL;");
        if (cust_reg_res && !cust_reg_res->HasError()) {
            for (size_t r = 0; r < cust_reg_res->RowCount(); ++r) {
                customer_to_region[cust_reg_res->GetValue(0, r).ToString()] = cust_reg_res->GetValue(1, r).ToString();
            }
        }

        struct PairHash {
            size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
                return std::hash<uint32_t>()(p.first) ^ (std::hash<uint32_t>()(p.second) << 1);
            }
        };
        std::unordered_map<std::pair<uint32_t, uint32_t>, double, PairHash> disaggregated_forecasts;

        size_t working_size = std::max(parts.size(), ipc::vocab.size());

        auto forecast_res = con.Query("SELECT part, customer, qty, unit_price, sales_qty, marketing_qty, statistical_qty FROM ipc_consensus_forecast;");
        if (forecast_res && !forecast_res->HasError()) {
            for (size_t row = 0; row < forecast_res->RowCount(); ++row) {
                ConsensusForecast cf;
                cf.part_code = forecast_res->GetValue(0, row).ToString();
                cf.customer = forecast_res->GetValue(1, row).ToString();
                cf.qty = ipc::get_double_value(forecast_res->GetValue(2, row));
                cf.unit_price = ipc::get_double_value(forecast_res->GetValue(3, row));
                cf.sales_qty = ipc::get_double_value(forecast_res->GetValue(4, row));
                cf.marketing_qty = ipc::get_double_value(forecast_res->GetValue(5, row));
                cf.statistical_qty = ipc::get_double_value(forecast_res->GetValue(6, row));
                consensus_list.push_back(cf);

                auto resolved_skus = product_resolver.get_leaves_by_code(get_raw_part_code(cf.part_code), active_skus);
                if (resolved_skus.empty()) {
                    uint32_t p_id_full = ipc::vocab.get_or_create(cf.part_code);
                    if (active_skus.count(p_id_full) > 0) {
                        resolved_skus.insert(p_id_full);
                    } else {
                        uint32_t p_id_raw = ipc::vocab.get_or_create(get_raw_part_code(cf.part_code));
                        if (active_skus.count(p_id_raw) > 0) {
                            resolved_skus.insert(p_id_raw);
                        }
                    }
                }
                
                std::unordered_set<uint32_t> resolved_customers;
                if (cf.customer.empty() || cf.customer == "*") {
                    resolved_customers = active_customers;
                } else {
                    auto region_leaves = region_resolver.get_leaves_by_code(cf.customer, active_customers);
                    auto cust_leaves = customer_resolver.get_leaves_by_code(cf.customer, active_customers);
                    
                    resolved_customers = region_leaves;
                    resolved_customers.insert(cust_leaves.begin(), cust_leaves.end());
                    
                    if (resolved_customers.empty()) {
                        uint32_t cust_id = ipc::vocab.get_or_create(cf.customer);
                        if (active_customers.count(cust_id) > 0) {
                            resolved_customers.insert(cust_id);
                        }
                    }
                }

                struct DisaggCandidate {
                    uint32_t part_leaf_id;
                    uint32_t customer_leaf_id;
                    double weight = 0.0;
                };
                std::vector<DisaggCandidate> candidates;
                double total_weight = 0.0;

                for (uint32_t part_leaf : resolved_skus) {
                    std::string part_code_raw = get_raw_part_code(ipc::vocab.get_code(part_leaf));
                    for (uint32_t cust_leaf : resolved_customers) {
                        std::string cust_code = ipc::vocab.get_code(cust_leaf);
                        std::string region_code = customer_to_region[cust_code];

                        double weight = 0.0;
                        bool found_override = false;
                        for (const auto& rec : comb_records) {
                            if (rec.part == part_code_raw) {
                                if (rec.customer == cust_code) {
                                    weight = (rec.ratio_override >= 0.0) ? rec.ratio_override : rec.ratio;
                                    found_override = true;
                                    break;
                                } else if (!region_code.empty() && rec.region == region_code) {
                                    weight = (rec.ratio_override >= 0.0) ? rec.ratio_override : rec.ratio;
                                    found_override = true;
                                }
                            }
                        }

                        if (!found_override) {
                            double hist_qty = 0.0;
                            for (const auto& d : demands) {
                                if (d.part_id == part_leaf && d.customer == cust_code) {
                                    hist_qty += d.qty;
                                }
                            }
                            weight = hist_qty;
                        }

                        candidates.push_back({part_leaf, cust_leaf, weight});
                        total_weight += weight;
                    }
                }

                if (total_weight <= 0.0 && !candidates.empty()) {
                    for (auto& cand : candidates) {
                        cand.weight = 1.0;
                    }
                    total_weight = static_cast<double>(candidates.size());
                }

                if (total_weight > 0.0) {
                    for (const auto& cand : candidates) {
                        double share = cand.weight / total_weight;
                        double disagg_qty = cf.qty * share;
                        auto key = std::make_pair(cand.part_leaf_id, cand.customer_leaf_id);
                        disaggregated_forecasts[key] += disagg_qty;
                    }
                }
            }
        }

        // Run Holt-Winters on resolved leaf SKUs
        std::vector<std::map<int, double>> parts_daily_sums(working_size);
        for (const auto& d : demands) {
            if (d.part_id < parts_daily_sums.size()) {
                parts_daily_sums[d.part_id][d.due_day] += d.qty;
            }
        }

        for (const auto& cf : consensus_list) {
            auto resolved_skus = product_resolver.get_leaves_by_code(get_raw_part_code(cf.part_code), active_skus);
            if (resolved_skus.empty()) {
                uint32_t p_id_full = ipc::vocab.get_or_create(cf.part_code);
                if (active_skus.count(p_id_full) > 0) {
                    resolved_skus.insert(p_id_full);
                } else {
                    uint32_t p_id_raw = ipc::vocab.get_or_create(get_raw_part_code(cf.part_code));
                    if (active_skus.count(p_id_raw) > 0) {
                        resolved_skus.insert(p_id_raw);
                    }
                }
            }
            for (uint32_t p_id : resolved_skus) {
                if (p_id >= working_size) continue;
                const auto& daily_sums = parts_daily_sums[p_id];
                std::vector<double> series;
                series.reserve(daily_sums.size());
                for (const auto& pair : daily_sums) {
                    series.push_back(pair.second);
                }

                auto hw_res = run_holt_winters(series, 7, 30);
                double point_fc = hw_res.first.empty() ? 0.0 : hw_res.first[0];
                double safety_stock = 1.645 * hw_res.second;

                calculated_safety_stocks[ipc::vocab.get_code(p_id)] = safety_stock;
                calculated_stat_forecasts[ipc::vocab.get_code(p_id)] = point_fc;
            }
        }

        // Apply proportional disaggregation to demands in memory
        struct PartCustKey {
            uint32_t part_id;
            uint32_t customer_id;
            bool operator==(const PartCustKey& o) const {
                return part_id == o.part_id && customer_id == o.customer_id;
            }
        };
        struct PartCustKeyHash {
            size_t operator()(const PartCustKey& k) const {
                return std::hash<uint32_t>()(k.part_id) ^ (std::hash<uint32_t>()(k.customer_id) << 1);
            }
        };
        
        std::unordered_map<PartCustKey, double, PartCustKeyHash> part_cust_total_current;
        std::unordered_map<PartCustKey, int, PartCustKeyHash> part_cust_match_count;
        std::unordered_map<PartCustKey, std::vector<size_t>, PartCustKeyHash> demands_by_part_cust;

        for (size_t i = 0; i < demands.size(); ++i) {
            uint32_t cust_id = ipc::vocab.get_or_create(demands[i].customer);
            PartCustKey key = {demands[i].part_id, cust_id};
            part_cust_total_current[key] += demands[i].qty;
            part_cust_match_count[key]++;
            demands_by_part_cust[key].push_back(i);
        }

        for (const auto& pair : disaggregated_forecasts) {
            PartCustKey key = {pair.first.first, pair.first.second};
            double target_qty = pair.second;
            
            double current_total = part_cust_total_current[key];
            int count = part_cust_match_count[key];
            if (count > 0) {
                for (size_t d_idx : demands_by_part_cust[key]) {
                    auto& d = demands[d_idx];
                    if (current_total > 0.0) {
                        d.qty = target_qty * (d.qty / current_total);
                    } else {
                        d.qty = target_qty / count;
                    }
                }
            }
        }
        std::cout << "[IBP] In-memory consensus disaggregation and Holt-Winters completed." << std::endl;

        if (parts.size() < ipc::vocab.size()) {
            size_t old_size = parts.size();
            parts.resize(ipc::vocab.size());
            for (size_t i = old_size; i < parts.size(); ++i) {
                std::string code = ipc::vocab.get_code(i);
                parts[i].part_code = code;
                parts[i].part_id = i;
                if (code.rfind("PART_", 0) == 0) {
                    try {
                        int num = std::stoi(code.substr(5));
                        if (num < 7000) parts[i].part_type = "FINISHED";
                        else if (num < 9000) parts[i].part_type = "SEMI";
                        else if (num < 9500) parts[i].part_type = "RAW";
                        else parts[i].part_type = "ALT";
                    } catch (...) {
                        parts[i].part_type = "RAW";
                    }
                } else {
                    parts[i].part_type = "RAW";
                }
                parts[i].low_level_code = 0;
                parts[i].round_to_integer = true;
                parts[i].lead_time = 3.0;
                parts[i].is_phantom = false;
                parts[i].on_hand = 0.0;
                parts[i].ipc_scheduled_receipt = 0.0;
            }
        }
        ipc::parent_to_bom_indices.assign(parts.size(), std::vector<size_t>());
        ipc::child_to_bom_indices.assign(parts.size(), std::vector<size_t>());

        ipc::alt_group_to_bom_indices.clear();

        for (size_t i = 0; i < boms.size(); ++i) {

            ipc::parent_to_bom_indices[boms[i].parent_id].push_back(i);

            ipc::child_to_bom_indices[boms[i].child_id].push_back(i);

            if (boms[i].alt_group_id != -1) {

                ipc::alt_group_to_bom_indices[boms[i].alt_group_id].push_back(i);

            }

        }

        auto t_extract_end = std::chrono::high_resolution_clock::now();

        double ms_extract = std::chrono::duration<double, std::milli>(t_extract_end - t_extract_start).count();

        std::cout << "[计时] [Benchmark] 逻辑数据库主表输入数据提取耗时: " << std::fixed << std::setprecision(2) << ms_extract << " ms" << std::endl;

        std::cout << "[OK] [ECL 提取成功] 物料数 = " << parts.size() << "，BOM嵌套数 = " << boms.size() << "，订单需求数 = " << demands.size() << std::endl;

        // E. 并行编译全部 LSC Trees

        auto t_lsc_start = std::chrono::high_resolution_clock::now();

        std::vector<LscTreeNode> global_lsctree = compile_all_lsc_trees(parts, boms);

        auto t_lsc_end = std::chrono::high_resolution_clock::now();

        double ms_lsc = std::chrono::duration<double, std::milli>(t_lsc_end - t_lsc_start).count();

        std::cout << "[OK] [LSCTREE 编译器] 维度条件多核展开完成！常驻节点总数 = " << global_lsctree.size() << std::endl;

        std::cout << "[计时] [Benchmark] 全局多维 LSCTREE 并行编译耗时: " << ms_lsc << " ms" << std::endl;

        // F. 编译全局低层码（LLC）

        auto t_llc_start = std::chrono::high_resolution_clock::now();

        int max_level = compile_low_level_codes(parts, boms);

        auto t_llc_end = std::chrono::high_resolution_clock::now();

        double ms_llc = std::chrono::duration<double, std::milli>(t_llc_end - t_llc_start).count();

        std::cout << "[OK] [LLC 拓扑编译器] LLC编译成功！最大低层码深度 = " << max_level << std::endl;

        std::cout << "[计时] [Benchmark] 全局物料 LLC 拓扑编译耗时: " << ms_llc << " ms" << std::endl;

        // =====================================================================
        // MEIO: Multi-Echelon Inventory Optimization
        // =====================================================================
        std::cout << "[MEIO] Running Multi-Echelon Inventory Optimization..." << std::endl;
        auto t_meio_start = std::chrono::high_resolution_clock::now();

        // 1. Determine horizon length from demands
        int max_demand_day = 30; // default minimum
        for (const auto& d : demands) {
            if (d.due_day > max_demand_day) {
                max_demand_day = d.due_day;
            }
        }
        int num_days = max_demand_day + 1;

        // 2. Load service level targets and lead time variances from db
        struct MEIOTarget {
            double service_level = 0.95;
            double lead_time_variance = 1.0;
        };
        std::vector<MEIOTarget> meio_targets(parts.size(), {0.95, 1.0});
        auto target_res = con.Query("SELECT part_code, service_level_target, lead_time_variance FROM ipc_service_level_target;");
        if (target_res && !target_res->HasError()) {
            for (size_t row = 0; row < target_res->RowCount(); ++row) {
                std::string part_code = target_res->GetValue(0, row).ToString();
                double sl = ipc::get_double_value(target_res->GetValue(1, row));
                double ltv = ipc::get_double_value(target_res->GetValue(2, row));

                uint32_t part_id = ipc::vocab.get_or_create(part_code);
                if (part_id >= meio_targets.size()) {
                    meio_targets.resize(part_id + 1, {0.95, 1.0});
                }
                meio_targets[part_id].service_level = sl;
                meio_targets[part_id].lead_time_variance = ltv;
            }
        }

        // 3. Initialize mean demand and variance arrays
        std::vector<double> mean_demand(parts.size(), 0.0);
        std::vector<double> demand_variance(parts.size(), 0.0);

        // Optimize daily demand extraction to O(N + M) using a flat vector
        std::vector<double> parts_daily_demand(parts.size() * num_days, 0.0);
        std::vector<bool> part_has_demand(parts.size(), false);
        for (const auto& d : demands) {
            if (d.part_id < parts.size() && d.due_day >= 0 && d.due_day < num_days) {
                parts_daily_demand[d.part_id * num_days + d.due_day] += d.qty;
                part_has_demand[d.part_id] = true;
            }
        }

        for (size_t part_id = 0; part_id < parts.size(); ++part_id) {
            if (parts[part_id].part_code.empty()) continue;
            if (!part_has_demand[part_id]) continue;

            size_t offset = part_id * num_days;
            double sum = 0.0;
            for (int day = 0; day < num_days; ++day) {
                sum += parts_daily_demand[offset + day];
            }
            mean_demand[part_id] = sum / num_days;

            double var_sum = 0.0;
            for (int day = 0; day < num_days; ++day) {
                double diff = parts_daily_demand[offset + day] - mean_demand[part_id];
                var_sum += diff * diff;
            }
            demand_variance[part_id] = num_days > 1 ? var_sum / (num_days - 1) : 0.0;
        }

        // 4. Forward demand & variance propagation down the BOM tree DAG in LLC order
        for (int level = 0; level <= max_level; ++level) {
            for (size_t p_id = 0; p_id < parts.size(); ++p_id) {
                if (parts[p_id].part_code.empty()) continue;
                if (static_cast<int>(parts[p_id].low_level_code) != level) continue;

                double D_P = mean_demand[p_id];
                double Var_P = demand_variance[p_id];

                if (p_id < ipc::parent_to_bom_indices.size()) {
                    for (size_t bom_idx : ipc::parent_to_bom_indices[p_id]) {
                        const auto& bom = boms[bom_idx];
                        uint32_t c_id = bom.child_id;

                        double factor = bom.per_qty * (1.0 + bom.scrap) * bom.target_ratio;
                        mean_demand[c_id] += D_P * factor;
                        demand_variance[c_id] += Var_P * factor * factor;
                    }
                }
            }
        }

        // 5. Calculate multi-echelon safety stock for each part and store in calculated_safety_stocks
        for (size_t part_id = 0; part_id < parts.size(); ++part_id) {
            if (parts[part_id].part_code.empty()) continue;

            double D_i = mean_demand[part_id];
            double sigma_D2 = demand_variance[part_id];

            double L_i = parts[part_id].lead_time;
            if (L_i < 0.0 || parts[part_id].is_phantom) {
                L_i = 0.0;
            }

            double sigma_L2 = meio_targets[part_id].lead_time_variance;
            double SL_i = meio_targets[part_id].service_level;

            double Z_i = normal_inverse_cdf(SL_i);

            double val_inside = L_i * sigma_D2 + D_i * D_i * sigma_L2;
            double SS_i = Z_i * std::sqrt(std::max(0.0, val_inside));

            calculated_safety_stocks[ipc::vocab.get_code(part_id)] = SS_i;
        }

        auto t_meio_end = std::chrono::high_resolution_clock::now();
        double ms_meio = std::chrono::duration<double, std::milli>(t_meio_end - t_meio_start).count();
        std::cout << "[MEIO] Multi-Echelon Safety Stock propagation computed successfully!" << std::endl;
        std::cout << "[计时] [Benchmark] MEIO 安全库存算法耗时: " << ms_meio << " ms" << std::endl;

        // G. 运行极致 C++ DOD LBL-MRP 动态消纳与替换料分配

        // Reused outer swap_records

        auto t_mrp_start = std::chrono::high_resolution_clock::now();

        ipc::run_lbl_mrp_engine(parts, boms, demands, ipc_planned_orders, alt_records, swap_records, max_level, srs);

        auto t_mrp_end = std::chrono::high_resolution_clock::now();

        ms_mrp = std::chrono::duration<double, std::milli>(t_mrp_end - t_mrp_start).count();

        std::cout << "[OK] [LBL 计算点火] 全局 MRP 排程消纳成功！" << std::endl;

        std::cout << "   -> 生成计划订单 (Planned Orders) 笔数 = " << ipc_planned_orders.size() << std::endl;

        std::cout << "   -> 运行专利替换消纳 (Alternate Allocations) 笔数 = " << alt_records.size() << std::endl;

        // G2. 执行供需分配 Pegging 匹配分配

        // Reused outer pegging_records

        auto t_peg_start = std::chrono::high_resolution_clock::now();

        generate_pegging_records(parts, demands, ipc_planned_orders, alt_records, pegging_records, srs);

        auto t_peg_end = std::chrono::high_resolution_clock::now();

        double ms_peg = std::chrono::duration<double, std::milli>(t_peg_end - t_peg_start).count();

        std::cout << "   -> 运行供需 Pegging 匹配分配笔数 = " << pegging_records.size() << "（用时: " << ms_peg << " ms）" << std::endl;
        }

        // G3. 运行双专利融合 MCDS DBD 微观派程调度引擎

        std::vector<PlannedOrder> scheduled_orders;

        std::vector<double> allocated_capacity;

        std::vector<double> order_capacities;

        auto t_dbd_start = std::chrono::high_resolution_clock::now();

        std::vector<double> order_routing_costs;
        if ((solver_mode == "iop" || solver_mode == "itp") && solver_step != "lbl") {
            std::cout << "[模式] " << (solver_mode == "iop" ? "IOP 滚动排产" : "ITP 战术平衡") << "启动，执行微观时空派程..." << std::endl;
            ipc::run_dbd_dispatch_engine(parts, boms, ipc_planned_orders, demands, scheduled_orders, allocated_capacity, order_capacities, order_routing_costs, solver_mode, wildcard_id, allotment_constraints);
            auto t_dbd_end = std::chrono::high_resolution_clock::now();
            double ms_dbd = std::chrono::duration<double, std::milli>(t_dbd_end - t_dbd_start).count();
            std::cout << "   -> 运行 DBD 派程完成（用时: " << ms_dbd << " ms）" << std::endl;
        } else {
            std::cout << "[模式] 跳过微观时空派程 (solver_step == 'lbl' 或 模式非 iop/itp)..." << std::endl;
            scheduled_orders = ipc_planned_orders;
            order_capacities.assign(ipc_planned_orders.size(), 0.0);
            order_routing_costs.assign(ipc_planned_orders.size(), 0.0);
        }

        std::cout << "[计时] [Benchmark] 5万多维订单 LBL MRP 核心消纳计算耗时: " << ms_mrp << " ms" << std::endl;

        std::cout << "=====================================================================" << std::endl;

        // H. 流式 Appender 极速将数据写入 DuckDB 便于查账

        std::cout << "\n[同步] [对账数据灌入] 正在将计算结果同步至 DuckDB 中..." << std::endl;

        // -------------------------------------------------------------
        // IBP: Save disaggregated demands and safety stocks back to DuckDB
        // -------------------------------------------------------------
        std::cout << "[IBP] Syncing disaggregated demands and safety stocks back to DuckDB..." << std::endl;

        // 1. Bulk update ipc_independent_demand
        con.Query("CREATE TEMP TABLE temp_demand_qtys (demand VARCHAR, new_qty DOUBLE);");
        {
            duckdb::Appender appender(con, "temp_demand_qtys");
            for (const auto& d : demands) {
                appender.BeginRow();
                appender.Append<const char*>(d.demand_code.c_str());
                appender.Append<double>(d.qty);
                appender.EndRow();
            }
        }
        con.Query("UPDATE ipc_independent_demand SET request_qty = t.new_qty, open_qty = t.new_qty FROM temp_demand_qtys t WHERE ipc_independent_demand.demand = t.demand;");
        con.Query("DROP TABLE temp_demand_qtys;");

        // 2. Bulk update safety stocks in ipc_material_node
        con.Query("CREATE TEMP TABLE temp_safety_stocks (part VARCHAR, site VARCHAR, ss DOUBLE);");
        {
            duckdb::Appender appender(con, "temp_safety_stocks");
            for (const auto& pair : calculated_safety_stocks) {
                appender.BeginRow();
                appender.Append<const char*>(get_raw_part_code(pair.first).c_str());
                appender.Append<const char*>(get_site_code(pair.first).c_str());
                appender.Append<double>(pair.second);
                appender.EndRow();
            }
        }
        con.Query("UPDATE ipc_material_node SET safety_stock = t.ss, ss_fixed_qty = t.ss FROM temp_safety_stocks t WHERE ipc_material_node.part = t.part AND ipc_material_node.site = t.site;");
        con.Query("DROP TABLE temp_safety_stocks;");

        // 3. Bulk update statistical forecast quantities in ipc_consensus_forecast
        con.Query("CREATE TEMP TABLE temp_stat_forecasts (part VARCHAR, stat_qty DOUBLE);");
        {
            duckdb::Appender appender(con, "temp_stat_forecasts");
            for (const auto& pair : calculated_stat_forecasts) {
                appender.BeginRow();
                appender.Append<const char*>(get_raw_part_code(pair.first).c_str());
                appender.Append<double>(pair.second);
                appender.EndRow();
            }
        }
        con.Query("UPDATE ipc_consensus_forecast SET statistical_qty = t.stat_qty FROM temp_stat_forecasts t WHERE ipc_consensus_forecast.part = t.part;");
        con.Query("DROP TABLE temp_stat_forecasts;");

        // 4. Update ledger total consensus revenue
        auto ledger_res = con.Query("SELECT sum(cast(consensus_forecast as double)) FROM ipc_consensus_forecast;");
        if (ledger_res && !ledger_res->HasError() && ledger_res->RowCount() > 0) {
            double total_rev = ipc::get_double_value(ledger_res->GetValue(0, 0));
            con.Query("UPDATE ipc_financial_ledger SET total_revenue = " + std::to_string(total_rev) + " WHERE scenario_code = 'baseline';");
        }
        std::cout << "[IBP] IBP database tables updated successfully." << std::endl;

        auto t_sync_start = std::chrono::high_resolution_clock::now();

        if (solver_step != "dbd") {
            con.Query("DROP TABLE IF EXISTS ipc_part_status;");

        con.Query("DROP TABLE IF EXISTS ipc_planned_order_ledger;");

        con.Query("DROP TABLE IF EXISTS ipc_alternate_allocation;");
        con.Query("DROP TABLE IF EXISTS ipc_allotment_ledger;");

        con.Query("DROP TABLE IF EXISTS ipc_bom_explosion_network;");

        con.Query("CREATE TABLE ipc_part_status (part_code VARCHAR(40), part_type VARCHAR(10), on_hand DOUBLE, llc INTEGER, round_to_integer BOOLEAN);");

        con.Query("CREATE TABLE ipc_planned_order_ledger (part_code VARCHAR(40), order_qty DOUBLE, start_day INTEGER, finish_day INTEGER, dimension_val DOUBLE);");

        con.Query("CREATE TABLE ipc_alternate_allocation (main_part VARCHAR(40), alt_part VARCHAR(40), allocated_qty DOUBLE, day INTEGER, alt_class INTEGER);");

        con.Query("CREATE TABLE ipc_bom_explosion_network (root_part VARCHAR(40), root_dim DOUBLE, node_part VARCHAR(40), parent_part VARCHAR(40), per_qty DOUBLE, root_per_qty DOUBLE, cum_lt INTEGER, node_level INTEGER, is_leaf BOOLEAN);");
        con.Query("CREATE TABLE ipc_allotment_ledger (scenario_id VARCHAR, part_code VARCHAR, site_code VARCHAR, region VARCHAR DEFAULT '*', customer_group VARCHAR DEFAULT '*', product_family VARCHAR, day INTEGER, allotment_limit DOUBLE, consumed_qty DOUBLE, available_qty DOUBLE, blocked_demand_qty DOUBLE, PRIMARY KEY(scenario_id, part_code, site_code, region, customer_group, product_family, day));");
        }

        // H3. 清空并同步回写至 199 逻辑模型表中的输出表（ipc_planned_order，ipc_dimension_attribute，ipc_dimension_grouping，ipc_supply_assignment，ipc_planned_supply_assignment）

        con.Query("DELETE FROM ipc_planned_order;");

        con.Query("DELETE FROM ipc_dimension_attribute;");

        con.Query("DELETE FROM ipc_dimension_grouping;");

        con.Query("DELETE FROM ipc_supply_assignment;");

        con.Query("DELETE FROM ipc_planned_supply_assignment;");

        // 写入维度主数据

        con.Query(R"(

            INSERT INTO ipc_dimension_attribute (dimension, description, value, value_description) VALUES

            ('RAM_SIZE', '内存容量', 100, '128MB'),

            ('RAM_SIZE', '内存容量', 101, '256MB'),

            ('RAM_SIZE', '内存容量', 102, '512MB');

        )");

        con.Query(R"(

            INSERT INTO ipc_dimension_grouping (ipc_dimension_grouping, dimension, value, relation_ship) VALUES

            ('DIM_100.0', 'RAM_SIZE', 100, 'EQ'),

            ('DIM_101.0', 'RAM_SIZE', 101, 'EQ'),

            ('DIM_102.0', 'RAM_SIZE', 102, 'EQ');

        )");

        // 写入临时计划订单表进行高效率 bulk SQL 灌库

        con.Query("CREATE TEMP TABLE temp_ipc_planned_order (part VARCHAR, qty DOUBLE, start_day INTEGER, finish_day INTEGER, dimension_val DOUBLE);");

        {

            duckdb::Appender app_po(con, "temp_ipc_planned_order");

            for (const auto& o : ipc_planned_orders) {

                app_po.BeginRow();

                app_po.Append(get_raw_part_code(ipc::vocab.get_code(o.part_id)).c_str());

                app_po.Append<double>(o.qty);

                app_po.Append<int32_t>(o.start_day);

                app_po.Append<int32_t>(o.finish_day);

                app_po.Append<double>(o.dimension_val);

                app_po.EndRow();

            }

        }

        // 写入 199 物理模型中的 ipc_planned_order 表

        con.Query(R"(

            INSERT INTO ipc_planned_order (ipc_planned_order, request_start_date, due_date, qty, eff_qty, dimension_grp, is_planned, part, source, site)

            SELECT 

                'PO_' || LPAD(CAST(row_number() OVER () AS VARCHAR), 6, '0'),

                '2026-05-29'::DATE + start_day,

                '2026-05-29'::DATE + finish_day,

                qty,

                qty,

                'DIM_' || CAST(dimension_val AS VARCHAR),

                'Y',

                part,

                'MRP',

                'SITE_001'

            FROM temp_ipc_planned_order;

        )");

        con.Query("DROP TABLE temp_ipc_planned_order;");

        // 写入临时 Pegging 表进行供需分配对账数据回写

        con.Query("CREATE TEMP TABLE temp_ipc_supply_assignment (demand VARCHAR, ind_part VARCHAR, part VARCHAR, assigned_qty DOUBLE, supply VARCHAR, supply_type VARCHAR, due_day INTEGER, dimension_val DOUBLE);");

        {

            duckdb::Appender app_peg(con, "temp_ipc_supply_assignment");

            for (const auto& r : pegging_records) {

                app_peg.BeginRow();

                app_peg.Append(r.demand_code.c_str());

                app_peg.Append(r.ind_part.c_str());

                app_peg.Append(r.part.c_str());

                app_peg.Append<double>(r.assigned_qty);

                app_peg.Append(r.supply_code.c_str());

                app_peg.Append(r.supply_type.c_str());

                app_peg.Append<int32_t>(r.due_day);

                app_peg.Append<double>(r.dimension_val);

                app_peg.EndRow();

            }

        }

        // 批量回写至逻辑模型 ipc_supply_assignment 和 ipc_planned_supply_assignment 关联对账表

        con.Query(R"(

            INSERT INTO ipc_supply_assignment (demand, item, ind_part, location, part, site, due_date, supply, supply_type, assigned_qty, dimension_grp, available_date)

            SELECT 

                demand,

                1.0,

                ind_part,

                'LOC_001',

                part,

                'SITE_001',

                '2026-05-29'::DATE + due_day,

                supply,

                supply_type,

                assigned_qty,

                'DIM_' || CAST(dimension_val AS VARCHAR),

                '2026-05-29'::DATE + due_day

            FROM temp_ipc_supply_assignment

            WHERE supply_type != 'Planned-Order';

        )");

        con.Query(R"(

            INSERT INTO ipc_planned_supply_assignment (demand, item, location, part, site, due_date, supply, supply_type, assigned_qty, dimension_grp, available_date, ipc_planned_order)

            SELECT 

                demand,

                1.0,

                'LOC_001',

                part,

                'SITE_001',

                '2026-05-29'::DATE + due_day,

                supply,

                supply_type,

                assigned_qty,

                'DIM_' || CAST(dimension_val AS VARCHAR),

                '2026-05-29'::DATE + due_day,

                supply

            FROM temp_ipc_supply_assignment

            WHERE supply_type = 'Planned-Order';

        )");

        con.Query("DROP TABLE temp_ipc_supply_assignment;");

        // 5. 同步不完全替代 (Swap) 结果

        con.Query("DELETE FROM ipc_swap_result;");

        con.Query("CREATE TEMP TABLE temp_ipc_swap_result (demand_code VARCHAR, from_part VARCHAR, to_part VARCHAR, swapped_qty DOUBLE, day INTEGER, alt_group VARCHAR, swap_reason VARCHAR);");

        {

            duckdb::Appender app_swap(con, "temp_ipc_swap_result");

            for (const auto& s : swap_records) {

                app_swap.BeginRow();

                app_swap.Append(s.demand_code.c_str());

                app_swap.Append(s.from_part.c_str());

                app_swap.Append(s.to_part.c_str());

                app_swap.Append<double>(s.swapped_qty);

                app_swap.Append<int32_t>(s.day);

                app_swap.Append(s.alt_group.c_str());

                app_swap.Append(s.swap_reason.c_str());

                app_swap.EndRow();

            }

        }

        con.Query("INSERT INTO ipc_swap_result SELECT * FROM temp_ipc_swap_result;");

        con.Query("DROP TABLE temp_ipc_swap_result;");

        // 1. 同步物料结果

        if (solver_step != "dbd") {
        {

            duckdb::Appender app(con, "ipc_part_status");

            for (const auto& p : parts) {

                app.BeginRow();

                app.Append(p.part_code.c_str());

                app.Append(p.part_type.c_str());

                app.Append<double>(p.on_hand);

                app.Append<int32_t>(p.low_level_code);

                app.Append<bool>(p.round_to_integer);

                app.EndRow();

            }

            if (DEBUG_CSV_EXPORT) ipc::csv_export_parts(parts);

        }

        // 2. 同步计划订单结果

        {

            duckdb::Appender app(con, "ipc_planned_order_ledger");

            for (const auto& o : ipc_planned_orders) {

                app.BeginRow();

                app.Append(get_raw_part_code(ipc::vocab.get_code(o.part_id)).c_str());

                app.Append<double>(o.qty);

                app.Append<int32_t>(o.start_day);

                app.Append<int32_t>(o.finish_day);

                app.Append<double>(o.dimension_val);

                app.EndRow();

            }

            if (DEBUG_CSV_EXPORT) ipc::csv_export_ipc_planned_orders(ipc_planned_orders);

        }

        // 3. 同步替换料分配结果

        {

            duckdb::Appender app(con, "ipc_alternate_allocation");

            for (const auto& a : alt_records) {

                app.BeginRow();

                app.Append(get_raw_part_code(ipc::vocab.get_code(a.main_part_id)).c_str());

                app.Append(get_raw_part_code(ipc::vocab.get_code(a.alt_part_id)).c_str());

                app.Append<double>(a.allocated_qty);

                app.Append<int32_t>(a.day);

                app.Append<int32_t>(a.alt_class);

                app.EndRow();

            }

            if (DEBUG_CSV_EXPORT) ipc::csv_export_alternates(alt_records);

        }

        // 4. 同步展开的 LSCTREE 结果

        {

            duckdb::Appender app(con, "ipc_bom_explosion_network");

            for (const auto& n : global_lsctree) {

                app.BeginRow();

                app.Append(get_raw_part_code(ipc::vocab.get_code(n.root_part_id)).c_str());

                app.Append<double>(n.root_dimension_val);

                app.Append(get_raw_part_code(ipc::vocab.get_code(n.node_part_id)).c_str());

                app.Append(get_raw_part_code(ipc::vocab.get_code(n.parent_part_id)).c_str());

                app.Append<double>(n.per_qty);

                app.Append<double>(n.root_per_qty);

                app.Append<int32_t>(n.cumulative_lt);

                app.Append<int32_t>(n.node_level);

                app.Append<bool>(n.is_leaf);

                app.EndRow();

            }

        }
        }

        // 5. 同步 DBD 微观派程结果

        con.Query("DROP TABLE IF EXISTS ipc_dispatch_ledger;");

        con.Query("CREATE TABLE ipc_dispatch_ledger (part_code VARCHAR(40), order_qty DOUBLE, original_start_day INTEGER, original_due_day INTEGER, scheduled_day INTEGER, dimension_val DOUBLE, allocated_capacity DOUBLE, routing_cost DOUBLE);");

        {

            duckdb::Appender app_dbd(con, "ipc_dispatch_ledger");

            for (size_t i = 0; i < scheduled_orders.size(); ++i) {
                const auto& sched = scheduled_orders[i];
                int32_t orig_start = (i < ipc_planned_orders.size()) ? ipc_planned_orders[i].start_day : sched.original_lbl_start;
                int32_t orig_finish = (i < ipc_planned_orders.size()) ? ipc_planned_orders[i].finish_day : sched.original_lbl_finish;
                double capacity_used = (i < order_capacities.size()) ? order_capacities[i] : 0.0;
                double r_cost = (i < order_routing_costs.size()) ? order_routing_costs[i] : 0.0;
                app_dbd.BeginRow();
                app_dbd.Append(get_raw_part_code(ipc::vocab.get_code(sched.part_id)).c_str());
                app_dbd.Append<double>(sched.qty);
                app_dbd.Append<int32_t>(orig_start);
                app_dbd.Append<int32_t>(orig_finish);
                app_dbd.Append<int32_t>(sched.finish_day);
                app_dbd.Append<double>(sched.dimension_val);
                app_dbd.Append<double>(capacity_used);
                app_dbd.Append<double>(r_cost);
                app_dbd.EndRow();
            }

        }

        auto t_sync_end = std::chrono::high_resolution_clock::now();

        double ms_sync = std::chrono::duration<double, std::milli>(t_sync_end - t_sync_start).count();

        std::cout << "[计时] [Benchmark] 流式数据同步导入 DuckDB 耗时: " << ms_sync << " ms" << std::endl;

        std::cout << "[OK] [对账数据灌入] 成功！全部数据表已就绪。" << std::endl;

        // 运行联副产品维度规划并持久化

        ipc::run_ipc_coproduct_dimension_planning_and_persist(con);

        // 运行全场景业务对账与校验

        if (solver_mode == "itp") {
            std::cout << "[ȷȨ] itp ģʽԶۼƽ־һ..." << std::endl;
            std::string agg_sql = R"(
                INSERT INTO ipc_allotment_constraint (
                    scenario_id, part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked
                )
                SELECT 
                    scenario_id, part_code, site_code, region, customer_group, product_family, day, SUM(qty) AS itp_calculated_qty, NULL AS override_qty, FALSE AS is_locked
                FROM (
                    SELECT 
                        ')" + scenario_id + R"(' AS scenario_id,
                        split_part(psa.part, '@', 1) AS part_code,
                        psa.site AS site_code,
                        COALESCE(c.region, '*') AS region,
                        COALESCE(hc.parent_customer, '*') AS customer_group,
                        COALESCE(pf.family_num, '*') AS product_family,
                        date_diff('day', '2026-05-29'::DATE, psa.available_date) AS day,
                        psa.assigned_qty AS qty
                    FROM (
                        SELECT part, site, available_date, assigned_qty, demand FROM ipc_planned_supply_assignment
                        UNION ALL
                        SELECT part, site, available_date, assigned_qty, demand FROM ipc_supply_assignment
                    ) psa
                    LEFT JOIN ipc_independent_demand d ON psa.demand = d.demand
                    LEFT JOIN ipc_customer c ON d.customer = c.customer
                    LEFT JOIN ipc_hierarchy_customer hc ON d.customer = hc.customer
                    LEFT JOIN ipc_hierarchy_product_family pf ON split_part(d.part, '@', 1) = pf.material

                    UNION ALL

                    SELECT 
                        ')" + scenario_id + R"(' AS scenario_id,
                        alt.alt_part AS part_code,
                        'SITE_001' AS site_code,
                        '*' AS region,
                        '*' AS customer_group,
                        COALESCE(pf.family_num, '*') AS product_family,
                        alt.day AS day,
                        alt.allocated_qty AS qty
                    FROM ipc_alternate_allocation alt
                    LEFT JOIN ipc_hierarchy_product_family pf ON alt.main_part = pf.material
                ) t
                GROUP BY scenario_id, part_code, site_code, region, customer_group, product_family, day
                ON CONFLICT (scenario_id, part_code, site_code, region, customer_group, product_family, day) DO UPDATE SET
                    itp_calculated_qty = excluded.itp_calculated_qty;
            )";
            con.Query(agg_sql);
        }

        ipc::run_post_sync_scenario_verifications(con);

        // -------------------------------------------------------------
        // Save Allotment Ledger back to DuckDB
        // -------------------------------------------------------------
        std::cout << "[配额确权] 正在将配额确权明细与缺口日志写入 DuckDB 账本..." << std::endl;
        con.Query("DELETE FROM ipc_allotment_ledger WHERE scenario_id = '" + scenario_id + "';");
        {
            duckdb::Appender appender(con, "ipc_allotment_ledger");
            for (const auto& pair : allotment_constraints) {
                const auto& key = pair.first;
                const auto& state = pair.second;

                appender.BeginRow();
                appender.Append<const char*>(scenario_id.c_str());
                appender.Append<const char*>(state.part_code.c_str());
                appender.Append<const char*>(state.site_code.c_str());
                appender.Append<const char*>(state.region_code.c_str());
                appender.Append<const char*>(state.cust_group_code.c_str());
                appender.Append<const char*>(state.family_code.c_str());
                appender.Append<int32_t>(key.day);
                appender.Append<double>(state.limit);
                appender.Append<double>(state.consumed);
                appender.Append<double>(state.limit >= 0 ? (state.limit - state.consumed) : -1.0); // available
                appender.Append<double>(state.blocked);
                appender.EndRow();
            }
        }
        std::cout << "[配额确权] 配额确权账本同步完成。" << std::endl;

        // I. 打印计算统计大盘

        auto summary_res = con.Query(R"(

            SELECT 

                (SELECT COUNT(*) FROM ipc_part_status) AS 总物料数,

                (SELECT COUNT(*) FROM ipc_planned_order_ledger) AS 计划订单总数,

                (SELECT SUM(order_qty) FROM ipc_planned_order_ledger) AS 计划订单总量,

                (SELECT COUNT(*) FROM ipc_alternate_allocation) AS 替换消纳频次,

                (SELECT SUM(allocated_qty) FROM ipc_alternate_allocation) AS 替换分配总量

        )");

        std::cout << "\n[大盘] [[大盘] 计划大盘] 2026 C++ 维度感知 MRP 引擎最终计划总览：\n" << std::endl;

        summary_res->Print();

        // Detailed Scheduling & Call-off pull optimization
        ipc::run_detailed_scheduling_and_calloff(con, scenario_id);

        // ETO WBS CPM & Project Financial calculations
        ipc::run_wbs_cpm_and_project_financials(con, scenario_id);

        std::cout << "=====================================================================" << std::endl;

        // J. 交互式 SQL REPL

        if (start_repl) {
        std::cout << "\n[提示] 交互式对账查询启动！输入 SQL 并回车执行（例如：SELECT * FROM ipc_alternate_allocation LIMIT 10;）。" << std::endl;

        std::cout << "输入 EXIT 或 QUIT 退出。" << std::endl;

        std::string line;

        while (true) {

            std::cout << "grits_ipc_db> ";

            if (!std::getline(std::cin, line)) break; // EOF

            auto l = line.find_first_not_of(" \t\r\n");

            if (l == std::string::npos) continue;

            auto r = line.find_last_not_of(" \t\r\n");

            std::string sql = line.substr(l, r - l + 1);

            std::string up = sql;

            std::transform(up.begin(), up.end(), up.begin(), [](unsigned char c){ return std::toupper(c); });

            if (up == "EXIT" || up == "QUIT") break;

            try {

                auto res = con.Query(sql);

                if (res) res->Print();

                else std::cout << "(no result)" << std::endl;

            }

            catch (std::exception &e) {

                std::cerr << "SQL 执行错误: " << e.what() << std::endl;

            }

        }
        }

    }

    catch (std::exception& e) {

        std::cerr << "[错误] [运算崩溃] 捕获逻辑错位: " << e.what() << std::endl;

    }

    return 0;

}
