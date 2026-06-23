// MLang Language Support - VS Code client. Launches the mls language server and
// wires it to the editor over LSP (docs/design/24-mls.md). This is the thin
// reference client; the same mls binary serves any LSP editor.
const { workspace, commands, window } = require("vscode");
const { LanguageClient, TransportKind } = require("vscode-languageclient/node");

let client;

function start() {
    const config = workspace.getConfiguration("mlang");
    const serverPath = config.get("server.path", "mls");

    const serverOptions = {
        run: { command: serverPath, args: ["--stdio"], transport: TransportKind.stdio },
        debug: { command: serverPath, args: ["--stdio"], transport: TransportKind.stdio },
    };

    const clientOptions = {
        documentSelector: [{ scheme: "file", language: "mlang" }],
        synchronize: {
            fileEvents: workspace.createFileSystemWatcher("**/*.m"),
        },
    };

    client = new LanguageClient("mlang", "MLang Language Server", serverOptions, clientOptions);
    client.start();
}

function activate(context) {
    start();
    context.subscriptions.push(
        commands.registerCommand("mlang.restartServer", async () => {
            if (client) {
                await client.stop();
            }
            start();
            window.showInformationMessage("MLang language server restarted.");
        })
    );
}

function deactivate() {
    return client ? client.stop() : undefined;
}

module.exports = { activate, deactivate };
