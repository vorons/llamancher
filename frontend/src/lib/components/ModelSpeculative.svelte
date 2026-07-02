<script lang="ts">
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { NativeSelect, NativeSelectOption } from '$lib/ui/native-select';
  import { api } from '$lib/saucer';
  import { t } from '$lib/i18n';
  import type { Preset } from '$lib/types';

  let {
    preset,
    debouncedSave,
    draftModelOptions = [],
  }: {
    preset: Preset;
    debouncedSave: () => void;
    draftModelOptions: string[];
  } = $props();

  const isDraftMode = $derived(preset.spec_type === 'draft-model' || preset.spec_type === 'draft-mtp');

  // Dropdown options: draft models in same dir + current value if external + Browse
  const draftSelectOptions = $derived.by(() => {
    const opts: { value: string; label: string }[] = [];
    for (const path of draftModelOptions) {
      const filename = path.split('/').pop() || path;
      opts.push({ value: path, label: filename });
    }
    // Include the currently selected path even if it's outside the model dir
    if (preset.draft_model && !draftModelOptions.includes(preset.draft_model)) {
      const filename = preset.draft_model.split('/').pop() || preset.draft_model;
      opts.push({ value: preset.draft_model, label: filename });
    }
    return opts;
  });

  async function handleDraftModelChange(e: Event) {
    const val = (e.currentTarget as HTMLSelectElement).value;
    if (val === '__browse__') {
      const path = await api.pickFile();
      if (path) { preset.draft_model = path; debouncedSave(); }
    } else {
      preset.draft_model = val;
      debouncedSave();
    }
  }
</script>

<div class="space-y-1">
  <div class="flex items-center justify-between gap-4 py-1.5">
    <div class="min-w-0">
      <Label for="spec_type" class="text-sm">{$t('detail.field.specType')} <span class="font-mono text-[10px] text-muted-foreground">--spec-type</span></Label>
      <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.specType.desc')}</p>
    </div>
    <NativeSelect id="spec_type" class="w-36" value={preset.spec_type} onchange={(e) => { preset.spec_type = e.currentTarget.value; debouncedSave(); }}>
      <NativeSelectOption value="">{$t('detail.spec.off')}</NativeSelectOption>
      <NativeSelectOption value="draft-mtp">Draft MTP</NativeSelectOption>
      <NativeSelectOption value="draft-model">Draft Model</NativeSelectOption>
    </NativeSelect>
  </div>

  {#if isDraftMode}
    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="draft_model" class="text-sm">{$t('detail.field.draftModel')} <span class="font-mono text-[10px] text-muted-foreground">-md, --model-draft</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.draftModel.desc')}</p>
      </div>
      <NativeSelect id="draft_model" class="w-52" value={preset.draft_model} onchange={handleDraftModelChange}>
        {#if !preset.draft_model}
          <NativeSelectOption value="">—</NativeSelectOption>
        {/if}
        {#each draftSelectOptions as opt}
          <NativeSelectOption value={opt.value}>{opt.label}</NativeSelectOption>
        {/each}
        <NativeSelectOption value="__browse__">{$t('settings.browse')}…</NativeSelectOption>
      </NativeSelect>
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="hf_repo_draft" class="text-sm">{$t('detail.field.hfRepoDraft')} <span class="font-mono text-[10px] text-muted-foreground">--hf-repo-draft</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.hfRepoDraft.desc')}</p>
      </div>
      <Input id="hf_repo_draft" placeholder="org/model" class="w-36" value={preset.hf_repo_draft}
        oninput={(e) => { preset.hf_repo_draft = e.currentTarget.value; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="draft_gpu_layers" class="text-sm">{$t('detail.field.draftGpuLayers')} <span class="font-mono text-[10px] text-muted-foreground">--draft-gpu-layers</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.draftGpuLayers.desc')}</p>
      </div>
      <Input id="draft_gpu_layers" type="number" class="w-36" value={String(preset.draft_gpu_layers)}
        oninput={(e) => { preset.draft_gpu_layers = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_n_max" class="text-sm">{$t('detail.field.draftNMax')} <span class="font-mono text-[10px] text-muted-foreground">--draft-max</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.draftNMax.desc')}</p>
      </div>
      <Input id="spec_draft_n_max" type="number" class="w-36" value={String(preset.spec_draft_n_max)}
        oninput={(e) => { preset.spec_draft_n_max = parseInt(e.currentTarget.value) || 16; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_n_min" class="text-sm">{$t('detail.field.draftNMin')} <span class="font-mono text-[10px] text-muted-foreground">--draft-min</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.draftNMin.desc')}</p>
      </div>
      <Input id="spec_draft_n_min" type="number" class="w-36" value={String(preset.spec_draft_n_min)}
        oninput={(e) => { preset.spec_draft_n_min = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_p_split" class="text-sm">{$t('detail.field.draftPSplit')} <span class="font-mono text-[10px] text-muted-foreground">--draft-p-split</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.draftPSplit.desc')}</p>
      </div>
      <Input id="spec_draft_p_split" type="number" step="0.01" min="0" max="1" class="w-36" value={String(preset.spec_draft_p_split)}
        oninput={(e) => { preset.spec_draft_p_split = parseFloat(e.currentTarget.value) || 0.5; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_p_min" class="text-sm">{$t('detail.field.draftPMin')} <span class="font-mono text-[10px] text-muted-foreground">--draft-p-min</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">{$t('detail.field.draftPMin.desc')}</p>
      </div>
      <Input id="spec_draft_p_min" type="number" step="0.01" min="0" max="1" class="w-36" value={String(preset.spec_draft_p_min)}
        oninput={(e) => { preset.spec_draft_p_min = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }} />
    </div>
  {/if}
</div>
