## Emulate OpenGL texture buffers

You can now use `vtkTextureObject` to create a texture buffer on all
OpenGL implementations that support 2D textures. Earlier, texture buffers
were unavailable when VTK was configured to use OpenGL ES.

When texture buffers are emulated, `vtkOpenGLShaderCache` ensures all
usages of `texelFetch(samplerBuffer, int)` are mapped from a 1D
indexing scheme to a 2D scheme `texelFetch(sampler2D, ivec2)`

User provided shaders and in-VTK shaders which use texture buffers
will continue to work as expected on Desktop OpenGL and OpenGL ES.
