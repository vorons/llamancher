<script lang="ts">
  import { view, selectedModel, serverStatus, serverModel, settingsOpen, scanning } from '$lib/stores.svelte';
  import { ArrowLeft, RefreshCw, Settings, Minus, Square, X } from '@lucide/svelte';
  import { cn } from '$lib/utils';
  import { Separator } from '$lib/ui/separator';

  function handleRefresh() {
    window.dispatchEvent(new CustomEvent('refresh-models'));
  }

  function goBack() {
    selectedModel.set(null);
    view.set('list');
  }

  const statusVariant = $derived.by(() => {
    switch ($serverStatus) {
      case 'running': return 'bg-green-500';
      case 'starting': return 'bg-yellow-500 animate-pulse';
      case 'error': return 'bg-red-500';
      default: return 'bg-gray-500';
    }
  });

  const headerTitle = $derived(
    $serverStatus === 'running' && $serverModel
      ? `${$serverModel} — llamancher`
      : $view === 'detail' && $selectedModel
        ? $selectedModel.name
        : 'llamancher'
  );

</script>

<header
  data-webview-drag
  class="flex items-center h-11 px-3 gap-2 bg-background border-b border-border shrink-0"
>
  <!-- Left section -->
  <div class="flex items-center gap-2" data-webview-ignore>
    {#if $view === 'detail'}
      <button
        class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all active:scale-95"
        onclick={goBack}
        data-webview-ignore
      >
        <ArrowLeft size={16} />
      </button>
    {:else}
      <button
        class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all active:scale-95"
        onclick={handleRefresh}
        disabled={$scanning}
        data-webview-ignore
      >
        <RefreshCw size={16} class={$scanning ? 'animate-spin' : ''} />
      </button>
    {/if}

    <Separator orientation="vertical" class="h-5" />

    <div class="flex items-center gap-1.5 text-xs text-muted-foreground" data-webview-ignore>
      <span class={cn('w-2 h-2 rounded-full', statusVariant)}></span>
      <span>{$serverStatus}</span>
    </div>
  </div>

  <!-- Center -->
  <div class="flex-1 text-center">
    <span class="text-sm font-medium">{headerTitle}</span>
  </div>

  <!-- Right section -->
  <div class="flex items-center gap-2" data-webview-ignore>
    <button
      class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all active:scale-95"
      onclick={() => settingsOpen.set(true)}
      data-webview-ignore
    >
      <Settings size={16} />
    </button>

    <Separator orientation="vertical" class="h-5" />

    <button
      class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all active:scale-95"
      data-webview-minimize
      data-webview-ignore
    >
      <Minus size={16} />
    </button>
    <button
      class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent text-muted-foreground hover:text-foreground transition-all active:scale-95"
      data-webview-maximize
      data-webview-ignore
    >
      <Square size={14} />
    </button>
    <button
      class="flex items-center justify-center h-7 w-7 rounded-md hover:bg-red-500/20 text-muted-foreground hover:text-red-400 transition-all active:scale-95"
      data-webview-close
      data-webview-ignore
    >
      <X size={16} />
    </button>
  </div>
</header>
