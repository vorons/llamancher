<script lang="ts">
  import { selectedModel, serverStatus, serverModel, settings } from '$lib/stores.svelte';
  import ServerButton from '$lib/components/ServerButton.svelte';
  import { Loader2 } from '@lucide/svelte';
  import { api } from '$lib/saucer';
  import { toast } from 'svelte-sonner';
  import { get } from 'svelte/store';

  import { Separator } from '$lib/ui/separator';
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { Switch } from '$lib/ui/switch';
  import { Slider } from '$lib/ui/slider';
  import { NativeSelect, NativeSelectOption } from '$lib/ui/native-select';
  import type { Preset } from '$lib/types';

  let preset = $state<Preset>({
    ctx_size: 2048,
    threads: 4,
    gpu_layers: 0,
    tensor_split: '',
    numa: '',
    split_mode: 'none',
    main_gpu: 0,
    device: '',
    mlock: false,
    no_mmap: false,
    jinja: true,
    grammar: '',
    grammar_file: '',
    json_schema: '',
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
    spec_type: 'none',
    spec_draft_n_max: 16,
    spec_draft_n_min: 0,
    spec_draft_p_split: 0.50,
    draft_model: '',
    draft_gpu_layers: 0,
    threads_draft: 0,
    threads_batch_draft: 0,
    spec_draft_poll: false,
    spec_ngram_mod_n_min: 48,
    spec_ngram_mod_n_max: 64,
    spec_ngram_mod_n_match: 24,
    spec_ngram_simple_size_n: 12,
    spec_ngram_simple_size_m: 48,
    spec_ngram_simple_min_hits: 1,
    spec_ngram_map_k_size_n: 12,
    spec_ngram_map_k_size_m: 48,
    spec_ngram_map_k_min_hits: 1,
    spec_ngram_map_k4v_size_n: 12,
    spec_ngram_map_k4v_size_m: 48,
    spec_ngram_map_k4v_min_hits: 1,
    fit: true,
    fit_target_mib: '',
    fit_ctx: 4096,
  });

  let loadingPreset = $state(true);
  let serverLoading = $state(false);
  let saveTimer: ReturnType<typeof setTimeout> | undefined;

  const model = $derived($selectedModel);

  function pf(v: string | undefined, def: number): number {
    if (v === undefined || v === '') return def;
    const n = parseFloat(v);
    return isNaN(n) ? def : n;
  }

  function pi(v: string | undefined, def: number): number {
    if (v === undefined || v === '') return def;
    const n = parseInt(v);
    return isNaN(n) ? def : n;
  }

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
          split_mode: kv.split_mode || 'none',
          main_gpu: pi(kv.main_gpu, 0),
          device: kv.device || '',
          mlock: kv.mlock === 'true',
          no_mmap: kv.no_mmap === 'true',
          jinja: kv.jinja !== 'false',
          grammar: kv.grammar || '',
          grammar_file: kv.grammar_file || '',
          json_schema: kv.json_schema || '',
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
          spec_type: kv.spec_type || 'none',
          spec_draft_n_max: pi(kv.spec_draft_n_max, 16),
          spec_draft_n_min: pi(kv.spec_draft_n_min, 0),
          spec_draft_p_split: pf(kv.spec_draft_p_split, 0.50),
          draft_model: kv.draft_model || '',
          draft_gpu_layers: pi(kv.draft_gpu_layers, 0),
          threads_draft: pi(kv.threads_draft, 0),
          threads_batch_draft: pi(kv.threads_batch_draft, 0),
          spec_draft_poll: kv.spec_draft_poll === 'true',
          spec_ngram_mod_n_min: pi(kv.spec_ngram_mod_n_min, 48),
          spec_ngram_mod_n_max: pi(kv.spec_ngram_mod_n_max, 64),
          spec_ngram_mod_n_match: pi(kv.spec_ngram_mod_n_match, 24),
          spec_ngram_simple_size_n: pi(kv.spec_ngram_simple_size_n, 12),
          spec_ngram_simple_size_m: pi(kv.spec_ngram_simple_size_m, 48),
          spec_ngram_simple_min_hits: pi(kv.spec_ngram_simple_min_hits, 1),
          spec_ngram_map_k_size_n: pi(kv.spec_ngram_map_k_size_n, 12),
          spec_ngram_map_k_size_m: pi(kv.spec_ngram_map_k_size_m, 48),
          spec_ngram_map_k_min_hits: pi(kv.spec_ngram_map_k_min_hits, 1),
          spec_ngram_map_k4v_size_n: pi(kv.spec_ngram_map_k4v_size_n, 12),
          spec_ngram_map_k4v_size_m: pi(kv.spec_ngram_map_k4v_size_m, 48),
          spec_ngram_map_k4v_min_hits: pi(kv.spec_ngram_map_k4v_min_hits, 1),
          fit: kv.fit !== 'false',
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
      jinja: preset.jinja ? 'true' : 'false',
      grammar: preset.grammar,
      grammar_file: preset.grammar_file,
      json_schema: preset.json_schema,
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
      spec_ngram_mod_n_min: String(preset.spec_ngram_mod_n_min),
      spec_ngram_mod_n_max: String(preset.spec_ngram_mod_n_max),
      spec_ngram_mod_n_match: String(preset.spec_ngram_mod_n_match),
      spec_ngram_simple_size_n: String(preset.spec_ngram_simple_size_n),
      spec_ngram_simple_size_m: String(preset.spec_ngram_simple_size_m),
      spec_ngram_simple_min_hits: String(preset.spec_ngram_simple_min_hits),
      spec_ngram_map_k_size_n: String(preset.spec_ngram_map_k_size_n),
      spec_ngram_map_k_size_m: String(preset.spec_ngram_map_k_size_m),
      spec_ngram_map_k_min_hits: String(preset.spec_ngram_map_k_min_hits),
      spec_ngram_map_k4v_size_n: String(preset.spec_ngram_map_k4v_size_n),
      spec_ngram_map_k4v_size_m: String(preset.spec_ngram_map_k4v_size_m),
      spec_ngram_map_k4v_min_hits: String(preset.spec_ngram_map_k4v_min_hits),
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

  async function waitForStatus(target: string, timeout = 8000) {
    const deadline = Date.now() + timeout;
    while (Date.now() < deadline) {
      const st = await api.serverStatus();
      if (st.status === target) return;
      await new Promise(r => setTimeout(r, 200));
    }
  }

  async function handlePlayStop() {
    if (!model) return;
    serverLoading = true;

    try {
      const status = await api.serverStatus();
      if (status.status === 'stopped') {
        const s = get(settings);
        if (!s.llama_server_path) {
          toast.error('llama-server executable path is not set. Configure it in Settings.');
          return;
        }
        await api.savePreset(model.name, savePayload());
        const result = await api.startServer(model.name, model.path);
        if (result === 'server_not_found') {
          toast.error('llama-server executable not found. Set the path in Settings.');
          return;
        }
        serverModel.set(model.display_name && model.display_name.trim() !== '' ? model.display_name : model.name);
        serverStatus.set('starting');
      } else if (status.model === model.name) {
        await api.stopServer();
        await waitForStatus('stopped');
        serverModel.set('');
        serverStatus.set('stopped');
      } else {
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
    } catch {
      toast.error('Server operation failed');
    } finally {
      serverLoading = false;
    }
  }

  const isSpecActive = $derived(preset.spec_type !== 'none');
  const isDraftMode = $derived(preset.spec_type === 'draft-model' || preset.spec_type === 'draft-mtp');
  const isNgramMode = $derived(preset.spec_type.startsWith('ngram-'));


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
        <ServerButton
          modelName={model!.name}
          serverModelName={$serverModel}
          serverStatus={$serverStatus}
          loading={serverLoading}
          onclick={handlePlayStop}
        />
      </div>

      <!-- Chips -->
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

      <!-- ════ Basic ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Basic</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="ctx_size" class="text-sm">Context Size</Label>
          <Input id="ctx_size" type="number" class="w-36" value={String(preset.ctx_size)}
            oninput={(e) => { preset.ctx_size = parseInt(e.currentTarget.value) || 2048; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="threads" class="text-sm">Threads</Label>
          <Input id="threads" type="number" class="w-36" value={String(preset.threads)}
            oninput={(e) => { preset.threads = parseInt(e.currentTarget.value) || 4; debouncedSave(); }} />
        </div>
      </div>

      <Separator />

      <!-- ════ Model & Loading ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Model &amp; Loading</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="gpu_layers" class="text-sm">GPU Layers</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Layers offloaded to GPU VRAM</p>
          </div>
          <div class="flex items-center gap-2 w-44 shrink-0">
            <Slider id="gpu_layers" type="single"
              value={preset.gpu_layers}
              onValueChange={(v) => { preset.gpu_layers = v; debouncedSave(); }}
              min={0} max={100} />
            <span class="text-sm tabular-nums w-8 text-right shrink-0">{preset.gpu_layers}</span>
          </div>
        </div>

        {#if preset.split_mode !== 'none'}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="tensor_split" class="text-sm">Tensor Split</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">GPU memory distribution ratios</p>
          </div>
          <Input id="tensor_split" placeholder="e.g. 8,8" class="w-36" value={preset.tensor_split}
            oninput={(e) => { preset.tensor_split = e.currentTarget.value; debouncedSave(); }} />
        </div>
        {/if}

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="numa" class="text-sm">NUMA</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">NUMA memory allocation strategy</p>
          </div>
          <NativeSelect id="numa" class="w-36" value={preset.numa} onchange={(e) => { preset.numa = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="">Off</NativeSelectOption>
            <NativeSelectOption value="distribute">Distribute</NativeSelectOption>
            <NativeSelectOption value="isolate">Isolate</NativeSelectOption>
          </NativeSelect>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="split_mode" class="text-sm">Split Mode</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">How to split model across multiple GPUs</p>
          </div>
          <NativeSelect id="split_mode" class="w-36" value={preset.split_mode} onchange={(e) => { preset.split_mode = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="none">None</NativeSelectOption>
            <NativeSelectOption value="layer">Layer</NativeSelectOption>
            <NativeSelectOption value="row">Row</NativeSelectOption>
            <NativeSelectOption value="tensor">Tensor</NativeSelectOption>
          </NativeSelect>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="main_gpu" class="text-sm">Main GPU</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Main GPU when split-mode is none</p>
          </div>
          <Input id="main_gpu" type="number" placeholder="0" class="w-36" value={String(preset.main_gpu)}
            oninput={(e) => { preset.main_gpu = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="device" class="text-sm">Devices</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">GPU devices (comma-separated)</p>
          </div>
          <Input id="device" placeholder="Vulkan0,Vulkan1" class="w-36" value={preset.device}
            oninput={(e) => { preset.device = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="mlock" class="text-sm">mlock</Label>
          <Switch checked={preset.mlock} onCheckedChange={(c) => { preset.mlock = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="no_mmap" class="text-sm">no_mmap</Label>
          <Switch checked={preset.no_mmap} onCheckedChange={(c) => { preset.no_mmap = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="jinja" class="text-sm">Jinja</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Use Jinja template engine for chat</p>
          </div>
          <Switch checked={preset.jinja} onCheckedChange={(c) => { preset.jinja = c; debouncedSave(); }} />
        </div>

        {#if !preset.jinja}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="grammar" class="text-sm">Grammar</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">BNF-like grammar constraint</p>
          </div>
          <Input id="grammar" placeholder="root ::= ..." class="w-36" value={preset.grammar}
            oninput={(e) => { preset.grammar = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="grammar_file" class="text-sm">Grammar File</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Path to grammar file</p>
          </div>
          <Input id="grammar_file" placeholder="/path/to/grammar.gbnf" class="w-36" value={preset.grammar_file}
            oninput={(e) => { preset.grammar_file = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="json_schema" class="text-sm">JSON Schema</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">JSON schema to constrain generations</p>
          </div>
          <Input id="json_schema" placeholder={'{}'} class="w-36" value={preset.json_schema}
            oninput={(e) => { preset.json_schema = e.currentTarget.value; debouncedSave(); }} />
        </div>
        {/if}
      </div>

      <Separator />

      <!-- ════ Context & Cache ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Context &amp; Cache</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="batch_size" class="text-sm">Batch Size</Label>
          <Input id="batch_size" type="number" class="w-36" value={String(preset.batch_size)}
            oninput={(e) => { preset.batch_size = parseInt(e.currentTarget.value) || 2048; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="ubatch_size" class="text-sm">UBatch Size</Label>
          <Input id="ubatch_size" type="number" class="w-36" value={String(preset.ubatch_size)}
            oninput={(e) => { preset.ubatch_size = parseInt(e.currentTarget.value) || 512; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="cache_type_k" class="text-sm">Cache Type K</Label>
          <NativeSelect id="cache_type_k" class="w-36" value={preset.cache_type_k} onchange={(e) => { preset.cache_type_k = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="">Default (f16)</NativeSelectOption>
            <NativeSelectOption value="f16">f16</NativeSelectOption>
            <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
            <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
            <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
          </NativeSelect>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="cache_type_v" class="text-sm">Cache Type V</Label>
          <NativeSelect id="cache_type_v" class="w-36" value={preset.cache_type_v} onchange={(e) => { preset.cache_type_v = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="">Default (f16)</NativeSelectOption>
            <NativeSelectOption value="f16">f16</NativeSelectOption>
            <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
            <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
            <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
          </NativeSelect>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="flash_attn" class="text-sm">Flash Attention</Label>
          <Switch checked={preset.flash_attn} onCheckedChange={(c) => { preset.flash_attn = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="defrag_thold" class="text-sm">Defrag Threshold</Label>
          <Input id="defrag_thold" type="number" placeholder="-1" class="w-36" value={String(preset.defrag_thold)}
            oninput={(e) => { preset.defrag_thold = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
        </div>
      </div>

      <Separator />

      <!-- ════ Sampling ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Sampling</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="samplers" class="text-sm">Samplers</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Sampler chain order (semicolon-separated)</p>
          </div>
          <Input id="samplers" placeholder="top_k;top_p;temperature" class="w-36" value={preset.samplers}
            oninput={(e) => { preset.samplers = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="seed" class="text-sm">Seed</Label>
          <Input id="seed" type="number" placeholder="-1" class="w-36" value={String(preset.seed)}
            oninput={(e) => { preset.seed = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="mirostat" class="text-sm">Mirostat</Label>
          <NativeSelect id="mirostat" class="w-36" value={String(preset.mirostat)} onchange={(e) => { preset.mirostat = parseInt(e.currentTarget.value) || 0; debouncedSave(); }}>
            <NativeSelectOption value="0">Off</NativeSelectOption>
            <NativeSelectOption value="1">Mirostat v1</NativeSelectOption>
            <NativeSelectOption value="2">Mirostat v2</NativeSelectOption>
          </NativeSelect>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="temp" class="text-sm">Temperature</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Higher values increase creativity</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="temp" type="single" value={preset.temp}
              onValueChange={(v) => { preset.temp = v; debouncedSave(); }} min={0} max={2} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.temp.toFixed(2)}</span>
          </div>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="top_k" class="text-sm">Top K</Label>
          <Input id="top_k" type="number" class="w-36" value={String(preset.top_k)}
            oninput={(e) => { preset.top_k = parseInt(e.currentTarget.value) || 40; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="top_p" class="text-sm">Top P</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Nucleus sampling — cumulative probability</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="top_p" type="single" value={preset.top_p}
              onValueChange={(v) => { preset.top_p = v; debouncedSave(); }} min={0} max={1} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.top_p.toFixed(2)}</span>
          </div>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="min_p" class="text-sm">Min P</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Minimum probability for token selection</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="min_p" type="single" value={preset.min_p}
              onValueChange={(v) => { preset.min_p = v; debouncedSave(); }} min={0} max={1} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.min_p.toFixed(2)}</span>
          </div>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="repeat_penalty" class="text-sm">Repeat Penalty</Label>
          <Input id="repeat_penalty" type="number" step="0.01" class="w-36" value={String(preset.repeat_penalty)}
            oninput={(e) => { preset.repeat_penalty = parseFloat(e.currentTarget.value) || 1.0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="presence_penalty" class="text-sm">Presence Penalty</Label>
          <Input id="presence_penalty" type="number" step="0.01" class="w-36" value={String(preset.presence_penalty)}
            oninput={(e) => { preset.presence_penalty = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="frequency_penalty" class="text-sm">Frequency Penalty</Label>
          <Input id="frequency_penalty" type="number" step="0.01" class="w-36" value={String(preset.frequency_penalty)}
            oninput={(e) => { preset.frequency_penalty = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>
      </div>

      <Separator />

      <!-- ════ Server ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Server</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="parallel" class="text-sm">Parallel Slots</Label>
          <Input id="parallel" type="number" min="1" class="w-36" value={String(preset.parallel)}
            oninput={(e) => { preset.parallel = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="no_repack" class="text-sm">No Repack</Label>
          <Switch checked={preset.no_repack} onCheckedChange={(c) => { preset.no_repack = c; debouncedSave(); }} />
        </div>
      </div>

      <Separator />

      <!-- ════ Logging ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Logging</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="verbose" class="text-sm">Verbose</Label>
          <Switch checked={preset.verbose} onCheckedChange={(c) => { preset.verbose = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="verbosity" class="text-sm">Verbosity</Label>
          <Input id="verbosity" type="number" min="0" max="5" class="w-36" value={String(preset.verbosity)}
            oninput={(e) => { preset.verbosity = Math.min(5, Math.max(0, parseInt(e.currentTarget.value) || 0)); debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="log_file" class="text-sm">Log File</Label>
          <Input id="log_file" placeholder="/path/to/llama.log" class="w-36" value={preset.log_file}
            oninput={(e) => { preset.log_file = e.currentTarget.value; debouncedSave(); }} />
        </div>
      </div>

      <Separator />

      <!-- ════ Speculative Decoding ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Speculative Decoding</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="spec_type" class="text-sm">Spec Type</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Speculative decoding method</p>
          </div>
          <NativeSelect id="spec_type" class="w-36" value={preset.spec_type} onchange={(e) => { preset.spec_type = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="none">Off</NativeSelectOption>
            <NativeSelectOption value="draft-mtp">Draft MTP</NativeSelectOption>
            <NativeSelectOption value="draft-model">Draft Model</NativeSelectOption>
            <NativeSelectOption value="ngram-simple">N-gram Simple</NativeSelectOption>
            <NativeSelectOption value="ngram-map-k">N-gram Map K</NativeSelectOption>
            <NativeSelectOption value="ngram-map-k4v">N-gram Map K4V</NativeSelectOption>
            <NativeSelectOption value="ngram-mod">N-gram Mod</NativeSelectOption>
          </NativeSelect>
        </div>

        {#if isSpecActive}
        <!-- Common speculative decoding params -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="spec_draft_n_max" class="text-sm">Draft N Max</Label>
          <Input id="spec_draft_n_max" type="number" class="w-36" value={String(preset.spec_draft_n_max)}
            oninput={(e) => { preset.spec_draft_n_max = parseInt(e.currentTarget.value) || 16; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="spec_draft_n_min" class="text-sm">Draft N Min</Label>
          <Input id="spec_draft_n_min" type="number" class="w-36" value={String(preset.spec_draft_n_min)}
            oninput={(e) => { preset.spec_draft_n_min = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="spec_draft_p_split" class="text-sm">P Split</Label>
          <Input id="spec_draft_p_split" type="number" step="0.01" class="w-36" value={String(preset.spec_draft_p_split)}
            oninput={(e) => { preset.spec_draft_p_split = parseFloat(e.currentTarget.value) || 0.5; debouncedSave(); }} />
        </div>

        {#if isDraftMode}
        <!-- Draft model-specific params -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="draft_model" class="text-sm">Draft Model</Label>
          <Input id="draft_model" placeholder="/path/to/draft.gguf" class="w-36" value={preset.draft_model}
            oninput={(e) => { preset.draft_model = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="draft_gpu_layers" class="text-sm">Draft GPU Layers</Label>
          <Input id="draft_gpu_layers" type="number" placeholder="-1" class="w-36" value={String(preset.draft_gpu_layers)}
            oninput={(e) => { preset.draft_gpu_layers = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="threads_draft" class="text-sm">Draft Threads</Label>
          <Input id="threads_draft" type="number" placeholder="0" class="w-36" value={String(preset.threads_draft)}
            oninput={(e) => { preset.threads_draft = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="threads_batch_draft" class="text-sm">Draft Batch Threads</Label>
          <Input id="threads_batch_draft" type="number" placeholder="0" class="w-36" value={String(preset.threads_batch_draft)}
            oninput={(e) => { preset.threads_batch_draft = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="spec_draft_poll" class="text-sm">Draft Polling</Label>
          <Switch checked={preset.spec_draft_poll} onCheckedChange={(c) => { preset.spec_draft_poll = c; debouncedSave(); }} />
        </div>
        {/if}

        {#if isNgramMode}
        <!-- N-gram params -->
        <div class="mt-2 space-y-1">
          <p class="text-[11px] font-medium text-muted-foreground">N-gram Mod</p>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_mod_n_min" class="text-sm">N Min</Label>
            <Input id="spec_ngram_mod_n_min" type="number" class="w-36" value={String(preset.spec_ngram_mod_n_min)}
              oninput={(e) => { preset.spec_ngram_mod_n_min = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_mod_n_max" class="text-sm">N Max</Label>
            <Input id="spec_ngram_mod_n_max" type="number" class="w-36" value={String(preset.spec_ngram_mod_n_max)}
              oninput={(e) => { preset.spec_ngram_mod_n_max = parseInt(e.currentTarget.value) || 64; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_mod_n_match" class="text-sm">N Match</Label>
            <Input id="spec_ngram_mod_n_match" type="number" class="w-36" value={String(preset.spec_ngram_mod_n_match)}
              oninput={(e) => { preset.spec_ngram_mod_n_match = parseInt(e.currentTarget.value) || 24; debouncedSave(); }} />
          </div>
        </div>

        <div class="mt-2 space-y-1">
          <p class="text-[11px] font-medium text-muted-foreground">N-gram Simple / Map-K / Map-K4V</p>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_simple_size_n" class="text-sm">Size N</Label>
            <Input id="spec_ngram_simple_size_n" type="number" class="w-36" value={String(preset.spec_ngram_simple_size_n)}
              oninput={(e) => { preset.spec_ngram_simple_size_n = parseInt(e.currentTarget.value) || 12; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_simple_size_m" class="text-sm">Size M</Label>
            <Input id="spec_ngram_simple_size_m" type="number" class="w-36" value={String(preset.spec_ngram_simple_size_m)}
              oninput={(e) => { preset.spec_ngram_simple_size_m = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_simple_min_hits" class="text-sm">Min Hits</Label>
            <Input id="spec_ngram_simple_min_hits" type="number" class="w-36" value={String(preset.spec_ngram_simple_min_hits)}
              oninput={(e) => { preset.spec_ngram_simple_min_hits = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
          </div>
        </div>

        <div class="mt-2 space-y-1">
          <p class="text-[11px] font-medium text-muted-foreground">N-gram Map-K (separate)</p>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_map_k_size_n" class="text-sm">Size N</Label>
            <Input id="spec_ngram_map_k_size_n" type="number" class="w-36" value={String(preset.spec_ngram_map_k_size_n)}
              oninput={(e) => { preset.spec_ngram_map_k_size_n = parseInt(e.currentTarget.value) || 12; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_map_k_size_m" class="text-sm">Size M</Label>
            <Input id="spec_ngram_map_k_size_m" type="number" class="w-36" value={String(preset.spec_ngram_map_k_size_m)}
              oninput={(e) => { preset.spec_ngram_map_k_size_m = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_map_k_min_hits" class="text-sm">Min Hits</Label>
            <Input id="spec_ngram_map_k_min_hits" type="number" class="w-36" value={String(preset.spec_ngram_map_k_min_hits)}
              oninput={(e) => { preset.spec_ngram_map_k_min_hits = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
          </div>
        </div>

        <div class="mt-2 space-y-1">
          <p class="text-[11px] font-medium text-muted-foreground">N-gram Map-K4V (separate)</p>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_map_k4v_size_n" class="text-sm">Size N</Label>
            <Input id="spec_ngram_map_k4v_size_n" type="number" class="w-36" value={String(preset.spec_ngram_map_k4v_size_n)}
              oninput={(e) => { preset.spec_ngram_map_k4v_size_n = parseInt(e.currentTarget.value) || 12; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_map_k4v_size_m" class="text-sm">Size M</Label>
            <Input id="spec_ngram_map_k4v_size_m" type="number" class="w-36" value={String(preset.spec_ngram_map_k4v_size_m)}
              oninput={(e) => { preset.spec_ngram_map_k4v_size_m = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="spec_ngram_map_k4v_min_hits" class="text-sm">Min Hits</Label>
            <Input id="spec_ngram_map_k4v_min_hits" type="number" class="w-36" value={String(preset.spec_ngram_map_k4v_min_hits)}
              oninput={(e) => { preset.spec_ngram_map_k4v_min_hits = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
          </div>
        </div>
        {/if}
        {/if}
      </div>

      <Separator />

      <!-- ════ Auto-fit ════ -->
      <div>
        <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Auto-fit</h3>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="fit" class="text-sm">Fit</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Auto-tune params based on available memory</p>
          </div>
          <Switch checked={preset.fit} onCheckedChange={(c) => { preset.fit = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="fit_target_mib" class="text-sm">Fit Target</Label>
          <Input id="fit_target_mib" placeholder="1024" class="w-36" value={preset.fit_target_mib}
            oninput={(e) => { preset.fit_target_mib = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="fit_ctx" class="text-sm">Fit Min Ctx</Label>
          <Input id="fit_ctx" type="number" class="w-36" value={String(preset.fit_ctx)}
            oninput={(e) => { preset.fit_ctx = parseInt(e.currentTarget.value) || 4096; debouncedSave(); }} />
        </div>
      </div>
    {/if}
  </div>
{/if}
