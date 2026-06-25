## Fix depth buffer readback for multi-viewport and render pass configurations

`vtkRenderer::GetZ` now correctly returns depth values when multiple viewports
are active. Previously, the method rejected pixel values that were outside the
viewport-local width, and height in pixels, causing incorrect results in any
layout with more than one renderer.

`vtkFramebufferPass` now blits the depth buffer — in addition to color — back to
the outer framebuffer after rendering. Without this blit, depth written inside the
pass was silently discarded, making post-pass depth readback always return the far
plane value.

`vtkSSAAPass` now attaches depth textures (instead of a depth renderbuffer) to its
internal framebuffers. Renderbuffers are opaque to shaders, so the depth blit from
`vtkFramebufferPass` was effectively unreachable. Switching to `vtkTextureObject`
lets the depth values propagate through the horizontal and vertical downsample passes
and reach the final framebuffer, so `GetZ` and `GetZbufferDataAtPoint` return
correct world-space depth when SSAA is enabled.
