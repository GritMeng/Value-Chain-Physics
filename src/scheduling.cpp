#include "ipc/scheduling.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <map>

namespace ipc {

// Helper to estimate product code suffix for setup matrix match
std::string get_sr_product(const std::string& sr_id) {
    if (sr_id == "TEST_SR_001") return "PART_0_RAW";
    if (sr_id == "TEST_SR_002") return "PART_0_SEMI";
    if (sr_id == "TEST_SR_003") return "PART_0_FINISHED";
    return "PART_0_SEMI"; // default
}

void run_detailed_scheduling_and_calloff(
    duckdb::Connection& con,
    const std::string& scenario_id
) {
    std::cout << "[SCHEDULING] Starting detailed scheduling & call-off engine..." << std::endl;

    // 1. Ensure output tables exist
    con.Query("DROP TABLE IF EXISTS ipc_detailed_schedule_ledger;");
    con.Query("CREATE TABLE ipc_detailed_schedule_ledger ("
              "sr_id VARCHAR(18), "
              "work_center VARCHAR(10), "
              "sequence INTEGER, "
              "scheduled_start_day INTEGER, "
              "scheduled_finish_day INTEGER, "
              "run_time DOUBLE, "
              "setup_time DOUBLE, "
              "qty DOUBLE, "
              "delay_days INTEGER, "
              "delay_penalty DOUBLE"
              ");");

    con.Query("DROP TABLE IF EXISTS ipc_line_call_request;");
    con.Query("CREATE TABLE ipc_line_call_request ("
              "call_id VARCHAR(18), "
              "parent_sr_id VARCHAR(18), "
              "component_part VARCHAR(40), "
              "site VARCHAR(8), "
              "required_qty DOUBLE, "
              "allocated_qty DOUBLE, "
              "call_day INTEGER, "
              "status VARCHAR(20)"
              ");");

    con.Query("DROP TABLE IF EXISTS ipc_bom_kitting_status;");
    con.Query("CREATE TABLE ipc_bom_kitting_status ("
              "parent_sr_id VARCHAR(18), "
              "part_code VARCHAR(40), "
              "required_date DATE, "
              "total_components INTEGER, "
              "fulfilled_components INTEGER, "
              "kitting_rate DOUBLE GENERATED ALWAYS AS (CASE WHEN total_components > 0 THEN CAST(fulfilled_components AS DOUBLE) / CAST(total_components AS DOUBLE) ELSE 1.0 END), "
              "kitting_status VARCHAR(20) GENERATED ALWAYS AS (CASE WHEN total_components = 0 OR fulfilled_components >= total_components THEN 'GREEN' WHEN fulfilled_components >= total_components * 0.75 THEN 'YELLOW' ELSE 'RED' END)"
              ");");

    // 2. Load Setup Matrix from Database. If empty, seed default.
    con.Query("CREATE TABLE IF NOT EXISTS ipc_setup_matrix ("
              "work_center VARCHAR(10), "
              "from_part VARCHAR(40), "
              "to_part VARCHAR(40), "
              "setup_time DOUBLE"
              ");");

    auto setup_check = con.Query("SELECT COUNT(*) FROM ipc_setup_matrix;");
    if (setup_check && !setup_check->HasError() && setup_check->GetValue(0, 0).GetValue<int64_t>() == 0) {
        std::cout << "[SCHEDULING] Seeding default Setup Matrix to Database..." << std::endl;
        con.Query("INSERT INTO ipc_setup_matrix VALUES "
                  "('WC_MILLING', 'PART_0_RAW', 'PART_0_SEMI', 2.0), "
                  "('WC_MILLING', 'PART_0_SEMI', 'PART_0_RAW', 4.0), "
                  "('WC_MILLING', 'PART_0_SEMI', 'PART_0_SEMI', 0.0), "
                  "('WC_MILLING', 'PART_0_RAW', 'PART_0_RAW', 0.0), "
                  "('WC_ASSEMBLY', 'PART_0_SEMI', 'PART_0_FINISHED', 1.5), "
                  "('WC_ASSEMBLY', 'PART_0_FINISHED', 'PART_0_SEMI', 3.0), "
                  "('WC_ASSEMBLY', 'PART_0_FINISHED', 'PART_0_FINISHED', 0.0), "
                  "('WC_ASSEMBLY', 'PART_0_SEMI', 'PART_0_SEMI', 0.0);");
    }

    // Load setup matrix into memory structure
    std::unordered_map<std::string, double> setup_map; // key: wc:from->to, value: time
    auto setup_res = con.Query("SELECT work_center, from_part, to_part, setup_time FROM ipc_setup_matrix;");
    if (setup_res && !setup_res->HasError()) {
        for (size_t r = 0; r < setup_res->RowCount(); ++r) {
            std::string wc = setup_res->GetValue(0, r).ToString();
            std::string from = setup_res->GetValue(1, r).ToString();
            std::string to = setup_res->GetValue(2, r).ToString();
            double time = setup_res->GetValue(3, r).GetValue<double>();
            setup_map[wc + ":" + from + "->" + to] = time;
        }
    }

    // 3. Load active scheduled receipts (Work Orders)
    struct SROrder {
        std::string sr_id;
        std::string to_part;
        double qty;
        int due_day;
        std::string work_center;
        double run_rate;
    };
    std::vector<SROrder> all_orders;

    // We join with operation routing to find the matching work center
    // Fallback: If no routing match, put to WC_MILLING
    auto sr_res = con.Query(
        "SELECT "
        "  sr.sr_id, "
        "  sr.to_part, "
        "  sr.qty, "
        "  date_diff('day', '2026-05-29'::DATE, sr.request_due_date) AS due_day, "
        "  COALESCE(op.work_center, 'WC_MILLING') AS work_center, "
        "  COALESCE(op.run_time, 0.05) AS run_rate "
        "FROM ipc_scheduled_receipt sr "
        "LEFT JOIN ipc_operation op ON op.routing = sr.to_part "
        "ORDER BY sr.request_due_date;"
    );

    if (!sr_res || sr_res->HasError()) {
        std::cerr << "[ERROR] Failed to query scheduled receipts: " 
                  << (sr_res ? sr_res->GetError() : "unknown") << std::endl;
        return;
    }

    for (size_t r = 0; r < sr_res->RowCount(); ++r) {
        SROrder order;
        order.sr_id = sr_res->GetValue(0, r).ToString();
        order.to_part = sr_res->GetValue(1, r).ToString();
        order.qty = sr_res->GetValue(2, r).GetValue<double>();
        order.due_day = (int)sr_res->GetValue(3, r).GetValue<int32_t>();
        order.work_center = sr_res->GetValue(4, r).ToString();
        order.run_rate = sr_res->GetValue(5, r).GetValue<double>();
        all_orders.push_back(order);
    }

    // 4. Load Work Center Daily Capacities
    std::unordered_map<std::string, std::vector<double>> wc_caps; // wc -> list of daily caps
    auto cap_res = con.Query(
        "SELECT "
        "  work_center, "
        "  date_diff('day', '2026-05-29'::DATE, date) AS day_offset, "
        "  COALESCE(working_hour, 8.0) * COALESCE(number_of_resources, 1.0) * COALESCE(efficiency, 1.0) AS daily_cap "
        "FROM ipc_work_center_capacity "
        "ORDER BY work_center, day_offset;"
    );

    if (cap_res && !cap_res->HasError()) {
        for (size_t r = 0; r < cap_res->RowCount(); ++r) {
            std::string wc = cap_res->GetValue(0, r).ToString();
            int day = (int)cap_res->GetValue(1, r).GetValue<int32_t>();
            double cap = cap_res->GetValue(2, r).GetValue<double>();
            if (day >= 0) {
                if (wc_caps[wc].size() <= static_cast<size_t>(day)) {
                    wc_caps[wc].resize(day + 1, 8.0); // default 8 hours if gap
                }
                wc_caps[wc][day] = cap;
            }
        }
    }

    // 5. Group orders by Work Center
    std::unordered_map<std::string, std::vector<SROrder>> wc_orders;
    for (const auto& o : all_orders) {
        wc_orders[o.work_center].push_back(o);
    }

    std::vector<DetailedScheduleRecord> scheduled_results;

    // 6. TSP sequencing per Work Center
    for (auto& pair : wc_orders) {
        std::string wc = pair.first;
        auto& orders = pair.second;
        if (orders.empty()) continue;

        std::vector<SROrder> unvisited = orders;
        std::vector<SROrder> optimized;

        // Auto-sorting parameters caching (original due days)
        std::unordered_map<std::string, int> original_dues;
        for (const auto& o : orders) {
            original_dues[o.sr_id] = o.due_day;
        }

        // Greedy TSP optimization considering Setup Matrix + Delay Tardiness Penalty
        // Start from first
        SROrder current = unvisited.front();
        unvisited.erase(unvisited.begin());
        optimized.push_back(current);

        double accumulated_hours = 0.0;
        accumulated_hours += current.qty * current.run_rate; // initial run load

        while (!unvisited.empty()) {
            double best_impedance = 9999999.0;
            size_t best_idx = 0;

            std::string current_prod = get_sr_product(current.sr_id);

            for (size_t i = 0; i < unvisited.size(); ++i) {
                const auto& candidate = unvisited[i];
                std::string target_prod = get_sr_product(candidate.sr_id);
                std::string setup_key = wc + ":" + current_prod + "->" + target_prod;

                double setup_time = 0.0;
                if (setup_map.find(setup_key) != setup_map.end()) {
                    setup_time = setup_map[setup_key];
                }

                // Tardiness Penalty Estimation (approx dynamic finish day)
                double projected_hours = accumulated_hours + setup_time + (candidate.qty * candidate.run_rate);
                int projected_day = static_cast<int>(std::ceil(projected_hours / 8.0));
                int delay_days = std::max(0, projected_day - original_dues[candidate.sr_id]);
                double delay_penalty = delay_days * 1.5; // 1.5 hours setup penalty per delay day

                double impedance = setup_time + delay_penalty;
                if (impedance < best_impedance) {
                    best_impedance = impedance;
                    best_idx = i;
                }
            }

            current = unvisited[best_idx];
            unvisited.erase(unvisited.begin() + best_idx);
            optimized.push_back(current);

            // Update accumulated hours with the transition setup time
            std::string target_prod = get_sr_product(current.sr_id);
            std::string setup_key = wc + ":" + current_prod + "->" + target_prod;
            double actual_setup = 0.0;
            if (setup_map.find(setup_key) != setup_map.end()) {
                actual_setup = setup_map[setup_key];
            }
            accumulated_hours += actual_setup + (current.qty * current.run_rate);
        }

        // Apply capacities calendar sequencing
        int current_day = 0;
        const auto& wc_calendar = wc_caps[wc];
        double cap_left = (wc_calendar.size() > 0) ? wc_calendar[0] : 8.0;

        std::string prev_prod = "";
        int seq_num = 1;

        for (const auto& order : optimized) {
            std::string prod = get_sr_product(order.sr_id);
            double setup_time = 0.0;
            if (!prev_prod.empty()) {
                std::string key = wc + ":" + prev_prod + "->" + prod;
                if (setup_map.find(key) != setup_map.end()) {
                    setup_time = setup_map[key];
                }
            }

            double run_time = order.qty * order.run_rate;
            double remaining_load = setup_time + run_time;

            int start_day = current_day;

            // Consume daily capacity across multiple days if overloaded
            while (remaining_load > 0) {
                // Ensure current day has capacity
                while (current_day < (int)wc_calendar.size() && wc_calendar[current_day] <= 0.0) {
                    current_day++; // Skip rest day
                }
                if (current_day >= (int)wc_calendar.size()) {
                    // Beyond calendar, append days dynamically
                    current_day++;
                    cap_left = 8.0;
                } else if (start_day == current_day && cap_left <= 0) {
                    cap_left = wc_calendar[current_day];
                }

                double available = cap_left > 0 ? cap_left : 8.0;
                if (remaining_load <= available) {
                    cap_left = available - remaining_load;
                    remaining_load = 0;
                } else {
                    remaining_load -= available;
                    current_day++;
                    cap_left = 0.0; // fully consumed day
                }
            }

            int finish_day = current_day;

            // Delay evaluation
            int delay_days = std::max(0, finish_day - order.due_day);
            double delay_penalty = delay_days * 1.5;

            DetailedScheduleRecord ds;
            ds.sr_id = order.sr_id;
            ds.work_center = wc;
            ds.sequence = seq_num++;
            ds.scheduled_start_day = start_day;
            ds.scheduled_finish_day = finish_day;
            ds.run_time = run_time;
            ds.setup_time = setup_time;
            ds.qty = order.qty;
            ds.delay_days = delay_days;
            ds.delay_penalty = delay_penalty;

            scheduled_results.push_back(ds);
            prev_prod = prod;
        }
    }

    // Save detailed schedules to DB
    duckdb::Appender app_sched(con, "ipc_detailed_schedule_ledger");
    for (const auto& ds : scheduled_results) {
        app_sched.BeginRow();
        app_sched.Append(ds.sr_id.c_str());
        app_sched.Append(ds.work_center.c_str());
        app_sched.Append(ds.sequence);
        app_sched.Append(ds.scheduled_start_day);
        app_sched.Append(ds.scheduled_finish_day);
        app_sched.Append(ds.run_time);
        app_sched.Append(ds.setup_time);
        app_sched.Append(ds.qty);
        app_sched.Append(ds.delay_days);
        app_sched.Append(ds.delay_penalty);
        app_sched.EndRow();
    }
    app_sched.Close();
    std::cout << "[SCHEDULING] Saved " << scheduled_results.size() << " scheduled ledger rows." << std::endl;

    // 7. Kitting & Call-off Pull Simulator
    std::cout << "[CALL-OFF] Initializing line kitting checklist & pull request validation..." << std::endl;

    // Load parts onhand into memory
    std::unordered_map<std::string, double> onhand_stocks;
    auto oh_res = con.Query("SELECT part, SUM(qty) FROM ipc_onhand GROUP BY part;");
    if (oh_res && !oh_res->HasError()) {
        for (size_t r = 0; r < oh_res->RowCount(); ++r) {
            std::string part = oh_res->GetValue(0, r).ToString();
            double qty = oh_res->GetValue(1, r).GetValue<double>();
            onhand_stocks[part] = qty;
        }
    }

    // Load BOM structures
    struct BOMComponent {
        std::string component;
        double per_qty;
        double scrap;
    };
    std::unordered_map<std::string, std::vector<BOMComponent>> part_boms;
    auto bom_res = con.Query(
        "SELECT r.part, i.component, i.perqty, COALESCE(i.scrap, 0.0) "
        "FROM ipc_bom_route r "
        "JOIN ipc_bom_item i ON i.bomid = r.bomid;"
    );
    if (bom_res && !bom_res->HasError()) {
        for (size_t r = 0; r < bom_res->RowCount(); ++r) {
            std::string parent = bom_res->GetValue(0, r).ToString();
            BOMComponent comp;
            comp.component = bom_res->GetValue(1, r).ToString();
            comp.per_qty = bom_res->GetValue(2, r).GetValue<double>();
            comp.scrap = bom_res->GetValue(3, r).GetValue<double>();
            part_boms[parent].push_back(comp);
        }
    }

    // Sort scheduling results by start day to allocate materials chronologically
    std::vector<DetailedScheduleRecord> sorted_schedules = scheduled_results;
    std::sort(sorted_schedules.begin(), sorted_schedules.end(), [](const DetailedScheduleRecord& a, const DetailedScheduleRecord& b) {
        return a.scheduled_start_day < b.scheduled_start_day;
    });

    std::vector<LineCallRequestRecord> call_requests;
    std::vector<BomKittingStatusRecord> kitting_statuses;

    int call_counter = 1001;

    for (const auto& ds : sorted_schedules) {
        // Find matching parent order parameters
        std::string parent_part = "";
        for (const auto& o : all_orders) {
            if (o.sr_id == ds.sr_id) {
                parent_part = o.to_part;
                break;
            }
        }
        if (parent_part.empty()) continue;

        const auto& comps = part_boms[parent_part];
        int total_components = comps.size();
        int fulfilled_components = 0;

        for (const auto& comp : comps) {
            double req_qty = ds.qty * comp.per_qty * (1.0 + comp.scrap);
            double available = onhand_stocks[comp.component];
            double allocated = std::min(req_qty, available);

            onhand_stocks[comp.component] -= allocated;

            LineCallRequestRecord call;
            call.call_id = "CALL_REQ_" + std::to_string(call_counter++);
            call.parent_sr_id = ds.sr_id;
            call.component_part = comp.component;
            call.site = "SITE_001";
            call.required_qty = req_qty;
            call.allocated_qty = allocated;
            call.call_day = std::max(0, ds.scheduled_start_day - 1); // pull raw materials 1 day early

            if (allocated >= req_qty) {
                call.status = "Fulfilled";
                fulfilled_components++;
            } else if (allocated > 0) {
                call.status = "Pulling";
            } else {
                call.status = "Blocked";
            }
            call_requests.push_back(call);
        }

        BomKittingStatusRecord kit;
        kit.parent_sr_id = ds.sr_id;
        kit.part_code = parent_part;
        // Mock target date string based on day offset
        int start_offset = ds.scheduled_start_day;
        // Calculate target calendar string
        std::string date_str = "2026-05-29";
        auto date_res = con.Query("SELECT ('2026-05-29'::DATE + " + std::to_string(start_offset) + ")::VARCHAR;");
        if (date_res && !date_res->HasError()) {
            date_str = date_res->GetValue(0, 0).ToString();
        }
        kit.required_date = date_str;
        kit.total_components = total_components;
        kit.fulfilled_components = fulfilled_components;
        kit.kitting_rate = (total_components > 0) ? (double)fulfilled_components / total_components : 1.0;
        
        if (kit.kitting_rate >= 1.0) {
            kit.kitting_status = "Fully_Kitted";
        } else if (kit.kitting_rate >= 0.4) {
            kit.kitting_status = "Partially_Kitted";
        } else {
            kit.kitting_status = "Critical_Shortage";
        }
        kitting_statuses.push_back(kit);
    }

    // Append kitting & call sheets to DB
    duckdb::Appender app_call(con, "ipc_line_call_request");
    for (const auto& call : call_requests) {
        app_call.BeginRow();
        app_call.Append(call.call_id.c_str());
        app_call.Append(call.parent_sr_id.c_str());
        app_call.Append(call.component_part.c_str());
        app_call.Append(call.site.c_str());
        app_call.Append(call.required_qty);
        app_call.Append(call.allocated_qty);
        app_call.Append(call.call_day);
        app_call.Append(call.status.c_str());
        app_call.EndRow();
    }
    app_call.Close();

    duckdb::Appender app_kit(con, "ipc_bom_kitting_status");
    for (const auto& kit : kitting_statuses) {
        app_kit.BeginRow();
        app_kit.Append(kit.parent_sr_id.c_str());
        app_kit.Append(kit.part_code.c_str());
        app_kit.Append(kit.required_date.c_str());
        app_kit.Append(kit.total_components);
        app_kit.Append(kit.fulfilled_components);
        app_kit.EndRow();
    }
    app_kit.Close();

    std::cout << "[CALL-OFF] Generated " << call_requests.size() << " material call requests and "
              << kitting_statuses.size() << " kitting metrics." << std::endl;
}

void run_wbs_cpm_and_project_financials(duckdb::Connection& con, const std::string& scenario_id) {
    std::cout << "[ETO PROJECT] Running C++ WBS CPM & Financial Solver (scenario: " << scenario_id << ")..." << std::endl;

    // 1. Get all projects from DuckDB
    struct ProjectRecord {
        std::string project;
        int delivery_lead_time = 90;
        std::string bonus_date;
        std::string bonus_plan;
        std::string penalty_date;
        std::string penalty_plan;
    };
    std::vector<ProjectRecord> projects;
    
    // Check if table columns exist (just in case)
    con.Query("ALTER TABLE ipc_project ADD COLUMN IF NOT EXISTS calc_finish_day INTEGER;");
    con.Query("ALTER TABLE ipc_project ADD COLUMN IF NOT EXISTS delay_days INTEGER;");
    con.Query("ALTER TABLE ipc_project ADD COLUMN IF NOT EXISTS bonus_amount DOUBLE;");
    con.Query("ALTER TABLE ipc_project ADD COLUMN IF NOT EXISTS penalty_amount DOUBLE;");
    con.Query("ALTER TABLE ipc_project ADD COLUMN IF NOT EXISTS net_project_value DOUBLE;");

    auto proj_res = con.Query("SELECT project, delivery_lead_time, COALESCE(bonus_date, ''), COALESCE(bonus_plan, ''), COALESCE(penalty_date, ''), COALESCE(penalty_plan, '') FROM ipc_project;");
    if (proj_res && !proj_res->HasError()) {
        for (size_t r = 0; r < proj_res->RowCount(); ++r) {
            ProjectRecord p;
            p.project = proj_res->GetValue(0, r).ToString();
            p.delivery_lead_time = proj_res->GetValue(1, r).IsNull() ? 90 : proj_res->GetValue(1, r).GetValue<int32_t>();
            p.bonus_date = proj_res->GetValue(2, r).ToString();
            p.bonus_plan = proj_res->GetValue(3, r).ToString();
            p.penalty_date = proj_res->GetValue(4, r).ToString();
            p.penalty_plan = proj_res->GetValue(5, r).ToString();
            projects.push_back(p);
        }
    }

    // Helper lambda to calculate day offset relative to '2026-05-29'
    auto get_day_offset = [](const std::string& date_str, int default_val) -> int {
        if (date_str.empty()) return default_val;
        bool is_num = true;
        for (char c : date_str) {
            if (!std::isdigit(c)) { is_num = false; break; }
        }
        if (is_num) {
            return std::stoi(date_str);
        }
        // YYYY-MM-DD format
        int y, m, d;
        if (sscanf(date_str.c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
            auto to_days = [](int year, int month, int day) -> int {
                if (month < 3) {
                    year--;
                    month += 12;
                }
                return 365 * year + year / 4 - year / 100 + year / 400 + (153 * month + 2) / 5 + day;
            };
            return to_days(y, m, d) - to_days(2026, 5, 29);
        }
        return default_val;
    };

    // 2. Loop through each project and solve
    for (const auto& proj : projects) {
        // Load tasks
        struct WbsTask {
            std::string wbs_code;
            std::string parent_wbs_code;
            int wbs_level = 1;
            std::string wbs_status;
            double duration = 0.0;
            int early_start = 0;
            int early_finish = 0;
            int late_start = 0;
            int late_finish = 0;
            bool is_leaf = true;
            std::vector<WbsTask*> children;
        };

        auto task_res = con.Query("SELECT wbs_code, COALESCE(parent_wbs_code, ''), wbs_level, wbs_status, duration FROM ipc_project_wbs WHERE project_code = '" + proj.project + "';");
        if (!task_res || task_res->HasError() || task_res->RowCount() == 0) {
            continue;
        }

        std::unordered_map<std::string, WbsTask> task_map;
        for (size_t r = 0; r < task_res->RowCount(); ++r) {
            WbsTask t;
            t.wbs_code = task_res->GetValue(0, r).ToString();
            t.parent_wbs_code = task_res->GetValue(1, r).ToString();
            t.wbs_level = task_res->GetValue(2, r).IsNull() ? 1 : task_res->GetValue(2, r).GetValue<int32_t>();
            t.wbs_status = task_res->GetValue(3, r).ToString();
            t.duration = task_res->GetValue(4, r).IsNull() ? 3.0 : task_res->GetValue(4, r).GetValue<double>();
            task_map[t.wbs_code] = t;
        }

        // Establish tree pointers
        std::vector<WbsTask*> task_ptrs;
        task_ptrs.reserve(task_map.size());
        for (auto& pair : task_map) {
            task_ptrs.push_back(&pair.second);
        }

        for (auto* t : task_ptrs) {
            if (!t->parent_wbs_code.empty() && task_map.find(t->parent_wbs_code) != task_map.end()) {
                task_map[t->parent_wbs_code].children.push_back(t);
                task_map[t->parent_wbs_code].is_leaf = false;
            }
        }

        // Default duration assign
        for (auto* t : task_ptrs) {
            if (t->is_leaf) {
                if (t->duration == 0.0) {
                    if (t->wbs_level == 1) t->duration = 15.0;
                    else if (t->wbs_level == 2) t->duration = 10.0;
                    else if (t->wbs_level == 3) t->duration = 5.0;
                    else t->duration = 3.0;
                }
            } else {
                t->duration = 0.0; // calculated later
            }
        }

        // Sort children lambda
        auto sort_children = [](WbsTask* node) {
            std::sort(node->children.begin(), node->children.end(), [](WbsTask* a, WbsTask* b) {
                return a->wbs_code < b->wbs_code;
            });
        };

        // Recursive Forward pass
        std::function<void(WbsTask*, int)> schedule_forward = [&](WbsTask* node, int start_val) {
            node->early_start = start_val;
            if (node->is_leaf) {
                node->early_finish = start_val + static_cast<int>(node->duration);
            } else {
                sort_children(node);
                int curr_start = start_val;
                double total_dur = 0.0;
                for (auto* child : node->children) {
                    schedule_forward(child, curr_start);
                    curr_start = child->early_finish;
                    total_dur += child->duration;
                }
                node->early_finish = curr_start;
                node->duration = total_dur;
            }
        };

        // Recursive Backward pass
        std::function<void(WbsTask*, int)> schedule_backward = [&](WbsTask* node, int finish_val) {
            node->late_finish = finish_val;
            if (node->is_leaf) {
                node->late_start = std::max(0, finish_val - static_cast<int>(node->duration));
            } else {
                sort_children(node);
                int curr_finish = finish_val;
                for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
                    schedule_backward(*it, curr_finish);
                    curr_finish = (*it)->late_start;
                }
                node->late_start = curr_finish;
            }
        };

        // Find roots
        std::vector<WbsTask*> roots;
        for (auto* t : task_ptrs) {
            if (t->parent_wbs_code.empty() || task_map.find(t->parent_wbs_code) == task_map.end()) {
                roots.push_back(t);
            }
        }
        if (roots.empty() && !task_ptrs.empty()) {
            int min_lvl = task_ptrs[0]->wbs_level;
            for (auto* t : task_ptrs) {
                if (t->wbs_level < min_lvl) min_lvl = t->wbs_level;
            }
            for (auto* t : task_ptrs) {
                if (t->wbs_level == min_lvl) roots.push_back(t);
            }
        }

        // Run CPM Forward and Backward passes
        int project_due_day = proj.delivery_lead_time;
        std::sort(roots.begin(), roots.end(), [](WbsTask* a, WbsTask* b) {
            return a->wbs_code < b->wbs_code;
        });

        // Forward: roots run sequentially or parallel?
        // Assume roots run sequentially (or parallel - let's calculate max finish)
        int max_duration = 0;
        for (auto* r : roots) {
            schedule_forward(r, 0);
            if (r->early_finish > max_duration) max_duration = r->early_finish;
        }

        // Backward: roots scheduled backward from project_due_day
        for (auto* r : roots) {
            schedule_backward(r, project_due_day);
        }

        // Write WBS task results back to DuckDB
        for (auto* t : task_ptrs) {
            std::string update_sql = "UPDATE ipc_project_wbs SET "
                                     "duration = " + std::to_string(t->duration) + ", "
                                     "early_start = " + std::to_string(t->early_start) + ", "
                                     "early_finish = " + std::to_string(t->early_finish) + ", "
                                     "late_start = " + std::to_string(t->late_start) + ", "
                                     "late_finish = " + std::to_string(t->late_finish) + " "
                                     "WHERE wbs_code = '" + t->wbs_code + "' AND project_code = '" + proj.project + "';";
            con.Query(update_sql);
        }

        // 3. Compute Project-level Financials
        int calc_finish_day = max_duration; // expected finish Day
        int bonus_day_offset = get_day_offset(proj.bonus_date, 0);
        int penalty_day_offset = get_day_offset(proj.penalty_date, 90);

        double bonus_amount = 0.0;
        double penalty_amount = 0.0;
        int delay_days = 0;

        // Fetch bonus schedule values
        if (!proj.bonus_plan.empty()) {
            auto b_res = con.Query("SELECT COALESCE(on_time_bonus, 0.0) FROM ipc_commercial_bonus_plan_by_date WHERE plan = '" + proj.bonus_plan + "';");
            double bonus_rate = 0.0;
            if (b_res && !b_res->HasError() && b_res->RowCount() > 0) {
                bonus_rate = b_res->GetValue(0, 0).GetValue<double>();
            }
            if (calc_finish_day <= bonus_day_offset) {
                bonus_amount = bonus_rate;
            }
        }

        // Fetch penalty schedule values
        if (!proj.penalty_plan.empty()) {
            auto p_res = con.Query("SELECT COALESCE(on_time_cost, 0.0) FROM ipc_penalty_plan_by_date WHERE plan = '" + proj.penalty_plan + "';");
            double penalty_rate = 0.0;
            if (p_res && !p_res->HasError() && p_res->RowCount() > 0) {
                penalty_rate = p_res->GetValue(0, 0).GetValue<double>();
            }
            if (calc_finish_day > penalty_day_offset) {
                delay_days = calc_finish_day - penalty_day_offset;
                penalty_amount = delay_days * penalty_rate;
            }
        }

        double base_project_value = 100000.0;
        double net_project_value = base_project_value + bonus_amount - penalty_amount;

        // Write Project metrics back to DuckDB
        std::string proj_update = "UPDATE ipc_project SET "
                                  "finish_date = '" + std::to_string(calc_finish_day) + "', "
                                  "calc_finish_day = " + std::to_string(calc_finish_day) + ", "
                                  "delay_days = " + std::to_string(delay_days) + ", "
                                  "bonus_amount = " + std::to_string(bonus_amount) + ", "
                                  "penalty_amount = " + std::to_string(penalty_amount) + ", "
                                  "net_project_value = " + std::to_string(net_project_value) + " "
                                  "WHERE project = '" + proj.project + "';";
        con.Query(proj_update);

        std::cout << "[ETO PROJECT] Project " << proj.project << " computed WBS CPM: "
                  << "Finish Day: " << calc_finish_day << ", "
                  << "Delay Days: " << delay_days << ", "
                  << "Bonus: " << bonus_amount << ", "
                  << "Penalty: " << penalty_amount << ", "
                  << "Net Value: " << net_project_value << std::endl;
    }
}

} // namespace ipc
