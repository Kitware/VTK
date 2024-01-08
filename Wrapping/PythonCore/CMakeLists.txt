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

if (NOT VTK_ABI_NAMESPACE_NAME STREQUAL "<DEFAULT>" AND NOT DEFINED ENV{CI})
  message(WARNING "Wrappings::PythonCore ABI does not not support the VTK_ABI_NAMESPACE_NAME "
                  "and the symbols will not be mangled.")
endif ()

if (Python3_VERSION VERSION_LESS "3.7" AND NOT CMAKE_VERSION VERSION_LESS "3.19")
  include(CheckCompilerFlag)
  set(old_python_ignore_warning_flag "-Wno-writable-strings")
  check_compiler_flag(CXX "${old_python_ignore_warning_flag}" "vtk_have_compiler_flag-CXX-${old_python_ignore_warning_flag}")
  if (vtk_have_compiler_flag-CXX-${old_python_ignore_warning_flag})
    vtk_module_compile_options(VTK::WrappingPythonCore
      PRIVATE
        "${old_python_ignore_warning_flag}")
  endif ()
endif ()
