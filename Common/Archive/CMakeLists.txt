set(classes
  vtkBufferedArchiver
  vtkPartitionedArchiver)

vtk_module_find_package(PACKAGE LibArchive)

vtk_module_add_module(VTK::CommonArchive
  CLASSES ${classes})

vtk_module_link(VTK::CommonArchive PRIVATE LibArchive::LibArchive)
