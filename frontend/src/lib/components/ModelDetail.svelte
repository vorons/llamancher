<script lang="ts">
  import { selectedModel, serverStatus, serverModel, settings } from '$lib/stores.svelte';
  import ServerButton from '$lib/components/ServerButton.svelte';
  import { Loader2, Terminal, FileText, FileJson, Eye } from '@lucide/svelte';
  import { api } from '$lib/saucer';
  import { toast } from 'svelte-sonner';
  import { get } from 'svelte/store';
  import LogViewer from './LogViewer.svelte';

  import { Separator } from '$lib/ui/separator';
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { Switch } from '$lib/ui/switch';
  import { Slider } from '$lib/ui/slider';
  import { NativeSelect, NativeSelectOption } from '$lib/ui/native-select';
  import ModelSpeculative from './ModelSpeculative.svelte';
  import type { ModelInfo, Preset } from '$lib/types';

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
    chat_template: '',
    fit: true,
    fit_target_mib: '',
    fit_ctx: 4096,
  });

  let loadingPreset = $state(true);
  let serverLoading = $state(false);
  let saveTimer: ReturnType<typeof setTimeout> | undefined;
  let logOpen = $state(false);

  // Popover editors
  let popoverField = $state<'grammar' | 'json_schema' | 'chat_template' | null>(null);
  let popoverValue = $state('');
  let popoverPos = $state<{ top: number; left: number } | null>(null);

  function openPopover(field: typeof popoverField, value: string, el: HTMLElement) {
    popoverField = field;
    popoverValue = value;
    const rect = el.getBoundingClientRect();
    const pw = 320; // w-80 ≈ 20rem
    let left = rect.right - pw;
    left = Math.max(8, Math.min(left, window.innerWidth - pw - 8));
    popoverPos = { top: rect.bottom + 4, left };
  }

  function closePopover() {
    popoverField = null;
    popoverPos = null;
  }

  function savePopover() {
    if (!popoverField || !model) return;
    if (popoverField === 'grammar') preset.grammar = popoverValue;
    else if (popoverField === 'json_schema') preset.json_schema = popoverValue;
    else if (popoverField === 'chat_template') {
      // Store chat_template override — will be saved to preset
      preset.chat_template = popoverValue;
    }
    closePopover();
    debouncedSave();
  }

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
      const modelName = model.name;
      loadingPreset = true;
      api.loadPreset(modelName).then((kv) => {
        if (model?.name !== modelName) {
          loadingPreset = false;
          return; // stale — user switched model
        }
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
          chat_template: kv.chat_template || '',
          fit: kv.fit !== 'false',
          fit_target_mib: kv.fit_target_mib || '',
          fit_ctx: pi(kv.fit_ctx, 4096),
        };
        loadingPreset = false;
      }).catch(() => {
        if (model?.name !== modelName) {
          loadingPreset = false;
          return;
        }
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
      chat_template: preset.chat_template,
      fit: preset.fit ? 'true' : 'false',
      fit_target_mib: preset.fit_target_mib,
      fit_ctx: String(preset.fit_ctx),
    };
  }

  function debouncedSave() {
    const modelName = model?.name;
    if (saveTimer) clearTimeout(saveTimer);
    saveTimer = setTimeout(() => {
      if (modelName && model?.name === modelName) {
        api.savePreset(modelName, savePayload());
        if ($serverStatus === 'running' && modelDisplayName(model) && $serverModel === modelDisplayName(model)) {
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

  function modelDisplayName(m: ModelInfo | null): string {
    return m?.display_name && m.display_name.trim() !== '' ? m.display_name : m?.name ?? '';
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
        serverModel.set(modelDisplayName(model));
        serverStatus.set('starting');
      } else if (modelDisplayName(model) && status.model === modelDisplayName(model)) {
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

  // isSpecActive, isDraftMode, isNgramMode — defined in ModelSpeculative


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
        <div class="flex items-center gap-2 shrink-0">
          <button
            class="flex items-center justify-center h-8 w-8 rounded-md border border-border bg-secondary hover:bg-accent text-muted-foreground hover:text-foreground transition-all active:scale-95"
            onclick={() => logOpen = true}
            title="Server logs"
          >
            <Terminal size={14} />
          </button>
          <ServerButton
            modelName={modelDisplayName(model)}
            serverModelName={$serverModel}
            serverStatus={$serverStatus}
            loading={serverLoading}
            onclick={handlePlayStop}
          />
        </div>
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

        {#if preset.jinja}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="chat_template" class="text-sm">Chat Template</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Jinja chat template for formatting messages</p>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-muted-foreground truncate max-w-40">
              {model?.chat_template
                ? (model.chat_template.length > 40 ? model.chat_template.slice(0, 40) + '…' : model.chat_template)
                : '—'}
            </span>
            <button
              class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all"
              onclick={(e) => openPopover('chat_template', model?.chat_template || '', e.currentTarget)}
              title="Edit chat template"
            >
              <Eye size={14} />
            </button>
          </div>
        </div>
        {/if}

        {#if !preset.jinja}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="grammar" class="text-sm">Grammar</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">BNF-like grammar constraint</p>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-muted-foreground truncate max-w-32">{preset.grammar ? (preset.grammar.length > 30 ? preset.grammar.slice(0, 30) + '…' : preset.grammar) : '—'}</span>
            <button
              class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all"
              onclick={(e) => openPopover('grammar', preset.grammar, e.currentTarget)}
              title="Edit grammar"
            >
              <FileText size={14} />
            </button>
          </div>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="grammar_file" class="text-sm">Grammar File</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Path to grammar file</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="grammar_file" placeholder="/path/to/grammar.gbnf" class="w-28" value={preset.grammar_file}
              oninput={(e) => { preset.grammar_file = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-all whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickGrammarFile();
                if (path) {
                  preset.grammar_file = path;
                  debouncedSave();
                }
              }}
              title="Browse for grammar file"
            >
              Browse
            </button>
          </div>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="json_schema" class="text-sm">JSON Schema</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">JSON schema to constrain generations</p>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-muted-foreground truncate max-w-32">{preset.json_schema ? (preset.json_schema.length > 30 ? preset.json_schema.slice(0, 30) + '…' : preset.json_schema) : '—'}</span>
            <button
              class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all"
              onclick={(e) => openPopover('json_schema', preset.json_schema, e.currentTarget)}
              title="Edit JSON schema"
            >
              <FileJson size={14} />
            </button>
          </div>
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
      <ModelSpeculative {preset} {debouncedSave} />

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

<!-- Popover editor -->
{#if popoverField && popoverPos}
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div class="fixed inset-0 z-50" onclick={closePopover} role="presentation"></div>
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
  <div
    class="fixed z-50 w-80 rounded-lg border border-border bg-popover shadow-xl"
    style="top: {popoverPos.top}px; left: {popoverPos.left}px;"
    onclick={(e) => e.stopPropagation()}
    role="dialog"
    aria-label="Editor"
    tabindex="-1"
  >
    <div class="px-3 py-2 border-b border-border">
      <span class="text-xs font-semibold uppercase tracking-wider text-muted-foreground">
        {popoverField === 'chat_template' ? 'Chat Template' : popoverField === 'grammar' ? 'Grammar (BNF)' : 'JSON Schema'}
      </span>
    </div>
    <div class="p-2">
      <textarea
        class="w-full h-40 rounded-md border border-border bg-background p-2 text-xs font-mono resize-none focus:outline-none focus:ring-1 focus:ring-ring"
        bind:value={popoverValue}
        placeholder={popoverField === 'chat_template' ? 'Jinja chat template…' : popoverField === 'grammar' ? 'root ::= ...' : '{ }'}
      ></textarea>
    </div>
    <div class="flex items-center justify-end gap-2 px-3 py-2 border-t border-border">
      <button
        class="px-3 py-1 rounded-md text-xs text-muted-foreground hover:text-foreground hover:bg-accent transition-colors"
        onclick={closePopover}
      >
        Cancel
      </button>
      <button
        class="px-3 py-1 rounded-md text-xs bg-primary text-primary-foreground hover:bg-primary/80 transition-colors"
        onclick={savePopover}
      >
        Save
      </button>
    </div>
  </div>
{/if}

<!-- Log viewer -->
<LogViewer open={logOpen} onclose={() => logOpen = false} />
