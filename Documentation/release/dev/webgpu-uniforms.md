## Keep custom uniforms in the WebGPU backend

The WebGPU backend now overrides `vtkUniforms` with `vtkWebGPUUniforms`, so values set on a shader
property's custom uniforms (`GetFragmentCustomUniforms()`, `GetVertexCustomUniforms()`) are kept
with the same rules as the OpenGL backend: array setters are distinct from scalar and vector ones, a
uniform keeps its type, unsigned char colors are normalized, and matrices are stored row by row.
