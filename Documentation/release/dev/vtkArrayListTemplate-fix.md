## vtkArrayListTemplate Fixes

In VTK 9.1.0, the helper class `vtkArrayListTemplate` that enhances `vtkDataSetAttributes`
was wrongly holding `vtkDataArray` pointers. This caused filters that are using this helper class to
not be able to pass array types not inheriting from `vtkDataArray`, including `vtkStringArray`.
The API of `vtkArrayListTemplate` has been changed to hold `vtkAbstractArray` instead, potentially
causing compiling issues with further VTK versions.

There are 2 ways to handle those issues. If the filter using `vtkArrayListTemplate` can ensure that
all arrays that are handled are indeed `vtkDataArray`, then a call to
`vtkArrayDownCast<vtkDataArray>` where the compiler complains should be sufficient. If not, the
pointer assigned to `ArrayList::AddArrayPair` needs to be turned into a `vtkAsbtractArray`.

Filters impacted by the bug in 9.1.0 are the following (this is not an exhaustive list):
* vtk3DLinearGridCrinkleExtractor
* vtk3DLinearGridPlaneCutter
* vtkBinnedDecimation
* vtkCellDataToPointData
* vtkContour3DLinearGrid
* vtkFlyingEdges3D
* vtkFlyingEdgesPlaneCutter
* vtkPointDataToCellData.cxx
* vtkPolyDataPlaneClipper.cxx
* vtkStaticCleanPolyData.cxx
* vtkStaticCleanUnstructuredGrid.cxx
* vtkDiscreteFlyingEdges3D.cxx
* vtkRemovePolyData.cxx
* vtkGeometryFilter.cxx
* vtkDensifyPointCloudFilter.cxx
* vtkPointCloudFilter.cxx
* vtkPointDensityFilter.cxx
* vtkPointInterpolator.cxx
* vtkPointInterpolator2D.cxx
* vtkSPHInterpolator.cxx
* vtkSignedDistance.cxx
* vtkUnsignedDistance.cxx
* vtkVoxelGrid.cxx
* vtkDepthImageToPointCloud.cxx
