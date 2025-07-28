# Add UseOIT flag to vtkRenderer

`vtkRenderer` now has a new flag (`UseOIT`) to toggle the use of the new
`vtkOrderIndependentTranslucentPass` (OIT) for rendering translucent polygonal geometry. This flag
is enabled by default which means the renderer uses OIT by default.

OIT is a newer approach to translucent rendering that yields better results without any performance
trade-offs. However, this conflicts with other features of the vtkRenderer, namely MSAA based
anti-aliasing. If you would like to use MSAA for anti-aliasing in a scene with translucent geometry,
it is best to disable `UseOIT`.

The following pair of images show the use of this flag.

|MultiSamples: 8, UseOIT: True|MultiSamples: 8, UseOIT: False|
|:--:|:--:|
|i.e. MSAA with OIT |i.e. MSAA without OIT |
|![](../imgs/9.5/OIT_with_MSAA.png)|![](https://vtk.org/files/ExternalData/SHA512/b5dce5a56db0d685c638e3383536bcbb1cbf0b71040989c7fad040f221f4527f761d7cd22c7e4dd63aaf368cc236e366d37c4832b07f59f50b6928d223f7da9e)|
