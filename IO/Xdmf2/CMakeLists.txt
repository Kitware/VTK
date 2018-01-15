set(classes
  vtkSILBuilder
  vtkXdmfDataArray
  vtkXdmfHeavyData
  vtkXdmfReader
  vtkXdmfReaderInternal
  vtkXdmfWriter)

vtk_module_add_module(VTK::IOXdmf2
  CLASSES ${classes})
