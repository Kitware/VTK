vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_TWISTED "Use system Twisted Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_TWISTED)

if(NOT VTK_USE_SYSTEM_TWISTED)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  if(NOT vtkWebPython_BUILD_DEST)
    set(vtkWebPython_BUILD_DEST
      "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../Wrapping/Python")
  endif()

  set(Twisted_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/twisted")
  set(Twisted_BINARY_DIR "${vtkWebPython_BUILD_DEST}/twisted")

  build_python_package("Twisted" ${Twisted_SOURCE_DIR} ${Twisted_BINARY_DIR})

  install(DIRECTORY ${Twisted_BINARY_DIR}
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
    COMPONENT Runtime)
endif()
