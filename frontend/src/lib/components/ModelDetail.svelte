<script lang="ts">
  import { selectedModel, serverStatus, serverModel } from '$lib/stores.svelte';
  import { Play, Square, Loader2, ChevronDown } from '@lucide/svelte';
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
    numa: '',
    split_mode: '',
    main_gpu: -1,
    device: '',
    mlock: false,
    no_mmap: false,
    batch_size: 2048,
    ubatch_size: 512,
    cache_type_k: '',
    cache_type_v: '',
    flash_attn: false,
    defrag_thold: -1,
    samplers: '',
    seed: -1,
    temp: 0.80,
    top_k: 40,
    top_p: 0.95,
    min_p: 0.05,
    repeat_penalty: 1.00,
    presence_penalty: 0.00,
    frequency_penalty: 0.00,
    mirostat: 0,
    parallel: 1,
    no_repack: false,
    verbose: false,
    verbosity: 0,
    log_file: '',
    spec_type: '',
    spec_draft_n_max: 16,
    spec_draft_n_min: 0,
    spec_draft_p_split: 0.50,
    draft_model: '',
    draft_gpu_layers: 0,
    threads_draft: 0,
    threads_batch_draft: 0,
    spec_draft_poll: false,
    fit: true,
    fit_target_mib: '',
    fit_ctx: 4096,
  });

  let loadingPreset = $state(true);
  let serverLoading = $state(false);
  let saveTimer: ReturnType<typeof setTimeout> | undefined;
  let collapsedSections = $state<Record<string, boolean>>({
    advanced: true,
    cache: true,
    sampling: true,
    server: true,
    logging: true,
    speculative: true,
    fit: true,
  });

  function toggleSection(key: string) {
    collapsedSections[key] = !collapsedSections[key];
  }

  const model = $derived($selectedModel);

  // Helper: parse float from string, fallback to default
  function pf(v: string | undefined, def: number): number {
    if (v === undefined || v === '') return def;
    const n = parseFloat(v);
    return isNaN(n) ? def : n;
  }

  // Helper: parse int from string, fallback to default
  function pi(v: string | undefined, def: number): number {
    if (v === undefined || v === '') return def;
    const n = parseInt(v);
    return isNaN(n) ? def : n;
  }

  // Load preset when model changes
  $effect(() => {
    if (model) {
      loadingPreset = true;
      api.loadPreset(model.name).then((kv) => {
        preset = {
          ctx_size: pi(kv.ctx_size, 2048),
          threads: pi(kv.threads, 4),
          gpu_layers: pi(kv.gpu_layers, 0),
          tensor_split: kv.tensor_split || '',
          numa: kv.numa || '',
          split_mode: kv.split_mode || '',
          main_gpu: pi(kv.main_gpu, -1),
          device: kv.device || '',
          mlock: kv.mlock === 'true',
          no_mmap: kv.no_mmap === 'true',
          batch_size: pi(kv.batch_size, 2048),
          ubatch_size: pi(kv.ubatch_size, 512),
          cache_type_k: kv.cache_type_k || '',
          cache_type_v: kv.cache_type_v || '',
          flash_attn: kv.flash_attn === 'true',
          defrag_thold: pi(kv.defrag_thold, -1),
          samplers: kv.samplers || '',
          seed: pi(kv.seed, -1),
          temp: pf(kv.temp, 0.80),
          top_k: pi(kv.top_k, 40),
          top_p: pf(kv.top_p, 0.95),
          min_p: pf(kv.min_p, 0.05),
          repeat_penalty: pf(kv.repeat_penalty, 1.00),
          presence_penalty: pf(kv.presence_penalty, 0.00),
          frequency_penalty: pf(kv.frequency_penalty, 0.00),
          mirostat: pi(kv.mirostat, 0),
          parallel: pi(kv.parallel, 1),
          no_repack: kv.no_repack === 'true',
          verbose: kv.verbose === 'true',
          verbosity: pi(kv.verbosity, 0),
          log_file: kv.log_file || '',
          spec_type: kv.spec_type || '',
          spec_draft_n_max: pi(kv.spec_draft_n_max, 16),
          spec_draft_n_min: pi(kv.spec_draft_n_min, 0),
          spec_draft_p_split: pf(kv.spec_draft_p_split, 0.50),
          draft_model: kv.draft_model || '',
          draft_gpu_layers: pi(kv.draft_gpu_layers, 0),
          threads_draft: pi(kv.threads_draft, 0),
          threads_batch_draft: pi(kv.threads_batch_draft, 0),
          spec_draft_poll: kv.spec_draft_poll === 'true',
          fit: kv.fit !== 'false', // default true
          fit_target_mib: kv.fit_target_mib || '',
          fit_ctx: pi(kv.fit_ctx, 4096),
        };
        loadingPreset = false;
      }).catch(() => {
        loadingPreset = false;
      });
    }
  });

  function savePayload(): Record<string, string> {
    return {
      ctx_size: String(preset.ctx_size),
      threads: String(preset.threads),
      gpu_layers: String(preset.gpu_layers),
      tensor_split: preset.tensor_split,
      numa: preset.numa,
      split_mode: preset.split_mode,
      main_gpu: String(preset.main_gpu),
      device: preset.device,
      mlock: preset.mlock ? 'true' : 'false',
      no_mmap: preset.no_mmap ? 'true' : 'false',
      batch_size: String(preset.batch_size),
      ubatch_size: String(preset.ubatch_size),
      cache_type_k: preset.cache_type_k,
      cache_type_v: preset.cache_type_v,
      flash_attn: preset.flash_attn ? 'true' : 'false',
      defrag_thold: String(preset.defrag_thold),
      samplers: preset.samplers,
      seed: String(preset.seed),
      temp: String(preset.temp),
      top_k: String(preset.top_k),
      top_p: String(preset.top_p),
      min_p: String(preset.min_p),
      repeat_penalty: String(preset.repeat_penalty),
      presence_penalty: String(preset.presence_penalty),
      frequency_penalty: String(preset.frequency_penalty),
      mirostat: String(preset.mirostat),
      parallel: String(preset.parallel),
      no_repack: preset.no_repack ? 'true' : 'false',
      verbose: preset.verbose ? 'true' : 'false',
      verbosity: String(preset.verbosity),
      log_file: preset.log_file,
      spec_type: preset.spec_type,
      spec_draft_n_max: String(preset.spec_draft_n_max),
      spec_draft_n_min: String(preset.spec_draft_n_min),
      spec_draft_p_split: String(preset.spec_draft_p_split),
      draft_model: preset.draft_model,
      draft_gpu_layers: String(preset.draft_gpu_layers),
      threads_draft: String(preset.threads_draft),
      threads_batch_draft: String(preset.threads_batch_draft),
      spec_draft_poll: preset.spec_draft_poll ? 'true' : 'false',
      fit: preset.fit ? 'true' : 'false',
      fit_target_mib: preset.fit_target_mib,
      fit_ctx: String(preset.fit_ctx),
    };
  }

  function debouncedSave() {
    if (saveTimer) clearTimeout(saveTimer);
    saveTimer = setTimeout(() => {
      if (model) {
        api.savePreset(model.name, savePayload());
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
        await api.savePreset(model.name, savePayload());
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
      <div class="flex items-start justify-between gap-4">
        <div class="min-w-0">
          <h2 class="text-lg font-semibold truncate">{model.name}</h2>
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

      <!-- Chips: layers, context, file type -->
      <div class="flex flex-wrap gap-2">
        {#if model.block_count && model.block_count !== '0'}
          <span class="inline-flex items-center gap-1 rounded-md bg-secondary px-2 py-0.5 text-[11px] font-medium text-muted-foreground">
            {model.block_count} layers
          </span>
        {/if}
        {#if model.context_length && model.context_length !== '0'}
          <span class="inline-flex items-center gap-1 rounded-md bg-secondary px-2 py-0.5 text-[11px] font-medium text-muted-foreground">
            {model.context_length} ctx
          </span>
        {/if}
      </div>

      <Separator />

      <!-- ═══ Basic ═══ -->
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

      <!-- ═══ Model & Loading ═══ -->
      <div class="space-y-3">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground">Model &amp; Loading</h3>

        <div class="space-y-2">
          <div class="flex items-center justify-between">
            <Label for="gpu_layers">GPU Layers</Label>
            <div class="flex items-center gap-2">
              <span class="text-sm tabular-nums">{preset.gpu_layers}</span>
              <label class="flex items-center gap-1 text-xs text-muted-foreground cursor-pointer select-none">
                <input
                  type="checkbox"
                  checked={preset.gpu_layers === -1}
                  onchange={(e) => { preset.gpu_layers = e.currentTarget.checked ? -1 : 0; debouncedSave(); }}
                  class="accent-primary"
                />
                All
              </label>
            </div>
          </div>
          <Slider
            id="gpu_layers"
            type="single"
            value={preset.gpu_layers === -1 ? 0 : preset.gpu_layers}
            onValueChange={(v) => { preset.gpu_layers = v; debouncedSave(); }}
            min={0}
            max={200}
            disabled={preset.gpu_layers === -1}
          />
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div class="space-y-1">
            <Label for="tensor_split">Tensor Split</Label>
            <Input
              id="tensor_split"
              placeholder="e.g. 8,8"
              value={preset.tensor_split}
              oninput={(e) => { preset.tensor_split = e.currentTarget.value; debouncedSave(); }}
            />
          </div>
          <div class="space-y-1">
            <Label for="numa">NUMA</Label>
            <select
              id="numa"
              class="flex h-9 w-full rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              value={preset.numa}
              onchange={(e) => { preset.numa = e.currentTarget.value; debouncedSave(); }}
            >
              <option value="">Off</option>
              <option value="distribute">Distribute</option>
              <option value="isolate">Isolate</option>
            </select>
          </div>
        </div>

        <div class="grid grid-cols-2 gap-3">
          <div class="space-y-1">
            <Label for="split_mode">Split Mode</Label>
            <select
              id="split_mode"
              class="flex h-9 w-full rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              value={preset.split_mode}
              onchange={(e) => { preset.split_mode = e.currentTarget.value; debouncedSave(); }}
            >
              <option value="">Default</option>
              <option value="none">None</option>
              <option value="layer">Layer</option>
              <option value="row">Row</option>
              <option value="tensor">Tensor</option>
            </select>
          </div>
          <div class="space-y-1">
            <Label for="main_gpu">Main GPU</Label>
            <Input
              id="main_gpu"
              type="number"
              placeholder="-1 = auto"
              value={String(preset.main_gpu)}
              oninput={(e) => { preset.main_gpu = parseInt(e.currentTarget.value) || -1; debouncedSave(); }}
            />
          </div>
        </div>

        <div class="space-y-1">
          <Label for="device">Devices</Label>
          <Input
            id="device"
            placeholder="e.g. Vulkan0,Vulkan1"
            value={preset.device}
            oninput={(e) => { preset.device = e.currentTarget.value; debouncedSave(); }}
          />
        </div>

        <div class="space-y-3 pt-1">
          {#each [{ key: 'mlock' as const, label: 'mlock' }, { key: 'no_mmap' as const, label: 'no_mmap' }] as flag}
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

      <Separator />

      <!-- ═══ Context & Cache (collapsible) ═══ -->
      <div class="space-y-3">
        <button
          class="flex items-center gap-2 text-xs font-semibold uppercase tracking-wider text-muted-foreground w-full text-left"
          onclick={() => toggleSection('cache')}
        >
          <ChevronDown
            size={12}
            class={cn('transition-transform', collapsedSections.cache ? '-rotate-90' : '')}
          />
          Context &amp; Cache
        </button>

        {#if !collapsedSections.cache}
          <div class="grid grid-cols-2 gap-3">
            <div class="space-y-1">
              <Label for="batch_size">Batch Size</Label>
              <Input
                id="batch_size"
                type="number"
                value={String(preset.batch_size)}
                oninput={(e) => { preset.batch_size = parseInt(e.currentTarget.value) || 2048; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="ubatch_size">UBatch Size</Label>
              <Input
                id="ubatch_size"
                type="number"
                value={String(preset.ubatch_size)}
                oninput={(e) => { preset.ubatch_size = parseInt(e.currentTarget.value) || 512; debouncedSave(); }}
              />
            </div>
          </div>

          <div class="grid grid-cols-2 gap-3">
            <div class="space-y-1">
              <Label for="cache_type_k">Cache Type K</Label>
              <select
                id="cache_type_k"
                class="flex h-9 w-full rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
                value={preset.cache_type_k}
                onchange={(e) => { preset.cache_type_k = e.currentTarget.value; debouncedSave(); }}
              >
                <option value="">Default (f16)</option>
                <option value="f16">f16</option>
                <option value="q8_0">q8_0</option>
                <option value="q4_0">q4_0</option>
                <option value="q4_1">q4_1</option>
              </select>
            </div>
            <div class="space-y-1">
              <Label for="cache_type_v">Cache Type V</Label>
              <select
                id="cache_type_v"
                class="flex h-9 w-full rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
                value={preset.cache_type_v}
                onchange={(e) => { preset.cache_type_v = e.currentTarget.value; debouncedSave(); }}
              >
                <option value="">Default (f16)</option>
                <option value="f16">f16</option>
                <option value="q8_0">q8_0</option>
                <option value="q4_0">q4_0</option>
                <option value="q4_1">q4_1</option>
              </select>
            </div>
          </div>

          <div class="flex items-center justify-between">
            <Label for="flash_attn">Flash Attention</Label>
            <div class="flex items-center gap-3">
              <select
                id="flash_attn"
                class="flex h-9 rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
                value={preset.flash_attn ? 'on' : 'off'}
                onchange={(e) => { preset.flash_attn = e.currentTarget.value === 'on'; debouncedSave(); }}
              >
                <option value="off">Off</option>
                <option value="on">On</option>
              </select>
            </div>
          </div>

          <div class="space-y-1">
            <Label for="defrag_thold">Defrag Threshold (deprecated)</Label>
            <Input
              id="defrag_thold"
              type="number"
              placeholder="-1 = off"
              value={String(preset.defrag_thold)}
              oninput={(e) => { preset.defrag_thold = parseInt(e.currentTarget.value) || -1; debouncedSave(); }}
            />
          </div>
        {/if}
      </div>

      <Separator />

      <!-- ═══ Sampling (collapsible) ═══ -->
      <div class="space-y-3">
        <button
          class="flex items-center gap-2 text-xs font-semibold uppercase tracking-wider text-muted-foreground w-full text-left"
          onclick={() => toggleSection('sampling')}
        >
          <ChevronDown
            size={12}
            class={cn('transition-transform', collapsedSections.sampling ? '-rotate-90' : '')}
          />
          Sampling
        </button>

        {#if !collapsedSections.sampling}
          <div class="space-y-1">
            <Label for="samplers">Samplers (order)</Label>
            <Input
              id="samplers"
              placeholder="e.g. top_k;top_p;temperature"
              value={preset.samplers}
              oninput={(e) => { preset.samplers = e.currentTarget.value; debouncedSave(); }}
            />
          </div>

          <div class="grid grid-cols-2 gap-3">
            <div class="space-y-1">
              <Label for="seed">Seed</Label>
              <Input
                id="seed"
                type="number"
                placeholder="-1 = random"
                value={String(preset.seed)}
                oninput={(e) => { preset.seed = parseInt(e.currentTarget.value) || -1; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="mirostat">Mirostat</Label>
              <select
                id="mirostat"
                class="flex h-9 w-full rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
                value={preset.mirostat}
                onchange={(e) => { preset.mirostat = parseInt(e.currentTarget.value) || 0; debouncedSave(); }}
              >
                <option value="0">Off</option>
                <option value="1">Mirostat v1</option>
                <option value="2">Mirostat v2</option>
              </select>
            </div>
          </div>

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
            <div class="space-y-2">
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

          <div class="space-y-2">
            <div class="flex items-center justify-between">
              <Label for="min_p">Min P</Label>
              <span class="text-sm tabular-nums">{preset.min_p.toFixed(2)}</span>
            </div>
            <Slider
              id="min_p"
              type="single"
              value={preset.min_p}
              onValueChange={(v) => { preset.min_p = v; debouncedSave(); }}
              min={0}
              max={1}
              step={0.01}
            />
          </div>

          <div class="grid grid-cols-3 gap-3">
            <div class="space-y-1">
              <Label for="repeat_penalty">Repeat Penalty</Label>
              <Input
                id="repeat_penalty"
                type="number"
                step="0.01"
                value={String(preset.repeat_penalty)}
                oninput={(e) => { preset.repeat_penalty = parseFloat(e.currentTarget.value) || 1.0; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="presence_penalty">Presence Penalty</Label>
              <Input
                id="presence_penalty"
                type="number"
                step="0.01"
                value={String(preset.presence_penalty)}
                oninput={(e) => { preset.presence_penalty = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="frequency_penalty">Frequency Penalty</Label>
              <Input
                id="frequency_penalty"
                type="number"
                step="0.01"
                value={String(preset.frequency_penalty)}
                oninput={(e) => { preset.frequency_penalty = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }}
              />
            </div>
          </div>
        {/if}
      </div>

      <Separator />

      <!-- ═══ Server (collapsible) ═══ -->
      <div class="space-y-3">
        <button
          class="flex items-center gap-2 text-xs font-semibold uppercase tracking-wider text-muted-foreground w-full text-left"
          onclick={() => toggleSection('server')}
        >
          <ChevronDown
            size={12}
            class={cn('transition-transform', collapsedSections.server ? '-rotate-90' : '')}
          />
          Server
        </button>

        {#if !collapsedSections.server}
          <div class="grid grid-cols-2 gap-3">
            <div class="space-y-1">
              <Label for="parallel">Parallel Slots</Label>
              <Input
                id="parallel"
                type="number"
                min="1"
                value={String(preset.parallel)}
                oninput={(e) => { preset.parallel = parseInt(e.currentTarget.value) || 1; debouncedSave(); }}
              />
            </div>
            <div class="flex items-center justify-between pt-5">
              <Label for="no_repack">No Repack</Label>
              <Switch
                checked={preset.no_repack}
                onCheckedChange={(c) => {
                  preset.no_repack = c;
                  debouncedSave();
                }}
              />
            </div>
          </div>
        {/if}
      </div>

      <Separator />

      <!-- ═══ Logging (collapsible) ═══ -->
      <div class="space-y-3">
        <button
          class="flex items-center gap-2 text-xs font-semibold uppercase tracking-wider text-muted-foreground w-full text-left"
          onclick={() => toggleSection('logging')}
        >
          <ChevronDown
            size={12}
            class={cn('transition-transform', collapsedSections.logging ? '-rotate-90' : '')}
          />
          Logging
        </button>

        {#if !collapsedSections.logging}
          <div class="flex items-center justify-between">
            <Label for="verbose">Verbose</Label>
            <Switch
              checked={preset.verbose}
              onCheckedChange={(c) => {
                preset.verbose = c;
                debouncedSave();
              }}
            />
          </div>

          <div class="grid grid-cols-2 gap-3">
            <div class="space-y-1">
              <Label for="verbosity">Verbosity (0-5)</Label>
              <Input
                id="verbosity"
                type="number"
                min="0"
                max="5"
                value={String(preset.verbosity)}
                oninput={(e) => { preset.verbosity = Math.min(5, Math.max(0, parseInt(e.currentTarget.value) || 0)); debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="log_file">Log File</Label>
              <Input
                id="log_file"
                placeholder="path to log file"
                value={preset.log_file}
                oninput={(e) => { preset.log_file = e.currentTarget.value; debouncedSave(); }}
              />
            </div>
          </div>
        {/if}
      </div>

      <Separator />

      <!-- ═══ Speculative Decoding (collapsible) ═══ -->
      <div class="space-y-3">
        <button
          class="flex items-center gap-2 text-xs font-semibold uppercase tracking-wider text-muted-foreground w-full text-left"
          onclick={() => toggleSection('speculative')}
        >
          <ChevronDown
            size={12}
            class={cn('transition-transform', collapsedSections.speculative ? '-rotate-90' : '')}
          />
          Speculative Decoding
        </button>

        {#if !collapsedSections.speculative}
          <div class="space-y-1">
            <Label for="spec_type">Spec Type</Label>
            <select
              id="spec_type"
              class="flex h-9 w-full rounded-md border border-border bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              value={preset.spec_type}
              onchange={(e) => { preset.spec_type = e.currentTarget.value; debouncedSave(); }}
            >
              <option value="">Off</option>
              <option value="draft-mtp">Draft MTP</option>
              <option value="draft-model">Draft Model</option>
              <option value="ngram-simple">N-gram Simple</option>
              <option value="ngram-map-k">N-gram Map K</option>
              <option value="ngram-map-k4v">N-gram Map K4V</option>
              <option value="ngram-mod">N-gram Mod</option>
            </select>
          </div>

          <div class="grid grid-cols-3 gap-3">
            <div class="space-y-1">
              <Label for="spec_draft_n_max">Draft N Max</Label>
              <Input
                id="spec_draft_n_max"
                type="number"
                value={String(preset.spec_draft_n_max)}
                oninput={(e) => { preset.spec_draft_n_max = parseInt(e.currentTarget.value) || 16; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="spec_draft_n_min">Draft N Min</Label>
              <Input
                id="spec_draft_n_min"
                type="number"
                value={String(preset.spec_draft_n_min)}
                oninput={(e) => { preset.spec_draft_n_min = parseInt(e.currentTarget.value) || 0; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="spec_draft_p_split">P Split</Label>
              <Input
                id="spec_draft_p_split"
                type="number"
                step="0.01"
                value={String(preset.spec_draft_p_split)}
                oninput={(e) => { preset.spec_draft_p_split = parseFloat(e.currentTarget.value) || 0.5; debouncedSave(); }}
              />
            </div>
          </div>

          <div class="space-y-1">
            <Label for="draft_model">Draft Model Path</Label>
            <Input
              id="draft_model"
              placeholder="path to draft GGUF"
              value={preset.draft_model}
              oninput={(e) => { preset.draft_model = e.currentTarget.value; debouncedSave(); }}
            />
          </div>

          <div class="grid grid-cols-3 gap-3">
            <div class="space-y-1">
              <Label for="draft_gpu_layers">Draft GPU Layers</Label>
              <Input
                id="draft_gpu_layers"
                type="number"
                placeholder="-1 = all"
                value={String(preset.draft_gpu_layers)}
                oninput={(e) => { preset.draft_gpu_layers = parseInt(e.currentTarget.value) || 0; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="threads_draft">Draft Threads</Label>
              <Input
                id="threads_draft"
                type="number"
                placeholder="0 = auto"
                value={String(preset.threads_draft)}
                oninput={(e) => { preset.threads_draft = parseInt(e.currentTarget.value) || 0; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="threads_batch_draft">Draft Batch Threads</Label>
              <Input
                id="threads_batch_draft"
                type="number"
                placeholder="0 = auto"
                value={String(preset.threads_batch_draft)}
                oninput={(e) => { preset.threads_batch_draft = parseInt(e.currentTarget.value) || 0; debouncedSave(); }}
              />
            </div>
          </div>

          <div class="flex items-center justify-between">
            <Label for="spec_draft_poll">Draft Polling</Label>
            <Switch
              checked={preset.spec_draft_poll}
              onCheckedChange={(c) => {
                preset.spec_draft_poll = c;
                debouncedSave();
              }}
            />
          </div>
        {/if}
      </div>

      <Separator />

      <!-- ═══ Auto-fit ═══ -->
      <div class="space-y-3">
        <button
          class="flex items-center gap-2 text-xs font-semibold uppercase tracking-wider text-muted-foreground w-full text-left"
          onclick={() => toggleSection('fit')}
        >
          <ChevronDown
            size={12}
            class={cn('transition-transform', collapsedSections.fit ? '-rotate-90' : '')}
          />
          Auto-fit
        </button>

        {#if !collapsedSections.fit}
          <div class="flex items-center justify-between">
            <Label for="fit">Fit (auto-tune params)</Label>
            <Switch
              checked={preset.fit}
              onCheckedChange={(c) => {
                preset.fit = c;
                debouncedSave();
              }}
            />
          </div>

          <div class="grid grid-cols-2 gap-3">
            <div class="space-y-1">
              <Label for="fit_target_mib">Fit Target (MiB)</Label>
              <Input
                id="fit_target_mib"
                placeholder="1024"
                value={preset.fit_target_mib}
                oninput={(e) => { preset.fit_target_mib = e.currentTarget.value; debouncedSave(); }}
              />
            </div>
            <div class="space-y-1">
              <Label for="fit_ctx">Fit Min Ctx</Label>
              <Input
                id="fit_ctx"
                type="number"
                value={String(preset.fit_ctx)}
                oninput={(e) => { preset.fit_ctx = parseInt(e.currentTarget.value) || 4096; debouncedSave(); }}
              />
            </div>
          </div>
        {/if}
      </div>
    {/if}
  </div>
{/if}
