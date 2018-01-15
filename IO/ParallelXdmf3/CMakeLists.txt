set(classes
  vtkPXdmf3Writer)

vtk_module_add_module(VTK::IOParallelXdmf3
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
