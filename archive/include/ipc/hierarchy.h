#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace ipc {

class HierarchyResolver {
private:
    std::string dimension_name;
    std::unordered_map<uint32_t, std::vector<uint32_t>> parent_to_children;
    std::unordered_map<uint32_t, std::vector<uint32_t>> child_to_parents;

    void get_leaves_recursive(uint32_t node_id, 
                              const std::unordered_set<uint32_t>& leaves_universe,
                              std::unordered_set<uint32_t>& out_leaves,
                              std::unordered_set<uint32_t>& visited) const;

public:
    HierarchyResolver(const std::string& name);
    
    void add_relation(uint32_t parent_id, uint32_t child_id);
    void add_relation_by_code(const std::string& parent_code, const std::string& child_code);
    
    std::unordered_set<uint32_t> get_leaves(uint32_t node_id, const std::unordered_set<uint32_t>& leaves_universe) const;
    std::unordered_set<uint32_t> get_leaves_by_code(const std::string& node_code, const std::unordered_set<uint32_t>& leaves_universe) const;
    
    void clear();
};

} // namespace ipc
