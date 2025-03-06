set(classes
  vtkAvmeshReader)

set(private_classes
  AvmeshMetadata)

vtk_module_add_module(VTK::IOAvmesh
  CLASSES ${classes})
