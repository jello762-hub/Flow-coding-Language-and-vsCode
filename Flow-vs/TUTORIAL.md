# Flow Language Tutorial

This tutorial file lives in the `flow-vscode` folder as `TUTORIAL.md`.

Welcome to Flow! This tutorial shows you how to write Flow scripts, run them in VS Code, and use the built-in tutorial command.

## 1. Open the tutorial

Use the command palette:
- `Ctrl+Shift+P`
- type `Flow: Open Tutorial`
- press Enter

You can also open the `Flow Welcome` panel from the Activity Bar and click **Open Tutorial**.

## 2. Run a Flow file

Open a `.fl` file in VS Code, then run:
- `Flow: Run File`

This opens a terminal and executes the current file with `flow.exe`.

## 3. Debug a Flow file

Open a `.fl` file in VS Code, then run:
- `Flow: Debug File`

This starts the Flow interpreter in debug mode inside the terminal.

## 4. Build the Flow runtime

Open `flow.c` in VS Code, then run:
- `Flow: Build Flow Runtime`

This compiles `flow.c` into `flow.exe` using your configured GCC path.

If `flow.exe` is not in your PATH, set `flow.compilerPath` in your VS Code settings.

If you are in the `flow-vscode` folder already, the build command will write `flow.exe` next to `flow.c`.

> Note: The VSIX package also gets generated in `flow-vscode` when you package the extension.

## 5. Basic Flow syntax

### Hello world

```fl
say "Hello, Flow!"
```

### Variables

```fl
let x = 10
say x
```

### Conditionals

```fl
if x > 5
    say "x is bigger than 5"
end
```

### Loops

```fl
repeat 3 times
    say "looping"
end
```

### Functions

```fl
func add(a, b)
    return a + b
end

say add(2, 3)
```

## 6. Debugger commands

When the Flow interpreter runs in debug mode, use these commands in the terminal:

The terminal is opened automatically by the extension when you run `Flow: Debug File`.

- `s` or `step` — execute the next statement
- `c` or `continue` — continue until the next breakpoint
- `p` or `print` — print current variables
- `b <token>` — add a breakpoint on the next matching token
- `bl` — list breakpoints
- `db <id>` — delete a breakpoint by index
- `q` or `quit` — exit the debugger

## 7. Settings

Set your paths in VS Code settings if needed:

```json
{
  "flow.compilerPath": "flow.exe",
  "flow.gccPath": "gcc"
}
```

## 8. Notes

- The extension supports syntax highlighting, completions, and hover help for Flow.
- The tutorial is built into the extension and can be opened anytime with `Flow: Open Tutorial`.
- If `flow.exe` is not in your PATH, set `flow.compilerPath` to the correct location.

## 9. Want more?

Open the `flow/` folder to explore the language runtime and examples.
