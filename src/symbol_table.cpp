#include "symbol_table.h"

// /**
//  * @brief Adds a new label to the symbol table.
//  * @param name The name of the label.
//  * @param address The address (instruction index) of the label.
//  * @return True if the label was added successfully, false if it's a duplicate.
//  */
bool SymbolTable::add_label(const std::string& name, uint32_t address) {
    // Check if the symbol already exists in the table.
    if (table.count(name)) {
        return false; // Return false to indicate a duplicate symbol.
    }
    // If not, create a new Symbol struct and add it to the map.
    table[name] = {name, address, SymbolType::LABEL};
    return true;
}

/**
 * @brief Retrieves the address of a symbol.
 * @param name The name of the symbol to look up.
 * @return An optional containing the address if found, otherwise an empty optional.
 */
std::optional<uint32_t> SymbolTable::get_address(const std::string& name) const {
    // Find the symbol in the map.
    auto it = table.find(name);
    if (it != table.end()) {
        // If found, return the address wrapped in an std::optional.
        return it->second.address;
    }
    // If not found, return an empty optional.
    return std::nullopt;
}