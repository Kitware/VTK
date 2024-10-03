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

set(private_classes
  vtkDelimitedTextCodecIteratorPrivate)

vtk_module_add_module(VTK::IOInfovis
  PRIVATE_CLASSES ${private_classes}
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOInfovis)
