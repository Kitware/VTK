set(classes
  vtkMINCImageAttributes
  vtkMINCImageReader
  vtkMINCImageWriter
  vtkMNIObjectReader
  vtkMNIObjectWriter
  vtkMNITagPointReader
  vtkMNITagPointWriter
  vtkMNITransformReader
  vtkMNITransformWriter)

set(headers
  vtkMINC.h)

vtk_module_add_module(VTK::IOMINC
  CLASSES ${classes}
  HEADERS ${headers})
