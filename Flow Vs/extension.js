const vscode = require('vscode');

function activate(context) {
    console.log('Flow language extension is now active!');

    const config = vscode.workspace.getConfiguration('flow');
    const flowPath = config.get('compilerPath', 'flow.exe');

    const runCommand = vscode.commands.registerCommand('flow.runFile', () => runFlow(flowPath, false));
    const compileCommand = vscode.commands.registerCommand('flow.compileFile', () => runFlow(flowPath, false));
    const debugCommand = vscode.commands.registerCommand('flow.debugFile', () => runFlow(flowPath, true));

    context.subscriptions.push(runCommand, compileCommand, debugCommand);

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

function runFlow(flowPath, debug) {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('Open a .fl file first');
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
        terminal.sendText(`$env:FLOW_DEBUG = '1'; "${flowPath}" "${filePath}"`);
    } else {
        terminal.sendText(`"${flowPath}" "${filePath}"`);
    }
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
