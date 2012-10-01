set(Module_SRCS
vtkXMLDataParser.cxx
vtkXMLParser.cxx
vtkXMLUtilities.cxx  )

vtk_module_library(vtkIOXMLParser ${Module_SRCS})
