#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <map>
#include <cstdint>
#include <optional> // Used for safely returning lookups

// Defines the different types of symbols we might encounter.
enum class SymbolType {
    LABEL,
    // Future types can be added here, e.g., DATA, GLOBAL_LABEL
};

// Represents a single entry in the symbol table.
struct Symbol {
    std::string name;
    uint32_t address;
    SymbolType type;
};

// The SymbolTable class encapsulates all logic for managing symbols.
class SymbolTable {
public:
    // Tries to add a new label symbol. Returns false if the symbol already exists.
    bool add_label(const std::string& name, uint32_t address);

    // Alias for adding symbols (future expansion).
    inline bool add_symbol(const std::string& name, uint32_t address) {
        return add_label(name, address);
    }

    // Looks up a symbol's address by its name.
    std::optional<uint32_t> get_address(const std::string& name) const;

    // Returns the total number of symbols in the table.
    size_t size() const;

private:
    std::map<std::string, Symbol> table;
};

#endif // SYMBOL_TABLE_H