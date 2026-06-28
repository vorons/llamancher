<script lang="ts">
	import { Dialog as DialogPrimitive } from "bits-ui";
	import DialogPortal from "./dialog-portal.svelte";
	import Overlay from "./dialog-overlay.svelte";
	import type { Snippet } from "svelte";
	import { cn, type WithoutChildrenOrChild } from "$lib/utils.js";
	import type { ComponentProps } from "svelte";
	import { Button } from "$lib/ui/button/index.js";
	import XIcon from '@lucide/svelte/icons/x';

	let {
		ref = $bindable(null),
		class: className,
		portalProps,
		children,
		showCloseButton = true,
		...restProps
	}: WithoutChildrenOrChild<DialogPrimitive.ContentProps> & {
		portalProps?: WithoutChildrenOrChild<ComponentProps<typeof DialogPortal>>;
		children: Snippet;
		showCloseButton?: boolean;
	} = $props();
</script>

<DialogPortal {...portalProps}>
	<Overlay />
	<!--
		ponytail: flexbox centering instead of translate(-50%, -50%) to avoid
		sub-pixel positioning and GPU compositing-layer blur in Chromium.
	-->
	<div class="fixed inset-0 z-50 flex items-center justify-center pointer-events-none">
		<DialogPrimitive.Content
			bind:ref
			data-slot="dialog-content"
			class={cn(
				"bg-popover text-popover-foreground data-open:animate-in data-closed:animate-out data-closed:fade-out-0 data-open:fade-in-0 data-closed:zoom-out-95 data-open:zoom-in-95 ring-foreground/10 grid max-w-[calc(100%-2rem)] gap-4 rounded-xl p-4 text-sm ring-1 duration-100 sm:max-w-sm w-full outline-none pointer-events-auto",
				className
			)}
			{...restProps}
		>
			{@render children?.()}
			{#if showCloseButton}
				<DialogPrimitive.Close data-slot="dialog-close">
					{#snippet child({ props })}
						<Button variant="ghost" class="absolute top-2 right-2" size="icon-sm" {...props}>
							<XIcon  />
							<span class="sr-only">Close</span>
						</Button>
					{/snippet}
				</DialogPrimitive.Close>
			{/if}
		</DialogPrimitive.Content>
	</div>
</DialogPortal>
