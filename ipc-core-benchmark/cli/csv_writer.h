#pragma once
// IPC Engine CLI —— 最小 CSV 写出工具
//
// 说明：既有 data_loader.cpp 中的 read_csv 为朴素逗号切分，仅用于读取且不处理
//       引号/转义，不能用于可靠写出。此处实现一个独立的最小写入器，负责：
//         - 表头与数据行写出
//         - 含逗号、引号、换行、前后空白的字段加引号并按 RFC4180 转义
//         - 统一使用 LF 换行
//       既有引擎与基准不被修改，本文件为新增。
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdint>

namespace ipc_cli {

// 判断字段是否必须加引号
inline bool needs_quoting(const std::string& s) {
    if (s.empty()) return false;
    if (s.front() == ' ' || s.back() == ' ') return true;
    for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') return true;
    }
    return false;
}

// 按 RFC4180 转义单个字段
inline std::string escape_field(const std::string& s) {
    if (!needs_quoting(s)) return s;
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"') out.push_back('"'); // 双写引号
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

// 把一行字段拼成 CSV 行
inline std::string join_row(const std::vector<std::string>& fields) {
    std::string out;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i) out.push_back(',');
        out += escape_field(fields[i]);
    }
    return out;
}

// 数值 -> 字符串（避免科学计数法与多余精度丢失）
inline std::string num_to_string(double v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}
inline std::string num_to_string(long long v) { return std::to_string(v); }
inline std::string num_to_string(unsigned long long v) { return std::to_string(v); }
inline std::string num_to_string(int v) { return std::to_string(v); }
inline std::string num_to_string(unsigned int v) { return std::to_string(v); }

// CSV 写出器
class CsvWriter {
public:
    CsvWriter(const std::string& path, const std::vector<std::string>& header)
        : out_(path, std::ios::binary | std::ios::trunc) {
        if (!out_.is_open()) {
            throw std::runtime_error("无法写出结果文件: " + path);
        }
        out_ << join_row(header) << "\n";
    }

    CsvWriter(const std::string& path, const std::vector<std::string>& header,
              const std::vector<std::vector<std::string>>& rows)
        : CsvWriter(path, header) {
        for (const auto& r : rows) write_row(r);
    }

    void write_row(const std::vector<std::string>& fields) {
        out_ << join_row(fields) << "\n";
    }

    void flush() { out_.flush(); if (!out_) throw std::runtime_error("CSV 写出失败"); }

private:
    std::ofstream out_;
};

} // namespace ipc_cli
