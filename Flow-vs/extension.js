const vscode = require('vscode');
const path = require('path');

let extensionRoot = null;

function activate(context) {
    extensionRoot = context.extensionPath;
    console.log('Flow language extension is now active!');

    const config = vscode.workspace.getConfiguration('flow');
    const flowPath = config.get('compilerPath', 'flow.exe');

    const runCommand = vscode.commands.registerCommand('flow.runFile', () => runFlow(flowPath, false));
    const compileCommand = vscode.commands.registerCommand('flow.compileFile', () => buildFlow(config, flowPath));
    const debugCommand = vscode.commands.registerCommand('flow.debugFile', () => runFlow(flowPath, true));
    const tutorialCommand = vscode.commands.registerCommand('flow.openTutorial', openTutorial);
    const welcomeCommand = vscode.commands.registerCommand('flow.openWelcome', openWelcomeView);

    context.subscriptions.push(runCommand, compileCommand, debugCommand, tutorialCommand, welcomeCommand);

    const tutorialStatus = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    tutorialStatus.text = '$(book) Flow Tutorial';
    tutorialStatus.command = 'flow.openTutorial';
    tutorialStatus.tooltip = 'Open the Flow tutorial';
    tutorialStatus.show();
    context.subscriptions.push(tutorialStatus);

    const provider = new FlowWelcomeViewProvider(extensionRoot);
    context.subscriptions.push(vscode.window.registerWebviewViewProvider('flowWelcomeView', provider));

    if (!context.globalState.get('flowWelcomeShown')) {
        vscode.window.showInformationMessage('Welcome to Flow! Your welcome panel is opening now.', 'Open Tutorial')
            .then(selection => {
                if (selection === 'Open Tutorial') {
                    openTutorial();
                }
            });
        openWelcome();
        context.globalState.update('flowWelcomeShown', true);
    }

    const keywords = [
        'let','set','make','store','if','then','end','while','for','repeat','times',
        'func','function','return','increase','reduce','say','display','print',
        'and','or','not','true','false','null'
    ];
    const natives = [
        'say','input','length','to_number','to_string','random','abs','floor','ceil','type',
        'window','bg','color','thickness','text','rect','fill_rect','circle','fill_circle','line','refresh','key','mouse_x','mouse_y','mouse_down','sleep','width','height'
    ];

    const completionProvider = vscode.languages.registerCompletionItemProvider('flow', {
        provideCompletionItems(document, position) {
            const items = [];
            for (const keyword of keywords) {
                items.push(new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword));
            }
            for (const native of natives) {
                const item = new vscode.CompletionItem(`${native}()`, vscode.CompletionItemKind.Function);
                item.insertText = new vscode.SnippetString(`${native}($1)`);
                items.push(item);
            }
            return items;
        }
    }, ...'abcdefghijklmnopqrstuvwxyz_'.split(''));
    context.subscriptions.push(completionProvider);

    const hoverProvider = vscode.languages.registerHoverProvider('flow', {
        provideHover(document, position) {
            const wordRange = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
            if (!wordRange) return;
            const word = document.getText(wordRange);
            if (keywords.includes(word)) return new vscode.Hover(`**Flow keyword**: ${word}`);
            if (natives.includes(word)) return new vscode.Hover(`**Native function**: ${word}()`);
            return;
        }
    });
    context.subscriptions.push(hoverProvider);
}

function openWelcomeView() {
    vscode.commands.executeCommand('workbench.view.extension.flowWelcome');
}

function openTutorial() {
    if (!extensionRoot) return;
    const tutorialFile = vscode.Uri.file(path.join(extensionRoot, 'TUTORIAL.md'));
    vscode.commands.executeCommand('markdown.showPreviewToSide', tutorialFile)
        .catch(err => vscode.window.showErrorMessage(`Unable to open tutorial: ${err.message}`));
}

function buildFlow(config, flowPath) {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('Open flow.c or a Flow file first.');
        return;
    }

    const document = editor.document;
    const filePath = document.fileName;
    const ext = path.extname(filePath).toLowerCase();
    const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('Flow');
    terminal.show();

    if (ext === '.c' && path.basename(filePath).toLowerCase() === 'flow.c') {
        const gccPath = config.get('gccPath', 'gcc');
        const outputPath = path.join(path.dirname(filePath), 'flow.exe');
        terminal.sendText(`& "${gccPath}" -o "${outputPath}" "${filePath}" -lm`);
        vscode.window.showInformationMessage('Building Flow runtime from flow.c...');
        return;
    }

    if (ext === '.fl') {
        vscode.window.showInformationMessage('Flow scripts are interpreted by flow.exe. Use Run or Debug to execute them.');
        terminal.sendText(`"${flowPath}" "${filePath}"`);
        return;
    }

    vscode.window.showErrorMessage('Open flow.c to build the interpreter or a Flow script to run/debug it.');
}

function runFlow(flowPath, debug) {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('Open a .fl file first.');
        return;
    }

    const document = editor.document;
    if (document.languageId !== 'flow') {
        vscode.window.showErrorMessage('Not a Flow file!');
        return;
    }

    const filePath = document.fileName;
    const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('Flow');
    terminal.show();
    if (debug) {
        terminal.sendText(`$env:FLOW_DEBUG = '1'; & "${flowPath}" "${filePath}"`);
    } else {
        terminal.sendText(`& "${flowPath}" "${filePath}"`);
    }
}

class FlowWelcomeViewProvider {
    constructor(extensionRoot) {
        this.extensionRoot = extensionRoot;
    }

    resolveWebviewView(webviewView) {
        webviewView.webview.options = {
            enableScripts: true
        };

        webviewView.webview.html = this.getHtml(webviewView.webview);

        webviewView.webview.onDidReceiveMessage(message => {
            switch (message.command) {
                case 'run':
                    vscode.commands.executeCommand('flow.runFile');
                    break;
                case 'debug':
                    vscode.commands.executeCommand('flow.debugFile');
                    break;
                case 'build':
                    vscode.commands.executeCommand('flow.compileFile');
                    break;
                case 'tutorial':
                    vscode.commands.executeCommand('flow.openTutorial');
                    break;
                default:
                    break;
            }
        });
    }

    getHtml(webview) {
        const style = `
            body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; padding: 16px; }
            h1 { margin: 0 0 10px; }
            p { margin: 0 0 14px; line-height: 1.5; }
            button { margin: 4px 4px 4px 0; padding: 8px 12px; border: none; border-radius: 4px; background: #007acc; color: white; cursor: pointer; }
            button.secondary { background: #444; }
            button:hover { opacity: 0.9; }
            .actions { margin-top: 14px; }
        `;

        return `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>${style}</style>
</head>
<body>
    <h1>Flow</h1>
    <p>Welcome to Flow! Use the buttons below to run your current file, build the runtime, or open the tutorial.</p>
    <div class="actions">
        <button onclick="send('run')">Run File</button>
        <button onclick="send('debug')">Debug File</button>
        <button onclick="send('build')">Build Runtime</button>
        <button class="secondary" onclick="send('tutorial')">Open Tutorial</button>
    </div>
    <p>Open a <code>.fl</code> file or <code>flow.c</code> and use the buttons above to get started.</p>
    <script>
        const vscode = acquireVsCodeApi();
        function send(command) { vscode.postMessage({ command }); }
    </script>
</body>
</html>`;
    }
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
