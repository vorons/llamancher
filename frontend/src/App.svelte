<script lang="ts">
  import { onMount } from 'svelte';
  import { Toaster, toast } from 'svelte-sonner';
  import { models, serverStatus, serverModel, settings, view } from '$lib/stores.svelte';
  import { api, onServerStatus } from '$lib/saucer';
  import HeaderBar from '$lib/components/HeaderBar.svelte';
  import ModelList from '$lib/components/ModelList.svelte';
  import ModelDetail from '$lib/components/ModelDetail.svelte';
  import SettingsDialog from '$lib/components/SettingsDialog.svelte';
  import type { ModelInfo } from '$lib/types';

  onMount(async () => {
    // Load settings
    try {
      const kv = await api.settings();
      settings.set({
        llama_server_path: kv.llama_server_path,
        models_dir: kv.models_dir,
        auto_start_server: kv.auto_start_server === 'true',
      });
    } catch {
      // Running outside saucer — use defaults
    }

    // Initial model scan
    await scanModels();

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
    try {
      const raw = await api.scanModels();
      models.set(raw as unknown as ModelInfo[]);
    } catch {
      // Mock data for browser dev
      models.set([
        { name: 'llama-3.2-3b-instruct-q4_k_m', path: '', quantization: 'Q4_K_M', size: '1.98 GB' },
        { name: 'mistral-7b-instruct-v0.3-q5_k_m', path: '', quantization: 'Q5_K_M', size: '4.47 GB' },
        { name: 'phi-3-mini-4k-instruct-q4_0', path: '', quantization: 'Q4_0', size: '2.23 GB' },
        { name: 'deepseek-coder-6.7b-instruct-q8_0', path: '', quantization: 'Q8_0', size: '6.85 GB' },
      ]);
    }
  }
</script>

<div class="flex flex-col h-screen bg-background">
  <HeaderBar />
  {#if $view === 'detail'}
    <ModelDetail />
  {:else}
    <ModelList />
  {/if}
</div>

<SettingsDialog />
<Toaster
  position="bottom-right"
  toastOptions={{
    style: 'background: hsl(0 0% 7%); color: hsl(0 0% 98%); border: 1px solid hsl(0 0% 14.9%);',
  }}
/>
