<script lang="ts">
  import { models, view, selectedModel, serverStatus, serverModel } from '$lib/stores.svelte';
  import { Play, Square, Loader2 } from '@lucide/svelte';
  import { api } from '$lib/saucer';
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
        // Start server with this model
        const result = await api.startServer(model.name, model.path);
        if (result === 'server_not_stopped') {
          toast.error('Server is already running');
        }
      } else if (current === 'running' && status.model === model.name) {
        // Stop running model
        await api.stopServer();
        toast.info(`Stopped ${model.name}`);
      } else if (current === 'running') {
        // Another model is running — prompt stop+start
        toast('Another model is running', {
          action: {
            label: 'Stop & Start',
            onClick: async () => {
              await api.stopServer();
              // Tiny delay, then start
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

  const isRunningModel = $derived(
    (name: string) => $serverModel === name && $serverStatus === 'running'
  );
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

          <button
            class={cn(
              'flex items-center justify-center h-8 w-8 rounded-md transition-colors shrink-0 ml-3',
              isRunningModel(model.name)
                ? 'bg-green-500/20 text-green-400 hover:bg-green-500/30'
                : 'bg-secondary hover:bg-accent text-foreground',
            )}
            onclick={(e) => handlePlayStop(e, model)}
            disabled={loading === model.name}
          >
            {#if loading === model.name}
              <Loader2 size={16} class="animate-spin" />
            {:else if isRunningModel(model.name)}
              <Square size={14} />
            {:else}
              <Play size={14} />
            {/if}
          </button>
        </div>
      {/each}
    </div>
  {/if}
</div>
