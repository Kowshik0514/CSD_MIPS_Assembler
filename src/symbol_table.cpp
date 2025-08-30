#include "symbol_table.h"

bool SymbolTable::add_label(const std::string& name, uint32_t address) {
    if (table.count(name)) {
        return false; 
    }
    table[name] = {name, address, SymbolType::LABEL};
    return true;
}

std::optional<uint32_t> SymbolTable::get_address(const std::string& name) const {
    auto it = table.find(name);
    if (it != table.end()) {
        return it->second.address;
    }
    return std::nullopt; // Symbol not found
}

size_t SymbolTable::size() const {
    return table.size();
}