#pragma once
#include "ipc_types.h"
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>

namespace ipc {

extern std::mutex substitution_mutex;
extern std::vector<std::vector<OperationRecord>> global_part_routings;
extern std::unordered_map<std::string, std::vector<double>> global_wc_daily_capacity;
extern std::unordered_map<std::string, std::vector<double>> global_wc_allocated_capacity;
extern std::unordered_map<std::string, CalendarRecord> global_calendars;

extern std::vector<std::vector<size_t>> parent_to_bom_indices;
extern std::vector<std::vector<size_t>> child_to_bom_indices;
extern std::unordered_map<int, std::vector<size_t>> alt_group_to_bom_indices;

extern const int TIMELINE_DAYS;
extern const bool DEBUG_PERSIST;
extern const bool DEBUG_CSV_EXPORT;

} // namespace ipc
