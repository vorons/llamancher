<script lang="ts">
  import { settings, settingsOpen } from '$lib/stores.svelte';
  import { api } from '$lib/saucer';
  import * as Dialog from '$lib/ui/dialog';
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { Switch } from '$lib/ui/switch';
  import { Separator } from '$lib/ui/separator';
  import { Button } from '$lib/ui/button';

  // Local copies for editing
  let llamaServerPath = $state('');
  let modelsDir = $state('');
  let autoStart = $state(false);
  let isLight = $state(false);
  let port = $state('8080');
  let apiKey = $state('');

  // Sync from store when dialog opens
  $effect(() => {
    if ($settingsOpen) {
      llamaServerPath = $settings.llama_server_path;
      modelsDir = $settings.models_dir;
      autoStart = $settings.auto_start_server;
      isLight = $settings.theme === 'light';
      port = String($settings.port);
      apiKey = $settings.api_key || '';
    }
  });

  function saveSetting(key: string, value: string) {
    api.updateSetting(key, value).catch(() => {});
    settings.update((s) => {
      if (key === 'llama_server_path') s.llama_server_path = value;
      else if (key === 'models_dir') s.models_dir = value;
      else if (key === 'auto_start_server') s.auto_start_server = value === 'true';
      else if (key === 'theme') s.theme = value as 'dark' | 'light';
      else if (key === 'port') s.port = parseInt(value) || 8080;
      else if (key === 'api_key') s.api_key = value;
      return s;
    });
  }
</script>

<Dialog.Root bind:open={$settingsOpen}>
  <Dialog.Content>
    <Dialog.Header>
      <Dialog.Title>Settings</Dialog.Title>
      <Dialog.Description>Configure llama-server and model paths</Dialog.Description>
    </Dialog.Header>
    <div class="space-y-4">
      <div class="space-y-1">
        <Label for="llama_server_path">llama-server path</Label>
        <div class="flex gap-2">
          <Input
            id="llama_server_path"
            placeholder="/usr/local/bin/llama-server"
            value={llamaServerPath}
            oninput={(e) => {
              llamaServerPath = e.currentTarget.value;
              saveSetting('llama_server_path', e.currentTarget.value);
            }}
            class="flex-1"
          />
          <Button
            variant="outline"
            onclick={async () => {
              const path = await api.pickFile();
              if (path) {
                llamaServerPath = path;
                saveSetting('llama_server_path', path);
              }
            }}
          >
            Browse
          </Button>
        </div>
        <p class="text-xs text-muted-foreground mt-1">
          Path to the llama-server executable
        </p>
      </div>

      <div class="space-y-1">
        <Label for="models_dir">Models directory</Label>
        <div class="flex gap-2">
          <Input
            id="models_dir"
            placeholder="~/.llamancher/models"
            value={modelsDir}
            oninput={(e) => {
              modelsDir = e.currentTarget.value;
              saveSetting('models_dir', e.currentTarget.value);
            }}
            class="flex-1"
          />
          <Button
            variant="outline"
            onclick={async () => {
              const path = await api.pickFolder();
              if (path) {
                modelsDir = path;
                saveSetting('models_dir', path);
              }
            }}
          >
            Browse
          </Button>
        </div>
        <p class="text-xs text-muted-foreground mt-1">
          Directory containing GGUF model files
        </p>
      </div>

      <Separator />

      <div class="flex items-center justify-between">
        <div>
          <Label for="port">Server port</Label>
          <p class="text-xs text-muted-foreground">
            Port for llama-server (default 8080)
          </p>
        </div>
        <Input
          id="port"
          type="number"
          min="1"
          max="65535"
          class="w-24"
          value={port}
          oninput={(e) => {
            port = e.currentTarget.value;
            saveSetting('port', port);
          }}
        />
      </div>

      <div class="space-y-1">
        <Label for="api_key">API key</Label>
        <Input
          id="api_key"
          type="password"
          placeholder="sk-..."
          value={apiKey}
          oninput={(e) => {
            apiKey = e.currentTarget.value;
            saveSetting('api_key', apiKey);
          }}
          class="w-full"
        />
        <p class="text-xs text-muted-foreground mt-1">
          Optional API key for securing the server (--api-key)
        </p>
      </div>

      <Separator />

      <div class="flex items-center justify-between">
        <div>
          <Label for="auto_start">Auto-start server</Label>
          <p class="text-xs text-muted-foreground">
            Start last used model automatically when the app launches
          </p>
        </div>
        <Switch
          id="auto_start"
          checked={autoStart}
          onCheckedChange={(c) => {
            autoStart = c;
            saveSetting('auto_start_server', String(c));
          }}
        />
      </div>

      <Separator />

      <div class="flex items-center justify-between">
        <div>
          <Label for="theme">Light theme</Label>
          <p class="text-xs text-muted-foreground">
            Switch between dark and light appearance
          </p>
        </div>
        <Switch
          id="theme"
          checked={isLight}
          onCheckedChange={(c) => {
            isLight = c;
            saveSetting('theme', c ? 'light' : 'dark');
          }}
        />
      </div>
    </div>
  </Dialog.Content>
</Dialog.Root>
