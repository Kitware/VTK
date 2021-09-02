#19-node Pyramid

VTK's vtkCell API now includes support for the 19-node-pyramid named `vtkTriQuadraticPyramid`.

Along with the addition of this API, several filters, readers and tests have been updated to incorporate `vtkTriQuadraticPyramid`.

Filters:
* `vtkCellValidator`
* `vtkUnstructuredGridGeometryFilter`
* `vtkReflectionFilter`
* `vtkBoxClipDataset`
* `vtkCellTypeSource`

Readers:
* `vtkIossReader`
