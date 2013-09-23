vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_TWISTED "Use system Twisted Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_TWISTED)

if(NOT VTK_USE_SYSTEM_TWISTED)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  set(Twisted_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/twisted")
  set(Twisted_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}/twisted")

  build_python_package("Twisted" ${Twisted_SOURCE_DIR} ${Twisted_BINARY_DIR})

  install(DIRECTORY ${Twisted_BINARY_DIR}
    DESTINATION "${VTK_INSTALL_PYTHON_MODULE_DIR}"
    COMPONENT Runtime)
endif()
