set(classes
  vtkXMLDataParser
  vtkXMLParser
  vtkXMLUtilities)

vtk_module_add_module(VTK::IOXMLParser
  CLASSES ${classes})
