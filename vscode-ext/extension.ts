import * as path from 'path';
import * as fs from 'fs';
import { workspace, extensions, commands, ExtensionContext } from 'vscode';
import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions
} from 'vscode-languageclient/node';

let client: LanguageClient;

async function setupMaterialIconTheme(context: ExtensionContext): Promise<void>
{
	const materialIconExt = extensions.getExtension('PKief.material-icon-theme');
	if (!materialIconExt) {
		console.log('Material Icon Theme not installed, skipping icon setup.');
		return;
	}

	const vscodeExtensionsDir = path.join(
		process.env.HOME || process.env.USERPROFILE || '~',
		'.vscode', 'extensions', 'icons'
	);

	if (!fs.existsSync(vscodeExtensionsDir)) {
		fs.mkdirSync(vscodeExtensionsDir, { recursive: true });
	}

	const iconSource = path.join(context.extensionPath, 'icons', 'tdr-dark.svg');
	const iconDest = path.join(vscodeExtensionsDir, 'tdr.svg');

	const iconSourceLight = path.join(context.extensionPath, 'icons', 'tdr-light.svg');
	const iconDestLight = path.join(vscodeExtensionsDir, 'tdr_light.svg');

	// Check if icons already exist and are up-to-date
	let iconsCopied = false;
	try {
		const sourceContent = fs.readFileSync(iconSource);
		const sourceLightContent = fs.readFileSync(iconSourceLight);

		let needsCopy = true;
		if (fs.existsSync(iconDest) && fs.existsSync(iconDestLight)) {
			const destContent = fs.readFileSync(iconDest);
			const destLightContent = fs.readFileSync(iconDestLight);
			if (sourceContent.equals(destContent) && sourceLightContent.equals(destLightContent)) {
				needsCopy = false;
			}
		}

		if (needsCopy) {
			fs.copyFileSync(iconSource, iconDest);
			fs.copyFileSync(iconSourceLight, iconDestLight);
			iconsCopied = true;
			console.log('TDR icons copied to', vscodeExtensionsDir);
		} else {
			console.log('TDR icons already up-to-date in', vscodeExtensionsDir);
		}
	} catch (e) {
		console.error('Failed to copy TDR icon:', e);
		return;
	}

	const config = workspace.getConfiguration('material-icon-theme');
	const fileAssociations = config.get<Record<string, string>>('files.associations') || {};

	const iconRelativePath = '../../icons/tdr';

	let needsUpdate = false;
	for (const ext of ['*.tdr', '*.paf', '*.traceurs-de-rayons']) {
		if (fileAssociations[ext] !== iconRelativePath) {
			fileAssociations[ext] = iconRelativePath;
			needsUpdate = true;
		}
	}

	if (needsUpdate) {
		await config.update('files.associations', fileAssociations, true);
		console.log('Material Icon Theme file associations configured for TDR files.');
	}

	// If icons were copied or settings changed, activate Material Icon Theme
	// to force it to regenerate its icon definitions
	if (iconsCopied || needsUpdate) {
		try {
			await commands.executeCommand('material-icon-theme.activateIcons');
			console.log('Material Icon Theme icons regenerated.');
		} catch (e) {
			console.log('Could not trigger Material Icon Theme regeneration:', e);
		}
	}

	// Wait a bit for Material Icon Theme to finish regenerating
	await new Promise(resolve => setTimeout(resolve, 1000));

	// Patch material-icons.json to add light theme icon support
	// (Material Icon Theme doesn't natively support light variants for custom SVGs)
	patchMaterialIconsForLightTheme(materialIconExt);
}

function patchMaterialIconsForLightTheme(materialIconExt: ReturnType<typeof extensions.getExtension>): void {
	if (!materialIconExt) return;

	const materialIconsJsonPath = path.join(materialIconExt.extensionPath, 'dist', 'material-icons.json');
	try {
		const content = fs.readFileSync(materialIconsJsonPath, 'utf-8');
		const json = JSON.parse(content);

		let patched = false;

		// Add light icon definition if missing
		const lightDefKey = '../../icons/tdr_light';
		if (!json.iconDefinitions?.[lightDefKey]) {
			json.iconDefinitions[lightDefKey] = {
				iconPath: './../icons/../../icons/tdr_light.svg'
			};
			patched = true;
		}

		// Add light file extension mappings if missing
		if (!json.light) json.light = {};
		if (!json.light.fileExtensions) json.light.fileExtensions = {};
		for (const ext of ['tdr', 'paf', 'traceurs-de-rayons']) {
			if (json.light.fileExtensions[ext] !== lightDefKey) {
				json.light.fileExtensions[ext] = lightDefKey;
				patched = true;
			}
		}

		if (patched) {
			fs.writeFileSync(materialIconsJsonPath, JSON.stringify(json));
			console.log('Patched material-icons.json for light theme TDR icons.');
		} else {
			console.log('material-icons.json already patched for light theme.');
		}
	} catch (e) {
		console.error('Failed to patch material-icons.json for light theme:', e);
	}
}

export function activate(context: ExtensionContext)
{
	console.log('Scene Language Extension is activating...');

	// Setup Material Icon Theme integration (runs async, non-blocking)
	setupMaterialIconTheme(context).catch(e => {
		console.error('Error setting up Material Icon Theme:', e);
	});

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