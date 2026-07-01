<script lang="ts">
  import { Label } from '$lib/ui/label';
  import { Input } from '$lib/ui/input';
  import { NativeSelect, NativeSelectOption } from '$lib/ui/native-select';
  import { api } from '$lib/saucer';
  import type { Preset } from '$lib/types';

  let {
    preset,
    debouncedSave,
  }: {
    preset: Preset;
    debouncedSave: () => void;
  } = $props();

  const isDraftMode = $derived(preset.spec_type === 'draft-model' || preset.spec_type === 'draft-mtp');
</script>

<div class="space-y-1">
  <div class="flex items-center justify-between gap-4 py-1.5">
    <div class="min-w-0">
      <Label for="spec_type" class="text-sm">Spec Type <span class="font-mono text-[10px] text-muted-foreground">--spec-type</span></Label>
      <p class="text-[11px] leading-tight text-muted-foreground">Метод спекулятивного декодирования</p>
    </div>
    <NativeSelect id="spec_type" class="w-36" value={preset.spec_type} onchange={(e) => { preset.spec_type = e.currentTarget.value; debouncedSave(); }}>
      <NativeSelectOption value="">Off</NativeSelectOption>
      <NativeSelectOption value="draft-mtp">Draft MTP</NativeSelectOption>
      <NativeSelectOption value="draft-model">Draft Model</NativeSelectOption>
    </NativeSelect>
  </div>

  {#if isDraftMode}
    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="draft_model" class="text-sm">Draft Model <span class="font-mono text-[10px] text-muted-foreground">-md, --model-draft</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">Путь к модели для спекулятивного декодирования</p>
      </div>
      <div class="flex items-center gap-2">
        <Input id="draft_model" placeholder="/path/to/draft.gguf" class="w-28" value={preset.draft_model}
          oninput={(e) => { preset.draft_model = e.currentTarget.value; debouncedSave(); }} />
        <button
          class="flex items-center justify-center h-7 px-2 rounded-md border border-border bg-secondary hover:bg-accent text-[11px] text-muted-foreground hover:text-foreground transition-colors whitespace-nowrap"
          onclick={async () => {
            const path = await api.pickFile();
            if (path) { preset.draft_model = path; debouncedSave(); }
          }}
          title="Browse for draft model"
        >
          Browse
        </button>
      </div>
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="hf_repo_draft" class="text-sm">HF Repo Draft <span class="font-mono text-[10px] text-muted-foreground">--hf-repo-draft</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">HuggingFace репозиторий для draft-модели</p>
      </div>
      <Input id="hf_repo_draft" placeholder="org/model" class="w-36" value={preset.hf_repo_draft}
        oninput={(e) => { preset.hf_repo_draft = e.currentTarget.value; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="draft_gpu_layers" class="text-sm">Draft GPU Layers <span class="font-mono text-[10px] text-muted-foreground">--draft-gpu-layers</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">GPU слоёв для draft-модели (0 = none)</p>
      </div>
      <Input id="draft_gpu_layers" type="number" class="w-36" value={String(preset.draft_gpu_layers)}
        oninput={(e) => { preset.draft_gpu_layers = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_n_max" class="text-sm">Draft N Max <span class="font-mono text-[10px] text-muted-foreground">--draft-max</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">Максимум токенов на шаг спекуляции</p>
      </div>
      <Input id="spec_draft_n_max" type="number" class="w-36" value={String(preset.spec_draft_n_max)}
        oninput={(e) => { preset.spec_draft_n_max = parseInt(e.currentTarget.value) || 16; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_n_min" class="text-sm">Draft N Min <span class="font-mono text-[10px] text-muted-foreground">--draft-min</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">Минимум токенов на шаг спекуляции</p>
      </div>
      <Input id="spec_draft_n_min" type="number" class="w-36" value={String(preset.spec_draft_n_min)}
        oninput={(e) => { preset.spec_draft_n_min = parseInt(e.currentTarget.value) || 0; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_p_split" class="text-sm">P Split <span class="font-mono text-[10px] text-muted-foreground">--draft-p-split</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">Вероятность разделения внимания (0.0–1.0)</p>
      </div>
      <Input id="spec_draft_p_split" type="number" step="0.01" min="0" max="1" class="w-36" value={String(preset.spec_draft_p_split)}
        oninput={(e) => { preset.spec_draft_p_split = parseFloat(e.currentTarget.value) || 0.5; debouncedSave(); }} />
    </div>

    <div class="flex items-center justify-between gap-4 py-1.5">
      <div class="min-w-0">
        <Label for="spec_draft_p_min" class="text-sm">P Min <span class="font-mono text-[10px] text-muted-foreground">--draft-p-min</span></Label>
        <p class="text-[11px] leading-tight text-muted-foreground">Минимальная вероятность принятия токена (0.0–1.0)</p>
      </div>
      <Input id="spec_draft_p_min" type="number" step="0.01" min="0" max="1" class="w-36" value={String(preset.spec_draft_p_min)}
        oninput={(e) => { preset.spec_draft_p_min = parseFloat(e.currentTarget.value) || 0; debouncedSave(); }} />
    </div>
  {/if}
</div>
