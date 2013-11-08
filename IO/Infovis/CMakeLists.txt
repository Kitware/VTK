set(Module_SRCS
  vtkBiomTableReader.cxx
  vtkChacoGraphReader.cxx
  vtkDelimitedTextReader.cxx
  vtkDIMACSGraphReader.cxx
  vtkDIMACSGraphWriter.cxx
  vtkFixedWidthTextReader.cxx
  vtkISIReader.cxx
  vtkMultiNewickTreeReader.cxx
  vtkNewickTreeReader.cxx
  vtkNewickTreeWriter.cxx
  vtkRISReader.cxx
  vtkTulipReader.cxx
  vtkXGMLReader.cxx
  vtkXMLTreeReader.cxx
  )

vtk_module_library(vtkIOInfovis ${Module_SRCS})
