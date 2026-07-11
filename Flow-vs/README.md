# Flow VS Code Extension

This extension is stored in the `flow-vscode` folder.
It adds language support, terminal run/debug commands, and a built-in tutorial for beginners.

## What this extension does

- **Run Flow scripts** from VS Code
- **Build the Flow runtime** from `flow.c` using GCC
- **Debug Flow scripts** with the interpreter's built-in debug mode
- **Open the Flow tutorial** right from the command palette

## Commands

- `Flow: Run File` — execute the current `.fl` file in a terminal
- `Flow: Build Flow Runtime` — compile `flow.c` into `flow.exe` with GCC
- `Flow: Debug File` — run the current `.fl` file in debug mode
- `Flow: Open Tutorial` — open the Flow tutorial markdown inside VS Code

## Installation

### Install locally for testing
1. Open the `flow-vscode` folder in VS Code.
2. Run `npm install`.
3. Press `F5` to launch the extension host.

### Install from VSIX
1. Open the `flow-vscode` folder in File Explorer.
2. Run `npx @vscode/vsce package` in the `flow-vscode` folder.
3. In VS Code, press `Ctrl+Shift+P` and choose `Extensions: Install from VSIX...`.
4. Select `flow-language-1.0.0.vsix` from the `flow-vscode` folder.

> Note: The extension source, packaging command, and generated `.vsix` file all live inside `flow-vscode`.

## Usage

Open a `.fl` file and use the command palette to run or debug it.
If you open `flow.c`, use `Flow: Build Flow Runtime` to compile the interpreter.

### Settings
Add these to your user or workspace settings if needed:
```json
{
  "flow.compilerPath": "flow.exe",
  "flow.gccPath": "gcc"
}
```

> If the compiler is not in your PATH, set `flow.compilerPath` to the full path of `flow.exe`.

## How to learn Flow

This extension includes a tutorial file you can open with `Flow: Open Tutorial`.
It explains the language step by step and shows the most useful Flow commands.

## Notes

- `Flow: Debug File` currently launches the interpreter in debug mode inside the terminal.
- Breakpoints can be configured using the interpreter's `FLOW_DEBUG` and `FLOW_BREAKPOINTS` settings.

## Example script

```flow
say "Hello, Flow!"
let x = 5
if x > 3
    say "x is greater than 3"
end
```

## License

MIT
