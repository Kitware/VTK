# Wide lines and instancing fixes for OpenGL ES 3.0 / WebAssembly rendering

Three rendering fixes improve line drawing on GLES 3.0 and WebAssembly targets:

- **vtkOpenGLGlyph3DMapper now renders wide lines correctly under GLES 3.0.** Since GLES 3.0 lacks geometry shaders, wide lines are emulated by drawing multiple offset copies of each line segment using instanced rendering. Offsets are computed from a texture buffer so they are not limited by the uniform budget, and the instance count is scaled by the number of offsets so each line is drawn at the requested thickness.

- **VAO attribute divisors are now passed correctly to OpenGL.** `vtkOpenGLVertexArrayObject` previously hardcoded the divisor to `1` in `glVertexAttribDivisor` calls. It now uses the actual divisor value set by the caller, which is required for the wide lines instancing in `vtkOpenGLGlyph3DMapper` to work correctly.

- **GLSL type mismatch in `vtkOpenGLPolyDataMapper` is fixed.** The shader replacement for line drawing compared a `float` against the integer literal `0`, which can cause type mismatch errors on strict GLSL compilers (including GLES 3.0). The comparison now uses the correct float literal `0.0`.
