# Render passes preserve the color buffer correctly with multisampling

Render passes derived from `vtkImageProcessingPass` -- `vtkGaussianBlurPass`,
`vtkSobelGradientMagnitudePass`, `vtkSSAOPass`, `vtkFramebufferPass` and friends --
preserve the existing color buffer by copying the render window's framebuffer into
the target of the pass. That copy is now made through the render window's resolve
framebuffer when multisampling is on, because a scaled or format-converting blit out
of a multisampled framebuffer is undefined. It failed outright under GLES, which made
those passes unusable in WebAssembly builds when combined with a skybox blur or a
layered renderer.

`vtkOpenGLRenderWindow::GetResolveFramebuffer()` is now public, next to the existing
render and display framebuffer accessors.

Textures with three float components, such as an HDR environment map, no longer ask
for mipmaps in GLES builds. `glGenerateMipmap` cannot generate them for those formats,
and the failed call left an error behind that later showed up as a spurious "Failed to
read pixels" message. Such textures now use linear filtering.
