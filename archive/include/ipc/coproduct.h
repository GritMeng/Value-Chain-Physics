#pragma once
#include "duckdb.hpp"

namespace ipc {
    void run_ipc_coproduct_dimension_planning_and_persist(duckdb::Connection &con);
    void run_post_sync_scenario_verifications(duckdb::Connection &con);
}
