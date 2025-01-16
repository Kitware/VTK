## GLTF document loader default scene

The `vtkGLTFDocumentLoader` ensures that the internal cache of scenes is non-empty for consumer code
that have to rely on checking validity of the default scene.
