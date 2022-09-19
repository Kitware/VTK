set(classes
  vtkOctreeImageToPointSetFilter
  vtkPointSetToOctreeImageFilter
  vtkPointSetStreamer)

vtk_module_add_module(VTK::FiltersGeometryPreview
  CLASSES ${classes})
