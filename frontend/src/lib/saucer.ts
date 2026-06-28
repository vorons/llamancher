// Saucer IPC wrapper — typed access to exposed C++ functions
// Falls back to mock for pure-browser dev (no saucer runtime)

type SaucerWindow = Window & {
  saucer?: {
    exposed: Record<string, (...args: unknown[]) => Promise<unknown>>;
    close: () => void;
    minimize: (v: boolean) => void;
    maximize: (v: boolean) => void;
  };
  __onServerStatus?: (status: string) => void;
};

const w = window as SaucerWindow;

function getApi<T>(name: string): (...args: unknown[]) => Promise<T> {
  const fn = w.saucer?.exposed?.[name];
  if (fn) return fn as (...args: unknown[]) => Promise<T>;

  // Mock fallback for browser dev
  console.warn(`[saucer] ${name} not available — using mock`);
  return async () => {
    throw new Error(`saucer.${name} not available (running outside saucer?)`);
  };
}

export const api = {
  get settings() {
    return getApi<Record<string, string>>('get_settings');
  },
  get updateSetting() {
    return getApi<void>('update_setting');
  },
  get pickFile() {
    return getApi<string>('pick_file');
  },
  get pickFolder() {
    return getApi<string>('pick_folder');
  },
  get scanModels() {
    return getApi<Array<Record<string, string>>>('scan_models');
  },
  get loadPreset() {
    return getApi<Record<string, string>>('load_preset');
  },
  get savePreset() {
    return getApi<void>('save_preset');
  },
  get serverStatus() {
    return getApi<Record<string, string>>('server_status');
  },
  get startServer() {
    return getApi<string>('start_server');
  },
  get stopServer() {
    return getApi<void>('stop_server');
  },
  get setWindowTitle() {
    return getApi<void>('set_window_title');
  },
};

export function onServerStatus(cb: (status: string) => void) {
  w.__onServerStatus = cb;
}

export function isSaucer(): boolean {
  return !!w.saucer;
}
