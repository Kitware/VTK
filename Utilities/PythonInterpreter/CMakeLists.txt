set(classes
  vtkPythonInteractiveInterpreter
  vtkPythonInterpreter)

set(headers
  vtkPythonStdStreamCaptureHelper.h)

vtk_module_add_module(VTK::PythonInterpreter
  CLASSES ${classes}
  HEADERS ${headers})

if (Python3_VERSION VERSION_LESS "3.7" AND NOT CMAKE_VERSION VERSION_LESS "3.19")
  include(CheckCompilerFlag)
  set(old_python_ignore_warning_flag "-Wno-writable-strings")
  check_compiler_flag(CXX "${old_python_ignore_warning_flag}" "vtk_have_compiler_flag-CXX-${old_python_ignore_warning_flag}")
  if (vtk_have_compiler_flag-CXX-${old_python_ignore_warning_flag})
    vtk_module_compile_options(VTK::PythonInterpreter
      PRIVATE
        "${old_python_ignore_warning_flag}")
  endif ()
endif ()
