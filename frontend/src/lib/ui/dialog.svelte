<script lang="ts">
  import { cn } from '$lib/utils';
  import type { Snippet } from 'svelte';

  let {
    open = $bindable(false),
    title = '',
    description = '',
    class: className = '',
    children,
  }: {
    open?: boolean;
    title?: string;
    description?: string;
    class?: string;
    children: Snippet;
  } = $props();
</script>

<svelte:window onkeydown={(e) => { if (e.key === 'Escape') open = false; }} />

{#if open}
  <!-- svelte-ignore a11y_click_events_have_key_events a11y_no_static_element_interactions -->
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div
    role="presentation"
    class="fixed inset-0 z-50 bg-black/50"
    onclick={() => open = false}
    onkeydown={(e) => { if (e.key === 'Enter') open = false; }}
  ></div>
  <!-- Panel -->
  <div
    class={cn(
      'fixed left-1/2 top-1/2 z-50 -translate-x-1/2 -translate-y-1/2 w-full max-w-md rounded-lg border border-border bg-background p-6 shadow-lg',
      className,
    )}
  >
    {#if title}
      <h2 class="text-lg font-semibold mb-1">{title}</h2>
    {/if}
    {#if description}
      <p class="text-sm text-muted-foreground mb-4">{description}</p>
    {/if}
    {#if children}
      {@render children()}
    {/if}
  </div>
{/if}
