# ssefract

SSE-accelerated Mandelbrot / Julia fractal renderer with SDL browser.
The core iteration loop is written in hand-optimized x86 assembly (SSE2 + x87),
with a C implementation for comparison.

*University project.*

## Quick start

```sh
# Run the interactive browser (requires a display)
nix run github:komar007/ssefract

# Batch render a BMP from the command line
nix run github:komar007/ssefract#render -- output.bmp 1920 1080 \
  -2.0 -1.2 0.8 1.2 50 200 colorpalette1.pal 000000 8

# Benchmark the ASM vs C implementations
nix run github:komar007/ssefract#benchmark
```

## Programs

| Binary | Description |
|---|---|
| `browser` | SDL 1.2 GUI — pan, zoom, cycle palettes, save views, export BMP |
| `render` | CLI batch renderer — produces BMP from command-line parameters |
| `benchmark` | Throughput measurement — compares ASM vs C generators across sizes |

### `render` usage

```
render output.bmp width height min_x min_y max_x max_y N iter palette set_color nthreads
```

| Arg | Meaning |
|---|---|
| `output.bmp` | Output filename |
| `width height` | Image dimensions in pixels |
| `min_x min_y max_x max_y` | Complex-plane region |
| `N` | Bailout radius (e.g. 50) |
| `iter` | Max iterations (e.g. 200) |
| `palette` | Path to a `.pal` palette file |
| `set_color` | Hex color for points inside the set (e.g. `000000`) |
| `nthreads` | Number of render threads |

## Browser key bindings

| Key | Action |
|---|---|
| `h` `j` `k` `l` | Pan (vim-style: left, down, up, right) |
| `z` / `Z` | Zoom in / out |
| `i` / `I` | Increase / decrease max iterations |
| `p` / `P` | Next / previous color palette |
| `g` | Export current view to `output.bmp` (background thread) |
| `s` / `S` | Increase / decrease output resolution multiplier |
| `n` | Render anti-aliased high-quality view |
| `m` + `a`–`z` | Save current view to a named mark |
| `'` + `a`–`z` | Restore a saved mark |
| `d` | Dump all marks to file `marks` |
| `r` | Load marks from file `marks` |
| `q` | Quit |

### Mouse

| Action | Effect |
|---|---|
| Left-click + drag | Pan |
| Scroll wheel | Zoom in / out |

## Files

- `fractal.asm` — hand-written SSE2 + x87 Mandelbrot iteration (32-bit x86)
- `fractal.c` — reference C implementation
- `browser.c` — SDL 1.2 interactive GUI
- `render.c` — CLI batch renderer
- `benchmark.c` — ASM vs C performance comparison
- `*.pal` — color palette files (binary format)

## License

MIT — see [LICENSE](LICENSE).
