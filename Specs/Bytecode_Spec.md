# File: Bytecode_Spec.md
# Owner: Person A
# Role: Language Architect
# Description: Defines the binary bytecode format for our .vm files.

## File Structure
1. Header (8 bytes)
2. Code Section (variable size)

All multi-byte values are stored in **little-endian** format.

---
### Header (8 bytes)
| Offset | Size | Description |
|---|---|---|
| 0-3 | 4 | Magic Number (`0x5354414B` for "STAK") |
| 4-7 | 4 | Number of instructions |

---
### Opcodes (1 byte each)
| Opcode | Mnemonic |
|---|---|
| `0x01` | `iconst` |
| `0x02` | `iadd` |
| `0x03` | `invoke` |
| `0x04` | `ret` |

---
### Instruction Encoding
- `iconst`: `[opcode (1 byte)] [value (4 bytes)]`
- `iadd`: `[opcode (1 byte)]`
- `invoke`: `[opcode (1 byte)] [function_id (4 bytes)] [num_args (1 byte)]`
- `ret`: `[opcode (1 byte)]`