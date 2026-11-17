## WebGPU transparency budgets and overflow recovery

WebGPU transparency now reports fragment loss and can grow its fragment pool
after overflow. Applications can configure the pool before rendering, from C++
or Python, and changing its capacity refreshes cached render commands at the
same window size.

`vtkWebGPURenderer` exposes three independent controls:

- `DeepFramebufferAverageLayersPerPixel` sizes the initial shared pool. The
  default is eight, averaged across the whole window.
- `DeepFramebufferMaximumMemoryInBytes` limits node-pool memory. Zero, the
  default, uses the device limit. Head pointers and temporary allocations are
  additional memory.
- `DeepFramebufferMaximumLayersPerPixel` controls how many nearest fragments
  contribute at each pixel, from one to 64. The default remains sixteen.

Automatic pool growth is on by default. It uses asynchronous feedback and
retains the grown capacity until the window dimensions or explicit budget
changes. Overflow can therefore affect an interactive frame before a later
frame recovers. Raising the pool budget does not recover layers omitted by the
per-pixel resolve limit.

After a complete result, automatic feedback samples every eight frames to limit
readback overhead. New allocations and incomplete results are sampled immediately.
Use `renderer.GetDeepFramebuffer().SetStatisticsInterval(1)` to sample every
frame. Explicit statistics queries and checked rendering bypass this interval.

For a capture that must retain every recorded contribution, call
`vtkWebGPURenderWindow::RenderWithCheckedDeepFramebuffer()`. It waits for
feedback and retries recoverable pool overflow. It returns false if the memory
ceiling, resolve limit, 4,096-step traversal guard, or a resource failure prevents
a complete result. It supports mono rendering and checks the active deep
framebuffers; it does not validate other rendering techniques or remove color
quantization.

```python
renderer.SetDeepFramebufferAverageLayersPerPixel(12)
renderer.SetDeepFramebufferMaximumMemoryInBytes(512 * 1024 * 1024)
renderer.SetDeepFramebufferMaximumLayersPerPixel(32)
if not window.RenderWithCheckedDeepFramebuffer():
    raise RuntimeError("Transparency exceeded the configured rendering limits")
capture.SetInput(window)
capture.ShouldRerenderOff()
capture.Update()
```

Here `renderer` and `window` are WebGPU objects and `capture` is a
`vtkWindowToImageFilter`. An ordinary render remains asynchronous.
`renderer.ReadDeepFramebufferStatistics(True)` waits for the current frame's
status without rerendering. Its flags distinguish pool overflow, resolve-layer
truncation, traversal truncation, and readback failure. Tiled recovery and
multi-pass resolve beyond these limits are not implemented.
