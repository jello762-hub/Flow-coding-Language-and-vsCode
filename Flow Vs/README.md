# Flow VS Code Extension (minimal)

This extension adds three commands to VS Code: `Flow: Compile`, `Flow: Run`, and `Flow: Debug`.

How it works
------------
- `Run` and `Compile` execute the `flow` binary with the current open file.
- `Debug` runs `flow --debug <file>` which enables the interpreter's simple interactive debugger.

To install locally
------------------
1. In VS Code open the `flow-vscode` folder.
2. Run `npm install` (optional for packaging).
3. Press F5 to launch the extension host for testing.
# Flow Language Support for VS Code

Syntax highlighting and language support for the [Flow Programming Language](https://github.com/flow-lang).

## Features

- **Syntax Highlighting** — Full color highlighting for all Flow keywords, operators, strings, numbers, and comments
- **Auto-indentation** — Automatic indentation for `function`, `if`, `while`, `for` blocks
- **Bracket Matching** — Automatic closing of `()`, `[]`, `""`, `''`
- **Code Folding** — Fold blocks between `function`/`if`/`while`/`for` and `end`
- **Run Command** — Run `.fl` files directly from VS Code (requires Flow compiler in PATH)
- **Compile Command** — Compile `.fl` files to executables using GCC

## Installation

### From VSIX (recommended)
1. Open VS Code
2. Press `Ctrl+Shift+P` and select "Extensions: Install from VSIX..."
3. Navigate to this folder and select the `.vsix` file (build it first)

### Manual Installation
1. Copy this folder to your VS Code extensions directory:
   - Windows: `%USERPROFILE%\.vscode\extensions\flow-language`
   - macOS: `~/.vscode/extensions/flow-language`
   - Linux: `~/.vscode/extensions/flow-language`
2. Restart VS Code

### Building the VSIX
```bash
npm install -g @vscode/vsce
cd flow-vscode
vsce package
```

## Usage

Open any `.fl` file in VS Code and you'll get full syntax highlighting.

### Keyboard Shortcuts
- `Ctrl+F5` — Run the current Flow file
- `Ctrl+Shift+B` — Compile the current Flow file (requires GCC)

### Commands
- **Flow: Run File** — Execute the current `.fl` file
- **Flow: Compile File** — Compile to executable using GCC
- **Flow: Debug File** — Run the current `.fl` file in the Flow debugger (uses `FLOW_DEBUG=1`)

## Debugging
Use the `Flow: Debug File` command to start the interpreter in debug mode.
The Flow debugger supports these interactive commands:
- `s` / `step` — execute the next statement
- `c` / `continue` — continue until the next breakpoint
- `p` / `print` — print current variables
- `b <token>` — add a breakpoint on the next token matching `<token>`
- `bl` — list breakpoints
- `db <id>` — delete breakpoint by index
- `q` / `quit` — exit the debugger

Set breakpoints before running by using `FLOW_BREAKPOINTS`, e.g.:
```powershell
$env:FLOW_DEBUG = '1'
$env:FLOW_BREAKPOINTS = 'say'
& .\flow.exe .\examples\hello.fl
```

## Configuration

Add to your VS Code settings:
```json
{
  "flow.compilerPath": "C:\\path\\to\\flow.exe"
}
```

## Example

```flow
program hello
    function fibonacci taking n
        if n is less than 2 then
            return n
        end
        return fibonacci(n minus 1) plus fibonacci(n minus 2)
    end

    set i to 0
    while i is less than 20
        display fibonacci(i)
        increase i by 1
    end
end
```

## License

MIT
