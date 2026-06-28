# WebKitGTK font blur on scroll — root cause & fix

## Symptom

Small text (11–13 px) in an `overflow-y: auto` container blurs while scrolling. Only on saucer's WebKitGTK backend (Linux). Blur disappears the moment scrolling stops.

## Root cause

**WebKitGTK's GPU compositor rasterises text at lower quality during scroll** to maintain 60 fps. The default rendering path is:

1. The browser creates a GPU compositing layer for any element with `transform`, `will-change`, or `opacity` animation.
2. While scrolling, the compositor tiles this layer at reduced resolution.
3. Text rendered through the GPU path loses hinting and antialiasing — the smaller the font, the more visible the blur.

Forcing a GPU layer (e.g. `transform: translateZ(0)`) **makes the blur worse** because it pushes more content into the GPU path.

## Fix applied

### 1. Avoid GPU compositing on the scroll container

```css
/* Do NOT use: */
transform: translateZ(0);
will-change: transform;
will-change: scroll-position;
```

These trigger a compositing layer in WebKitGTK, which then rasterises at low quality during scroll.

Instead:

```css
-webkit-font-smoothing: antialiased;   /* disable sub-pixel rendering */
text-rendering: optimizeLegibility;    /* native CPU quality hint */
isolation: isolate;                    /* stacking context without GPU layer */
```

- `-webkit-font-smoothing: antialiased` prevents the browser from trying sub-pixel positioning during scroll.
- `isolation: isolate` creates a fresh stacking context without forcing GPU compositing.
- `text-rendering: optimizeLegibility` hints the CPU rasterizer (Cairo/Pango) to use native quality.

### 2. Limit paint to visible area

```css
[contain:layout] {
  contain: layout;
}
```

Applied to each of the three parameter blocks. Tells the browser that changes inside a block don't affect outside layout — off-screen blocks are not re-rasterised during scroll.

### 3. Reduce transition scope

```css
/* Before — animates ALL properties, keeps browser compositor on alert: */
transition-all

/* After — only colour properties, no layout/transform overhead: */
transition-colors
```

Replaced 12× `transition-all` on interactive elements (buttons). These elements only change `background-color` and `color` on hover — `transition-colors` is sufficient and avoids compositor interference.

## Test

- Open a model with many visible parameters (≥80 rows).
- Scroll the detail pane rapidly.
- Text should stay crisp. If blur reappears, check for any newly added `transform`, `will-change`, or `transition-all` in the component tree.

## Future notes

- If saucer ever switches to Chromium (via Electron or CEF), the same symptoms may have different causes — Chromium's GPU rasterizer is generally better at this, but `will-change` overuse can still cause quality degradation.
- The `contain:layout` hint is safe but requires that no child uses `position: absolute` referencing an ancestor outside the container — currently all positioned elements (`fixed` popover, etc.) are outside the containment boundary.
