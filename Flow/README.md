# Flow Programming Language

A completely original, human-readable programming language with its own compiler written from scratch in C.

## Philosophy

Code should read like English instructions. Flow uses natural language syntax
while being a fully-featured, complex programming language.

## Building

```bash
gcc -o flow flow.c -lm
```

## Running

```bash
./flow program.fl

REPL mode (interactive):
```

```bash
./flow        # starts interactive REPL
```
```

## Language Features

### Variables
```
let x = 5
let name = "Alice"
let x += 1
let x -= 1
```

### Output
```
say "Hello, World!"
say x
```

### Conditionals
```
if x > 10
    say "big"
end

if x < 5
    say "small"
end

if x == 0
    say "zero"
end

# One-line if
if x > 10 then say "big" end
```

### Loops
```
let i = 0
while i < 10
    say i
    let i += 1
end

repeat 5 times
    say "hello"
end
```

### Functions
```
func greet(name)
    say "Hello, " + name + "!"
end

func add(a, b)
    return a + b
end

greet("Alice")
say add(3, 4)
```

### Recursion
```
func factorial(n)
    if n == 0
        return 1
    end
    return n * factorial(n - 1)
end

say factorial(5)
```

### Operators
- `+` — addition
- `-` — subtraction
- `*` — multiplication
- `/` — division
- `%` — modulo
- `==` — equals
- `!=` — not equals
- `>` — greater than
- `<` — less than
- `>=` — greater than or equal
- `<=` — less than or equal
- `and` / `or` / `not` — boolean logic

### Verbose Syntax (also supported)
```
set x to 5
make name be "Alice"
display "Hello"
increase x by 1
reduce x by 1
function add taking a, b
    return a plus b
end
if x is greater than 10 then
    display "big"
end
```

## Quick Reference

```
func fibonacci(n)
    if n < 2
        return n
    end
    return fibonacci(n - 1) + fibonacci(n - 2)
end

let i = 0
while i < 20
    say fibonacci(i)
    let i += 1
end
```

## VS Code Extension

Install the `flow-language` extension for syntax highlighting and one-click running:
- `Ctrl+F5` — Run current file
- `Ctrl+Shift+B` — Compile to .exe
