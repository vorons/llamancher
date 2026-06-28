<script lang="ts">
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { NativeSelect, NativeSelectOption } from '$lib/ui/native-select';
  import { Switch } from '$lib/ui/switch';
  import type { Preset } from '$lib/types';

  let {
    preset,
    debouncedSave,
  }: {
    preset: Preset;
    debouncedSave: () => void;
  } = $props();

  const isActive = $derived(preset.spec_type !== 'none');
  const isDraftMode = $derived(preset.spec_type === 'draft-model' || preset.spec_type === 'draft-mtp');
  const isNgramMode = $derived(preset.spec_type.startsWith('ngram-'));
</script>

<div>
  <h3 class="text-xs font-semibold uppercase tracking-wider text-muted-foreground mb-2">Speculative Decoding</h3>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <div class="min-w-0">
      <Label for="spec_type" class="text-sm">Spec Type</Label>
      <p class="text-[11px] leading-tight text-muted-foreground">Speculative decoding method</p>
    </div>
    <NativeSelect id="spec_type" class="w-36" value={preset.spec_type} onchange={(e) => { preset.spec_type = e.currentTarget.value; debouncedSave(); }}>
      <NativeSelectOption value="none">Off</NativeSelectOption>
      <NativeSelectOption value="draft-mtp">Draft MTP</NativeSelectOption>
      <NativeSelectOption value="draft-model">Draft Model</NativeSelectOption>
      <NativeSelectOption value="ngram-simple">N-gram Simple</NativeSelectOption>
      <NativeSelectOption value="ngram-map-k">N-gram Map K</NativeSelectOption>
      <NativeSelectOption value="ngram-map-k4v">N-gram Map K4V</NativeSelectOption>
      <NativeSelectOption value="ngram-mod">N-gram Mod</NativeSelectOption>
    </NativeSelect>
  </div>

  {#if isActive}
  <!-- Common speculative decoding params -->
  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="spec_draft_n_max" class="text-sm">Draft N Max</Label>
    <Input id="spec_draft_n_max" type="number" class="w-36" value={String(preset.spec_draft_n_max)}
      oninput={(e) => { preset.spec_draft_n_max = parseInt(e.currentTarget.value) || 16; debouncedSave(); }} />
  </div>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="spec_draft_n_min" class="text-sm">Draft N Min</Label>
    <Input id="spec_draft_n_min" type="number" class="w-36" value={String(preset.spec_draft_n_min)}
      oninput={(e) => { preset.spec_draft_n_min = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
  </div>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="spec_draft_p_split" class="text-sm">P Split</Label>
    <Input id="spec_draft_p_split" type="number" step="0.01" class="w-36" value={String(preset.spec_draft_p_split)}
      oninput={(e) => { preset.spec_draft_p_split = parseFloat(e.currentTarget.value) || 0.5; debouncedSave(); }} />
  </div>

  {#if isDraftMode}
  <!-- Draft model-specific params -->
  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="draft_model" class="text-sm">Draft Model</Label>
    <Input id="draft_model" placeholder="/path/to/draft.gguf" class="w-36" value={preset.draft_model}
      oninput={(e) => { preset.draft_model = e.currentTarget.value; debouncedSave(); }} />
  </div>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="draft_gpu_layers" class="text-sm">Draft GPU Layers</Label>
    <Input id="draft_gpu_layers" type="number" placeholder="-1" class="w-36" value={String(preset.draft_gpu_layers)}
      oninput={(e) => { preset.draft_gpu_layers = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
  </div>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="threads_draft" class="text-sm">Draft Threads</Label>
    <Input id="threads_draft" type="number" placeholder="0" class="w-36" value={String(preset.threads_draft)}
      oninput={(e) => { preset.threads_draft = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
  </div>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="threads_batch_draft" class="text-sm">Draft Batch Threads</Label>
    <Input id="threads_batch_draft" type="number" placeholder="0" class="w-36" value={String(preset.threads_batch_draft)}
      oninput={(e) => { preset.threads_batch_draft = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
  </div>

  <div class="flex items-center justify-between gap-4 py-1.5">
    <Label for="spec_draft_poll" class="text-sm">Draft Polling</Label>
    <Switch checked={preset.spec_draft_poll} onCheckedChange={(c) => { preset.spec_draft_poll = c; debouncedSave(); }} />
  </div>
  {/if}

  {#if isNgramMode}
  <!-- N-gram params -->
  <div class="mt-2 space-y-1">
    <p class="text-[11px] font-medium text-muted-foreground">N-gram Mod</p>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_mod_n_min" class="text-sm">N Min</Label>
      <Input id="spec_ngram_mod_n_min" type="number" class="w-36" value={String(preset.spec_ngram_mod_n_min)}
        oninput={(e) => { preset.spec_ngram_mod_n_min = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_mod_n_max" class="text-sm">N Max</Label>
      <Input id="spec_ngram_mod_n_max" type="number" class="w-36" value={String(preset.spec_ngram_mod_n_max)}
        oninput={(e) => { preset.spec_ngram_mod_n_max = parseInt(e.currentTarget.value) || 64; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_mod_n_match" class="text-sm">N Match</Label>
      <Input id="spec_ngram_mod_n_match" type="number" class="w-36" value={String(preset.spec_ngram_mod_n_match)}
        oninput={(e) => { preset.spec_ngram_mod_n_match = parseInt(e.currentTarget.value) || 24; debouncedSave(); }} />
    </div>
  </div>

  <div class="mt-2 space-y-1">
    <p class="text-[11px] font-medium text-muted-foreground">N-gram Simple / Map-K / Map-K4V</p>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_simple_size_n" class="text-sm">Size N</Label>
      <Input id="spec_ngram_simple_size_n" type="number" class="w-36" value={String(preset.spec_ngram_simple_size_n)}
        oninput={(e) => { preset.spec_ngram_simple_size_n = parseInt(e.currentTarget.value) || 12; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_simple_size_m" class="text-sm">Size M</Label>
      <Input id="spec_ngram_simple_size_m" type="number" class="w-36" value={String(preset.spec_ngram_simple_size_m)}
        oninput={(e) => { preset.spec_ngram_simple_size_m = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_simple_min_hits" class="text-sm">Min Hits</Label>
      <Input id="spec_ngram_simple_min_hits" type="number" class="w-36" value={String(preset.spec_ngram_simple_min_hits)}
        oninput={(e) => { preset.spec_ngram_simple_min_hits = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
    </div>
  </div>

  <div class="mt-2 space-y-1">
    <p class="text-[11px] font-medium text-muted-foreground">N-gram Map-K (separate)</p>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_map_k_size_n" class="text-sm">Size N</Label>
      <Input id="spec_ngram_map_k_size_n" type="number" class="w-36" value={String(preset.spec_ngram_map_k_size_n)}
        oninput={(e) => { preset.spec_ngram_map_k_size_n = parseInt(e.currentTarget.value) || 12; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_map_k_size_m" class="text-sm">Size M</Label>
      <Input id="spec_ngram_map_k_size_m" type="number" class="w-36" value={String(preset.spec_ngram_map_k_size_m)}
        oninput={(e) => { preset.spec_ngram_map_k_size_m = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_map_k_min_hits" class="text-sm">Min Hits</Label>
      <Input id="spec_ngram_map_k_min_hits" type="number" class="w-36" value={String(preset.spec_ngram_map_k_min_hits)}
        oninput={(e) => { preset.spec_ngram_map_k_min_hits = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
    </div>
  </div>

  <div class="mt-2 space-y-1">
    <p class="text-[11px] font-medium text-muted-foreground">N-gram Map-K4V (separate)</p>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_map_k4v_size_n" class="text-sm">Size N</Label>
      <Input id="spec_ngram_map_k4v_size_n" type="number" class="w-36" value={String(preset.spec_ngram_map_k4v_size_n)}
        oninput={(e) => { preset.spec_ngram_map_k4v_size_n = parseInt(e.currentTarget.value) || 12; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_map_k4v_size_m" class="text-sm">Size M</Label>
      <Input id="spec_ngram_map_k4v_size_m" type="number" class="w-36" value={String(preset.spec_ngram_map_k4v_size_m)}
        oninput={(e) => { preset.spec_ngram_map_k4v_size_m = parseInt(e.currentTarget.value) || 48; debouncedSave(); }} />
    </div>
    <div class="flex items-center justify-between gap-4 py-1.5">
      <Label for="spec_ngram_map_k4v_min_hits" class="text-sm">Min Hits</Label>
      <Input id="spec_ngram_map_k4v_min_hits" type="number" class="w-36" value={String(preset.spec_ngram_map_k4v_min_hits)}
        oninput={(e) => { preset.spec_ngram_map_k4v_min_hits = parseInt(e.currentTarget.value) || 1; debouncedSave(); }} />
    </div>
  </div>
  {/if}
  {/if}
</div>
