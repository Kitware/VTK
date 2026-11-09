# WebGPU: texture coordinate transforms

The WebGPU backend now applies `vtkProp::GENERAL_TEXTURE_TRANSFORM()` to the texture coordinates,
as the OpenGL backend does. A transform set with `vtkTexture::SetTransform()` was previously
ignored, and models imported with `vtkGLTFImporter` were textured upside down, because the
importer carries the glTF to VTK flip of the vertical coordinate in that same matrix.
