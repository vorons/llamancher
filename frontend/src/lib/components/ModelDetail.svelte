<script lang="ts">
  import { selectedModel, serverStatus, serverModel } from '$lib/stores.svelte';
  import { Play, Square, Loader2 } from '@lucide/svelte';
  import { api } from '$lib/saucer';
  import { toast } from 'svelte-sonner';
  import { cn } from '$lib/utils';
  import { Separator } from '$lib/ui/separator';
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { Switch } from '$lib/ui/switch';
  import { Slider } from '$lib/ui/slider';
  import type { Preset } from '$lib/types';

  let preset = $state<Preset>({
    ctx_size: 2048,
    threads: 4,
    gpu_layers: 0,
    tensor_split: '',
    temp: 0.7,
    top_k: 40,
    top_p: 0.9,
    mlock: false,
    no_mmap: false,
    flash_attn: false,
  });

  let loadingPreset = $state(true);
  let serverLoading = $state(false);
  let saveTimer: ReturnType<typeof setTimeout> | undefined;

  const model = $derived($selectedModel);

  // Load preset when model changes
  $effect(() => {
    if (model) {
      loadingPreset = true;
      api.loadPreset(model.name).then((kv) => {
        preset = {
          ctx_size: parseInt(kv.ctx_size) || 2048,
          threads: parseInt(kv.threads) || 4,
          gpu_layers: parseInt(kv.gpu_layers) || 0,
          tensor_split: kv.tensor_split || '',
          temp: parseFloat(kv.temp) || 0.7,
          top_k: parseInt(kv.top_k) || 40,
          top_p: parseFloat(kv.top_p) || 0.9,
          mlock: kv.mlock === 'true',
          no_mmap: kv.no_mmap === 'true',
          flash_attn: kv.flash_attn === 'true',
        };
        loadingPreset = false;
      }).catch(() => {
        loadingPreset = false;
      });
    }
  });

  function debouncedSave() {
    if (saveTimer) clearTimeout(saveTimer);
    saveTimer = setTimeout(() => {
      if (model) {
        api.savePreset(model.name, {
          ctx_size: String(preset.ctx_size),
          threads: String(preset.threads),
          gpu_layers: String(preset.gpu_layers),
          tensor_split: preset.tensor_split,
          temp: String(preset.temp),
          top_k: String(preset.top_k),
          top_p: String(preset.top_p),
          mlock: preset.mlock ? 'true' : 'false',
          no_mmap: preset.no_mmap ? 'true' : 'false',
          flash_attn: preset.flash_attn ? 'true' : 'false',
        });
        if ($serverStatus === 'running' && $serverModel === model?.name) {
          toast.info('Changes will apply after restart');
        }
      }
    }, 300);
  }

  async function handlePlayStop() {
    if (!model) return;
    serverLoading = true;

    try {
      const status = await api.serverStatus();
      if (status.status === 'stopped') {
        await api.savePreset(model.name, {
          ctx_size: String(preset.ctx_size),
          threads: String(preset.threads),
          gpu_layers: String(preset.gpu_layers),
          tensor_split: preset.tensor_split,
          temp: String(preset.temp),
          top_k: String(preset.top_k),
          top_p: String(preset.top_p),
          mlock: preset.mlock ? 'true' : 'false',
          no_mmap: preset.no_mmap ? 'true' : 'false',
          flash_attn: preset.flash_attn ? 'true' : 'false',
        });
        await api.startServer(model.name, model.path);
      } else if (status.model === model.name) {
        await api.stopServer();
      } else {
        toast('Another model is running', {
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
    } catch {
      toast.error('Server operation failed');
    } finally {
      serverLoading = false;
    }
  }

  const isActive = $derived(
    model ? $serverModel === model.name && $serverStatus === 'running' : false
  );
  const isStarting = $derived(
    model ? $serverModel === model.name && $serverStatus === 'starting' : false
  );
</script>

{#if model}
  <div class="flex-1 overflow-y-auto p-4 space-y-5">
    {#if loadingPreset}
      <div class="flex items-center justify-center h-32">
        <Loader2 size={24} class="animate-spin text-muted-foreground" />
      </div>
    {:else}
      <!-- Model header -->
      <div class="flex items-center justify-between">
        <div>
          <h2 class="text-lg font-semibold">{model.name}</h2>
          <p class="text-xs text-muted-foreground mt-0.5">{model.quantization} · {model.size}</p>
        </div>
        <button
          class={cn(
            'flex items-center justify-center h-8 w-8 rounded-md transition-colors shrink-0',
            isActive
              ? 'bg-green-500/20 text-green-400 hover:bg-green-500/30'
              : 'bg-secondary hover:bg-accent text-foreground',
          )}
          onclick={handlePlayStop}
          disabled={serverLoading}
          aria-label={isActive ? 'Stop server' : 'Start server'}
        >
          {#if serverLoading || isStarting}
            <Loader2 size={16} class="animate-spin" />
          {:else if isActive}
            <Square size={14} />
          {:else}
            <Play size={14} />
          {/if}
        </button>
      </div>

      <Separator />

      <!-- Parameters -->

      <!-- Basic -->
      <div class="space-y-3">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground">Basic</h3>

        <div class="grid grid-cols-2 gap-3">
          <div class="space-y-1">
            <Label for="ctx_size">Context Size</Label>
            <Input
              id="ctx_size"
              type="number"
              value={String(preset.ctx_size)}
              oninput={(e) => { preset.ctx_size = parseInt(e.currentTarget.value) || 2048; debouncedSave(); }}
            />
          </div>
          <div class="space-y-1">
            <Label for="threads">Threads</Label>
            <Input
              id="threads"
              type="number"
              value={String(preset.threads)}
              oninput={(e) => { preset.threads = parseInt(e.currentTarget.value) || 4; debouncedSave(); }}
            />
          </div>
        </div>
      </div>

      <Separator />

      <!-- Hardware -->
      <div class="space-y-3">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground">Hardware</h3>

        <div class="space-y-2">
          <div class="flex items-center justify-between">
            <Label for="gpu_layers">GPU Layers</Label>
            <span class="text-sm tabular-nums">{preset.gpu_layers}</span>
          </div>
          <Slider
            id="gpu_layers"
            type="single"
            value={preset.gpu_layers}
            onValueChange={(v) => { preset.gpu_layers = v; debouncedSave(); }}
            min={0}
            max={200}
          />
        </div>

        <div class="space-y-1">
          <Label for="tensor_split">Tensor Split</Label>
          <Input
            id="tensor_split"
            placeholder="e.g. 8,8"
            value={preset.tensor_split}
            oninput={(e) => { preset.tensor_split = e.currentTarget.value; debouncedSave(); }}
          />
        </div>
      </div>

      <Separator />

      <!-- Sampling -->
      <div class="space-y-3">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground">Sampling</h3>

        <div class="space-y-2">
          <div class="flex items-center justify-between">
            <Label for="temp">Temperature</Label>
            <span class="text-sm tabular-nums">{preset.temp.toFixed(2)}</span>
          </div>
          <Slider
            id="temp"
            type="single"
            value={preset.temp}
            onValueChange={(v) => { preset.temp = v; debouncedSave(); }}
            min={0}
            max={2}
            step={0.01}
          />
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div class="space-y-1">
            <Label for="top_k">Top K</Label>
            <Input
              id="top_k"
              type="number"
              value={String(preset.top_k)}
              oninput={(e) => { preset.top_k = parseInt(e.currentTarget.value) || 40; debouncedSave(); }}
            />
          </div>
          <div class="space-y-1">
            <div class="flex items-center justify-between">
              <Label for="top_p">Top P</Label>
              <span class="text-xs tabular-nums text-muted-foreground">{preset.top_p.toFixed(2)}</span>
            </div>
            <Slider
              id="top_p"
              type="single"
              value={preset.top_p}
              onValueChange={(v) => { preset.top_p = v; debouncedSave(); }}
              min={0}
              max={1}
              step={0.01}
            />
          </div>
        </div>
      </div>

      <Separator />

      <!-- Flags -->
      <div class="space-y-3">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground">Flags</h3>

        <div class="space-y-3">
          {#each [{ key: 'mlock' as const, label: 'mlock' }, { key: 'no_mmap' as const, label: 'no_mmap' }, { key: 'flash_attn' as const, label: 'flash_attn' }] as flag}
            <div class="flex items-center justify-between">
              <Label for={flag.key}>{flag.label}</Label>
              <Switch
                checked={preset[flag.key]}
                onCheckedChange={(c) => {
                  preset[flag.key] = c;
                  debouncedSave();
                }}
              />
            </div>
          {/each}
        </div>
      </div>
    {/if}
  </div>
{/if}
