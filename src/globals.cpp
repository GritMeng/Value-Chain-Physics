#include "ipc/globals.h"

namespace ipc {

std::mutex substitution_mutex;
std::vector<std::vector<OperationRecord>> global_part_routings;
std::unordered_map<std::string, std::vector<double>> global_wc_daily_capacity;
std::unordered_map<std::string, std::vector<double>> global_wc_allocated_capacity;
std::unordered_map<std::string, CalendarRecord> global_calendars;

std::vector<std::vector<size_t>> parent_to_bom_indices;
std::vector<std::vector<size_t>> child_to_bom_indices;
std::unordered_map<int, std::vector<size_t>> alt_group_to_bom_indices;

const int TIMELINE_DAYS = 91;
const bool DEBUG_PERSIST = true;
const bool DEBUG_CSV_EXPORT = false;

} // namespace ipc
