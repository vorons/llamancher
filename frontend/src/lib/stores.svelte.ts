import { writable } from 'svelte/store';
import type { ModelInfo, ServerStatus, Settings } from './types';

export const models = writable<ModelInfo[]>([]);
export const selectedModel = writable<ModelInfo | null>(null);
export const serverStatus = writable<ServerStatus>('stopped');
export const serverModel = writable<string>('');
export const settings = writable<Settings>({
  llama_server_path: 'llama-server',
  models_dir: '',
  auto_start_server: false,
});
export const settingsOpen = writable(false);

// View state
export const view = writable<'list' | 'detail'>('list');
