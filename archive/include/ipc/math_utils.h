#pragma once
#include "ipc_types.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include "ipc/globals.h"

namespace ipc {

// Workday helper inline functions
inline bool is_wc_workday(int day, const std::string& cal_or_wc, const std::unordered_map<std::string, std::vector<double>>& wc_daily_capacity);
inline int get_workday_offset_backward(int start_day, int lead_days, const std::string& cal_or_wc, const std::unordered_map<std::string, std::vector<double>>& wc_daily_capacity);
inline int get_workday_offset_forward(int start_day, int lead_days, const std::string& cal_or_wc, const std::unordered_map<std::string, std::vector<double>>& wc_daily_capacity);

// Math helper functions
double normal_inverse_cdf(double p);
std::pair<std::vector<double>, double> run_holt_winters(const std::vector<double>& series, int seasonal_period = 7, int forecast_len = 30);

// Implementations of inline workday helpers

inline bool is_wc_workday(int day, const std::string& cal_or_wc, const std::unordered_map<std::string, std::vector<double>>& wc_daily_capacity) {
    auto it = global_calendars.find(cal_or_wc);
    if (it != global_calendars.end()) {
        const auto& working_days = it->second.working_days;
        if (day >= 0 && day < (int)working_days.size()) {
            return working_days[day];
        }
        return (day % 7 != 1 && day % 7 != 2);
    }
    
    auto def_it = global_calendars.find("DEFAULT");
    if (def_it != global_calendars.end() && (cal_or_wc == "DEFAULT" || cal_or_wc.find("-DAY") != std::string::npos || cal_or_wc.find("CAL") != std::string::npos || cal_or_wc.find("calendar") != std::string::npos)) {
        const auto& working_days = def_it->second.working_days;
        if (day >= 0 && day < (int)working_days.size()) {
            return working_days[day];
        }
        return (day % 7 != 1 && day % 7 != 2);
    }

    if (!wc_daily_capacity.empty() && !cal_or_wc.empty()) {
        auto cap_it = wc_daily_capacity.find(cal_or_wc);
        if (cap_it != wc_daily_capacity.end()) {
            if (day >= 0 && day < (int)cap_it->second.size()) {
                return cap_it->second[day] > 0.0;
            }
        }
    }
    
    return (day % 7 != 1 && day % 7 != 2);
}

inline int get_workday_offset_backward(int start_day, int lead_days, const std::string& cal_or_wc, const std::unordered_map<std::string, std::vector<double>>& wc_daily_capacity) {
    if (lead_days <= 0) return start_day;
    int day = start_day;
    int counted = 0;
    while (counted < lead_days && day > 0) {
        day--;
        if (is_wc_workday(day, cal_or_wc, wc_daily_capacity)) {
            counted++;
        }
    }
    return day;
}

inline int get_workday_offset_forward(int start_day, int lead_days, const std::string& cal_or_wc, const std::unordered_map<std::string, std::vector<double>>& wc_daily_capacity) {
    if (lead_days <= 0) return start_day;
    int day = start_day;
    int counted = 0;
    while (counted < lead_days && day < 365 - 1) {
        day++;
        if (is_wc_workday(day, cal_or_wc, wc_daily_capacity)) {
            counted++;
        }
    }
    return day;
}

} // namespace ipc
