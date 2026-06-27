<script lang="ts">
  import { cn } from '$lib/utils';
  import type { Snippet } from 'svelte';

  let {
    variant = 'default',
    size = 'default',
    class: className = '',
    disabled = false,
    onclick,
    children,
    ...rest
  }: {
    variant?: 'default' | 'destructive' | 'outline' | 'secondary' | 'ghost' | 'link';
    size?: 'default' | 'sm' | 'lg' | 'icon';
    class?: string;
    disabled?: boolean;
    onclick?: (e: MouseEvent) => void;
    children: Snippet;
  } = $props();
</script>

<button
  class={cn(
    'inline-flex items-center justify-center whitespace-nowrap rounded-md text-sm font-medium transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring disabled:pointer-events-none disabled:opacity-50',
    {
      'bg-primary text-primary-foreground hover:bg-primary/90': variant === 'default',
      'bg-destructive text-destructive-foreground hover:bg-destructive/90': variant === 'destructive',
      'border border-input bg-transparent hover:bg-accent hover:text-accent-foreground': variant === 'outline',
      'bg-secondary text-secondary-foreground hover:bg-secondary/80': variant === 'secondary',
      'hover:bg-accent hover:text-accent-foreground': variant === 'ghost',
      'text-primary underline-offset-4 hover:underline': variant === 'link',
    },
    {
      'h-9 px-4 py-2': size === 'default',
      'h-8 rounded-md px-3 text-xs': size === 'sm',
      'h-10 rounded-md px-8': size === 'lg',
      'h-9 w-9': size === 'icon',
    },
    className,
  )}
  {disabled}
  {onclick}
  {...rest}
>
  {#if children}
    {@render children()}
  {/if}
</button>
