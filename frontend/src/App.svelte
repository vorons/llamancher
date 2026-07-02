<script lang="ts">
  import { onMount } from 'svelte';
  import { fade } from 'svelte/transition';
  import { toast } from 'svelte-sonner';
  import Toaster from '$lib/ui/sonner/sonner.svelte';
  import { models, serverStatus, serverModel, settings, view, scanning } from '$lib/stores.svelte';
  import { api, onServerStatus } from '$lib/saucer';
  import HeaderBar from '$lib/components/HeaderBar.svelte';
  import ModelList from '$lib/components/ModelList.svelte';
  import ModelDetail from '$lib/components/ModelDetail.svelte';
  import SettingsDialog from '$lib/components/SettingsDialog.svelte';
  import { t, applyLocale } from '$lib/i18n';
  import type { ModelInfo, Settings } from '$lib/types';

  function applyTheme(t: string) {
    document.documentElement.dataset.theme = t === 'light' ? 'light' : 'dark';
  }

  // React to theme changes
  settings.subscribe((s) => applyTheme(s.theme));

  // Prevent double auto-start
  let autoStartDone = false;

  onMount(() => {
    // Load settings
    api.settings().then((kv) => {
      const s: Settings = {
        llama_server_path: kv.llama_server_path,
        models_dir: kv.models_dir,
        auto_start_server: kv.auto_start_server === 'true',
        theme: kv.theme === 'light' ? 'light' : 'dark',
        port: parseInt(kv.port) || 8080,
        api_key: kv.api_key || '',
        last_model: kv.last_model || '',
        locale: (kv.locale as Settings['locale']) || 'auto',
      };
      settings.set(s);
      applyLocale('auto');

      // Auto-start server on boot
      if (s.auto_start_server && s.last_model && !autoStartDone) {
        autoStartDone = true;
        // Wait for initial scan, then start
        const unsub = models.subscribe((m) => {
          if (m.length > 0) {
            unsub();
            const target = m.find((x) => x.name === s.last_model);
            if (target) {
              api.startServer(target.name, target.path).then((r) => {
                if (r === 'ok') {
                  serverStatus.set('starting');
                  serverModel.set(target.display_name || target.name);
                }
              }).catch(() => {});
            }
          }
        });
      }
    }).catch(() => {
      // Running outside saucer — use defaults
    });

    // Initial model scan
    scanModels();

    // Refresh handler
    window.addEventListener('refresh-models', scanModels);

    // Server status push handler
    onServerStatus((status) => {
      serverStatus.set(status as 'stopped' | 'starting' | 'running' | 'error');
      if (status === 'running') {
        toast.success($t('toast.serverRunning'));
      } else if (status === 'stopped') {
        toast.info($t('toast.serverStopped'));
      } else if (status === 'error') {
        // Fetch detailed error message from backend
        api.serverStatus().then(st => {
          toast.error(st.error || $t('toast.serverError'));
        }).catch(() => {
          toast.error($t('toast.serverError'));
        });
      }
    });

    // Poll server status
    const pollInterval = setInterval(async () => {
      try {
        const st = await api.serverStatus();
        serverStatus.set(st.status as 'stopped' | 'starting' | 'running' | 'error');
        serverModel.set(st.model);
      } catch {
        // Ignore
      }
    }, 5000);

    return () => {
      clearInterval(pollInterval);
      window.removeEventListener('refresh-models', scanModels);
    };
  });

  async function scanModels() {
    scanning.set(true);
    try {
      const raw = await api.scanModels();
      models.set(raw as unknown as ModelInfo[]);
    } catch {
      // Mock data for browser dev
      const blankMeta = () => ({
        display_name: '', size_label: '', license: '', author: '', version: '',
        url: '', source_url: '', languages: '', vocab_size: '0',
        embedding_length: '0', feed_forward_length: '0', head_count: '0',
        head_count_kv: '0', expert_count: '0', expert_used_count: '0',
        tokenizer_model: '', bos_token_id: '-1', eos_token_id: '-1',
        chat_template: '', chat_templates: '', has_vision: '', has_audio: '',
      });
      models.set([
        { name: 'llama-3.2-3b-instruct-q4_k_m', path: '', architecture: 'llama', quantization: 'Q4_K_M', size: '1.98 GB', block_count: '28', context_length: '8192', file_type: '15', ...blankMeta() },
        { name: 'mistral-7b-instruct-v0.3-q5_k_m', path: '', architecture: 'mistral', quantization: 'Q5_K_M', size: '4.47 GB', block_count: '32', context_length: '32768', file_type: '17', ...blankMeta() },
        { name: 'phi-3-mini-4k-instruct-q4_0', path: '', architecture: 'phi3', quantization: 'Q4_0', size: '2.23 GB', block_count: '32', context_length: '4096', file_type: '2', ...blankMeta() },
        { name: 'deepseek-coder-6.7b-instruct-q8_0', path: '', architecture: 'deepseek2', quantization: 'Q8_0', size: '6.85 GB', block_count: '30', context_length: '16384', file_type: '10', ...blankMeta() },
      ]);
    } finally {
      scanning.set(false);
    }
  }
</script>

<div class="flex flex-col h-screen bg-background">
  <HeaderBar />
  {#if $view === 'detail'}
    <div in:fade={{ duration: 120 }} class="flex-1 flex flex-col min-h-0">
      <ModelDetail />
    </div>
  {:else}
    <div in:fade={{ duration: 120 }} class="flex-1 flex flex-col min-h-0">
      <ModelList />
    </div>
  {/if}
</div>

<SettingsDialog />

<Toaster position="bottom-right" />
