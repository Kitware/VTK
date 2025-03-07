set(classes
  vtkAvmeshReader)

set(private_classes
  AvmeshMetadata
  BinaryFile)

vtk_module_add_module(VTK::IOAvmesh
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOAvmesh)
