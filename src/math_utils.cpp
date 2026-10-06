#include "ipc/math_utils.h"
#include <cmath>
#include <algorithm>

namespace ipc {

double normal_inverse_cdf(double p) {
    if (p <= 0.0 || p >= 1.0) {
        return 0.0;
    }
    double q = (p < 0.5) ? p : (1.0 - p);
    double t = std::sqrt(-2.0 * std::log(q));
    double num = 2.515517 + 0.802853 * t + 0.010328 * t * t;
    double den = 1.0 + 1.432788 * t + 0.189269 * t * t + 0.001308 * t * t * t;
    double val = t - num / den;
    return (p < 0.5) ? -val : val;
}

std::pair<std::vector<double>, double> run_holt_winters(const std::vector<double>& series, int seasonal_period, int forecast_len) {
    int n = static_cast<int>(series.size());
    if (n == 0) {
        return {std::vector<double>(forecast_len, 0.0), 0.0};
    }
    
    if (n < 2 * seasonal_period) {
        double alpha = 0.3;
        double level = series[0];
        for (int i = 1; i < n; ++i) {
            level = alpha * series[i] + (1.0 - alpha) * level;
        }
        std::vector<double> forecast(forecast_len, level);
        double sum_sq_residuals = 0.0;
        for (int i = 0; i < n; ++i) {
            double res = series[i] - level;
            sum_sq_residuals += res * res;
        }
        double std_err = n > 0 ? std::sqrt(sum_sq_residuals / n) : 0.0;
        return {forecast, std_err};
    }
    
    double alpha = 0.2;
    double beta = 0.1;
    double gamma = 0.3;
    
    double sum_initial = 0.0;
    for (int i = 0; i < seasonal_period; ++i) {
        sum_initial += series[i];
    }
    double level = sum_initial / seasonal_period;
    
    double sum_trend = 0.0;
    for (int i = 0; i < seasonal_period; ++i) {
        sum_trend += series[seasonal_period + i] - series[i];
    }
    double trend = sum_trend / (seasonal_period * seasonal_period);
    
    std::vector<double> seasonal(seasonal_period);
    for (int i = 0; i < seasonal_period; ++i) {
        seasonal[i] = series[i] - level;
    }
    
    std::vector<double> fitted;
    fitted.reserve(n);
    double level_t = level;
    double trend_t = trend;
    std::vector<double> seasonal_t = seasonal;
    
    for (int i = 0; i < n; ++i) {
        double val = series[i];
        double last_level = level_t;
        int seq_idx = i % seasonal_period;
        level_t = alpha * (val - seasonal_t[seq_idx]) + (1.0 - alpha) * (last_level + trend_t);
        trend_t = beta * (level_t - last_level) + (1.0 - beta) * trend_t;
        seasonal_t[seq_idx] = gamma * (val - level_t) + (1.0 - gamma) * seasonal_t[seq_idx];
        fitted.push_back(last_level + trend_t + seasonal_t[seq_idx]);
    }
    
    double sum_sq_residuals = 0.0;
    for (int i = 0; i < n; ++i) {
        double res = series[i] - fitted[i];
        sum_sq_residuals += res * res;
    }
    double mse = sum_sq_residuals / n;
    double std_err = std::sqrt(mse);
    
    std::vector<double> forecast;
    forecast.reserve(forecast_len);
    for (int h = 1; h <= forecast_len; ++h) {
        int seq_idx = (n + h - 1) % seasonal_period;
        double val = level_t + h * trend_t + seasonal_t[seq_idx];
        forecast.push_back(std::max(0.0, val));
    }
    
    return {forecast, std_err};
}

} // namespace ipc
