## WebGPU Performance: First Frame Rendering Time Improved

The WebGPU backend's first frame rendering time has been significantly
optimized by lazily compiling graphics pipelines. Previously,
`vtkWebGPUPolyDataMapper` would proactively compile all 14 possible graphics
pipeline variants (for points, lines, thick lines, triangles, homogeneous cell
sizes, etc.) whenever an actor was rendered for the first time. This eagerly
exhaustive compilation led to noticeable delays during the first frame.

Now, the mapper inspects the active actor representation properties (e.g.,
representation, point size, line width) and the actual topology data bound to
the renderer to build only the specific pipelines required to render the actor.
This has resulted in a roughly ~4.5x speedup for the first frame in benchmarks
(from ~17.2s to ~3.9s for a simple translucent spheres benchmark). Pipelines
are recompiled transparently if an actor's rendering properties change at
runtime, and only when the change actually selects a different pipeline: moving
a point size or line width between two values that are both wider than one pixel
no longer rebuilds anything.

Tracking the line join alongside those properties also fixes a pre-existing bug:
`vtkProperty::SetLineJoin()` was silently ignored once the first frame had been
rendered, because the render bundle holding the draw commands was never
invalidated and went on binding the previously selected pipeline. It now takes
effect at runtime, as it already did with `vtkWebGPURenderer` render bundles
turned off.
