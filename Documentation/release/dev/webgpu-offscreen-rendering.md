## WebGPU renders without a window

The WebGPU render window now honors `vtkWindow`'s off-screen API.

`SetShowWindow(false)` leaves the window unmapped, as it does with the OpenGL backend. Until now
the WebGPU backend ignored the setting and showed a window anyway.

`SetShowWindow(false)` together with `SetUseOffScreenBuffers(true)` goes further and creates no
window and no surface at all, so rendering works on a machine with no display. Read the result back
with `GetPixelData()` as usual: every frame is drawn into an offscreen color attachment either way,
and only presenting it needs a surface.

Both must be set before the first render. A window that has already been initialized keeps the
surface it was created with.
