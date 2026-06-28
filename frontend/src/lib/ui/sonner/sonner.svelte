<script lang="ts">
	import { Toaster as Sonner, type ToasterProps as SonnerProps } from "svelte-sonner";
	import { mode } from "mode-watcher";
	import Loader2Icon from '@lucide/svelte/icons/loader-2';
	import CircleCheckIcon from '@lucide/svelte/icons/circle-check';
	import OctagonXIcon from '@lucide/svelte/icons/octagon-x';
	import InfoIcon from '@lucide/svelte/icons/info';
	import TriangleAlertIcon from '@lucide/svelte/icons/triangle-alert';

	let { ...restProps }: SonnerProps = $props();
</script>

<Sonner
	theme={mode.current}
	class="toaster group"
	style="--normal-bg: var(--color-popover); --normal-text: var(--color-popover-foreground); --normal-border: var(--color-border); --error-bg: oklch(0.25 0.04 25); --error-text: oklch(0.7 0.15 25); --error-border: oklch(0.35 0.08 25); --info-bg: oklch(0.25 0.04 240); --info-text: oklch(0.7 0.12 240); --info-border: oklch(0.35 0.08 240); --warning-bg: oklch(0.28 0.05 80); --warning-text: oklch(0.75 0.12 80); --warning-border: oklch(0.38 0.08 80); --success-bg: oklch(0.25 0.04 150); --success-text: oklch(0.7 0.12 150); --success-border: oklch(0.35 0.08 150);"
	{...restProps}
>
	{#snippet loadingIcon()}
		<Loader2Icon class="size-4" />
	{/snippet}
	{#snippet successIcon()}
		<CircleCheckIcon class="size-4 text-green-400" />
	{/snippet}
	{#snippet errorIcon()}
		<OctagonXIcon class="size-4 text-red-400" />
	{/snippet}
	{#snippet infoIcon()}
		<InfoIcon class="size-4 text-blue-400" />
	{/snippet}
	{#snippet warningIcon()}
		<TriangleAlertIcon class="size-4 text-amber-400" />
	{/snippet}
</Sonner>
