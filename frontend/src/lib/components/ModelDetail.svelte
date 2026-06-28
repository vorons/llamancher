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
    // New fields from comprehensive spec
    predict: -1,
    json_schema_file: '',
    cache_type_k_draft: '',
    mmproj: '',
    rope_scaling: 'none',
    rope_scale: 1.0,
    yarn_attn_factor: 1.0,
    yarn_beta_fast: 1.0,
    yarn_beta_slow: 1.0,
    yarn_orig_ctx: 0,
    lora: '',
    lora_scaled: '',
    control_vector: '',
    metrics: false,
    offline: false,
    slot_save_path: '',
    sleep_idle_seconds: 0,
    webui: true,
    embedding: false,
    rerank: false,
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
    const pw = 320;
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
    else if (popoverField === 'chat_template') preset.chat_template = popoverValue;
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

  // Derived: max context from model metadata
  const modelMaxCtx = $derived(Math.max(512, pf(model?.context_length, 131072)));
  const isSpecActive = $derived(preset.spec_type !== 'none');
  const isDraftMode = $derived(preset.spec_type === 'draft-model' || preset.spec_type === 'draft-mtp');
  const isNgramMode = $derived(preset.spec_type.startsWith('ngram-'));
  const isRopeYarn = $derived(preset.rope_scaling === 'yarn');
  const hideSampling = $derived(preset.embedding || preset.rerank);
  const showTensorSplit = $derived(preset.split_mode !== 'none');
  const showGrammar = $derived(!preset.jinja);
  const showChatTemplate = $derived(preset.jinja);

  $effect(() => {
    if (model) {
      const modelName = model.name;
      loadingPreset = true;
      api.loadPreset(modelName).then((kv) => {
        if (model?.name !== modelName) {
          loadingPreset = false;
          return;
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
          // New fields
          predict: pi(kv.predict, -1),
          json_schema_file: kv.json_schema_file || '',
          cache_type_k_draft: kv.cache_type_k_draft || '',
          mmproj: kv.mmproj || '',
          rope_scaling: kv.rope_scaling || 'none',
          rope_scale: pf(kv.rope_scale, 1.0),
          yarn_attn_factor: pf(kv.yarn_attn_factor, 1.0),
          yarn_beta_fast: pf(kv.yarn_beta_fast, 1.0),
          yarn_beta_slow: pf(kv.yarn_beta_slow, 1.0),
          yarn_orig_ctx: pi(kv.yarn_orig_ctx, 0),
          lora: kv.lora || '',
          lora_scaled: kv.lora_scaled || '',
          control_vector: kv.control_vector || '',
          metrics: kv.metrics === 'true',
          offline: kv.offline === 'true',
          slot_save_path: kv.slot_save_path || '',
          sleep_idle_seconds: pi(kv.sleep_idle_seconds, 0),
          webui: kv.webui !== 'false',
          embedding: kv.embedding === 'true',
          rerank: kv.rerank === 'true',
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
      // New fields
      predict: String(preset.predict),
      json_schema_file: preset.json_schema_file,
      cache_type_k_draft: preset.cache_type_k_draft,
      mmproj: preset.mmproj,
      rope_scaling: preset.rope_scaling,
      rope_scale: String(preset.rope_scale),
      yarn_attn_factor: String(preset.yarn_attn_factor),
      yarn_beta_fast: String(preset.yarn_beta_fast),
      yarn_beta_slow: String(preset.yarn_beta_slow),
      yarn_orig_ctx: String(preset.yarn_orig_ctx),
      lora: preset.lora,
      lora_scaled: preset.lora_scaled,
      control_vector: preset.control_vector,
      metrics: preset.metrics ? 'true' : 'false',
      offline: preset.offline ? 'true' : 'false',
      slot_save_path: preset.slot_save_path,
      sleep_idle_seconds: String(preset.sleep_idle_seconds),
      webui: preset.webui ? 'true' : 'false',
      embedding: preset.embedding ? 'true' : 'false',
      rerank: preset.rerank ? 'true' : 'false',
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

  function ctxStep(ctx: number): number {
    // round to nice steps based on magnitude
    if (ctx <= 4096) return 256;
    if (ctx <= 32768) return 1024;
    return 4096;
  }

  const nCores = navigator.hardwareConcurrency || 4;
</script>

{#if model}
  <!-- ponytail: in WebKitGTK GPU compositing layers rasterise text at lower quality during scroll.
       No transform/will-change — let the CPU rasterizer (Cairo/Pango) handle text at native resolution.
       Antialiasing prevents sub-pixel re-rasterization, isolation creates a stacking context. -->
  <div class="flex-1 overflow-y-auto p-4 space-y-5" style="-webkit-font-smoothing: antialiased; text-rendering: optimizeLegibility; isolation: isolate;">
    {#if loadingPreset}
      <div class="flex items-center justify-center h-32">
        <Loader2 size={24} class="animate-spin text-muted-foreground" />
      </div>
    {:else}

      <!-- ════ Model header ════ -->
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
            class="flex items-center justify-center h-8 w-8 rounded-md border border-border bg-secondary hover:bg-accent text-muted-foreground hover:text-foreground transition-colors active:scale-95"
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
        {#if model.has_vision === 'true' || model.has_audio === 'true'}
          <span class="inline-flex items-center gap-1 rounded-md bg-secondary px-2 py-0.5 text-[11px] font-medium text-muted-foreground">
            {model.has_vision === 'true' ? 'Vision' : ''}
            {model.has_audio === 'true' ? 'Audio' : ''}
          </span>
        {/if}
      </div>

      <Separator />

      <!-- ============================================================ -->
      <!-- БЛОК 1: ОСНОВНЫЕ ПАРАМЕТРЫ (Main)                          -->
      <!-- ============================================================ -->
      <!-- ponytail: contain:layout limits paint to each block — off-screen sections aren't re-rasterised -->
      <div class="[contain:layout]">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">Основные параметры</h3>

        <!-- Model & Performance -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Производительность</div>

        <!-- ctx-size: slider 512 → modelMaxCtx -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="ctx_size" class="text-sm">Context Size <span class="font-mono text-[10px] text-muted-foreground">-c, --ctx-size</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Размер контекстного окна в токенах</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="ctx_size" type="single"
              value={preset.ctx_size}
              onValueChange={(v) => { preset.ctx_size = v; debouncedSave(); }}
              min={512} max={modelMaxCtx} step={ctxStep(modelMaxCtx)} />
            <span class="text-sm tabular-nums w-14 text-right shrink-0">{preset.ctx_size}</span>
          </div>
        </div>

        <!-- predict: input -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="predict" class="text-sm">Predict <span class="font-mono text-[10px] text-muted-foreground">-n, --predict</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Максимальное число токенов для генерации (-1 = без лимита)</p>
          </div>
          <Input id="predict" type="number" class="w-36" value={String(preset.predict)}
            oninput={(e) => { preset.predict = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
        </div>

        <!-- parallel: input (-1 = auto) -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="parallel" class="text-sm">Parallel Slots <span class="font-mono text-[10px] text-muted-foreground">-np, --parallel</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Количество слотов для параллельных запросов (-1 = по умолчанию)</p>
          </div>
          <Input id="parallel" type="number" min="-1" class="w-36" value={String(preset.parallel)}
            oninput={(e) => { preset.parallel = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
        </div>

        <!-- gpu-layers: slider 0-100 (0=auto, 100=all) -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="gpu_layers" class="text-sm">GPU Layers <span class="font-mono text-[10px] text-muted-foreground">-ngl, --gpu-layers</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Слоёв в VRAM. 0 = auto, 100 = all</p>
          </div>
          <div class="flex items-center gap-2 w-44 shrink-0">
            <Slider id="gpu_layers" type="single"
              value={preset.gpu_layers}
              onValueChange={(v) => { preset.gpu_layers = v; debouncedSave(); }}
              min={0} max={100} />
            <span class="text-sm tabular-nums w-8 text-right shrink-0">{preset.gpu_layers === 0 ? 'auto' : preset.gpu_layers}</span>
          </div>
        </div>

        <!-- threads: slider 1-64 -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="threads" class="text-sm">Threads <span class="font-mono text-[10px] text-muted-foreground">-t, --threads</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Потоков CPU. Ядер: {nCores}, по умолчанию {Math.max(1, nCores - 2)}</p>
          </div>
          <div class="flex items-center gap-2 w-44 shrink-0">
            <Slider id="threads" type="single"
              value={preset.threads}
              onValueChange={(v) => { preset.threads = v; debouncedSave(); }}
              min={1} max={64} step={1} />
            <span class="text-sm tabular-nums w-8 text-right shrink-0">{preset.threads}</span>
          </div>
        </div>

        <!-- batch-size: input -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="batch_size" class="text-sm">Batch Size <span class="font-mono text-[10px] text-muted-foreground">-b, --batch-size</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Логический максимум батча (по умолч. 2048)</p>
          </div>
          <Input id="batch_size" type="number" class="w-36" value={String(preset.batch_size)}
            oninput={(e) => { preset.batch_size = parseInt(e.currentTarget.value) || 2048; debouncedSave(); }} />
        </div>

        <Separator class="my-2" />

        <!-- Attention -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Внимание</div>

        <!-- flash-attn: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="flash_attn" class="text-sm">Flash Attention <span class="font-mono text-[10px] text-muted-foreground">-fa, --flash-attn</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Ускорение и экономия VRAM</p>
          </div>
          <Switch checked={preset.flash_attn} onCheckedChange={(c) => { preset.flash_attn = c; debouncedSave(); }} />
        </div>

        <Separator class="my-2" />

        <!-- Sampling (hidden when embedding/rerank mode) -->
        {#if !hideSampling}
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Сэмплирование</div>

        <!-- temperature: slider 0.0-2.0 -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="temp" class="text-sm">Temperature <span class="font-mono text-[10px] text-muted-foreground">--temp</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Креативность выборки (0.0 = детерминированно)</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="temp" type="single" value={preset.temp}
              onValueChange={(v) => { preset.temp = v; debouncedSave(); }}
              min={0} max={2} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.temp.toFixed(2)}</span>
          </div>
        </div>

        <!-- top-k: input -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="top_k" class="text-sm">Top K <span class="font-mono text-[10px] text-muted-foreground">--top-k</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Top-k sampling (0 = отключено)</p>
          </div>
          <Input id="top_k" type="number" class="w-36" value={String(preset.top_k)}
            oninput={(e) => { preset.top_k = parseInt(e.currentTarget.value) || 40; debouncedSave(); }} />
        </div>

        <!-- top-p: slider 0.0-1.0 -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="top_p" class="text-sm">Top P <span class="font-mono text-[10px] text-muted-foreground">--top-p</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Nucleus sampling — кумулятивная вероятность</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="top_p" type="single" value={preset.top_p}
              onValueChange={(v) => { preset.top_p = v; debouncedSave(); }}
              min={0} max={1} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.top_p.toFixed(2)}</span>
          </div>
        </div>

        <Separator class="my-2" />

        <!-- Memory / Penalty -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Память и штрафы</div>
        {/if}

        <!-- mmap/no-mmap: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="mmap" class="text-sm">Memory Mapping <span class="font-mono text-[10px] text-muted-foreground">--mmap / --no-mmap</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Memory-map модель (рекомендуется с mlock)</p>
          </div>
          <Switch checked={!preset.no_mmap} onCheckedChange={(c) => { preset.no_mmap = !c; debouncedSave(); }}
            title={preset.no_mmap ? 'mmap выключен (--no-mmap)' : 'mmap включён (--mmap)'} />
        </div>

        <!-- mlock: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="mlock" class="text-sm">mlock <span class="font-mono text-[10px] text-muted-foreground">--mlock</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Зафиксировать в RAM без свопа</p>
          </div>
          <Switch checked={preset.mlock} onCheckedChange={(c) => { preset.mlock = c; debouncedSave(); }} />
        </div>

        {#if !hideSampling}
        <!-- repeat-penalty: slider 1.0-2.0 -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="repeat_penalty" class="text-sm">Repeat Penalty <span class="font-mono text-[10px] text-muted-foreground">--repeat-penalty</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Штраф за повторения (1.0 = выключен)</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="repeat_penalty" type="single" value={preset.repeat_penalty}
              onValueChange={(v) => { preset.repeat_penalty = v; debouncedSave(); }}
              min={1.0} max={2.0} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.repeat_penalty.toFixed(2)}</span>
          </div>
        </div>

        <!-- Other sampling params not in tables but kept for completeness -->
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
            <Label for="samplers" class="text-sm">Sampler Chain</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Порядок сэмплеров (; разделитель)</p>
          </div>
          <Input id="samplers" placeholder="top_k;top_p;temperature" class="w-36" value={preset.samplers}
            oninput={(e) => { preset.samplers = e.currentTarget.value; debouncedSave(); }} />
        </div>
        {/if}
      </div>

      <Separator />

      <!-- ============================================================ -->
      <!-- БЛОК 2: ДОПОЛНИТЕЛЬНЫЕ ПАРАМЕТРЫ (Additional)               -->
      <!-- ============================================================ -->
      <div class="[contain:layout]">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">Дополнительные параметры</h3>

        <!-- min-p: slider 0.0-1.0 -->
        {#if !hideSampling}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="min_p" class="text-sm">Min P <span class="font-mono text-[10px] text-muted-foreground">--min-p</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Минимальная вероятность для выбора токена</p>
          </div>
          <div class="flex items-center gap-2 w-48 shrink-0">
            <Slider id="min_p" type="single" value={preset.min_p}
              onValueChange={(v) => { preset.min_p = v; debouncedSave(); }}
              min={0} max={1} step={0.01} />
            <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.min_p.toFixed(2)}</span>
          </div>
        </div>
        {/if}

        <!-- seed: input -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="seed" class="text-sm">Seed <span class="font-mono text-[10px] text-muted-foreground">--seed</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Зерно RNG (-1 = случайно)</p>
          </div>
          <Input id="seed" type="number" placeholder="-1" class="w-36" value={String(preset.seed)}
            oninput={(e) => { preset.seed = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
        </div>

        <Separator class="my-2" />

        <!-- GPU / Multi-GPU -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">GPU и распределение</div>

        <!-- split-mode: select -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="split_mode" class="text-sm">Split Mode <span class="font-mono text-[10px] text-muted-foreground">-sm, --split-mode</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Режим разделения модели по GPU</p>
          </div>
          <NativeSelect id="split_mode" class="w-36" value={preset.split_mode} onchange={(e) => { preset.split_mode = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="none">None</NativeSelectOption>
            <NativeSelectOption value="layer">Layer</NativeSelectOption>
            <NativeSelectOption value="row">Row</NativeSelectOption>
          </NativeSelect>
        </div>

        <!-- tensor-split: input (shown when split_mode != none) -->
        {#if showTensorSplit}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="tensor_split" class="text-sm">Tensor Split <span class="font-mono text-[10px] text-muted-foreground">-ts, --tensor-split</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Пропорции тензоров между GPU (напр. 3,1)</p>
          </div>
          <Input id="tensor_split" placeholder="e.g. 3,1" class="w-36" value={preset.tensor_split}
            oninput={(e) => { preset.tensor_split = e.currentTarget.value; debouncedSave(); }} />
        </div>
        {/if}

        <!-- Additional loading params (kept for completeness) -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="main_gpu" class="text-sm">Main GPU</Label>
          <Input id="main_gpu" type="number" placeholder="0" class="w-36" value={String(preset.main_gpu)}
            oninput={(e) => { preset.main_gpu = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="device" class="text-sm">Devices</Label>
          <Input id="device" placeholder="Vulkan0,Vulkan1" class="w-36" value={preset.device}
            oninput={(e) => { preset.device = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="numa" class="text-sm">NUMA</Label>
          <NativeSelect id="numa" class="w-36" value={preset.numa} onchange={(e) => { preset.numa = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="">Off</NativeSelectOption>
            <NativeSelectOption value="distribute">Distribute</NativeSelectOption>
            <NativeSelectOption value="isolate">Isolate</NativeSelectOption>
          </NativeSelect>
        </div>

        <Separator class="my-2" />

        <!-- KV Cache -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">KV-кэш</div>

        <!-- cache-type-k: select -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="cache_type_k" class="text-sm">Cache Type K <span class="font-mono text-[10px] text-muted-foreground">-ctk, --cache-type-k</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Тип данных KV кэша для K</p>
          </div>
          <NativeSelect id="cache_type_k" class="w-36" value={preset.cache_type_k} onchange={(e) => { preset.cache_type_k = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="">Default (f16)</NativeSelectOption>
            <NativeSelectOption value="f16">f16</NativeSelectOption>
            <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
            <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
            <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
          </NativeSelect>
        </div>

        <!-- cache-type-v: select -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="cache_type_v" class="text-sm">Cache Type V <span class="font-mono text-[10px] text-muted-foreground">-ctv, --cache-type-v</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Тип данных KV кэша для V</p>
          </div>
          <NativeSelect id="cache_type_v" class="w-36" value={preset.cache_type_v} onchange={(e) => { preset.cache_type_v = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="">Default (f16)</NativeSelectOption>
            <NativeSelectOption value="f16">f16</NativeSelectOption>
            <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
            <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
            <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
          </NativeSelect>
        </div>

        <Separator class="my-2" />

        <!-- Schema / Template -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Схемы и шаблоны</div>

        <!-- json-schema: popover -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="json_schema" class="text-sm">JSON Schema <span class="font-mono text-[10px] text-muted-foreground">-j, --json-schema</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">JSON-схема для ограничения генерации</p>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-muted-foreground truncate max-w-28">{preset.json_schema ? (preset.json_schema.length > 30 ? preset.json_schema.slice(0, 30) + '…' : preset.json_schema) : '—'}</span>
            <button
              class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-colors"
              onclick={(e) => openPopover('json_schema', preset.json_schema, e.currentTarget)}
              title="Edit JSON schema"
            >
              <FileJson size={14} />
            </button>
          </div>
        </div>

        <!-- json-schema-file: file picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="json_schema_file" class="text-sm">JSON Schema File <span class="font-mono text-[10px] text-muted-foreground">-jf, --json-schema-file</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Файл с JSON-схемой</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="json_schema_file" placeholder="/path/to/schema.json" class="w-28" value={preset.json_schema_file}
              oninput={(e) => { preset.json_schema_file = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickFile();
                if (path) { preset.json_schema_file = path; debouncedSave(); }
              }}
              title="Browse for JSON schema file"
            >
              Browse
            </button>
          </div>
        </div>

        <Separator class="my-2" />

        <!-- Template engine -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Шаблонизатор</div>

        <!-- jinja: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="jinja" class="text-sm">Jinja <span class="font-mono text-[10px] text-muted-foreground">--jinja / --no-jinja</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Использовать Jinja-шаблоны для чата</p>
          </div>
          <Switch checked={preset.jinja} onCheckedChange={(c) => { preset.jinja = c; debouncedSave(); }} />
        </div>

        <!-- grammar: popover (shown when jinja OFF) -->
        {#if showGrammar}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="grammar" class="text-sm">Grammar</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">BNF-грамматика (GBNF)</p>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-muted-foreground truncate max-w-28">{preset.grammar ? (preset.grammar.length > 30 ? preset.grammar.slice(0, 30) + '…' : preset.grammar) : '—'}</span>
            <button
              class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-colors"
              onclick={(e) => openPopover('grammar', preset.grammar, e.currentTarget)}
              title="Edit grammar"
            >
              <FileText size={14} />
            </button>
          </div>
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="grammar_file" class="text-sm">Grammar File</Label>
          <div class="flex items-center gap-2">
            <Input id="grammar_file" placeholder="/path/to/grammar.gbnf" class="w-28" value={preset.grammar_file}
              oninput={(e) => { preset.grammar_file = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickGrammarFile();
                if (path) { preset.grammar_file = path; debouncedSave(); }
              }}
              title="Browse for grammar file"
            >
              Browse
            </button>
          </div>
        </div>
        {/if}

        <!-- chat-template: popover (shown when jinja ON) -->
        {#if showChatTemplate}
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="chat_template" class="text-sm">Chat Template</Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Jinja-шаблон чата (из метаданных модели)</p>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-muted-foreground truncate max-w-28">
              {model?.chat_template
                ? (model.chat_template.length > 30 ? model.chat_template.slice(0, 30) + '…' : model.chat_template)
                : (preset.chat_template ? (preset.chat_template.length > 30 ? preset.chat_template.slice(0, 30) + '…' : preset.chat_template) : '—')}
            </span>
            <button
              class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-colors"
              onclick={(e) => openPopover('chat_template', preset.chat_template || model?.chat_template || '', e.currentTarget)}
              title="Edit chat template"
            >
              <Eye size={14} />
            </button>
          </div>
        </div>
        {/if}

        <Separator class="my-2" />

        <!-- Speculative decoding -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Спекулятивное декодирование</div>

        <!-- spec-type: select -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="spec_type" class="text-sm">Spec Type <span class="font-mono text-[10px] text-muted-foreground">--spec-type</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Метод спекулятивного декодирования</p>
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
          <!-- draft-max: input (shown when spec active) -->
          <div class="flex items-center justify-between gap-4 py-1.5">
            <div class="min-w-0">
              <Label for="spec_draft_n_max" class="text-sm">Draft N Max <span class="font-mono text-[10px] text-muted-foreground">--draft-max</span></Label>
              <p class="text-[11px] leading-tight text-muted-foreground">Кол-во токенов для спекулятивного декодирования</p>
            </div>
            <Input id="spec_draft_n_max" type="number" class="w-36" value={String(preset.spec_draft_n_max)}
              oninput={(e) => { preset.spec_draft_n_max = parseInt(e.currentTarget.value) || 16; debouncedSave(); }} />
          </div>

          {#if isDraftMode}
            <!-- model-draft: file picker -->
            <div class="flex items-center justify-between gap-4 py-1.5">
              <div class="min-w-0">
                <Label for="draft_model" class="text-sm">Draft Model <span class="font-mono text-[10px] text-muted-foreground">-md, --model-draft</span></Label>
                <p class="text-[11px] leading-tight text-muted-foreground">Модель для спекулятивного декодирования</p>
              </div>
              <div class="flex items-center gap-2">
                <Input id="draft_model" placeholder="/path/to/draft.gguf" class="w-28" value={preset.draft_model}
                  oninput={(e) => { preset.draft_model = e.currentTarget.value; debouncedSave(); }} />
                <button
                  class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
                  onclick={async () => {
                    const path = await api.pickFile();
                    if (path) { preset.draft_model = path; debouncedSave(); }
                  }}
                  title="Browse for draft model"
                >
                  Browse
                </button>
              </div>
            </div>

            <!-- cache-type-k-draft: select -->
            <div class="flex items-center justify-between gap-4 py-1.5">
              <div class="min-w-0">
                <Label for="cache_type_k_draft" class="text-sm">Cache Type K Draft <span class="font-mono text-[10px] text-muted-foreground">-ctkd, --cache-type-k-draft</span></Label>
                <p class="text-[11px] leading-tight text-muted-foreground">Тип KV кэша для draft-модели</p>
              </div>
              <NativeSelect id="cache_type_k_draft" class="w-36" value={preset.cache_type_k_draft} onchange={(e) => { preset.cache_type_k_draft = e.currentTarget.value; debouncedSave(); }}>
                <NativeSelectOption value="">Default (f16)</NativeSelectOption>
                <NativeSelectOption value="f16">f16</NativeSelectOption>
                <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
                <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
                <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
              </NativeSelect>
            </div>
          {/if}

          <!-- Deeper spec params (from ModelSpeculative) -->
          <ModelSpeculative {preset} {debouncedSave} />
        {/if}

        <Separator class="my-2" />

        <!-- Multi-modal -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Мультимодальность</div>

        <!-- mmproj: file picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="mmproj" class="text-sm">MMProj <span class="font-mono text-[10px] text-muted-foreground">-mm, --mmproj</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Файл проектора для мультимодальных моделей</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="mmproj" placeholder="/path/to/mmproj.gguf" class="w-28" value={preset.mmproj}
              oninput={(e) => { preset.mmproj = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickFile();
                if (path) { preset.mmproj = path; debouncedSave(); }
              }}
              title="Browse for multimodal projector"
            >
              Browse
            </button>
          </div>
        </div>

        <Separator class="my-2" />

        <!-- Server mode flags (for dependency logic) -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Режим сервера</div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="embedding" class="text-sm">Embedding <span class="font-mono text-[10px] text-muted-foreground">--embedding</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Включить эндпоинт эмбеддингов (скрывает сэмплирование)</p>
          </div>
          <Switch checked={preset.embedding} onCheckedChange={(c) => { preset.embedding = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="rerank" class="text-sm">Rerank <span class="font-mono text-[10px] text-muted-foreground">--rerank</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Включить эндпоинт ранжирования (скрывает сэмплирование)</p>
          </div>
          <Switch checked={preset.rerank} onCheckedChange={(c) => { preset.rerank = c; debouncedSave(); }} />
        </div>
      </div>

      <Separator />

      <!-- ============================================================ -->
      <!-- БЛОК 3: СПЕЦИАЛИЗИРОВАННЫЕ ПАРАМЕТРЫ (Specialized)          -->
      <!-- ============================================================ -->
      <div class="[contain:layout]">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">Специализированные параметры</h3>

        <!-- RoPE -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">RoPE (расширение контекста)</div>

        <!-- rope-scaling: select -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="rope_scaling" class="text-sm">RoPE Scaling <span class="font-mono text-[10px] text-muted-foreground">--rope-scaling</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Масштабирование RoPE для длинного контекста</p>
          </div>
          <NativeSelect id="rope_scaling" class="w-36" value={preset.rope_scaling} onchange={(e) => { preset.rope_scaling = e.currentTarget.value; debouncedSave(); }}>
            <NativeSelectOption value="none">None</NativeSelectOption>
            <NativeSelectOption value="linear">Linear</NativeSelectOption>
            <NativeSelectOption value="yarn">YaRN</NativeSelectOption>
          </NativeSelect>
        </div>

        {#if preset.rope_scaling !== 'none'}
        <!-- rope-scale: input -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="rope_scale" class="text-sm">RoPE Scale <span class="font-mono text-[10px] text-muted-foreground">--rope-scale</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Коэффициент масштабирования (по умолч. 1.0)</p>
          </div>
          <Input id="rope_scale" type="number" step="0.25" min="1.0" class="w-36" value={String(preset.rope_scale)}
            oninput={(e) => { preset.rope_scale = parseFloat(e.currentTarget.value) || 1.0; debouncedSave(); }} />
        </div>
        {/if}

        {#if isRopeYarn}
        <!-- YaRN params: group -->
        <div class="border-l-2 border-border pl-3 ml-0.5 mt-0.5 space-y-1">
          <p class="text-[10px] font-medium text-muted-foreground/70">YaRN (Yet another RoPE extensioN)</p>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="yarn_attn_factor" class="text-sm">Attn Factor <span class="font-mono text-[10px] text-muted-foreground">--yarn-attn-factor</span></Label>
            <Input id="yarn_attn_factor" type="number" step="0.1" class="w-36" value={String(preset.yarn_attn_factor)}
              oninput={(e) => { preset.yarn_attn_factor = parseFloat(e.currentTarget.value) || 1.0; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="yarn_beta_fast" class="text-sm">Beta Fast <span class="font-mono text-[10px] text-muted-foreground">--yarn-beta-fast</span></Label>
            <Input id="yarn_beta_fast" type="number" step="0.1" class="w-36" value={String(preset.yarn_beta_fast)}
              oninput={(e) => { preset.yarn_beta_fast = parseFloat(e.currentTarget.value) || 1.0; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="yarn_beta_slow" class="text-sm">Beta Slow <span class="font-mono text-[10px] text-muted-foreground">--yarn-beta-slow</span></Label>
            <Input id="yarn_beta_slow" type="number" step="0.1" class="w-36" value={String(preset.yarn_beta_slow)}
              oninput={(e) => { preset.yarn_beta_slow = parseFloat(e.currentTarget.value) || 1.0; debouncedSave(); }} />
          </div>
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="yarn_orig_ctx" class="text-sm">Orig Ctx <span class="font-mono text-[10px] text-muted-foreground">--yarn-orig-ctx</span></Label>
            <Input id="yarn_orig_ctx" type="number" placeholder="0 (auto)" class="w-36" value={String(preset.yarn_orig_ctx)}
              oninput={(e) => { preset.yarn_orig_ctx = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
          </div>
        </div>
        {/if}

        <Separator class="my-2" />

        <!-- Adapters -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Адаптеры</div>

        <!-- lora: file picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="lora" class="text-sm">LoRA <span class="font-mono text-[10px] text-muted-foreground">--lora</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Путь к адаптеру LoRA</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="lora" placeholder="/path/to/lora.gguf" class="w-28" value={preset.lora}
              oninput={(e) => { preset.lora = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickFile();
                if (path) { preset.lora = path; debouncedSave(); }
              }}
              title="Browse for LoRA adapter"
            >
              Browse
            </button>
          </div>
        </div>

        <!-- lora-scaled: file picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="lora_scaled" class="text-sm">LoRA Scaled <span class="font-mono text-[10px] text-muted-foreground">--lora-scaled</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Путь + масштаб: /path,lora_scale</p>
          </div>
          <Input id="lora_scaled" placeholder="/path,scale" class="w-36" value={preset.lora_scaled}
            oninput={(e) => { preset.lora_scaled = e.currentTarget.value; debouncedSave(); }} />
        </div>

        <!-- control-vector: file picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="control_vector" class="text-sm">Control Vector <span class="font-mono text-[10px] text-muted-foreground">--control-vector</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Вектор управления для стиля/поведения</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="control_vector" placeholder="/path/to/control.gguf" class="w-28" value={preset.control_vector}
              oninput={(e) => { preset.control_vector = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickFile();
                if (path) { preset.control_vector = path; debouncedSave(); }}
              }
              title="Browse for control vector"
            >
              Browse
            </button>
          </div>
        </div>

        <Separator class="my-2" />

        <!-- Monitoring -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Мониторинг и логирование</div>

        <!-- metrics: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="metrics" class="text-sm">Metrics <span class="font-mono text-[10px] text-muted-foreground">--metrics</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Prometheus-эндпоинт метрик</p>
          </div>
          <Switch checked={preset.metrics} onCheckedChange={(c) => { preset.metrics = c; debouncedSave(); }} />
        </div>

        <!-- verbose: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="verbose" class="text-sm">Verbose <span class="font-mono text-[10px] text-muted-foreground">-v, --verbose</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Максимальный уровень логирования</p>
          </div>
          <Switch checked={preset.verbose} onCheckedChange={(c) => { preset.verbose = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="verbosity" class="text-sm">Verbosity</Label>
          <Input id="verbosity" type="number" min="0" max="5" class="w-36" value={String(preset.verbosity)}
            oninput={(e) => { preset.verbosity = Math.min(5, Math.max(0, parseInt(e.currentTarget.value) || 0)); debouncedSave(); }} />
        </div>

        <!-- log-file: file picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="log_file" class="text-sm">Log File <span class="font-mono text-[10px] text-muted-foreground">--log-file</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Запись логов в файл</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="log_file" placeholder="/path/to/llama.log" class="w-28" value={preset.log_file}
              oninput={(e) => { preset.log_file = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickFile();
                if (path) { preset.log_file = path; debouncedSave(); }}
              }
              title="Browse for log file"
            >
              Browse
            </button>
          </div>
        </div>

        <Separator class="my-2" />

        <!-- Server behaviour -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Режимы сервера</div>

        <!-- offline: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="offline" class="text-sm">Offline <span class="font-mono text-[10px] text-muted-foreground">--offline</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Режим офлайн (только кэш)</p>
          </div>
          <Switch checked={preset.offline} onCheckedChange={(c) => { preset.offline = c; debouncedSave(); }} />
        </div>

        <!-- slot-save-path: folder picker -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="slot_save_path" class="text-sm">Slot Save Path <span class="font-mono text-[10px] text-muted-foreground">--slot-save-path</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Путь для сохранения KV кэша слотов</p>
          </div>
          <div class="flex items-center gap-2">
            <Input id="slot_save_path" placeholder="/path/to/slots" class="w-28" value={preset.slot_save_path}
              oninput={(e) => { preset.slot_save_path = e.currentTarget.value; debouncedSave(); }} />
            <button
              class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
              onclick={async () => {
                const path = await api.pickFolder();
                if (path) { preset.slot_save_path = path; debouncedSave(); }}
              }
              title="Browse for slots directory"
            >
              Browse
            </button>
          </div>
        </div>

        <!-- sleep-idle-seconds: input -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="sleep_idle_seconds" class="text-sm">Sleep Idle <span class="font-mono text-[10px] text-muted-foreground">--sleep-idle-seconds</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Таймаут простоя до сна (0 = отключено)</p>
          </div>
          <Input id="sleep_idle_seconds" type="number" min="0" class="w-36" value={String(preset.sleep_idle_seconds)}
            oninput={(e) => { preset.sleep_idle_seconds = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
        </div>

        <!-- webui: switch -->
        <div class="flex items-center justify-between gap-4 py-1.5">
          <div class="min-w-0">
            <Label for="webui" class="text-sm">Web UI <span class="font-mono text-[10px] text-muted-foreground">--webui / --no-webui</span></Label>
            <p class="text-[11px] leading-tight text-muted-foreground">Встроенный веб-интерфейс llama-server</p>
          </div>
          <Switch checked={preset.webui} onCheckedChange={(c) => { preset.webui = c; debouncedSave(); }} />
        </div>

        <Separator class="my-2" />

        <!-- Other pre-existing params not in the three tables -->
        <div class="text-[11px] font-medium text-muted-foreground mb-1">Прочее</div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="no_repack" class="text-sm">No Repack</Label>
          <Switch checked={preset.no_repack} onCheckedChange={(c) => { preset.no_repack = c; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="ubatch_size" class="text-sm">UBatch Size</Label>
          <Input id="ubatch_size" type="number" class="w-36" value={String(preset.ubatch_size)}
            oninput={(e) => { preset.ubatch_size = parseInt(e.currentTarget.value) || 512; debouncedSave(); }} />
        </div>

        <div class="flex items-center justify-between gap-4 py-1.5">
          <Label for="defrag_thold" class="text-sm">Defrag Threshold</Label>
          <Input id="defrag_thold" type="number" placeholder="-1" class="w-36" value={String(preset.defrag_thold)}
            oninput={(e) => { preset.defrag_thold = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
        </div>

        <!-- Auto-fit section -->
        <div class="mt-3 border-t border-border pt-3">
          <div class="text-[11px] font-medium text-muted-foreground mb-1">Auto-fit</div>

          <div class="flex items-center justify-between gap-4 py-1.5">
            <div class="min-w-0">
              <Label for="fit" class="text-sm">Fit</Label>
              <p class="text-[11px] leading-tight text-muted-foreground">Auto-tune params based on available memory</p>
            </div>
            <Switch checked={preset.fit} onCheckedChange={(c) => { preset.fit = c; debouncedSave(); }} />
          </div>

          {#if !preset.fit}
          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="fit_target_mib" class="text-sm">Fit Target MiB</Label>
            <Input id="fit_target_mib" placeholder="1024" class="w-36" value={preset.fit_target_mib}
              oninput={(e) => { preset.fit_target_mib = e.currentTarget.value; debouncedSave(); }} />
          </div>

          <div class="flex items-center justify-between gap-4 py-1.5">
            <Label for="fit_ctx" class="text-sm">Fit Min Ctx</Label>
            <Input id="fit_ctx" type="number" class="w-36" value={String(preset.fit_ctx)}
              oninput={(e) => { preset.fit_ctx = parseInt(e.currentTarget.value) || 4096; debouncedSave(); }} />
          </div>
          {/if}
        </div>
      </div>

    {/if}
  </div>
{/if}

<!-- Popover editor overlay -->
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
        {popoverField === 'chat_template' ? 'Chat Template' : popoverField === 'grammar' ? 'Grammar (GBNF)' : 'JSON Schema'}
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
