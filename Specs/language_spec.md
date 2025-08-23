# File: Language_Spec.md
# Owner: Person A
# Role: Language Architect
# Description: Defines the human-readable text format for our stack assembly language (.stkasm).

## Syntax Rules
- Instructions are one per line.
- Comments start with a '#' character.
- Labels are defined by a name followed by a colon, e.g., `main:`.
- Instructions are lowercase (e.g., `iconst`, `iadd`).

## Instruction Set
- `iconst <value>`: Pushes a 32-bit integer constant onto the stack.
  - Example: `iconst 42`
- `iadd`: Pops two integers, adds them, pushes the result.
  - Example: `iadd`
- `invoke <label> <num_args>`: Calls a function.
  - Example: `invoke main 0`
- `ret`: Returns from a function.
  - Example: `ret`