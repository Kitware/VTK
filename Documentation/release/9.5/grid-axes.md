# Grid Axes in VTK

VTK now has the `vtkGridAxesActor3D` that is the successor to the `vtkCubeAxesActor` for cube axes
annotation. This new actor was the default cube axes grid for ParaView and uses `vtkAxisActor`
underneath the covers for better label placement strategy over its precursor.

![](https://vtk.org/files/ExternalData/SHA512/70e9ad194620080a3f194edf08a8b726af2d39f019b61169a66a07a7ff71024bfefa9b0919dac0924e637169564ef0d11d50c31c0e318e72b3df9e2ca459d433)
