set(classes
  vtkSILBuilder
  vtkXdmfDataArray
  vtkXdmfReader
  vtkXdmfWriter)

set(nowrap_classes
  vtkXdmfHeavyData
  vtkXdmfReaderInternal)

vtk_module_add_module(VTK::IOXdmf2
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes})
vtk_add_test_mangling(VTK::IOXdmf2)
