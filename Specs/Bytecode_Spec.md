# File: Bytecode_Spec.md
# Description: Defines the binary bytecode format for our .vm files.

## 1. File Structure

A `.vm` file consists of two main sections:

1. **Header (8 bytes)** – Metadata about the bytecode file.  
2. **Code Section (variable size)** – Encoded instructions (opcodes + operands).

**All multi-byte values are stored in little-endian format.**

---

## 2. Header (8 bytes)

| Offset | Size | Description |
|--------|------|-------------|
| 0-3    | 4 B  | Magic Number (`0x5354414B` → "STAK") |
| 4-7    | 4 B  | Number of instructions in code section |

**Example:**
- `4B 41 54 53` = "STAK"  
- `0A 00 00 00` = 10 instructions (little-endian)

---

## 3. Opcodes (1 byte each)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x01   | iconst   | Push 32-bit integer onto stack |
| 0x02   | iadd     | Pop two integers, push sum |
| 0x03   | invoke   | Call function |
| 0x04   | ret      | Return from function |
| 0x05   | isub     | Pop two integers, push difference |
| 0x06   | imul     | Pop two integers, push product |
| 0x07   | idiv     | Pop two integers, push quotient |
| 0x08   | iand     | Pop two integers, push bitwise AND |
| 0x09   | ior      | Pop two integers, push bitwise OR |
| 0x0A   | ixor     | Pop two integers, push bitwise XOR |
| 0x0B   | inot     | Pop integer, push bitwise NOT |

> Additional opcodes can be assigned for future instructions.

---

## 4. Instruction Encoding

### 4.1 `iconst <value>`
- Bytes: `[opcode (1 B)] [value (4 B)]`  
- Value: 32-bit signed integer (little-endian)

**Example:** Push 42 onto stack

---

### 4.2 `iadd`
- Bytes: `[opcode (1 B)]`  
- Pops two integers from the stack, pushes their sum.


---

### 4.3 `invoke <function_id> <num_args>`
- Bytes: `[opcode (1 B)] [function_id (4 B)] [num_args (1 B)]`  
- `function_id`: Unique numeric identifier for function  
- `num_args`: Number of arguments to pop from stack

**Example:** Call function ID 3 with 2 arguments

---

### 4.4 `ret`
- Bytes: `[opcode (1 B)]`  
- Pops return address from call stack and resumes execution.

---

### 4.5 Arithmetic & Logical Instructions
| Instruction | Bytes | Description |
|-------------|-------|-------------|
| `isub`      | 0x05  | Pop two integers, push difference |
| `imul`      | 0x06  | Pop two integers, push product |
| `idiv`      | 0x07  | Pop two integers, push quotient |
| `iand`      | 0x08  | Pop two integers, push bitwise AND |
| `ior`       | 0x09  | Pop two integers, push bitwise OR |
| `ixor`      | 0x0A  | Pop two integers, push bitwise XOR |
| `inot`      | 0x0B  | Pop one integer, push bitwise NOT |

---

## 5. Optional Function Table

For more complex VM implementations, `.vm` files may include a **function table**:

| Function ID | Label / Name Offset | Number of Instructions |
|-------------|-------------------|-----------------------|

- `invoke` instructions use function IDs to resolve target functions.  
- Ensures compact references instead of storing label strings in the code.

---

## 6. Example `.vm` Bytecode

Assembly (`.stkasm`):
```asm
iconst 10
iconst 20
iadd
ret
Header: 4B 41 54 53 04 00 00 00  # "STAK", 4 instructions

