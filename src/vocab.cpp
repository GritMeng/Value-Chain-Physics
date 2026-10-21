#include "ipc/vocab.h"

namespace ipc {

PartVocab vocab;

uint32_t PartVocab::get_or_create(const std::string& code) {
    auto it = code_to_id.find(code);
    if (it != code_to_id.end()) return it->second;
    uint32_t new_id = static_cast<uint32_t>(id_to_code.size());
    id_to_code.push_back(code);
    code_to_id[code] = new_id;
    return new_id;
}

std::string PartVocab::get_code(uint32_t id) const {
    return (id < id_to_code.size()) ? id_to_code[id] : "UNKNOWN";
}

size_t PartVocab::size() const { 
    return id_to_code.size(); 
}

bool PartVocab::contains(const std::string& code) const { 
    return code_to_id.count(code) > 0; 
}

std::string get_raw_part_code(const std::string& key) {
    size_t pos = key.find('@');
    if (pos != std::string::npos) {
        return key.substr(0, pos);
    }
    return key;
}

std::string get_site_code(const std::string& key) {
    size_t pos = key.find('@');
    if (pos != std::string::npos) {
        return key.substr(pos + 1);
    }
    return "SITE_001";
}

} // namespace ipc
