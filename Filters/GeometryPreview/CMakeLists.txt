set(classes
  vtkOctreeImageToPointSetFilter
  vtkPointSetToOctreeImageFilter)

vtk_module_add_module(VTK::FiltersGeometryPreview
  CLASSES ${classes})
