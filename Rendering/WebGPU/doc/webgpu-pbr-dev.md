# WebGPU physically based rendering - Developer guide

This document describes how `vtkWebGPUPolyDataMapper` shades actors whose property uses
`SetInterpolationToPBR()`: where the material and environment data live on the GPU, how the
fragment shader is assembled, and the tools available to debug it. The shading model is the one of
the OpenGL backend, so that both backends render a scene the same way and share test baselines.

## Data flow

| Data | Producer | GPU resource | Bound at |
|------|----------|--------------|----------|
| Material parameters | `vtkWebGPUActor::CacheActorShadeOptions` | actor storage buffer, `ActorColorOptions` | group 1, binding 0 |
| Material textures | `vtkProperty::SetBaseColorTexture` and friends | property textures uploaded by `vtkWebGPUActor` | group 1, after the actor texture |
| Punctual lights | `vtkWebGPURenderer::UpdateLights` | `SceneLights` storage buffer | group 0, binding 1 |
| Image based lighting | `vtkWebGPUEnvironmentMap` | sampler, prefilter and irradiance cubes, BRDF table, parameters, spherical harmonics | group 0, bindings 2 to 7 |

### Material parameters

`ActorColorOptions` in `wgsl/ActorColorOptions.wgsl` mirrors
`vtkWebGPUActorInternals::ActorBlock::ColorOptions`. The C++ structure is checked against the WGSL
layout with `static_assert`s: vec3 members are 16 byte aligned in WGSL, so every vec3 is followed by
a scalar or explicit padding in C++. When adding a field, keep both declarations in the same order
and update the size assertion.

### Material textures

`vtkWebGPUActorInternals::GetPBRTextures()` returns the property textures sampled by the shading,
in a fixed order (base color, ORM, emissive, normal, anisotropy, coat normal). Both the actor bind
group and the WGSL declarations of the mapper are built from it, so they cannot disagree. Textures
with `UseSRGBColorSpaceOn()` use an sRGB format and are decoded by the hardware.

### Image based lighting

`vtkWebGPURenderer::UpdateEnvironmentMap()` uploads the environment texture and calls
`vtkWebGPUEnvironmentMap::Update()`, which regenerates the textures only when the environment
changed. The compute shaders live in `wgsl/IBL*.wgsl`:

- `IBLEnvironmentCube.wgsl` converts the source, a cube map or an equirectangular image, into a
  RGBA16Float cube map and fills its mip chain. Cube map sources are remapped with the environment
  basis (right, up, right x up) like `vtkPBRPrefilterTexture` does.
- `IBLPrefilter.wgsl` prefilters the cube with GGX importance sampling. Level `i` holds roughness
  `i / levels`, and the shading samples it at `roughness * (levels - 1)`, as the OpenGL backend.
- `IBLIrradiance.wgsl` and `IBLSphericalHarmonics.wgsl` compute the diffuse irradiance as a cube map
  and as nine spherical harmonics coefficients premultiplied for a Lambertian lobe.
- `IBLBRDFLookupTable.wgsl` computes the split-sum lookup table.

The environment is looked up in world space with
`environment_rotation * transpose(mat3(scene_transform.view))`, where `environment_rotation` holds
the transpose of `vtkRenderer::GetEnvironmentRotationMatrix()`.

## Shader assembly

The WGSL functions shared by every PBR pipeline are in `wgsl/PBRFunctions.wgsl`. They take two
structures, `PBRSurface` (normal, view vector and tangent frame) and `PBRMaterial` (albedo, metallic,
roughness, Fresnel terms, anisotropy and clear coat). Keeping the BRDF behind these structures is what
allows replacing them with Slang interfaces later without touching the mapper.

`vtkWebGPUPolyDataMapper::ReplaceFragmentShaderLights` builds the body of the fragment shader with
`BuildPBRLightingSource()`. What changes the bindings is specialized at compile time: image based
lighting, tangents and each material texture. What only changes values is a run time branch on the
actor uniforms, because a mapper builds one pipeline shared by all the actors that render it:
clear coat and anisotropy are enabled when their strength is not zero. `GetShadingFeatures()` packs
the compile time choices into a key, and the pipelines are rebuilt when the key of an actor changes.

## Debugging

- `VTK_WEBGPU_PBR_DUMP_DIR=<directory>` writes the generated lookup table, the irradiance and
  prefilter cube faces as PFM images and the spherical harmonics coefficients as text, every time
  the environment textures are regenerated.
- `vtkWebGPURenderer::SetPBRDebugView()` displays one input or term of the shading instead of the
  shaded color: albedo, normal, roughness, metallic, ambient occlusion, emissive, F0, N.V, direct
  lighting, diffuse or specular image based lighting.
- WGSL compilation errors and warnings are logged with the offending line and its neighbors.
- Every compute and render pass of the environment generation has a label, visible in GPU capture
  tools such as RenderDoc or the Xcode frame debugger.
