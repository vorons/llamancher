<script lang="ts">
  import { cn } from '$lib/utils';

  let {
    value = $bindable(0),
    min = 0,
    max = 100,
    step = 1,
    disabled = false,
    class: className = '',
  }: {
    value?: number;
    min?: number;
    max?: number;
    step?: number;
    disabled?: boolean;
    class?: string;
  } = $props();
</script>

<input
  type="range"
  {min}
  {max}
  {step}
  {disabled}
  bind:value
  class={cn(
    'h-2 w-full cursor-pointer appearance-none rounded-full bg-secondary accent-primary',
    'focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring',
    className,
  )}
/>
