set(classes
  vtkBiomTableReader
  vtkChacoGraphReader
  vtkDelimitedTextReader
  vtkDIMACSGraphReader
  vtkDIMACSGraphWriter
  vtkFixedWidthTextReader
  vtkISIReader
  vtkMultiNewickTreeReader
  vtkNewickTreeReader
  vtkNewickTreeWriter
  vtkPhyloXMLTreeReader
  vtkPhyloXMLTreeWriter
  vtkRISReader
  vtkTemporalDelimitedTextReader
  vtkTulipReader
  vtkXGMLReader
  vtkXMLTreeReader)

vtk_module_add_module(VTK::IOInfovis
  CLASSES ${classes})
