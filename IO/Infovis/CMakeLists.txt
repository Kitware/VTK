set(Module_SRCS
  vtkChacoGraphReader.cxx
  vtkChacoGraphReader.h
  vtkDelimitedTextReader.cxx
  vtkDelimitedTextReader.h
  vtkDIMACSGraphReader.cxx
  vtkDIMACSGraphReader.h
  vtkDIMACSGraphWriter.cxx
  vtkDIMACSGraphWriter.h
  vtkFixedWidthTextReader.cxx
  vtkFixedWidthTextReader.h
  vtkISIReader.cxx
  vtkISIReader.h
  vtkRISReader.cxx
  vtkRISReader.h
  vtkTulipReader.cxx
  vtkTulipReader.h
  vtkXGMLReader.cxx
  vtkXGMLReader.h
  vtkXMLTreeReader.cxx
  vtkXMLTreeReader.h
  )

vtk_module_library(vtkIOInfovis ${Module_SRCS})
