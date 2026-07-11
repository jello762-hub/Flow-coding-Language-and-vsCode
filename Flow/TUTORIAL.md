# Flow — Quick Tutorial

This short tutorial introduces the basics of Flow so you can get started quickly.

1) REPL

Start the REPL (no filename):

    ./flow

Commands in REPL:

- `:quit` — exit
- `:help` — show help
- `:vars` — list variables
- `:funcs` — list functions

2) Hello World

    say "Hello, world"

3) Variables

Concise:

    let x = 10

Verbose (also supported):

    set x to 10

Modify:

    increase x by 1
    reduce x by 2

4) Functions

    func add(a, b)
        return a + b
    end

Call: `say add(2, 3)`

5) Conditionals

Single-line:

    if x > 5 then say "big"

Block form:

    if x > 5
        say "big"
    end

6) Loops

Repeat N times:

    repeat 5 times
        say "hi"
    end

While:

    while x < 10
        increase x by 1
    end

For-each (string chars):

    for each ch in "abc" do
        say ch
    end

7) Examples

See the `examples/` folder for ready-to-run samples. To run an example file:

    ./flow examples/hello.fl

8) Next steps

- Read `LANGUAGE_SPEC.md` for more details on syntax and built-ins.
- If you want, I can convert these into a web-based interactive tutorial or integrate step-by-step lessons into the VS Code extension.
