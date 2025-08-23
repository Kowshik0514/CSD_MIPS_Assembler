# Language_Spec.md  
**Description:** Defines the human-readable text format for our stack assembly language (`.stkasm`).  

---

## 1. Overview  
The `.stkasm` language is a **stack-based assembly language** designed to represent low-level instructions in a human-readable format. It abstracts machine code into a simpler instruction set that operates primarily on a **stack data structure**, making it well-suited for implementing interpreters, virtual machines, and educational compilers.  

Each program is composed of **instructions, labels, and comments**. Execution flows sequentially unless altered by a control instruction such as `invoke` or `ret`.  

---

## 2. Syntax Rules  

1. **Instructions**  
   - One instruction per line.  
   - Case-sensitive: instructions must be lowercase.  
   - Operands are separated by spaces.  
   - Example:  
     ```asm
     iconst 42
     iadd
     ```

2. **Labels**  
   - Defined by an identifier followed by a colon `:`.  
   - Used as jump or function entry points.  
   - Example:  
     ```asm
     main:
       iconst 10
       ret
     ```

3. **Comments**  
   - Start with `#` and extend to the end of the line.  
   - Ignored by the assembler.  
   - Example:  
     ```asm
     iconst 5   # Push integer 5 onto stack
     ```

4. **File Extension**  
   - Programs must be saved with the `.stkasm` extension.  

---

## 3. Instruction Set  

### 3.1 Data Instructions  

- **`iconst <value>`**  
  Pushes a **32-bit signed integer constant** onto the stack.  
  - Operand: `<value>` = integer literal (e.g., `42`, `-5`).  
  - Example:  
    ```asm
    iconst 100   # Push 100 onto stack
    ```

- **`move` (pseudo-instruction)**  
  Copies the top value of the stack without removing it (similar to stack duplication).  
  - Example:  
    ```asm
    iconst 10
    move        # Stack: [10, 10]
    ```

---

### 3.2 Arithmetic Instructions  

- **`iadd`**  
  Pops the top two integers, adds them, pushes the result.  
  - Example:  
    ```asm
    iconst 5
    iconst 7
    iadd        # Result: 12
    ```

- **`isub`**  
  Pops the top two integers, subtracts (`second - top`), pushes the result.  
  - Example:  
    ```asm
    iconst 10
    iconst 4
    isub        # Result: 6
    ```

- **`imul`**  
  Pops the top two integers, multiplies them, pushes the result.  
  - Example:  
    ```asm
    iconst 3
    iconst 6
    imul        # Result: 18
    ```

- **`idiv`**  
  Pops the top two integers, divides (`second / top`), pushes the result.  
  - Example:  
    ```asm
    iconst 20
    iconst 5
    idiv        # Result: 4
    ```

---

### 3.3 Logical Instructions  

- **`iand`**  
  Pops the top two integers, performs bitwise AND, pushes result.  

- **`ior`**  
  Pops two integers, bitwise OR.  

- **`ixor`**  
  Pops two integers, bitwise XOR.  

- **`inot`**  
  Pops top integer, pushes bitwise NOT.  

---

### 3.4 Control Flow Instructions  

- **`invoke <label> <num_args>`**  
  Calls a function at the given label, passing the specified number of arguments.  
  - Operands:  
    - `<label>`: function entry point  
    - `<num_args>`: number of arguments to pop from the stack  
  - Example:  
    ```asm
    invoke main 0
    ```

- **`ret`**  
  Returns from the current function.  
  - Example:  
    ```asm
    ret
    ```

---

## 4. Example Program  

```asm
# Simple addition function in .stkasm

main:
  iconst 5        # Push 5
  iconst 7        # Push 7
  iadd            # Add -> result = 12
  ret             # Return result
