# Add texturing support in WebGPU

VTK's WebGPU backend now supports texture mapping for 3D models.

When using the WebGPU rendering backend (this is typically done by setting the `VTK_GRAPHICS_BACKEND` environment variable to "WEBGPU")
1. You can turn on `InterpolateScalarsBeforeMapping` on the polydata mapper when using webgpu.
2. You can also apply a texture on the actor (`actor->SetTexture(..)`), and expect the polydata mapper to calculate colors based on the texture and texture coordinates associated with the mesh.
