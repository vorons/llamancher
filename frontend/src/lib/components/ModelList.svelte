<script lang="ts">
  import { models, view, selectedModel, serverStatus, serverModel, settings } from '$lib/stores.svelte';
  import ServerButton from '$lib/components/ServerButton.svelte';
  import { api } from '$lib/saucer';
  import { get } from 'svelte/store';
  import { toast } from 'svelte-sonner';
  import { cn } from '$lib/utils';
  import type { ModelInfo } from '$lib/types';

  let loading = $state<string | null>(null); // model name being toggled

  async function handlePlayStop(e: MouseEvent, model: ModelInfo) {
    e.stopPropagation();
    loading = model.name;

    try {
      const status = await api.serverStatus();
      const current = status.status;

      if (current === 'stopped') {
        const s = get(settings);
        if (!s.llama_server_path) {
          toast.error('llama-server executable path is not set. Configure it in Settings.');
          loading = null;
          return;
        }
        // Start server with this model
        const result = await api.startServer(model.name, model.path);
        if (result === 'server_not_stopped') {
          toast.error('Server is already running');
        } else if (result === 'server_not_found') {
          toast.error('llama-server executable not found at ' + s.llama_server_path);
        } else {
          serverModel.set(model.name);
          serverStatus.set('starting');
        }
      } else if (current === 'running' && status.model === model.name) {
        // Stop running model
        await api.stopServer();
        toast.info(`Stopped ${model.name}`);
      } else if (current === 'running') {
        // Another model is running — prompt stop+start
        toast.warning('Another model is running', {
          action: {
            label: 'Stop & Start',
            onClick: async () => {
              await api.stopServer();
              setTimeout(async () => {
                await api.startServer(model.name, model.path);
              }, 500);
            },
          },
        });
      }
    } catch (err) {
      toast.error('Failed to control server');
    } finally {
      loading = null;
    }
  }

  function selectModel(model: ModelInfo) {
    selectedModel.set(model);
    view.set('detail');
  }


</script>

<div class="flex-1 overflow-y-auto p-4">
  {#if $models.length === 0}
    <div class="flex flex-col items-center justify-center h-full text-muted-foreground">
      <div class="text-4xl mb-2">📂</div>
      <p class="text-sm">No GGUF models found</p>
      <p class="text-xs mt-1">Add models to your models directory and click refresh</p>
    </div>
  {:else}
    <div class="space-y-2">
      {#each $models as model (model.name)}
        <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
        <div
          class="flex items-center justify-between p-3 rounded-lg border border-border bg-card hover:bg-accent/50 cursor-pointer transition-colors"
          onclick={() => selectModel(model)}
          onkeydown={(e) => { if (e.key === 'Enter') selectModel(model); }}
          role="button"
          tabindex="0"
        >
          <div class="min-w-0 flex-1">
            <div class="text-sm font-medium truncate">{model.name}</div>
            <div class="text-xs text-muted-foreground mt-0.5">
              {#if model.architecture}
                <span class="font-mono uppercase tracking-tight text-[10px]">{model.architecture}</span>
                <span class="mx-1.5 opacity-40">·</span>
              {/if}
              {model.quantization} · {model.size}
            </div>
          </div>

          <ServerButton
            modelName={model.name}
            serverModelName={$serverModel}
            serverStatus={$serverStatus}
            loading={loading === model.name}
            onclick={(e) => handlePlayStop(e, model)}
            class="ml-3"
          />
        </div>
      {/each}
    </div>
  {/if}
</div>
