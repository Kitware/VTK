## vtkGLTFReader: Fix texture coordinate handling

The GLTF reader now correctly handles models where multiple texture coordinate sets are present, and the material's texture does not use the first set (TEXCOORD_0). The reader now inspects the material to determine the correct texture coordinate set to use as the primary TCoords.
