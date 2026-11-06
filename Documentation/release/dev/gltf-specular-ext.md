# GLTF KHR_materials_specular extension

`vtkGLTFImporter` now supports the `KHR_materials_specular` extension.

To support it, `vtkProperty` gains `SetBaseSpecularWeight` and `SetBaseSpecularColor`, which respectively
scale the specular reflection (including at grazing angles) and the F0 color of the dielectric base layer in PBR interpolation,
as well as `SetSpecularTexture` and `SetSpecularColorTexture` to modulate them with textures.
The new parameters and textures are supported by both the OpenGL and the WebGPU polydata mappers.
