# WebGPU: debugging physically based rendering

- `vtkWebGPURenderer::SetPBRDebugView()` displays one input or term of the PBR shading instead of
  the shaded color, such as the normal, the roughness or the specular image based lighting.
- Setting the `VTK_WEBGPU_PBR_DUMP_DIR` environment variable writes the textures generated for image
  based lighting to that directory.

A developer guide for the WebGPU PBR implementation is available in
`Rendering/WebGPU/doc/webgpu-pbr-dev.md`.
