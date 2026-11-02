# Fix WebGPU window rendering on macOS

WebGPU windows on macOS now render a new frame after resizing. Native window
sizes and VTK rendering dimensions stay synchronized using backing pixels.

Tiled image captures preserve the native window size instead of repeatedly
enlarging it by the tile scale.
