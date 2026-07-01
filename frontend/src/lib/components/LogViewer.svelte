<script lang="ts">
  import { Terminal, RefreshCw, X } from '@lucide/svelte';
  import { api } from '$lib/saucer';
  import { t } from '$lib/i18n';

  let { open = $bindable(false) }: { open: boolean } = $props();

  let lines = $state<string[]>([]);
  let container: HTMLDivElement | undefined = $state();
  let autoScroll = $state(true);

  // ponytail: non-reactive locals — avoid triggering $effect when set by async callback
  let logPath = '';
  // ponytail: intentionally non-reactive — pollTimer is only used in $effect cleanup and fetchLog callback
  let pollTimer: ReturnType<typeof setInterval> | undefined;
  let fetching = false;

  async function fetchLog() {
    if (fetching) return;
    fetching = true;
    try {
      if (!logPath) {
        logPath = await api.getServerLogPath().catch(() => '');
      }
      if (!logPath) return;
      const data = await api.readLog(logPath, 500);
      lines = data;
    } catch {
      // IPC error — no-op, next poll will retry
    } finally {
      fetching = false;
    }
  }

  function scrollToBottom() {
    if (container && autoScroll) {
      container.scrollTop = container.scrollHeight;
    }
  }

  // ── React to open/close ─────────────────────────────────────────
  $effect(() => {
    if (open) {
      logPath = '';
      lines = [];
      fetching = false;
      fetchLog();
      pollTimer = setInterval(fetchLog, 2000);
    }

    return () => {
      if (pollTimer !== undefined) {
        clearInterval(pollTimer);
        pollTimer = undefined;
      }
    };
  });

  // ── Auto-scroll on new lines ─────────────────────────────────────
  $effect(() => {
    if (lines.length > 0) {
      requestAnimationFrame(scrollToBottom);
    }
  });

  function handleScroll() {
    if (!container) return;
    const threshold = 40;
    autoScroll = (container.scrollHeight - container.scrollTop - container.clientHeight) < threshold;
  }

  function copyAll() {
    navigator.clipboard.writeText(lines.join('\n'));
  }
</script>

<!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
{#if open}
  <!-- Backdrop -->
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div
    class="fixed inset-0 z-50 bg-black/40"
    onclick={() => open = false}
    role="presentation"
  ></div>
  <!-- Panel -->
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
  <div
    class="fixed bottom-0 left-0 right-0 z-50 h-64 md:h-80 border-t border-border bg-black text-green-400 font-mono text-xs flex flex-col"
    onclick={(e) => e.stopPropagation()}
    role="dialog"
    aria-label="Server log viewer"
    tabindex="-1"
  >
    <!-- Header -->
    <div class="flex items-center justify-between px-3 py-1.5 border-b border-border/50 bg-zinc-900 shrink-0">
      <div class="flex items-center gap-2">
        <Terminal size={14} />
        <span class="text-[11px] text-green-300 font-medium uppercase tracking-wider">{$t('log.title')}</span>
        {#if lines.length > 0}
          <span class="text-[10px] text-muted-foreground">{$t('log.lines', { n: lines.length })}</span>
        {/if}
      </div>
      <div class="flex items-center gap-1">
        <button
          class="flex items-center justify-center h-6 w-6 rounded hover:bg-white/10 text-green-300/70 hover:text-green-300 transition-colors"
          onclick={copyAll}
          title={$t('log.copyAll')}
        >
          <span class="text-[10px]">📋</span>
        </button>
        <button
          class="flex items-center justify-center h-6 w-6 rounded hover:bg-white/10 text-green-300/70 hover:text-green-300 transition-colors"
          onclick={fetchLog}
          title={$t('log.refresh')}
        >
          <RefreshCw size={12} />
        </button>
        <button
          class="flex items-center justify-center h-6 w-6 rounded hover:bg-white/10 text-muted-foreground hover:text-foreground transition-colors"
          onclick={() => open = false}
          title={$t('log.close')}
        >
          <X size={14} />
        </button>
      </div>
    </div>
    <!-- Terminal area -->
    <div
      class="flex-1 overflow-y-auto p-3 leading-relaxed whitespace-pre-wrap break-all select-text"
      style="-webkit-font-smoothing: antialiased; backface-visibility: hidden;"
      class:opacity-60={lines.length === 0}
      bind:this={container}
      onscroll={handleScroll}
    >
      {#if lines.length === 0}
        <div class="flex items-center justify-center h-full text-muted-foreground text-[11px]">
          <span>{$t('log.waiting')}</span>
        </div>
      {:else}
        {#each lines as line}
          <div>{line}</div>
        {/each}
        <div class="h-2"></div>
      {/if}
    </div>
    <!-- Status bar -->
    <div class="flex items-center gap-3 px-3 py-1 border-t border-border/50 bg-zinc-900 text-[10px] text-muted-foreground shrink-0">
      <span class="flex items-center gap-1">
        <span class="w-1.5 h-1.5 rounded-full bg-green-500"></span>
        {pollTimer ? $t('log.live') : $t('log.stopped')}
      </span>
      <span>{autoScroll ? $t('log.autoScrollOn') : $t('log.autoScrollOff')}</span>
    </div>
  </div>
{/if}
