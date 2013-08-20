vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_TWISTED "Use system Twisted Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_TWISTED)

if(NOT VTK_USE_SYSTEM_TWISTED)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  # Extract path where python module should be copied
  get_property(pymodule_build_dir GLOBAL PROPERTY VTK_PYMODULE_BUILD_DIR)
  get_property(pymodule_install_dir GLOBAL PROPERTY VTK_PYMODULE_INSTALL_DIR)

  set(Twisted_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/twisted")
  set(Twisted_BINARY_DIR "${pymodule_build_dir}/twisted")

  build_python_package("Twisted" ${Twisted_SOURCE_DIR} ${Twisted_BINARY_DIR})

  if(NOT VTK_INSTALL_NO_PYTHON)
    install(DIRECTORY ${Twisted_BINARY_DIR}
      DESTINATION "${pymodule_install_dir}"
      COMPONENT Runtime)
  endif()
endif()
