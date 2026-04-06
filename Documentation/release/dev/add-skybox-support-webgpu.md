## Add skybox rendering support in WebGPU

The WebGPU backend now includes support for skybox rendering using cube map
textures.

Cube map textures are now created with six array layers and their views
explicitly set to use the cube view dimension with an array layer count of 6.
This fixes incorrect texture allocation and sampling for cube maps and enables
correct rendering of skyboxes.

The WebGPU configuration also now requests the
`wgpu::FeatureName::Float32Filterable` feature so float32 textures (for example
HDR environment maps used by skyboxes) can be sampled with linear (bilinear)
filtering on backends that require this feature.
