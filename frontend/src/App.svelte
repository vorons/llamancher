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
  import type { ModelInfo } from '$lib/types';

  function applyTheme(t: string) {
    document.documentElement.dataset.theme = t === 'light' ? 'light' : 'dark';
  }

  // React to theme changes
  settings.subscribe((s) => applyTheme(s.theme));

  onMount(() => {
    // Load settings
    api.settings().then((kv) => {
      settings.set({
        llama_server_path: kv.llama_server_path,
        models_dir: kv.models_dir,
        auto_start_server: kv.auto_start_server === 'true',
        theme: kv.theme === 'light' ? 'light' : 'dark',
      });
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
        toast.success('Server is running');
      } else if (status === 'stopped') {
        toast.info('Server stopped');
      } else if (status === 'error') {
        toast.error('Server error');
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
      models.set([
        { name: 'llama-3.2-3b-instruct-q4_k_m', path: '', architecture: 'llama', quantization: 'Q4_K_M', size: '1.98 GB', block_count: '28', context_length: '8192', file_type: '15' },
        { name: 'mistral-7b-instruct-v0.3-q5_k_m', path: '', architecture: 'mistral', quantization: 'Q5_K_M', size: '4.47 GB', block_count: '32', context_length: '32768', file_type: '17' },
        { name: 'phi-3-mini-4k-instruct-q4_0', path: '', architecture: 'phi3', quantization: 'Q4_0', size: '2.23 GB', block_count: '32', context_length: '4096', file_type: '2' },
        { name: 'deepseek-coder-6.7b-instruct-q8_0', path: '', architecture: 'deepseek2', quantization: 'Q8_0', size: '6.85 GB', block_count: '30', context_length: '16384', file_type: '10' },
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
