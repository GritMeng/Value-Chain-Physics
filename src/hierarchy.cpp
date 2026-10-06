#include "ipc/hierarchy.h"
#include "ipc/vocab.h"

namespace ipc {

HierarchyResolver::HierarchyResolver(const std::string& name) : dimension_name(name) {}

void HierarchyResolver::add_relation(uint32_t parent_id, uint32_t child_id) {
    if (parent_id == child_id) return;
    parent_to_children[parent_id].push_back(child_id);
    child_to_parents[child_id].push_back(parent_id);
}

void HierarchyResolver::add_relation_by_code(const std::string& parent_code, const std::string& child_code) {
    if (parent_code.empty() || child_code.empty()) return;
    uint32_t parent_id = vocab.get_or_create(parent_code);
    uint32_t child_id = vocab.get_or_create(child_code);
    add_relation(parent_id, child_id);
}

void HierarchyResolver::get_leaves_recursive(uint32_t node_id, 
                                          const std::unordered_set<uint32_t>& leaves_universe,
                                          std::unordered_set<uint32_t>& out_leaves,
                                          std::unordered_set<uint32_t>& visited) const {
    if (visited.count(node_id) > 0) return;
    visited.insert(node_id);

    if (leaves_universe.count(node_id) > 0) {
        out_leaves.insert(node_id);
    }

    auto it = parent_to_children.find(node_id);
    if (it != parent_to_children.end()) {
        for (uint32_t child_id : it->second) {
            get_leaves_recursive(child_id, leaves_universe, out_leaves, visited);
        }
    }
}

std::unordered_set<uint32_t> HierarchyResolver::get_leaves(uint32_t node_id, const std::unordered_set<uint32_t>& leaves_universe) const {
    std::unordered_set<uint32_t> out_leaves;
    std::unordered_set<uint32_t> visited;
    get_leaves_recursive(node_id, leaves_universe, out_leaves, visited);
    
    if (leaves_universe.count(node_id) > 0) {
        out_leaves.insert(node_id);
    }
    return out_leaves;
}

std::unordered_set<uint32_t> HierarchyResolver::get_leaves_by_code(const std::string& node_code, const std::unordered_set<uint32_t>& leaves_universe) const {
    if (!vocab.contains(node_code)) {
        std::unordered_set<uint32_t> out_leaves;
        uint32_t id = vocab.get_or_create(node_code);
        if (leaves_universe.count(id) > 0) {
            out_leaves.insert(id);
        }
        return out_leaves;
    }
    uint32_t node_id = vocab.get_or_create(node_code);
    return get_leaves(node_id, leaves_universe);
}

void HierarchyResolver::clear() {
    parent_to_children.clear();
    child_to_parents.clear();
}

} // namespace ipc
