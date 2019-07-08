set(classes
  vtkPythonInteractiveInterpreter
  vtkPythonInterpreter)

set(headers
  vtkPythonStdStreamCaptureHelper.h)

if (NOT BUILD_SHARED_LIBS)
  set(relative_prefix "")
  set(bindir "${_vtk_build_RUNTIME_DESTINATION}")
  while (bindir)
    get_filename_component(bindir "${bindir}" DIRECTORY)
    string(APPEND relative_prefix "../")
  endwhile ()
  string(APPEND relative_prefix "${_vtk_build_LIBRARY_DESTINATION}")
  set_property(SOURCE vtkPythonInteractiveInterpreter.cxx APPEND
    PROPERTY
      COMPILE_DEFINITIONS "vtk_static_relative_prefix=\"${relative_prefix}\"")
endif ()

vtk_module_add_module(VTK::PythonInterpreter
  CLASSES ${classes}
  HEADERS ${headers})
