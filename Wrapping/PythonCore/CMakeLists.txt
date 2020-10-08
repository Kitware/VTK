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
  PyVTKExtras.h)

get_property(vtk_python_version_major GLOBAL
  PROPERTY _vtk_python_version_major)
get_property(vtk_python_version_minor GLOBAL
  PROPERTY _vtk_python_version_minor)

vtk_module_add_module(VTK::WrappingPythonCore
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  LIBRARY_NAME_SUFFIX "${vtk_python_version_major}.${vtk_python_version_minor}")
