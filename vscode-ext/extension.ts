import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';
import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext)
{
	console.log('Scene Language Extension is activating...');

	const platform = process.platform;
	const arch = process.arch;
	const binaryName = platform === 'win32' ? 'tdr-lsp.exe' : 'tdr-lsp';
	const serverExecutable = path.join(
		context.extensionPath, 
		'bin', 
		`${platform}-${arch}`,
		binaryName
	);
	// const serverExecutable = path.join(__dirname, './bin/linux-x64');
	console.log('LSP server path:', serverExecutable);
	
	const serverOptions: ServerOptions = {
		command: serverExecutable,
		args: []
	};
	
	const clientOptions: LanguageClientOptions = {
		documentSelector: [
			{ scheme: 'file', language: 'tdr' }
		],
		synchronize: {
			fileEvents: workspace.createFileSystemWatcher('**/*.{tdr,traceurs-de-rayons,paf}')
		}
	};
	
	client = new LanguageClient(
		'sceneLanguageServer',
		'Scene Language Server',
		serverOptions,
		clientOptions
	);
	
	client.start();
	console.log('Scene Language Server started!');
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}