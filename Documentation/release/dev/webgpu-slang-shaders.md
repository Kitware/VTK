# Slang shaders for WebGPU polydata

`vtkWebGPUPolyDataMapper` can render with Slang shaders (both stages, for
points, lines, and triangles, in
`Rendering/WebGPU/slang/vtkPolyDataSurfaceSlang.slang`) compiled through
`VTK::RenderingSlang`. By default they render exactly like the WGSL template.
`vtkWebGPUShaderProperty` turns them on for one prop (`UseSlangOn`), for every
prop (`SetGlobalUseSlang`, or the environment variable `VTK_WEBGPU_SLANG=1`),
or by selecting Slang types that implement SlangKit interfaces
(`SetSlangModule`, `LoadSlangModule`, `SetSlangSpecialization`). String shader
replacements and shader code are not supported on WebGPU and warn; their typed
equivalent is `IShaderHooks`. Actors with PBR interpolation keep their WGSL shaders.
