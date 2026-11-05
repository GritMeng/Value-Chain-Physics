#pragma once
#include <string>
#include <vector>
#include "duckdb.hpp"
#include "../ipc_types.h"

namespace ipc {

struct SetupMatrixRecord {
    std::string work_center;
    std::string from_part;
    std::string to_part;
    double setup_time;
};

struct DetailedScheduleRecord {
    std::string sr_id;
    std::string work_center;
    int sequence;
    int scheduled_start_day;
    int scheduled_finish_day;
    double run_time;
    double setup_time;
    double qty;
    int delay_days;
    double delay_penalty;
};

struct LineCallRequestRecord {
    std::string call_id;
    std::string parent_sr_id;
    std::string component_part;
    std::string site;
    double required_qty;
    double allocated_qty;
    int call_day;
    std::string status;
};

struct BomKittingStatusRecord {
    std::string parent_sr_id;
    std::string part_code;
    std::string required_date;
    int total_components;
    int fulfilled_components;
    double kitting_rate;
    std::string kitting_status;
};

void run_detailed_scheduling_and_calloff(
    duckdb::Connection& con,
    const std::string& scenario_id
);

void run_wbs_cpm_and_project_financials(
    duckdb::Connection& con,
    const std::string& scenario_id
);

} // namespace ipc
