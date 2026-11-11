# GLTF KHR_gaussian_splatting support

`vtkGLTFDocumentLoader` now reads the `KHR_gaussian_splatting` primitive extension properties
(`kernel`, `colorSpace`, `projection` and `sortingMethod`) into
`Primitive::Extensions::KHRGaussianSplattingMetaData`. The splat attributes are available in the
point data of the primitive geometry.

`vtkGLTFImporter` renders these primitives with a `vtkPointGaussianMapper`.
Note that a order-independent translucency method such as depth sorting should be applied in
order to have a correct rendering.
