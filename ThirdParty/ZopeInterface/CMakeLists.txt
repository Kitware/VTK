vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_ZOPE "Use system Zope Interface Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_ZOPE)

if(NOT VTK_USE_SYSTEM_ZOPE)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  set(zope_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zope")
  set(zope_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}/zope")

  build_python_package("zope" ${zope_SOURCE_DIR} ${zope_BINARY_DIR})

  install(DIRECTORY ${zope_BINARY_DIR}
    DESTINATION "${VTK_INSTALL_PYTHON_MODULE_DIR}"
    COMPONENT Runtime)
endif()
