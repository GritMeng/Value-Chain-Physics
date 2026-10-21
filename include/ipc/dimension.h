#pragma once
#include "ipc_types.h"
#include <cmath>

namespace ipc {

inline bool evaluate_dimension(double order_val, uint8_t op, double bom_val) {
    switch (static_cast<RelationOp>(op)) {
        case RelationOp::PASS: return true;
        case RelationOp::EQ: return std::abs(order_val - bom_val) < 1e-9;
        case RelationOp::LT: return order_val < bom_val;
        case RelationOp::LE: return order_val <= bom_val;
        case RelationOp::GE: return order_val >= bom_val;
        case RelationOp::GT: return order_val > bom_val;
        case RelationOp::NE: return std::abs(order_val - bom_val) > 1e-9;
        default: return true;
    }
}

} // namespace ipc

