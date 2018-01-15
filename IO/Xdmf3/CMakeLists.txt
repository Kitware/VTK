set(classes
  vtkXdmf3ArrayKeeper
  vtkXdmf3ArraySelection
  vtkXdmf3DataSet
  vtkXdmf3HeavyDataHandler
  vtkXdmf3LightDataHandler
  vtkXdmf3Reader
  vtkXdmf3SILBuilder
  vtkXdmf3Writer)

vtk_module_add_module(VTK::IOXdmf3
  CLASSES ${classes})
