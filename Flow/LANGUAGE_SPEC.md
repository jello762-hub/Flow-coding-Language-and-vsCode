Flow — Language Specification (Draft)

Purpose
-------
Flow is an English-first, easy-to-read programming language designed for beginners and power users. The goal: write clear, almost-pseudocode programs that are unambiguous for the interpreter and friendly to humans.

Core Principles
---------------
- Readability: prefer natural English phrases (set x to 10) and a concise form (let x = 10).
- Orthogonality: a small set of primitives composes to express complex logic.
- Interactivity: built-in REPL and friendly runtime errors.

Basic Syntax
------------
- Variable assignment (concise):
    let x = 10


- Variable assignment (verbose):

    set x to 10

- Output:

    say "hello"
    display "hello"    # synonym

- Function declaration (concise):

    func add(a, b)
        return a + b
    end

- Function declaration (verbose):

    function add taking a, b
        return a + b
    end

- Conditionals:

    if x > 5
        say "big"
    end

  Also supports if ... then single-statement form:

    if x > 5 then say "big"

- Loops:

    repeat 5 times
        say "hi"
    end

    while x < 10
        let x = x + 1
    end

- For-each over strings (characters):

    for each ch in "abc" do
        say ch
    end

Expressions and Operators
-------------------------
- Arithmetic: + - * / % and multi-word forms like multiplied by and divided by are supported by the lexer.
- Comparison: == != > < >= <= and English forms like is greater than.
- Logical: and, or, not.
- Strings: quoted with " or '. Triple-quoted strings supported.

Functions and Calls
-------------------
- Native functions (built-ins) include say, input, random, len/length, to_number, to_string, and platform drawing helpers (window, rect, text, etc.).
- Call syntax: say("hi") or say "hi" (both work where sensible).

Error Handling and Runtime Behavior
----------------------------------
- The REPL shows friendly errors with token position.
- return inside a function pushes a value to the function stack and exits the function early.

Editor & Tooling Notes
----------------------
- Parser accepts both concise and verbose keywords; lexer maps English phrases to operators.
- For editor support implement an LSP that exposes: completions for native functions, signature help for func declarations, hover docs for built-ins, and diagnostics for syntax errors.

Examples
--------
- Hello:

    say "Hello, world"

- Counter:

    let x = 0
    repeat 5 times
        increase x by 1
        say x
    end

- Function:

    function fib taking n
        if n <= 1 then return n
        return fib(n-1) + fib(n-2)
    end

Next Steps
----------
- Finalize this spec and add a beginner tutorial `flow/README.md` with step-by-step lessons and small examples.
- Improve the parser to accept more natural English variants (for example "if x is greater than 5 then...").

---

This is a draft intended to guide implementation and editor features. Suggestions welcome.
