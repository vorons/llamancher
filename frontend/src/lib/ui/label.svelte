<script lang="ts">
  import { cn } from '$lib/utils';
  import type { Snippet } from 'svelte';

  let {
    for: htmlFor,
    class: className = '',
    children,
  }: {
    for?: string;
    class?: string;
    children: Snippet;
  } = $props();
</script>

<label
  for={htmlFor}
  class={cn('text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70', className)}
>
  {#if children}{@render children()}{/if}
</label>
