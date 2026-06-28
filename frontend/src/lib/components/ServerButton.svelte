<script lang="ts">
  import { Play, Square, Loader2 } from '@lucide/svelte';
  import { cn } from '$lib/utils';
  import type { ServerStatus } from '$lib/types';

  let {
    modelName = '',
    serverModelName = '',
    serverStatus = 'stopped' as ServerStatus,
    loading = false,
    onclick,
    class: className = '',
  }: {
    modelName: string;
    serverModelName: string;
    serverStatus: ServerStatus;
    loading?: boolean;
    onclick: (e: MouseEvent) => void;
    class?: string;
  } = $props();

  const isActive = $derived(
    serverModelName === modelName && serverStatus === 'running'
  );
  const isStarting = $derived(
    serverModelName === modelName && serverStatus === 'starting'
  );
</script>

<button
  class={cn(
    'flex items-center justify-center h-8 w-8 rounded-md transition-all active:scale-95 shrink-0',
    isActive
      ? 'bg-red-500/20 text-red-400 hover:bg-red-500/30'
      : 'bg-secondary hover:bg-accent text-foreground',
    className,
  )}
  {onclick}
  disabled={loading || isStarting}
  aria-label={isActive ? 'Stop server' : 'Start server'}
>
  {#if loading || isStarting}
    <Loader2 size={16} class="animate-spin" />
  {:else if isActive}
    <Square size={14} />
  {:else}
    <Play size={14} />
  {/if}
</button>
