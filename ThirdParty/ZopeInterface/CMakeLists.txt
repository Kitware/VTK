vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_ZOPE "Use system Zope Interface Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_ZOPE)

if(NOT VTK_USE_SYSTEM_ZOPE)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  # Extract path where python module should be copied
  get_property(pymodule_build_dir GLOBAL PROPERTY VTK_PYMODULE_BUILD_DIR)
  get_property(pymodule_install_dir GLOBAL PROPERTY VTK_PYMODULE_INSTALL_DIR)

  set(zope_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zope")
  set(zope_BINARY_DIR "${pymodule_build_dir}/zope")

  build_python_package("zope" ${zope_SOURCE_DIR} ${zope_BINARY_DIR})

  if(NOT VTK_INSTALL_NO_PYTHON)
    install(DIRECTORY ${zope_BINARY_DIR}
      DESTINATION "${pymodule_install_dir}"
      COMPONENT Runtime)
  endif()
endif()
