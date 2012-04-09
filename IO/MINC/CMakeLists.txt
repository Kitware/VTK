set(Module_SRCS
  vtkMINCImageAttributes.cxx
  vtkMINCImageReader.cxx
  vtkMINCImageWriter.cxx
  vtkMNIObjectReader.cxx
  vtkMNIObjectWriter.cxx
  vtkMNITagPointReader.cxx
  vtkMNITagPointWriter.cxx
  vtkMNITransformReader.cxx
  vtkMNITransformWriter.cxx
  )

vtk_module_library(vtkIOMINC ${Module_SRCS})
