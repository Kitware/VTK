vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_ZOPE "Use system Zope Interface Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_ZOPE)

if(NOT VTK_USE_SYSTEM_ZOPE)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  if(NOT vtkWebPython_BUILD_DEST)
    set(vtkWebPython_BUILD_DEST
      "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../Wrapping/Python")
  endif()

  set(zope_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zope")
  set(zope_BINARY_DIR "${vtkWebPython_BUILD_DEST}/zope")

  build_python_package("zope" ${zope_SOURCE_DIR} ${zope_BINARY_DIR})

  install(DIRECTORY ${zope_BINARY_DIR}
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
    COMPONENT Runtime)
endif()
