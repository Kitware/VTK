# This module requires CMake 3.19 features (the `CheckCompilerFlag`
# module). Just skip it for older CMake versions.
if (CMAKE_VERSION VERSION_LESS "3.19")
  return ()
endif ()

include(CheckCompilerFlag)

set(classes
  vtkPythonInteractiveInterpreter
  vtkPythonInterpreter)

set(headers
  vtkPythonStdStreamCaptureHelper.h)

vtk_module_add_module(VTK::PythonInterpreter
  CLASSES ${classes}
  HEADERS ${headers})

if (Python3_VERSION VERSION_LESS "3.7")
  set(old_python_ignore_warning_flag "-Wno-writable-strings")
  check_compiler_flag(CXX "${old_python_ignore_warning_flag}" "vtk_have_compiler_flag-CXX-${old_python_ignore_warning_flag}")
  if (vtk_have_compiler_flag-CXX-${old_python_ignore_warning_flag})
    vtk_module_compile_options(VTK::PythonInterpreter
      PRIVATE
        "${old_python_ignore_warning_flag}")
  endif ()
endif ()
