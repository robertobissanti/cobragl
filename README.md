# cobragl
A lightweight, custom 3D graphics engine written in pure C.

## Features

### Core
- **Windowing**: Built on SDL3.
- **Rasterizer**: CPU-based software rendering with direct framebuffer access.

### Primitives
- **Points**: `draw_point`, `draw_point_aa`.
- **Lines**:
  - **Standard**: Integer-only Bresenham (`draw_line`).
  - **Thick AA**: High-quality lines with width control, round caps, and anti-aliasing (`draw_line_aa`).
    - **SDF Mode**: Fast, distance-field based AA.
    - **Supersampling Mode**: 4x4 sub-pixel sampling.
    - **Thin Line Support**: Perceptual gamma correction for sub-pixel widths.

### Documentation
- Thick Line Algorithm
