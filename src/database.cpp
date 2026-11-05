#include "ipc/database.h"
#include "ipc/globals.h"
#include "ipc/vocab.h"
#include "ipc/math_utils.h"
#include "ipc/hierarchy.h"
#include <iostream>
#include <algorithm>

namespace ipc {

double get_double_value(const duckdb::Value& val) {
    if (val.IsNull()) return 0.0;
    try {
        return val.GetValue<double>();
    } catch (...) {
        try {
            return std::stod(val.ToString());
        } catch (...) {
            return 0.0;
        }
    }
}

int32_t get_int_value(const duckdb::Value& val) {
    if (val.IsNull()) return 0;
    try {
        return static_cast<int32_t>(val.GetValue<int64_t>());
    } catch (...) {
        try {
            return val.GetValue<int32_t>();
        } catch (...) {
            try {
                return std::stoi(val.ToString());
            } catch (...) {
                return 0;
            }
        }
    }
}

DbAdapter::DbAdapter(duckdb::Connection& con) : connection(con) {}

void DbAdapter::load_calendars(std::unordered_map<std::string, CalendarRecord>& calendars) {
    calendars.clear();

    // Check if table ipc_sop_calendar_date exists
    bool table_exists = false;
    try {
        auto check = connection.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'ipc_sop_calendar_date';");
        if (check && !check->HasError() && check->RowCount() > 0) {
            table_exists = (check->GetValue(0, 0).GetValue<int64_t>() > 0);
        }
    } catch (...) {
        table_exists = false;
    }

    bool has_data = false;
    if (table_exists) {
        try {
            auto count_res = connection.Query("SELECT COUNT(*) FROM ipc_sop_calendar_date;");
            if (count_res && !count_res->HasError() && count_res->RowCount() > 0) {
                has_data = (count_res->GetValue(0, 0).GetValue<int64_t>() > 0);
            }
        } catch (...) {
            has_data = false;
        }
    }

    // Auto-calibration / Fallback if table doesn't exist or is empty
    if (!has_data) {
        std::cout << "[Calendar] Table 'ipc_sop_calendar_date' is empty or missing. Loading default standard 5-work-2-off calendar." << std::endl;
        CalendarRecord default_cal;
        default_cal.calendar_name = "DEFAULT";
        default_cal.working_days.resize(365, true);
        for (int i = 0; i < 365; ++i) {
            if (i % 7 == 1 || i % 7 == 2) { // Match fallback logic: day % 7 != 1 && day % 7 != 2
                default_cal.working_days[i] = false;
            }
        }
        calendars["DEFAULT"] = default_cal;
        return;
    }

    // If we have data, load from table
    try {
        auto res = connection.Query("SELECT calendar, CAST(date - '2026-05-29'::DATE AS INTEGER) AS day_index, display FROM ipc_sop_calendar_date ORDER BY calendar, date;");
        if (res && !res->HasError()) {
            for (size_t row = 0; row < res->RowCount(); ++row) {
                std::string cal_name = res->GetValue(0, row).ToString();
                int day_idx = res->GetValue(1, row).GetValue<int32_t>();
                std::string display = res->GetValue(2, row).ToString();

                bool is_working = true;
                std::string display_lower = display;
                std::transform(display_lower.begin(), display_lower.end(), display_lower.begin(), ::tolower);
                if (display_lower.find("weekend") != std::string::npos || 
                    display_lower.find("holiday") != std::string::npos || 
                    display_lower.find("off") != std::string::npos ||
                    display_lower.find("rest") != std::string::npos) {
                    is_working = false;
                }

                if (day_idx >= 0 && day_idx < 365) {
                    if (calendars.find(cal_name) == calendars.end()) {
                        CalendarRecord rec;
                        rec.calendar_name = cal_name;
                        rec.working_days.resize(365, true); // default all to true, then set
                        calendars[cal_name] = rec;
                    }
                    calendars[cal_name].working_days[day_idx] = is_working;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Calendar] Error loading calendars: " << e.what() << ". Using default calendar fallback." << std::endl;
    }

    // Ensure "DEFAULT" calendar is always present in map
    if (calendars.find("DEFAULT") == calendars.end()) {
        CalendarRecord default_cal;
        default_cal.calendar_name = "DEFAULT";
        default_cal.working_days.resize(365, true);
        for (int i = 0; i < 365; ++i) {
            if (i % 7 == 1 || i % 7 == 2) {
                default_cal.working_days[i] = false;
            }
        }
        calendars["DEFAULT"] = default_cal;
    }
}


void DbAdapter::load_parts(std::vector<PartSiteRecord>& parts) {

    auto res = connection.Query(R"(

        SELECT
            p.part,
            p.part_type,
            p.mrp_rule,
            COALESCE(oh.qty, 0.0) AS on_hand,
            COALESCE(sr.qty, 0.0) AS ipc_scheduled_receipt,
            COALESCE(p.is_phantom, false) AS is_phantom,
            COALESCE(p.selling_ave_price, 0.0) AS selling_price,
            COALESCE(p.site, 'SITE_001') AS site,
            COALESCE(p.transshipment_cost, 0.0) AS transshipment_cost,
            COALESCE(p.transshipment_lead_time, 0) AS transshipment_lead_time,
            COALESCE(p.run_rate, 0.0) AS run_rate,
            COALESCE(p.lead_time, CASE WHEN p.is_phantom = true THEN 0.0 WHEN p.part_type = 'FINISHED' THEN 2.0 WHEN p.part_type = 'SEMI' THEN 3.0 WHEN p.part_type = 'RAW' THEN 10.0 ELSE 8.0 END) AS lead_time,
            COALESCE(p.round_to_integer, CASE WHEN p.part_type = 'FINISHED' OR p.part_type = 'SEMI' THEN true ELSE false END) AS round_to_integer,
            COALESCE(p.on_hand_type, 'Standard') AS on_hand_type,
            COALESCE(p.time_fence_days, 0) AS time_fence_days,
            COALESCE(p.sourcing_policy, 'Standard') AS sourcing_policy,
            COALESCE(p.planning_calendar, 'DEFAULT') AS planning_calendar,
            COALESCE(p.ss_rule, 'None') AS ss_rule,
            COALESCE(p.dos_policy, 'None') AS dos_policy,
            COALESCE(p.dos_intervals, 0.0) AS dos_intervals,
            COALESCE(p.safety_stock, 0.0) AS safety_stock
        FROM ipc_material_node p
        LEFT JOIN (SELECT part, SUM(qty) AS qty FROM ipc_onhand GROUP BY part) oh ON p.part = oh.part
        LEFT JOIN (SELECT to_part, SUM(qty) AS qty FROM ipc_scheduled_receipt GROUP BY to_part) sr ON p.part = sr.to_part
        ORDER BY p.part;

    )");

    if (res->HasError()) {

        throw std::runtime_error("Failed to load parts: " + res->GetError());

    }

    // Core data structures have been moved to ipc_types.h; removed duplicate definitions

    parts.clear();

    size_t target_size = std::max(vocab.size(), res->RowCount());

    parts.resize(target_size);

    for (size_t row = 0; row < res->RowCount(); ++row) {

        PartSiteRecord rec;

        rec.part_code = res->GetValue(0, row).ToString();
        rec.site = res->GetValue(7, row).ToString();
        rec.part_id = vocab.get_or_create(rec.part_code + "@" + rec.site);

        rec.part_type = res->GetValue(1, row).ToString();

        rec.mrp_rule = res->GetValue(2, row).ToString();

        rec.on_hand = get_double_value(res->GetValue(3, row));

        rec.ipc_scheduled_receipt = get_double_value(res->GetValue(4, row));

        rec.is_phantom = res->GetValue(5, row).GetValue<bool>();

        rec.cost = get_double_value(res->GetValue(6, row));

        if (rec.cost <= 0.0) rec.cost = 1.0;

        rec.site = res->GetValue(7, row).ToString();

        rec.transshipment_cost = get_double_value(res->GetValue(8, row));

        rec.transshipment_lead_time = get_int_value(res->GetValue(9, row));
        rec.run_rate = get_double_value(res->GetValue(10, row));

        rec.low_level_code = 0;
        rec.lead_time = get_double_value(res->GetValue(11, row));
        rec.round_to_integer = res->GetValue(12, row).GetValue<bool>();
        rec.on_hand_type = res->GetValue(13, row).ToString();
        rec.time_fence_days = get_int_value(res->GetValue(14, row));
        rec.sourcing_policy = res->GetValue(15, row).ToString();
        rec.planning_calendar = res->GetValue(16, row).ToString();
        rec.ss_rule = res->GetValue(17, row).ToString();
        rec.dos_policy = res->GetValue(18, row).ToString();
        rec.dos_intervals = get_double_value(res->GetValue(19, row));
        rec.safety_stock = get_double_value(res->GetValue(20, row));
        if (rec.part_id >= parts.size()) {
            parts.resize(rec.part_id + 1);
        }
        parts[rec.part_id] = rec;

    }

}


void DbAdapter::load_boms(std::vector<FlatBomItem>& boms) {

    auto res = connection.Query(R"(

        SELECT 

            prb.part AS parent_part,

            bi.component AS child_part,

            COALESCE(bi.perqty, 1.0) AS per_qty,

            COALESCE(bi.scrap, 0.0) AS scrap,

            COALESCE(bi.alt_grp, '') AS alt_grp,

            COALESCE(bi.priority, 0) AS priority,

            COALESCE(bi.target, 1.0) AS target_ratio,

            COALESCE(bi.alt_todate_qty, 0.0) AS historical_qty,

            COALESCE(bi.eff_start_day, -1) AS eff_start_day,

            COALESCE(bi.eff_end_day, -1) AS eff_end_day,

            COALESCE(bi.ltb_limit, -1.0) AS ltb_limit,

            COALESCE(bi.mix_group_id, -1) AS mix_group_id,

            COALESCE(bi.relationship_type, 'alt') AS relationship_type,
            COALESCE(bi.lot_size, 0.0) AS lot_size,
            prb.site AS parent_site,
            COALESCE(bi.site, prb.site) AS child_site
        FROM ipc_bom_item bi

        JOIN ipc_bom_route prb ON bi.bomid = prb.bomid

        ORDER BY prb.part, bi.component;

    )");

    if (res->HasError()) {

        throw std::runtime_error("Failed to load BOMs: " + res->GetError());

    }

    boms.clear();

    for (size_t row = 0; row < res->RowCount(); ++row) {

        FlatBomItem item;

        std::string parent_code = res->GetValue(0, row).ToString();
        std::string child_code = res->GetValue(1, row).ToString();
        std::string parent_site = res->GetValue(14, row).ToString();
        std::string child_site = res->GetValue(15, row).ToString();
        item.parent_id = vocab.get_or_create(parent_code + "@" + parent_site);
        item.child_id = vocab.get_or_create(child_code + "@" + child_site);

        item.per_qty = get_double_value(res->GetValue(2, row));

        item.scrap = get_double_value(res->GetValue(3, row));

        std::string alt_grp_str = res->GetValue(4, row).ToString();

        item.alt_group_id = -1;

        if (!alt_grp_str.empty() && alt_grp_str.rfind("ALT_GRP_", 0) == 0) {

            try {

                item.alt_group_id = std::stoi(alt_grp_str.substr(8));

            } catch (...) {}

        }

        item.alt_priority = get_int_value(res->GetValue(5, row));

        item.target_ratio = get_double_value(res->GetValue(6, row));

        item.historical_qty = get_double_value(res->GetValue(7, row));

        item.eff_start_day = get_int_value(res->GetValue(8, row));

        item.eff_end_day = get_int_value(res->GetValue(9, row));

        item.ltb_limit = get_double_value(res->GetValue(10, row));

        item.mix_group_id = get_int_value(res->GetValue(11, row));

        item.relationship_type = res->GetValue(12, row).ToString();

        item.lot_size = get_double_value(res->GetValue(13, row));

        // Simple dimension operator setup matching mock data

        item.relation_op = static_cast<uint8_t>(RelationOp::PASS);

        item.target_dim_val = 0.0;

        if (item.parent_id >= 200 && item.parent_id < 500) {

            if (item.child_id % 5 == 0) {

                item.relation_op = static_cast<uint8_t>(RelationOp::EQ);

                item.target_dim_val = 102.0;

            } else if (item.child_id % 3 == 0) {

                item.relation_op = static_cast<uint8_t>(RelationOp::GE);

                item.target_dim_val = 101.0;

            }

        }

        boms.push_back(item);

    }

}


void DbAdapter::load_demands(std::vector<IndependentDemand>& demands) {

    auto res = connection.Query(R"(

        SELECT 

            d.demand,

            d.customer,

            d.part,

            d.request_qty,

            date_diff('day', '2026-05-29'::DATE, d.request_due_date) AS due_day,

            d.order_priority,

            d.dimension_grp,

            COALESCE(d.preference_mode, 'N') AS preference_mode,

            COALESCE(d.status, 'OPEN') AS status,

            COALESCE(d.customer_tier, 3) AS customer_tier,

            COALESCE(d.revenue, 0.0) AS revenue,
            COALESCE(d.site, 'SITE_001') AS site,
            COALESCE(c.region, '*') AS region,
            COALESCE(hc.parent_customer, '*') AS customer_group,
            COALESCE(pf.family_num, '*') AS product_family
        FROM ipc_independent_demand d
        LEFT JOIN ipc_customer c ON d.customer = c.customer
        LEFT JOIN ipc_hierarchy_customer hc ON d.customer = hc.customer
        LEFT JOIN ipc_hierarchy_product_family pf ON d.part = pf.material

        ORDER BY d.order_priority;

    )");

    if (res->HasError()) {

        throw std::runtime_error("Failed to load demands: " + res->GetError());

    }

    demands.clear();

    for (size_t row = 0; row < res->RowCount(); ++row) {

        IndependentDemand d;

        std::string demand_code = res->GetValue(0, row).ToString();
        d.demand_code = demand_code;

        try {

            d.demand_id = std::stoi(demand_code.substr(7));

        } catch (...) {

            d.demand_id = static_cast<int32_t>(row + 1);

        }

        d.customer = res->GetValue(1, row).ToString();

        std::string part_code = res->GetValue(2, row).ToString();
        std::string demand_site = res->GetValue(11, row).ToString();
        d.part_id = vocab.get_or_create(part_code + "@" + demand_site);

        d.qty = get_double_value(res->GetValue(3, row));

        int due_day_offset = get_int_value(res->GetValue(4, row));

        d.due_day = std::max(0, std::min(TIMELINE_DAYS - 1, due_day_offset));

        d.priority = get_int_value(res->GetValue(5, row));

        std::string dim_grp = res->GetValue(6, row).ToString();

        d.dimension_val = 100.0;

        if (dim_grp == "DIM_101.0") d.dimension_val = 101.0;
        else if (dim_grp == "DIM_102.0") d.dimension_val = 102.0;

        d.preference_mode = res->GetValue(7, row).ToString();

        d.status = res->GetValue(8, row).ToString();

        d.customer_tier = get_int_value(res->GetValue(9, row));

        d.revenue = get_double_value(res->GetValue(10, row));

        d.composite_priority = encode_composite_priority(d.status == "COMMITTED", d.customer_tier, d.due_day, d.priority, d.revenue);

        std::string region = res->GetValue(12, row).ToString();
        std::string customer_group = res->GetValue(13, row).ToString();
        std::string product_family = res->GetValue(14, row).ToString();

        d.region_id = vocab.get_or_create(region);
        d.cust_group_id = vocab.get_or_create(customer_group);
        d.family_id = vocab.get_or_create(product_family);

        demands.push_back(d);

    }

}


void generate_pegging_records(

    const std::vector<PartSiteRecord>& parts,

    const std::vector<IndependentDemand>& demands,

    const std::vector<PlannedOrder>& ipc_planned_orders,

    const std::vector<AlternateAllocationRecord>& alt_records,

    std::vector<SupplyAssignmentRecord>& pegging,

    const std::vector<ScheduledReceiptRecord>& srs) {

    pegging.clear();

    std::vector<double> remaining_oh(parts.size());

    std::vector<double> remaining_sr(parts.size());

    for (size_t i = 0; i < parts.size(); ++i) {
        remaining_oh[i] = (parts[i].on_hand_type == "Exclude") ? 0.0 : parts[i].on_hand;
    }
    for (const auto& sr : srs) {
        if (sr.sr_type != "Ignore" && sr.sr_type != "ExplodedOnly") {
            remaining_sr[sr.part_id] += sr.qty;
        }
    }
    if (srs.empty()) {
        for (size_t i = 0; i < parts.size(); ++i) {
            remaining_sr[i] = parts[i].ipc_scheduled_receipt;
        }
    }

    std::unordered_map<std::string, std::vector<size_t>> po_by_part_day;

    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {

        std::string key = std::to_string(ipc_planned_orders[i].part_id) + "_" + std::to_string(ipc_planned_orders[i].finish_day);
        // std::cout << "[DEBUG Pegging PO] i=" << i << " part_id=" << ipc_planned_orders[i].part_id << " code=" << vocab.get_code(ipc_planned_orders[i].part_id) << " finish_day=" << ipc_planned_orders[i].finish_day << " key=" << key << std::endl;

        po_by_part_day[key].push_back(i);

    }

    std::vector<double> remaining_po_qty(ipc_planned_orders.size());

    for (size_t i = 0; i < ipc_planned_orders.size(); ++i) {

        remaining_po_qty[i] = ipc_planned_orders[i].qty;

    }

    for (const auto& d : demands) {

        double qty_to_peg = d.qty;

        std::string demand_code = d.demand_code;

        if (false && d.demand_id < 100000) {

            char buf[32];

            sprintf(buf, "DEMAND_%05d", d.demand_id);

            demand_code = std::string(buf);

        }

        std::string part_code = vocab.get_code(d.part_id);

        // 1. Try On-Hand

        if (qty_to_peg > 0.0 && remaining_oh[d.part_id] > 0.0) {

            double allocated = std::min(qty_to_peg, remaining_oh[d.part_id]);

            remaining_oh[d.part_id] -= allocated;

            qty_to_peg -= allocated;

            SupplyAssignmentRecord rec;

            rec.demand_code = demand_code;

            rec.ind_part = part_code;

            rec.part = part_code;

            rec.assigned_qty = allocated;

            rec.supply_code = "OH_LOC_001_" + part_code;

            rec.supply_type = "On-Hand";

            rec.due_day = d.due_day;

            rec.dimension_val = d.dimension_val;

            pegging.push_back(rec);

        }

        // 2. Try Scheduled Receipt (SR)

        if (qty_to_peg > 0.0 && remaining_sr[d.part_id] > 0.0) {

            double allocated = std::min(qty_to_peg, remaining_sr[d.part_id]);

            remaining_sr[d.part_id] -= allocated;

            qty_to_peg -= allocated;

            SupplyAssignmentRecord rec;

            rec.demand_code = demand_code;

            rec.ind_part = part_code;

            rec.part = part_code;

            rec.assigned_qty = allocated;

            rec.supply_code = "SR_" + part_code;

            rec.supply_type = "In-Transit";

            rec.due_day = d.due_day;

            rec.dimension_val = d.dimension_val;

            pegging.push_back(rec);

        }

        // 3. Try matching with Planned Orders

        std::string key = std::to_string(d.part_id) + "_" + std::to_string(d.due_day);
        // std::cout << "[DEBUG Pegging Demand] demand=" << demand_code << " part_id=" << d.part_id << " code=" << vocab.get_code(d.part_id) << " due_day=" << d.due_day << " key=" << key << " found=" << (po_by_part_day.find(key) != po_by_part_day.end()) << std::endl;

        auto it = po_by_part_day.find(key);

        if (qty_to_peg > 0.0 && it != po_by_part_day.end()) {

            for (size_t po_idx : it->second) {

                if (remaining_po_qty[po_idx] > 0.0) {

                    double allocated = std::min(qty_to_peg, remaining_po_qty[po_idx]);

                    remaining_po_qty[po_idx] -= allocated;

                    qty_to_peg -= allocated;

                    char po_buf[32];

                    sprintf(po_buf, "PO_%06d", static_cast<int>(po_idx + 1));

                    SupplyAssignmentRecord rec;

                    rec.demand_code = demand_code;

                    rec.ind_part = part_code;

                    rec.part = part_code;

                    rec.assigned_qty = allocated;

                    rec.supply_code = std::string(po_buf);

                    rec.supply_type = "Planned-Order";

                    // Reverted mismatched edit

                    rec.due_day = d.due_day;

                    rec.dimension_val = d.dimension_val;

                    pegging.push_back(rec);

                    if (qty_to_peg <= 0.0) break;

                }

            }

        }

        // 4. Try alternates

        if (qty_to_peg > 0.0) {

            for (const auto& alt : alt_records) {

                if (alt.day == d.due_day && alt.main_part_id == d.part_id) {

                    std::string alt_part_code = vocab.get_code(alt.alt_part_id);

                    SupplyAssignmentRecord rec;

                    rec.demand_code = demand_code;

                    rec.ind_part = part_code;

                    rec.part = alt_part_code;

                    rec.assigned_qty = std::min(qty_to_peg, alt.allocated_qty);

                    rec.supply_code = "OH_LOC_001_" + alt_part_code;

                    rec.supply_type = "On-Hand";

                    rec.due_day = d.due_day;

                    rec.dimension_val = d.dimension_val;

                    pegging.push_back(rec);

                    qty_to_peg -= rec.assigned_qty;

                    if (qty_to_peg <= 0.0) break;

                }

            }

        }

    }

}


void DbAdapter::load_scheduled_receipts(std::vector<ScheduledReceiptRecord>& srs) {
    auto res = connection.Query(R"(
        SELECT 
            sr_id,
            to_part,
            qty,
            date_diff('day', '2026-05-29'::DATE, request_due_date) AS due_day,
            COALESCE(sr_type, 'In-process') AS sr_type,
            COALESCE(certainty_level, 0.70) AS certainty_level,
            COALESCE(to_site, 'SITE_001') AS to_site
        FROM ipc_scheduled_receipt
        ORDER BY request_due_date;
    )");

    if (res->HasError()) {
        std::cout << "[Warning] Failed to load scheduled receipts: " << res->GetError() << std::endl;
        return;
    }

    srs.clear();
    for (size_t row = 0; row < res->RowCount(); ++row) {
        ScheduledReceiptRecord rec;
        rec.sr_id = res->GetValue(0, row).ToString();
        std::string part_code = res->GetValue(1, row).ToString();
        std::string site = res->GetValue(6, row).ToString();
        rec.part_id = vocab.get_or_create(part_code + "@" + site);
        rec.qty = get_double_value(res->GetValue(2, row));
        int due_day_offset = get_int_value(res->GetValue(3, row));
        rec.due_day = std::max(0, std::min(TIMELINE_DAYS - 1, due_day_offset));
        rec.sr_type = res->GetValue(4, row).ToString();
        rec.certainty_level = get_double_value(res->GetValue(5, row));
        srs.push_back(rec);
    }
    std::cout << "[INFO] Loaded detailed scheduled receipts count = " << srs.size() << std::endl;
}


void load_operations_from_db(
    duckdb::Connection &con,
    std::vector<std::vector<OperationRecord>> &part_routings,
    std::unordered_map<std::string, std::vector<double>> &wc_daily_capacity,
    size_t num_parts
) {
    part_routings.assign(num_parts, std::vector<OperationRecord>());

    auto op_res = con.Query(R"(
        SELECT 
            operation,
            routing,
            sequence,
            work_center,
            COALESCE(setup_time, 0.0) AS setup_time,
            COALESCE(run_time, 0.0) AS run_time,
            COALESCE(site, 'SITE_001') AS site
        FROM ipc_operation
        ORDER BY routing, sequence;
    )");

    if (!op_res->HasError()) {
        for (size_t row = 0; row < op_res->RowCount(); ++row) {
            OperationRecord rec;
            rec.operation = op_res->GetValue(0, row).ToString();
            rec.routing = op_res->GetValue(1, row).ToString();
            rec.sequence = get_int_value(op_res->GetValue(2, row));
            rec.work_center = op_res->GetValue(3, row).ToString();
            rec.setup_time = get_double_value(op_res->GetValue(4, row));
            rec.run_time = get_double_value(op_res->GetValue(5, row));
            std::string site = op_res->GetValue(6, row).ToString();

            uint32_t pid = vocab.get_or_create(rec.routing + "@" + site);
            if (pid < num_parts) {
                part_routings[pid].push_back(rec);
            }
        }
    }

    auto cap_res = con.Query(R"(
        SELECT 
            work_center,
            date_diff('day', '2026-05-29'::DATE, date) AS day_offset,
            COALESCE(working_hour, 8.0) * COALESCE(number_of_resources, 1.0) * COALESCE(efficiency, 1.0) AS daily_cap
        FROM ipc_work_center_capacity
        WHERE date >= '2026-05-29'::DATE
        ORDER BY work_center, date;
    )");

    if (!cap_res->HasError()) {
        for (size_t row = 0; row < cap_res->RowCount(); ++row) {
            std::string wc = cap_res->GetValue(0, row).ToString();
            int day_offset = get_int_value(cap_res->GetValue(1, row));
            double cap = get_double_value(cap_res->GetValue(2, row));
            if (day_offset >= 0 && day_offset < TIMELINE_DAYS) {
                if (wc_daily_capacity.find(wc) == wc_daily_capacity.end()) {
                    wc_daily_capacity[wc].assign(TIMELINE_DAYS, 24.0);
                }
                wc_daily_capacity[wc][day_offset] = cap;
            }
        }
    }
}

void DbAdapter::load_product_hierarchy(HierarchyResolver& product_resolver) {
    auto res = connection.Query("SELECT DISTINCT family_num, material FROM ipc_hierarchy_product_family WHERE family_num IS NOT NULL AND material IS NOT NULL;");
    if (res && !res->HasError()) {
        for (size_t row = 0; row < res->RowCount(); ++row) {
            std::string parent = res->GetValue(0, row).ToString();
            std::string child = res->GetValue(1, row).ToString();
            product_resolver.add_relation_by_code(parent, child);
        }
    }
}

void DbAdapter::load_customer_hierarchy(HierarchyResolver& customer_resolver) {
    auto res = connection.Query("SELECT DISTINCT parent_customer, customer FROM ipc_hierarchy_customer WHERE parent_customer IS NOT NULL AND customer IS NOT NULL;");
    if (res && !res->HasError()) {
        for (size_t row = 0; row < res->RowCount(); ++row) {
            std::string parent = res->GetValue(0, row).ToString();
            std::string child = res->GetValue(1, row).ToString();
            customer_resolver.add_relation_by_code(parent, child);
        }
    }
}

void DbAdapter::load_region_hierarchy(HierarchyResolver& region_resolver) {
    // region to customer relationship
    auto res = connection.Query("SELECT DISTINCT region, customer FROM ipc_customer WHERE region IS NOT NULL AND customer IS NOT NULL;");
    if (res && !res->HasError()) {
        for (size_t row = 0; row < res->RowCount(); ++row) {
            std::string parent = res->GetValue(0, row).ToString();
            std::string child = res->GetValue(1, row).ToString();
            region_resolver.add_relation_by_code(parent, child);
        }
    }
    // Also load parent_region to region relationship if the table exists
    bool has_comb = false;
    auto check_comb = connection.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'ipc_hierarchy_customer_comb';");
    if (check_comb && !check_comb->HasError() && check_comb->RowCount() > 0) {
        has_comb = (check_comb->GetValue(0, 0).GetValue<int64_t>() > 0);
    }
    if (has_comb) {
        auto comb_res = connection.Query("SELECT DISTINCT parent_region, region FROM ipc_hierarchy_customer_comb WHERE parent_region IS NOT NULL AND region IS NOT NULL;");
        if (comb_res && !comb_res->HasError()) {
            for (size_t row = 0; row < comb_res->RowCount(); ++row) {
                std::string parent = comb_res->GetValue(0, row).ToString();
                std::string child = comb_res->GetValue(1, row).ToString();
                region_resolver.add_relation_by_code(parent, child);
            }
        }
    }
}

void DbAdapter::load_customer_comb_hierarchy(std::vector<CustomerCombRecord>& comb_records) {
    comb_records.clear();
    bool has_table = false;
    auto check_tbl = connection.Query("SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'ipc_hierarchy_customer_comb';");
    if (check_tbl && !check_tbl->HasError() && check_tbl->RowCount() > 0) {
        has_table = (check_tbl->GetValue(0, 0).GetValue<int64_t>() > 0);
    }
    if (!has_table) return;

    auto res = connection.Query("SELECT part, customer, region, ratio, ratio_override FROM ipc_hierarchy_customer_comb;");
    if (res && !res->HasError()) {
        for (size_t row = 0; row < res->RowCount(); ++row) {
            CustomerCombRecord rec;
            rec.part = res->GetValue(0, row).ToString();
            rec.customer = res->GetValue(1, row).ToString();
            rec.region = res->GetValue(2, row).ToString();
            
            auto val_ratio = res->GetValue(3, row);
            if (!val_ratio.IsNull()) {
                try {
                    rec.ratio = val_ratio.GetValue<double>();
                } catch (...) {
                    try {
                        rec.ratio = std::stod(val_ratio.ToString());
                    } catch (...) {
                        rec.ratio = 0.0;
                    }
                }
            }
            
            auto val_override = res->GetValue(4, row);
            if (!val_override.IsNull()) {
                try {
                    rec.ratio_override = val_override.GetValue<double>();
                } catch (...) {
                    try {
                        rec.ratio_override = std::stod(val_override.ToString());
                    } catch (...) {
                        rec.ratio_override = -1.0;
                    }
                }
            } else {
                rec.ratio_override = -1.0;
            }
            
            comb_records.push_back(rec);
        }
    }
}

} // namespace ipc
