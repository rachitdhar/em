# Em Programming Language Specification (Draft)

**Version:** 0.1  
**Status:** Experimental / Subset of C  
**Paradigm:** Imperative, Procedural  
**Syntax Style:** C-like

---

## 1. Overview

**Em** is a small, C-style imperative programming language designed for simplicity and compiler experimentation.  
It currently implements a minimal subset of C, supporting variables, functions, control flow, and expressions, but without type checking or casting.

---

## 2. Lexical Structure

### 2.1 Identifiers

Identifiers represent variable names, function names, and user-defined symbols.

**Rules:**
- Must begin with a letter (`a–z`, `A–Z`) or underscore (`_`)
- May contain letters, digits, and underscores
- Case-sensitive

---

### 2.2 Keywords

#### Control Flow
- if
- else
- for
- while

#### Jumps
- return
- break
- continue

---

### 2.3 Data Types

- void
- bool
- int
- float
- char
- string

> No type checking or casting is currently performed.

---

### 2.4 Literals

- Numeric: `123`, `3.14`
- Boolean: `true`, `false`
- Character: `'a'`
- String: `"hello"`

---

## 3. Operators

### Unary
`! ~ ++ -- * &`

### Binary
Arithmetic: `+ - * / %`  
Comparison: `< > <= >= == !=`  
Logical: `&& ||`  
Bitwise: `| ^ << >>`  
Assignment: `= += -= *= /= %= <<= >>= &= |= ^= &&= ||=`

---

## 4. Delimiters

- `;` statement terminator
- `,` separator

---

## 5. Grouping

- `{ }` blocks
- `( )` expressions

> Indexing (using `[]`) is NOT yet implemented

---

## 6. Preprocessor

```em
#include "module.em"

#define __PI 3.14
#define PI __PI
```

---

## 7. Variables

```em
int x;
int y = 5;
```

Only one variable per declaration.

---

## 8. Functions

```em
int add(int a, int b) {
    return a + b;
}
```

Prototypes allowed.

---

## 9. Control Flow

```em
if (x > 0) { }
while (x > 0) { }
for (int i = 0; i < 10; i++) { }
```

---

## 10. Unsupported Features

- Arrays / Pointers
- Type casting
- Structs / enums
- Multiple declarations
- Switch statements
