<script lang="ts">
  import { cn } from '$lib/utils';

  let {
    checked = $bindable(false),
    disabled = false,
    class: className = '',
  }: {
    checked?: boolean;
    disabled?: boolean;
    class?: string;
  } = $props();
</script>

<!-- svelte-ignore a11y_consider_explicit_label -->
<button
  role="switch"
  aria-checked={checked}
  disabled={disabled}
  class={cn(
    'peer inline-flex h-5 w-9 shrink-0 cursor-pointer items-center rounded-full border-2 border-transparent transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring disabled:cursor-not-allowed disabled:opacity-50',
    checked ? 'bg-primary' : 'bg-input',
    className,
  )}
  onclick={() => { checked = !checked; }}
>
  <span
    class={cn(
      'pointer-events-none block h-4 w-4 rounded-full bg-background shadow-lg ring-0 transition-transform',
      checked ? 'translate-x-4' : 'translate-x-0',
    )}
  ></span>
</button>
