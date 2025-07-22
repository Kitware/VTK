# GLTF document loader selectively toggle model data

The `vtkGLTFDocumentLoader` learned to selectively enable/disable loading of certain types of model
data from the gltf file. The loader exposes the following new API:

  - `SetLoadAnimation(bool)`: Load animation keyframes (default: true)
  - `SetLoadImages(bool)`: Load model images (default: true)
  - `SetLoadSkinMatrix(bool)`: Load inverse bind matrices for model Skin (default: true)
