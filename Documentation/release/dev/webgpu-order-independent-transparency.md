## Order independent transparency in the WebGPU backend

`vtkWebGPURenderer` now composites translucent geometry from per-pixel fragment
lists, in an order independent way.

This is on by default. To disable it, call
`vtkRenderer::UseDeepFramebufferTransparencyOff()` on the renderer. In C++:

```cpp
if (auto* wgpu = vtkWebGPURenderer::SafeDownCast(renderer))
{
  wgpu->UseDeepFramebufferTransparencyOff();
}
```

### How it works

A translucent fragment is not blended when it rasterizes. It is appended to a
linked list belonging to its pixel, held in a storage buffer that VTK calls the
deep framebuffer. Once all geometry has been recorded, a full-screen pass walks
each pixel's list, sorts it by depth, and composites it over the opaque image.
This is the technique usually called an [A-buffer](https://en.wikipedia.org/wiki/A-buffer).

### How it compares to the OpenGL backend

The OpenGL backend offers two techniques, and this is a third point in the same
design space rather than a port of either.

`vtkOrderIndependentTranslucentPass`, which `vtkRenderer::UseOIT` selects,
accumulates an alpha-weighted average of the translucent fragments covering a
pixel. It is order independent because addition commutes, not because it sorts:
nothing in it depends on depth, so a layer at the back contributes as much as an
equally transparent layer at the front. It is a single geometry pass with fixed
memory, and it is an approximation.

`vtkDepthPeelingPass` captures the nearest layer, then the next, and so on. Each
layer is exact, but each one costs a full pass over the geometry.

The deep framebuffer sorts, so within its layer budget it produces the same image
that correctly ordered alpha blending would, like depth peeling. It collects
every layer in one geometry pass, like the average pass. What it spends instead
is memory: a node pool proportional to the viewport and to the number of
translucent layers expected per pixel.

That trade was the right one for this backend. WebGPU exposes atomics on storage
buffers as core functionality, with no optional feature to request, so the
per-pixel lists cost nothing in portability. Re-rasterizing the scene once per
layer, as depth peeling does, would have been the more expensive option for the
kind of layered scientific data this backend is aimed at.

The resolve currently keeps the sixteen nearest fragments of a pixel, and the
node pool is sized for an average of four layers per pixel. A deeper pixel
loses its farthest layers, which are the ones most attenuated by everything in
front of them; fragments past the end of the pool are dropped rather than
written. Both limits are chosen by the renderer and are not yet configurable.

### Hardware selection

Fragment records carry the cell, prop, composite and process ids, so a pick
through translucent geometry reports the nearest visible layer rather than
whichever fragment was blended last. Selecting points now draws the points of a
cell rather than the cell, and hides the points behind the surface, matching what
the OpenGL backend reports.
