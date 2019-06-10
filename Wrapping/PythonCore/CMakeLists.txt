set(classes
  vtkPythonArgs
  vtkPythonCommand
  vtkPythonOverload
  vtkPythonUtil
  vtkSmartPyObject)

set(headers
  PyVTKEnum.h
  PyVTKMethodDescriptor.h
  PyVTKNamespace.h
  PyVTKObject.h
  PyVTKReference.h
  PyVTKSpecialObject.h
  PyVTKTemplate.h
  vtkPythonCompatibility.h)

set(sources
  PyVTKEnum.cxx
  PyVTKExtras.cxx
  PyVTKMethodDescriptor.cxx
  PyVTKNamespace.cxx
  PyVTKObject.cxx
  PyVTKReference.cxx
  PyVTKSpecialObject.cxx
  PyVTKTemplate.cxx)

set(private_headers
  PyVTKExtras.h
)

vtk_module_add_module(VTK::WrappingPythonCore
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  LIBRARY_NAME_SUFFIX "${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}")
