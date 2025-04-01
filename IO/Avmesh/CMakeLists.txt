set(classes
  vtkAvmeshReader)

set(private_classes
  AvmeshInternals
  AvmeshMetadata
  BinaryFile)

vtk_module_add_module(VTK::IOAvmesh
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})
vtk_add_test_mangling(VTK::IOAvmesh)
