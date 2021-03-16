set(classes
  PyVTKEnum
  PyVTKExtras
  PyVTKNamespace
  PyVTKObject
  PyVTKSpecialObject
  PyVTKTemplate
  vtkPythonArgs
  vtkPythonCommand
  vtkPythonOverload
  vtkPythonUtil
  vtkSmartPyObject)

set(private_classes
  PyVTKMethodDescriptor
  PyVTKReference)

set(headers
  vtkPythonCompatibility.h)

set(private_headers
  PyVTKExtras.h)

get_property(vtk_python_version_major GLOBAL
  PROPERTY _vtk_python_version_major)
get_property(vtk_python_version_minor GLOBAL
  PROPERTY _vtk_python_version_minor)

vtk_module_add_module(VTK::WrappingPythonCore
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers}
  LIBRARY_NAME_SUFFIX "${vtk_python_version_major}.${vtk_python_version_minor}")
