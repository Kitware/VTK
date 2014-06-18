vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_SIX "Use system Six Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_SIX)

if(NOT VTK_USE_SYSTEM_SIX)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  set(SixPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  set(SixPython_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}")

  build_python_package("SixPython" ${SixPython_SOURCE_DIR}
    ${SixPython_BINARY_DIR})

  install(DIRECTORY ${SixPython_BINARY_DIR}
    DESTINATION "${VTK_INSTALL_PYTHON_MODULE_DIR}"
    COMPONENT Runtime)
endif()
