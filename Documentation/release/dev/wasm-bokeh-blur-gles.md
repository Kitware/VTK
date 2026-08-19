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

Copying the depth buffer into a texture, which `vtkDepthPeelingPass` does whenever it
owns its opaque Z texture, now uses a framebuffer blit in GLES builds. `glCopyTexImage2D`
cannot target a depth format there, so depth peeling failed inside any render pass that
renders to its own framebuffer. The texture takes the depth format of the framebuffer it
is copied from, since a depth blit is only defined between matching formats -- including
the packed depth-stencil format of a stencil-capable render window.

`vtkTextureObject::CopyFromFrameBuffer` resolves a multisampled source before copying
from it, and the framebuffer it resolves into likewise takes the depth format of the
source instead of assuming stencil-less 24 fixed bits, which broke the resolve on
stencil-capable render windows and on the default 32 bit desktop depth buffer.

`vtkDepthPeelingPass` composites its result with a textured quad instead of a blit when
the destination framebuffer is multisampled in GLES builds, where such a blit is
forbidden; depth peeling therefore works with multisampling under WebAssembly.

The fragment shader that normalizes integer textures for reading pixels back declares a
precision for its `usampler2D`, which has no default in GLES and made the shader fail to
compile on strict drivers.
