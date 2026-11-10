#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace ipc {

class PartVocab {
private:
    std::vector<std::string> id_to_code;
    std::unordered_map<std::string, uint32_t> code_to_id;
public:
    uint32_t get_or_create(const std::string& code);
    std::string get_code(uint32_t id) const;
    size_t size() const;
    bool contains(const std::string& code) const;
};

extern PartVocab vocab;

std::string get_raw_part_code(const std::string& key);
std::string get_site_code(const std::string& key);

} // namespace ipc
