# WebGPU: physically based rendering

The WebGPU backend now renders actors whose property uses `SetInterpolationToPBR()` with the same
shading model as the OpenGL backend, and the PBR rendering tests run on both backends.

## Image based lighting

`vtkRenderer::SetEnvironmentTexture()` is honored by `vtkWebGPURenderer`. From a cube map or an
equirectangular image, compute shaders generate a GGX prefiltered cube map, an irradiance cube map,
the nine spherical harmonics coefficients of the irradiance and a split-sum BRDF lookup table. They
are regenerated only when the environment texture changes. Spherical harmonics can be computed from
cube maps as well as from equirectangular images.

`UseSphericalHarmonics` moved from `vtkOpenGLRenderer` to `vtkRenderer` so that it can be set
independently of the backend.

## Materials

All PBR parameters of `vtkProperty` are used: metallic, roughness, base and coat index of
refraction, anisotropy and its rotation, clear coat strength, roughness, color and normal scale,
normal scale, occlusion strength, emissive factor and edge tint. Punctual lights (headlight,
directional, positional and spot lights with attenuation) are evaluated with the same BRDF as the
image based lighting. The base color, ORM, emissive, normal, anisotropy and coat normal textures are
sampled when the polydata has texture coordinates, and normal maps require tangents.

## Textures

Textures with `UseSRGBColorSpaceOn()` are uploaded in an sRGB format so that the hardware decodes
them, and mipmapped 2D and cube textures now have their whole mip chain generated on the GPU.
