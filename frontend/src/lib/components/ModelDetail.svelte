<script lang="ts">
  import { selectedModel, serverStatus, serverModel, settings } from '$lib/stores.svelte';
  import ServerButton from '$lib/components/ServerButton.svelte';
  import { Camera, ExternalLink, Headphones, Loader2, Terminal, Eye, Plus, X } from '@lucide/svelte';
  import { api } from '$lib/saucer';
  import { toast } from 'svelte-sonner';
  import { get } from 'svelte/store';
  import LogViewer from './LogViewer.svelte';
  import ModelSpeculative from './ModelSpeculative.svelte';

  import { t } from '$lib/i18n';
  import { Separator } from '$lib/ui/separator';
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { Switch } from '$lib/ui/switch';
  import { Slider } from '$lib/ui/slider';
  import { NativeSelect, NativeSelectOption } from '$lib/ui/native-select';
  import { Button } from '$lib/ui/button';
  import type { ModelInfo, Preset, CustomArg } from '$lib/types';

  let preset = $state<Preset>({
    ctx_size: 2048, threads: 4, threads_batch: 0, gpu_layers: 0,
    batch_size: 2048, ubatch_size: 512, mmproj: '', cache_type_k: '', cache_type_v: '',
    parallel: 1, timeout: 0, seed: -1, flash_attn: false, mlock: false, no_mmap: false,
    temp: 0.80, predict: -1, min_p: 0.05, top_k: 40, top_p: 0.95,
    repeat_penalty: 1.00, presence_penalty: 0.00, frequency_penalty: 0.00,
    reasoning_mode: false, reasoning_budget: 0,
    spec_type: '', draft_model: '', hf_repo_draft: '', draft_gpu_layers: 0,
    spec_draft_n_max: 16, spec_draft_n_min: 0, spec_draft_p_split: 0.50, spec_draft_p_min: 0.0,
    cont_batching: true, webui: true, embedding: false, slots: true,
    metrics: false, cache_prompt: true, context_shift: true, offline: false, alias: '',
    custom_args: [],
  });

  let loadingPreset = $state(true);
  let serverLoading = $state(false);
  let saveTimer: ReturnType<typeof setTimeout> | undefined;
  let logOpen = $state(false);
  let popoverField = $state<'chat_template' | null>(null);
  let popoverValue = $state('');
  let popoverPos = $state<{ top: number; left: number } | null>(null);

  function openPopover(value: string, el: HTMLElement) {
    popoverField = 'chat_template';
    popoverValue = value;
    const rect = el.getBoundingClientRect();
    const pw = 360;
    let left = Math.round(rect.right) - pw;
    left = Math.max(8, Math.min(left, window.innerWidth - pw - 8));
    popoverPos = { top: Math.round(rect.bottom) + 4, left };
  }

  function closePopover() { popoverField = null; popoverPos = null; }

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

  const modelMaxCtx = $derived(Math.max(512, pf(model?.context_length, 131072)));
  const hideSampling = $derived(preset.embedding);
  const nCores = navigator.hardwareConcurrency || 4;

  $effect(() => {
    if (model) {
      const modelName = model.name;
      loadingPreset = true;
      api.loadPreset(modelName).then((kv) => {
        if (model?.name !== modelName) { loadingPreset = false; return; }
        preset = {
          ctx_size: pi(kv.ctx_size, 2048),
          threads: pi(kv.threads, 4),
          threads_batch: pi(kv.threads_batch, 0),
          gpu_layers: pi(kv.gpu_layers, 0),
          batch_size: pi(kv.batch_size, 2048),
          ubatch_size: pi(kv.ubatch_size, 512),
          mmproj: kv.mmproj || '',
          cache_type_k: kv.cache_type_k || '',
          cache_type_v: kv.cache_type_v || '',
          parallel: pi(kv.parallel, 1),
          timeout: pi(kv.timeout, 0),
          seed: pi(kv.seed, -1),
          flash_attn: kv.flash_attn === 'true',
          mlock: kv.mlock === 'true',
          no_mmap: kv.no_mmap === 'true',
          temp: pf(kv.temp, 0.80),
          predict: pi(kv.predict, -1),
          min_p: pf(kv.min_p, 0.05),
          top_k: pi(kv.top_k, 40),
          top_p: pf(kv.top_p, 0.95),
          repeat_penalty: pf(kv.repeat_penalty, 1.00),
          presence_penalty: pf(kv.presence_penalty, 0.00),
          frequency_penalty: pf(kv.frequency_penalty, 0.00),
          reasoning_mode: kv.reasoning_mode === 'true',
          reasoning_budget: pi(kv.reasoning_budget, 0),
          spec_type: kv.spec_type || '',
          draft_model: kv.draft_model || '',
          hf_repo_draft: kv.hf_repo_draft || '',
          draft_gpu_layers: pi(kv.draft_gpu_layers, 0),
          spec_draft_n_max: pi(kv.spec_draft_n_max, 16),
          spec_draft_n_min: pi(kv.spec_draft_n_min, 0),
          spec_draft_p_split: pf(kv.spec_draft_p_split, 0.50),
          spec_draft_p_min: pf(kv.spec_draft_p_min, 0.00),
          cont_batching: kv.cont_batching !== 'false',
          webui: kv.webui !== 'false',
          embedding: kv.embedding === 'true',
          slots: kv.slots !== 'false',
          metrics: kv.metrics === 'true',
          cache_prompt: kv.cache_prompt !== 'false',
          context_shift: kv.context_shift !== 'false',
          offline: kv.offline === 'true',
          alias: kv.alias || '',
          custom_args: parseCustomArgs(kv.custom_args),
        };
        loadingPreset = false;
      }).catch(() => {
        if (model?.name !== modelName) { loadingPreset = false; return; }
        loadingPreset = false;
      });
    }
  });

  function parseCustomArgs(json?: string): CustomArg[] {
    if (!json || json === '[]') return [];
    try { return JSON.parse(json); } catch { return []; }
  }

  function savePayload(): Record<string, string> {
    return {
      ctx_size: String(preset.ctx_size),
      threads: String(preset.threads),
      threads_batch: String(preset.threads_batch),
      gpu_layers: String(preset.gpu_layers),
      batch_size: String(preset.batch_size),
      ubatch_size: String(preset.ubatch_size),
      mmproj: preset.mmproj,
      cache_type_k: preset.cache_type_k,
      cache_type_v: preset.cache_type_v,
      parallel: String(preset.parallel),
      timeout: String(preset.timeout),
      seed: String(preset.seed),
      flash_attn: preset.flash_attn ? 'true' : 'false',
      mlock: preset.mlock ? 'true' : 'false',
      no_mmap: preset.no_mmap ? 'true' : 'false',
      temp: String(preset.temp),
      predict: String(preset.predict),
      min_p: String(preset.min_p),
      top_k: String(preset.top_k),
      top_p: String(preset.top_p),
      repeat_penalty: String(preset.repeat_penalty),
      presence_penalty: String(preset.presence_penalty),
      frequency_penalty: String(preset.frequency_penalty),
      reasoning_mode: preset.reasoning_mode ? 'true' : 'false',
      reasoning_budget: String(preset.reasoning_budget),
      spec_type: preset.spec_type,
      draft_model: preset.draft_model,
      hf_repo_draft: preset.hf_repo_draft,
      draft_gpu_layers: String(preset.draft_gpu_layers),
      spec_draft_n_max: String(preset.spec_draft_n_max),
      spec_draft_n_min: String(preset.spec_draft_n_min),
      spec_draft_p_split: String(preset.spec_draft_p_split),
      spec_draft_p_min: String(preset.spec_draft_p_min),
      cont_batching: preset.cont_batching ? 'true' : 'false',
      webui: preset.webui ? 'true' : 'false',
      embedding: preset.embedding ? 'true' : 'false',
      slots: preset.slots ? 'true' : 'false',
      metrics: preset.metrics ? 'true' : 'false',
      cache_prompt: preset.cache_prompt ? 'true' : 'false',
      context_shift: preset.context_shift ? 'true' : 'false',
      offline: preset.offline ? 'true' : 'false',
      alias: preset.alias,
      custom_args: JSON.stringify(preset.custom_args),
    };
  }

  function debouncedSave() {
    const modelName = model?.name;
    if (saveTimer) clearTimeout(saveTimer);
    saveTimer = setTimeout(() => {
      if (modelName && model?.name === modelName) {
        api.savePreset(modelName, savePayload());
        if ($serverStatus === 'running' && modelDisplayName(model) && $serverModel === modelDisplayName(model)) {
          toast.info($t('toast.changesApplyRestart'));
        }
      }
    }, 300);
  }

  function addCustomArg() {
    preset.custom_args = [...preset.custom_args, { arg: '', enabled: true }];
    debouncedSave();
  }

  function removeCustomArg(i: number) {
    preset.custom_args = preset.custom_args.filter((_, idx) => idx !== i);
    debouncedSave();
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
          toast.error($t('toast.setPath'));
          return;
        }
        await api.savePreset(model.name, savePayload());
        const result = await api.startServer(model.name, model.path);
        if (result === 'server_not_found') {
          toast.error($t('toast.serverNotFound'));
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
        toast.warning($t('toast.anotherRunning'), {
          action: {
            label: $t('toast.stopAndStart'),
            onClick: async () => {
              await api.stopServer();
              setTimeout(async () => { await api.startServer(model.name, model.path); }, 500);
            },
          },
        });
      }
    } catch { toast.error($t('toast.failedOp')); }
    finally { serverLoading = false; }
  }

  function ctxStep(ctx: number): number {
    if (ctx <= 4096) return 256;
    if (ctx <= 32768) return 1024;
    return 4096;
  }
</script>

{#if model}
<div class="flex-1 overflow-y-auto p-4 space-y-5"
     style="-webkit-font-smoothing: antialiased; backface-visibility: hidden; text-rendering: optimizeLegibility; isolation: isolate;">

  {#if loadingPreset}
    <div class="flex items-center justify-center h-32">
      <Loader2 size={24} class="animate-spin text-muted-foreground" />
    </div>
  {:else}

    <!-- ════ Model header ════ -->
    <div class="flex items-start justify-between gap-4">
      <div class="min-w-0">
        <h2 class="text-lg font-semibold truncate">
          {model.name}
          {#if model.has_vision === 'true'}
            <span class="inline-flex items-center align-middle ml-1.5" title="Vision support">
              <Camera size={15} class="text-muted-foreground" />
            </span>
          {/if}
          {#if model.has_audio === 'true'}
            <span class="inline-flex items-center align-middle ml-1" title="Audio support">
              <Headphones size={15} class="text-muted-foreground" />
            </span>
          {/if}
        </h2>
        <div class="text-xs text-muted-foreground mt-0.5">
          {#if model.architecture}
            <span class="font-mono uppercase tracking-tight text-[10px]">{model.architecture}</span>
            <span class="mx-1.5 opacity-40">·</span>
          {/if}
          {model.quantization} · {model.size}
        </div>
      </div>
      <div class="flex items-center gap-2 shrink-0">
        <button class="flex items-center justify-center h-8 w-8 rounded-md border border-border bg-secondary hover:bg-accent text-muted-foreground hover:text-foreground transition-colors active:scale-95"
          onclick={() => logOpen = true} title={$t('detail.serverLogs')}>
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

    <!-- Model info -->
    <div class="rounded-lg border border-border bg-card divide-y divide-border/50 text-xs">
      <!-- Network architecture -->
      <div class="p-3 space-y-1">
        <h3 class="text-[11px] font-semibold uppercase tracking-wider text-foreground/80 mb-1.5">{$t('detail.network')}</h3>
        <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-x-5 gap-y-1">
          {#if model.block_count && model.block_count !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.layers')}</span>
              <span class="tabular-nums font-medium">{model.block_count}</span>
            </div>
          {/if}
          {#if model.embedding_length && model.embedding_length !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.embedding')}</span>
              <span class="tabular-nums font-medium">{model.embedding_length}</span>
            </div>
          {/if}
          {#if model.head_count && model.head_count !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.heads')}</span>
              <span class="tabular-nums font-medium">{model.head_count}</span>
            </div>
          {/if}
          {#if model.head_count_kv && model.head_count_kv !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.kvHeads')}</span>
              <span class="tabular-nums font-medium">{model.head_count_kv}</span>
            </div>
          {/if}
          {#if model.feed_forward_length && model.feed_forward_length !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.ffLength')}</span>
              <span class="tabular-nums font-medium">{model.feed_forward_length}</span>
            </div>
          {/if}
          {#if model.vocab_size && model.vocab_size !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.vocab')}</span>
              <span class="tabular-nums font-medium">{model.vocab_size}</span>
            </div>
          {/if}
          {#if model.expert_count && model.expert_count !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.experts')}</span>
              <span class="tabular-nums font-medium">{model.expert_count}{#if model.expert_used_count && model.expert_used_count !== '0'}/{model.expert_used_count}{/if}</span>
            </div>
          {/if}
          {#if model.context_length && model.context_length !== '0'}
            <div class="flex items-baseline gap-1.5">
              <span class="text-muted-foreground text-[11px] shrink-0">{$t('detail.context')}</span>
              <span class="tabular-nums font-medium">{model.context_length}</span>
            </div>
          {/if}
        </div>
      </div>

      <!-- File -->
      <div class="p-3 space-y-1">
        <h3 class="text-[11px] font-semibold uppercase tracking-wider text-foreground/80 mb-1.5">{$t('detail.file')}</h3>
        <div class="font-mono text-[11px] truncate text-muted-foreground" title={model.path}>
          {model.path || '—'}
        </div>
        <div class="text-muted-foreground">
          {model.size}
          {#if model.file_type && model.file_type !== '0'}
            · GGUF Type {model.file_type}
          {/if}
        </div>
      </div>

      <!-- Source -->
      {#if model.author || model.license || model.version || model.url || model.source_url || model.languages}
      <div class="p-3 space-y-1">
        <h3 class="text-[11px] font-semibold uppercase tracking-wider text-foreground/80 mb-1.5">{$t('detail.source')}</h3>
        {#if model.author}
          <div class="text-muted-foreground flex"><span class="w-16 shrink-0">{$t('detail.author')}</span>{model.author}</div>
        {/if}
        {#if model.license}
          <div class="text-muted-foreground flex"><span class="w-16 shrink-0">{$t('detail.license')}</span>{model.license}</div>
        {/if}
        {#if model.version}
          <div class="text-muted-foreground flex"><span class="w-16 shrink-0">{$t('detail.version')}</span>{model.version}</div>
        {/if}
        {#if model.languages}
          <div class="text-muted-foreground flex"><span class="w-16 shrink-0">{$t('detail.languages')}</span>{model.languages}</div>
        {/if}
        {#if model.source_url || model.url}
          <a href={model.source_url || model.url} target="_blank" rel="noopener noreferrer"
             class="flex items-center gap-1 text-blue-400 hover:underline truncate mt-1">
            <ExternalLink size={11} class="shrink-0" />
            <span class="truncate">{model.source_url || model.url}</span>
          </a>
        {/if}
      </div>
      {/if}

      <!-- Chat template -->
      {#if model.chat_template}
      <div class="p-3">
        <button
          class="flex items-center gap-1.5 text-muted-foreground hover:text-foreground transition-colors"
          onclick={(e) => openPopover(model.chat_template!, e.currentTarget)}
          title="View chat template"
        >
          <Eye size={12} />
          <span>{$t('detail.viewChatTemplate')}</span>
        </button>
      </div>
      {/if}
    </div>

    <Separator />

    <!-- ============================================================ -->
    <!-- Генерация                                                    -->
    <!-- ============================================================ -->
    {#if !hideSampling}
    <div class="[contain:layout]">
      <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">{$t('detail.section.gen')}</h3>

      <!-- Temperature -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="temp" class="text-sm">{$t('detail.field.temp')} <span class="font-mono text-[10px] text-muted-foreground">--temp</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.temp.desc')}</p>
        </div>
        <div class="flex items-center gap-2 w-48 shrink-0">
          <Slider id="temp" type="single" value={preset.temp}
            onValueChange={(v) => { preset.temp = v; debouncedSave(); }}
            min={0} max={2} step={0.01} />
          <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.temp.toFixed(2)}</span>
        </div>
      </div>

      <!-- Context Size -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="ctx_size" class="text-sm">{$t('detail.field.ctxSize')} <span class="font-mono text-[10px] text-muted-foreground">-c, --ctx-size</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.ctxSize.desc')}</p>
        </div>
        <div class="flex items-center gap-2 w-48 shrink-0">
          <Slider id="ctx_size" type="single" value={preset.ctx_size}
            onValueChange={(v) => { preset.ctx_size = v; debouncedSave(); }}
            min={512} max={modelMaxCtx} step={ctxStep(modelMaxCtx)} />
          <span class="text-sm tabular-nums w-14 text-right shrink-0">{preset.ctx_size}</span>
        </div>
      </div>

      <!-- Predict -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="predict" class="text-sm">{$t('detail.field.predict')} <span class="font-mono text-[10px] text-muted-foreground">-n, --predict</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.predict.desc')}</p>
        </div>
        <Input id="predict" type="number" class="w-36" value={String(preset.predict)}
          oninput={(e) => { preset.predict = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
      </div>

      <!-- Min-P -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="min_p" class="text-sm">{$t('detail.field.minP')} <span class="font-mono text-[10px] text-muted-foreground">--min-p</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.minP.desc')}</p>
        </div>
        <div class="flex items-center gap-2 w-48 shrink-0">
          <Slider id="min_p" type="single" value={preset.min_p}
            onValueChange={(v) => { preset.min_p = v; debouncedSave(); }}
            min={0} max={1} step={0.01} />
          <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.min_p.toFixed(2)}</span>
        </div>
      </div>

      <!-- Top-K -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="top_k" class="text-sm">{$t('detail.field.topK')} <span class="font-mono text-[10px] text-muted-foreground">--top-k</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.topK.desc')}</p>
        </div>
        <Input id="top_k" type="number" class="w-36" value={String(preset.top_k)}
          oninput={(e) => { preset.top_k = parseInt(e.currentTarget.value) || 40; debouncedSave(); }} />
      </div>

      <!-- Top-P -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="top_p" class="text-sm">{$t('detail.field.topP')} <span class="font-mono text-[10px] text-muted-foreground">--top-p</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.topP.desc')}</p>
        </div>
        <div class="flex items-center gap-2 w-48 shrink-0">
          <Slider id="top_p" type="single" value={preset.top_p}
            onValueChange={(v) => { preset.top_p = v; debouncedSave(); }}
            min={0} max={1} step={0.01} />
          <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.top_p.toFixed(2)}</span>
        </div>
      </div>

      <!-- Repeat Penalty -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="repeat_penalty" class="text-sm">{$t('detail.field.repeatPenalty')} <span class="font-mono text-[10px] text-muted-foreground">--repeat-penalty</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.repeatPenalty.desc')}</p>
        </div>
        <div class="flex items-center gap-2 w-48 shrink-0">
          <Slider id="repeat_penalty" type="single" value={preset.repeat_penalty}
            onValueChange={(v) => { preset.repeat_penalty = v; debouncedSave(); }}
            min={1.0} max={2.0} step={0.01} />
          <span class="text-sm tabular-nums w-10 text-right shrink-0">{preset.repeat_penalty.toFixed(2)}</span>
        </div>
      </div>

      <!-- Presence Penalty -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="presence_penalty" class="text-sm">{$t('detail.field.presencePenalty')} <span class="font-mono text-[10px] text-muted-foreground">--presence-penalty</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.presencePenalty.desc')}</p>
        </div>
        <Input id="presence_penalty" type="number" step="0.01" class="w-36" value={String(preset.presence_penalty)}
          oninput={(e) => { preset.presence_penalty = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }} />
      </div>

      <!-- Frequency Penalty -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="frequency_penalty" class="text-sm">{$t('detail.field.frequencyPenalty')} <span class="font-mono text-[10px] text-muted-foreground">--frequency-penalty</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.frequencyPenalty.desc')}</p>
        </div>
        <Input id="frequency_penalty" type="number" step="0.01" class="w-36" value={String(preset.frequency_penalty)}
          oninput={(e) => { preset.frequency_penalty = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }} />
      </div>

      <!-- Reasoning Mode -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="reasoning_mode" class="text-sm">{$t('detail.field.reasoning')} <span class="font-mono text-[10px] text-muted-foreground">-rea, --reasoning</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.reasoning.desc')}</p>
        </div>
        <Switch checked={preset.reasoning_mode} onCheckedChange={(c) => { preset.reasoning_mode = c; debouncedSave(); }} />
      </div>

      {#if preset.reasoning_mode}
      <div class="flex items-center justify-between gap-4 py-1.5 pl-4">
        <div class="min-w-0">
          <Label for="reasoning_budget" class="text-sm">{$t('detail.field.reasoningBudget')} <span class="font-mono text-[10px] text-muted-foreground">--reasoning-budget</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.reasoningBudget.desc')}</p>
        </div>
        <Input id="reasoning_budget" type="number" class="w-36" value={String(preset.reasoning_budget)}
          oninput={(e) => { preset.reasoning_budget = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
      </div>
      {/if}

      <!-- Seed -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="seed" class="text-sm">{$t('detail.field.seed')} <span class="font-mono text-[10px] text-muted-foreground">-s, --seed</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.seed.desc')}</p>
        </div>
        <Input id="seed" type="number" placeholder="-1" class="w-36" value={String(preset.seed)}
          oninput={(e) => { preset.seed = parseInt(e.currentTarget.value) || -1; debouncedSave(); }} />
      </div>
    </div>
    <Separator />
    {/if}

    <!-- ============================================================ -->
    <!-- Производительность                                           -->
    <!-- ============================================================ -->
    <div class="[contain:layout]">
      <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">{$t('detail.section.perf')}</h3>

      <!-- Threads -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="threads" class="text-sm">{$t('detail.field.threads')} <span class="font-mono text-[10px] text-muted-foreground">-t, --threads</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.threads.desc', { n: nCores })}</p>
        </div>
        <div class="flex items-center gap-2 w-44 shrink-0">
          <Slider id="threads" type="single" value={preset.threads}
            onValueChange={(v) => { preset.threads = v; debouncedSave(); }}
            min={1} max={64} step={1} />
          <span class="text-sm tabular-nums w-8 text-right shrink-0">{preset.threads}</span>
        </div>
      </div>

      <!-- Batch Threads -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="threads_batch" class="text-sm">{$t('detail.field.threadsBatch')} <span class="font-mono text-[10px] text-muted-foreground">-tb, --threads-batch</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.threadsBatch.desc')}</p>
        </div>
        <Input id="threads_batch" type="number" min="0" class="w-36" value={String(preset.threads_batch)}
          oninput={(e) => { preset.threads_batch = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
      </div>

      <!-- GPU Layers -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="gpu_layers" class="text-sm">{$t('detail.field.gpuLayers')} <span class="font-mono text-[10px] text-muted-foreground">-ngl, --gpu-layers</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.gpuLayers.desc')}</p>
        </div>
        <div class="flex items-center gap-2 w-44 shrink-0">
          <Slider id="gpu_layers" type="single" value={preset.gpu_layers}
            onValueChange={(v) => { preset.gpu_layers = v; debouncedSave(); }}
            min={0} max={100} />
          <span class="text-sm tabular-nums w-8 text-right shrink-0">
            {preset.gpu_layers === 0 ? 'auto' : preset.gpu_layers === 100 ? 'all' : preset.gpu_layers}
          </span>
        </div>
      </div>

      <!-- Batch Size -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="batch_size" class="text-sm">{$t('detail.field.batchSize')} <span class="font-mono text-[10px] text-muted-foreground">-b, --batch-size</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.batchSize.desc')}</p>
        </div>
        <Input id="batch_size" type="number" class="w-36" value={String(preset.batch_size)}
          oninput={(e) => { preset.batch_size = parseInt(e.currentTarget.value) || 2048; debouncedSave(); }} />
      </div>

      <!-- UBatch Size -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="ubatch_size" class="text-sm">{$t('detail.field.ubatchSize')} <span class="font-mono text-[10px] text-muted-foreground">-ub, --ubatch-size</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.ubatchSize.desc')}</p>
        </div>
        <Input id="ubatch_size" type="number" class="w-36" value={String(preset.ubatch_size)}
          oninput={(e) => { preset.ubatch_size = parseInt(e.currentTarget.value) || 512; debouncedSave(); }} />
      </div>

      <!-- Parallel Slots -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="parallel" class="text-sm">{$t('detail.field.parallel')} <span class="font-mono text-[10px] text-muted-foreground">-np, --parallel</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.parallel.desc')}</p>
        </div>
        <Input id="parallel" type="number" min="1" class="w-36" value={String(preset.parallel)}
          oninput={(e) => { preset.parallel = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
      </div>

      <!-- Timeout -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="timeout" class="text-sm">{$t('detail.field.timeout')} <span class="font-mono text-[10px] text-muted-foreground">-to, --timeout</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.timeout.desc')}</p>
        </div>
        <Input id="timeout" type="number" min="0" class="w-36" value={String(preset.timeout)}
          oninput={(e) => { preset.timeout = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
      </div>

      <!-- Flash Attention -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="flash_attn" class="text-sm">{$t('detail.field.flashAttn')} <span class="font-mono text-[10px] text-muted-foreground">-fa, --flash-attn</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.flashAttn.desc')}</p>
        </div>
        <Switch checked={preset.flash_attn} onCheckedChange={(c) => { preset.flash_attn = c; debouncedSave(); }} />
      </div>

      <!-- Memory Lock -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="mlock" class="text-sm">{$t('detail.field.mlock')} <span class="font-mono text-[10px] text-muted-foreground">--mlock</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.mlock.desc')}</p>
        </div>
        <Switch checked={preset.mlock} onCheckedChange={(c) => { preset.mlock = c; debouncedSave(); }} />
      </div>

      <!-- Memory Mapping -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="mmap" class="text-sm">{$t('detail.field.mmap')} <span class="font-mono text-[10px] text-muted-foreground">--mmap / --no-mmap</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.mmap.desc')}</p>
        </div>
        <Switch checked={!preset.no_mmap} onCheckedChange={(c) => { preset.no_mmap = !c; debouncedSave(); }}
          title={preset.no_mmap ? $t('detail.field.mmap.off') : $t('detail.field.mmap.on')} />
      </div>
    </div>
    <Separator />

    <!-- ============================================================ -->
    <!-- KV Cache                                                     -->
    <!-- ============================================================ -->
    <div class="[contain:layout]">
      <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">{$t('detail.section.kv')}</h3>

      <!-- Cache Type K -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="cache_type_k" class="text-sm">{$t('detail.field.cacheTypeK')} <span class="font-mono text-[10px] text-muted-foreground">-ctk, --cache-type-k</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.cacheTypeK.desc')}</p>
        </div>
        <NativeSelect id="cache_type_k" class="w-36" value={preset.cache_type_k} onchange={(e) => { preset.cache_type_k = e.currentTarget.value; debouncedSave(); }}>
          <NativeSelectOption value="">Default (f16)</NativeSelectOption>
          <NativeSelectOption value="f16">f16</NativeSelectOption>
          <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
          <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
          <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
        </NativeSelect>
      </div>

      <!-- Cache Type V -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="cache_type_v" class="text-sm">{$t('detail.field.cacheTypeV')} <span class="font-mono text-[10px] text-muted-foreground">-ctv, --cache-type-v</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.cacheTypeV.desc')}</p>
        </div>
        <NativeSelect id="cache_type_v" class="w-36" value={preset.cache_type_v} onchange={(e) => { preset.cache_type_v = e.currentTarget.value; debouncedSave(); }}>
          <NativeSelectOption value="">Default (f16)</NativeSelectOption>
          <NativeSelectOption value="f16">f16</NativeSelectOption>
          <NativeSelectOption value="q8_0">q8_0</NativeSelectOption>
          <NativeSelectOption value="q4_0">q4_0</NativeSelectOption>
          <NativeSelectOption value="q4_1">q4_1</NativeSelectOption>
        </NativeSelect>
      </div>
    </div>
    <Separator />

    <!-- ============================================================ -->
    <!-- Мультимодальность                                            -->
    <!-- ============================================================ -->
    <div class="[contain:layout]">
      <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">{$t('detail.section.mm')}</h3>

      <!-- MMProj -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="mmproj" class="text-sm">{$t('detail.field.mmproj')} <span class="font-mono text-[10px] text-muted-foreground">-mm, --mmproj</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.mmproj.desc')}</p>
        </div>
        <div class="flex items-center gap-2">
          <Input id="mmproj" placeholder="/path/to/mmproj.gguf" class="w-28" value={preset.mmproj}
            oninput={(e) => { preset.mmproj = e.currentTarget.value; debouncedSave(); }} />
          <Button variant="outline" size="sm"
            onclick={async () => { const p = await api.pickFile(); if (p) { preset.mmproj = p; debouncedSave(); } }}>
            {$t('settings.browse')}
          </Button>
        </div>
      </div>
    </div>
    <Separator />

    <!-- ============================================================ -->
    <!-- Дополнительно                                                -->
    <!-- ============================================================ -->
    <div class="[contain:layout]">
      <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">{$t('detail.section.extra')}</h3>

      <!-- Alias -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="alias" class="text-sm">{$t('detail.field.alias')} <span class="font-mono text-[10px] text-muted-foreground">-a, --alias</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.alias.desc')}</p>
        </div>
        <Input id="alias" placeholder="my-model" class="w-36" value={preset.alias}
          oninput={(e) => { preset.alias = e.currentTarget.value; debouncedSave(); }} />
      </div>

      <!-- Continuous Batching -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="cont_batching" class="text-sm">{$t('detail.field.contBatching')} <span class="font-mono text-[10px] text-muted-foreground">-cb, --cont-batching</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.contBatching.desc')}</p>
        </div>
        <Switch checked={preset.cont_batching} onCheckedChange={(c) => { preset.cont_batching = c; debouncedSave(); }} />
      </div>

      <!-- WebUI -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="webui" class="text-sm">{$t('detail.field.webui')} <span class="font-mono text-[10px] text-muted-foreground">--webui</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.webui.desc')}</p>
        </div>
        <Switch checked={preset.webui} onCheckedChange={(c) => { preset.webui = c; debouncedSave(); }} />
      </div>

      <!-- Embedding Mode -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="embedding" class="text-sm">{$t('detail.field.embedding')} <span class="font-mono text-[10px] text-muted-foreground">--embedding</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.embedding.desc')}</p>
        </div>
        <Switch checked={preset.embedding} onCheckedChange={(c) => { preset.embedding = c; debouncedSave(); }} />
      </div>

      <!-- Slots Management -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="slots" class="text-sm">{$t('detail.field.slots')} <span class="font-mono text-[10px] text-muted-foreground">--slots</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.slots.desc')}</p>
        </div>
        <Switch checked={preset.slots} onCheckedChange={(c) => { preset.slots = c; debouncedSave(); }} />
      </div>

      <!-- Metrics -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="metrics" class="text-sm">{$t('detail.field.metrics')} <span class="font-mono text-[10px] text-muted-foreground">--metrics</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.metrics.desc')}</p>
        </div>
        <Switch checked={preset.metrics} onCheckedChange={(c) => { preset.metrics = c; debouncedSave(); }} />
      </div>

      <!-- Cache Prompt -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="cache_prompt" class="text-sm">{$t('detail.field.cachePrompt')} <span class="font-mono text-[10px] text-muted-foreground">--cache-prompt</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.cachePrompt.desc')}</p>
        </div>
        <Switch checked={preset.cache_prompt} onCheckedChange={(c) => { preset.cache_prompt = c; debouncedSave(); }} />
      </div>

      <!-- Context Shift -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="context_shift" class="text-sm">{$t('detail.field.contextShift')} <span class="font-mono text-[10px] text-muted-foreground">--context-shift</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.contextShift.desc')}</p>
        </div>
        <Switch checked={preset.context_shift} onCheckedChange={(c) => { preset.context_shift = c; debouncedSave(); }} />
      </div>

      <!-- Offline Mode -->
      <div class="flex items-center justify-between gap-4 py-1.5">
        <div class="min-w-0">
          <Label for="offline_model" class="text-sm">{$t('detail.field.offline')} <span class="font-mono text-[10px] text-muted-foreground">{$t('detail.field.offlineFlag')}</span></Label>
          <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.offline.desc')}</p>
        </div>
        <Switch
          id="offline_model"
          checked={preset.offline}
          onCheckedChange={(c) => { preset.offline = c; debouncedSave(); }}
        />
      </div>
    </div>
    <Separator />

    <!-- ============================================================ -->
    <!-- Speculative Decoding                                         -->
    <!-- ============================================================ -->
    <div class="[contain:layout]">
      <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80 mb-3">{$t('detail.section.spec')}</h3>
      <ModelSpeculative {preset} {debouncedSave} />
    </div>
    <Separator />

    <!-- ============================================================ -->
    <!-- Custom CLI Arguments                                         -->
    <!-- ============================================================ -->
    <div class="[contain:layout]">
      <div class="flex items-center justify-between mb-3">
        <h3 class="text-xs font-semibold uppercase tracking-wider text-foreground/80">Custom CLI Arguments</h3>
        <Button variant="outline" size="sm" onclick={addCustomArg}>
          <Plus size={12} class="mr-1" /> Add
        </Button>
      </div>

      {#if preset.custom_args.length === 0}
        <p class="text-[11px] text-muted-foreground py-2">Нет дополнительных аргументов. Нажмите «Add» чтобы добавить.</p>
      {:else}
        {#each preset.custom_args as arg, i}
          <div class="flex items-center gap-2 py-1.5">
            <Switch checked={arg.enabled} onCheckedChange={(c) => { preset.custom_args[i].enabled = c; debouncedSave(); }} />
            <Input class="flex-1" placeholder="--flag=value" value={arg.arg}
              oninput={(e) => { preset.custom_args[i].arg = e.currentTarget.value; debouncedSave(); }} />
            <button class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-destructive/20 text-muted-foreground hover:text-destructive transition-colors"
              onclick={() => removeCustomArg(i)} title="Remove">
              <X size={13} />
            </button>
          </div>
        {/each}
      {/if}
    </div>

  {/if}
</div>

<!-- Chat Template Popover -->
{#if popoverField === 'chat_template' && popoverPos}
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
  <div class="fixed inset-0 z-40" onclick={closePopover}></div>
  <div class="fixed z-50 w-[360px] max-h-[300px] overflow-y-auto rounded-lg border border-border bg-popover p-3 shadow-lg text-xs font-mono"
       style="top: {popoverPos.top}px; left: {popoverPos.left}px;">
    <pre class="whitespace-pre-wrap break-all">{popoverValue || '—'}</pre>
  </div>
{/if}

<LogViewer bind:open={logOpen} />
{/if}
