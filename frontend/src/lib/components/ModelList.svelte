<script lang="ts">
  import { models, view, selectedModel, serverStatus, serverModel, settings } from '$lib/stores.svelte';
  import ServerButton from '$lib/components/ServerButton.svelte';
  import { api } from '$lib/saucer';
  import { get } from 'svelte/store';
  import { t } from '$lib/i18n';
  import { toast } from 'svelte-sonner';
  import { Search, ArrowUpDown, ChevronDown } from '@lucide/svelte';
  import { cn } from '$lib/utils';
  import type { ModelInfo } from '$lib/types';

  let loading = $state<string | null>(null); // model name being toggled

  // ── Search & sort ────────────────────────────────────────────────
  let searchQuery = $state('');
  let sortMode = $state<'name-asc' | 'name-desc' | 'size-desc' | 'size-asc' | 'ctx-desc' | 'ctx-asc'>('name-asc');
  let sortOpen = $state(false);

  const sortOptions: { value: typeof sortMode; label: () => string }[] = [
    { value: 'name-asc', label: () => $t('modelList.sort.nameAsc') },
    { value: 'name-desc', label: () => $t('modelList.sort.nameDesc') },
    { value: 'size-desc', label: () => $t('modelList.sort.sizeDesc') },
    { value: 'size-asc', label: () => $t('modelList.sort.sizeAsc') },
    { value: 'ctx-desc', label: () => $t('modelList.sort.ctxDesc') },
    { value: 'ctx-asc', label: () => $t('modelList.sort.ctxAsc') },
  ];

  function parseSizeGb(s: string): number {
    const m = s.match(/^([\d.]+)\s*GB/i);
    return m ? parseFloat(m[1]) : 0;
  }

  const filtered = $derived.by(() => {
    let list = [...$models];

    // Hide draft/MTP models from the main list
    list = list.filter((m) => {
      const name = m.name.toLowerCase();
      return !name.includes('draft') && !name.includes('mtp');
    });

    // Filter
    if (searchQuery.trim()) {
      const q = searchQuery.toLowerCase();
      list = list.filter((m) =>
        m.name.toLowerCase().includes(q) ||
        m.architecture.toLowerCase().includes(q) ||
        m.quantization.toLowerCase().includes(q)
      );
    }

    // Sort
    list.sort((a, b) => {
      let cmp = 0;
      switch (sortMode) {
        case 'name-asc':  cmp = a.name.localeCompare(b.name); break;
        case 'name-desc': cmp = b.name.localeCompare(a.name); break;
        case 'size-desc': cmp = parseSizeGb(b.size) - parseSizeGb(a.size); break;
        case 'size-asc':  cmp = parseSizeGb(a.size) - parseSizeGb(b.size); break;
        case 'ctx-desc':  cmp = (parseInt(b.context_length) || 0) - (parseInt(a.context_length) || 0); break;
        case 'ctx-asc':   cmp = (parseInt(a.context_length) || 0) - (parseInt(b.context_length) || 0); break;
      }
      return cmp;
    });

    return list;
  });

  function modelDisplayName(m: ModelInfo): string {
    return m.display_name && m.display_name.trim() !== '' ? m.display_name : m.name;
  }

  async function handlePlayStop(e: MouseEvent, model: ModelInfo) {
    e.stopPropagation();
    loading = model.name;

    try {
      const status = await api.serverStatus();
      const current = status.status;

      if (current === 'stopped') {
        const s = get(settings);
        if (!s.llama_server_path) {
          toast.error($t('toast.setPath'));
          loading = null;
          return;
        }
        const result = await api.startServer(model.name, model.path);
        if (result === 'server_not_stopped') {
          toast.error($t('toast.serverNotStopped'));
        } else if (result === 'server_not_found') {
          toast.error($t('toast.serverNotFound', { path: s.llama_server_path }));
        } else {
          serverModel.set(modelDisplayName(model));
          serverStatus.set('starting');
        }
      } else if (current === 'running' && status.model === modelDisplayName(model)) {
        await api.stopServer();
        const deadline = Date.now() + 8000;
        while (Date.now() < deadline) {
          const st = await api.serverStatus();
          if (st.status === 'stopped') break;
          await new Promise(r => setTimeout(r, 200));
        }
        serverStatus.set('stopped');
        serverModel.set('');
        toast.info($t('toast.stopped', { name: modelDisplayName(model) }));
      } else if (current === 'running') {
        toast.warning($t('toast.anotherRunning'), {
          action: {
            label: $t('toast.stopAndStart'),
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
      toast.error($t('toast.failedControl'));
    } finally {
      loading = null;
    }
  }

  function selectModel(model: ModelInfo) {
    selectedModel.set(model);
    view.set('detail');
  }

  function closeSort() { sortOpen = false; }
</script>

<div class="flex-1 overflow-y-auto p-4" style="-webkit-font-smoothing: antialiased; backface-visibility: hidden;">
  {#if $models.length === 0}
    <div class="flex flex-col items-center justify-center h-full text-muted-foreground">
      <div class="text-4xl mb-2">📂</div>
      <p class="text-sm">{$t('modelList.empty')}</p>
      <p class="text-xs mt-1">{$t('modelList.emptyHint')}</p>
    </div>
  {:else}
    <!-- Search & Sort bar -->
    {#snippet sortDropdown()}
      <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
      {#if sortOpen}
        <div
          class="fixed inset-0 z-40"
          onclick={closeSort}
          role="presentation"
        ></div>
        <div class="absolute top-full right-0 mt-1 z-50 w-40 rounded-lg border border-border bg-popover shadow-lg py-1">
          {#each sortOptions as opt}
            <button
              class="flex items-center w-full px-3 py-1.5 text-xs text-left hover:bg-accent transition-colors"
              class:font-semibold={sortMode === opt.value}
              class:text-foreground={sortMode === opt.value}
              onclick={() => { sortMode = opt.value; closeSort(); }}
            >
              {opt.label()}
            </button>
          {/each}
        </div>
      {/if}
    {/snippet}

    <div class="relative flex items-center gap-2 mb-3">
      <!-- Search input -->
      <div class="relative flex-1">
        <Search size={14} class="absolute left-2.5 top-1/2 -translate-y-1/2 text-muted-foreground pointer-events-none" />
        <input
          type="text"
          placeholder={$t('modelList.searchPlaceholder')}
          aria-label={$t('modelList.searchLabel')}
          bind:value={searchQuery}
          class="w-full h-8 pl-8 pr-3 rounded-md border border-border bg-card text-xs placeholder:text-muted-foreground focus:outline-none focus:ring-1 focus:ring-ring transition-colors"
        />
      </div>

      {#if filtered.length !== $models.length}
        <span class="text-[10px] text-muted-foreground whitespace-nowrap">{$t('modelList.of', { n: filtered.length, total: $models.length })}</span>
      {/if}

      <!-- Sort dropdown trigger -->
      <div class="relative">
        <button
          class="flex items-center gap-1 h-8 px-2 rounded-md border border-border bg-card hover:bg-accent text-xs text-muted-foreground transition-colors"
          onclick={() => sortOpen = !sortOpen}
        >
          <ArrowUpDown size={12} />
          <ChevronDown size={10} />
        </button>
        {@render sortDropdown()}
      </div>
    </div>

    <div class="space-y-2">
      {#each filtered as model (model.name)}
        <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
        <div
          class="flex items-center justify-between p-3 rounded-lg border border-border bg-card hover:bg-accent/50 cursor-pointer transition-colors"
          onclick={() => selectModel(model)}
          onkeydown={(e) => { if (e.key === 'Enter') selectModel(model); }}
          role="button"
          tabindex="0"
        >
          <div class="min-w-0 flex-1">
            <div class="text-sm font-medium truncate">{model.display_name || model.name}</div>
            <div class="text-xs text-muted-foreground mt-0.5">
              {#if model.architecture}
                <span class="font-mono uppercase tracking-tight text-[10px]">{model.architecture}</span>
                <span class="mx-1.5 opacity-40">·</span>
              {/if}
              {model.quantization} · {model.size}
            </div>
          </div>

          <ServerButton
            modelName={modelDisplayName(model)}
            serverModelName={$serverModel}
            serverStatus={$serverStatus}
            loading={loading === model.name}
            onclick={(e) => handlePlayStop(e, model)}
            class="ml-3"
          />
        </div>
      {/each}

      {#if filtered.length === 0}
        <div class="flex items-center justify-center py-8 text-muted-foreground text-sm">
          {$t('modelList.noMatch', { query: searchQuery })}
        </div>
      {/if}
    </div>
  {/if}
</div>
