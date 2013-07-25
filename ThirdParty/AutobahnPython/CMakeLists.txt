vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_AUTOBAHN "Use system Autobahn Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_AUTOBAHN)

if(NOT VTK_USE_SYSTEM_AUTOBAHN)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  if(NOT vtkWebPython_BUILD_DEST)
    set(vtkWebPython_BUILD_DEST
      "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../Wrapping/Python")
  endif()

  set(AutobahnPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/autobahn")
  set(AutobahnPython_BINARY_DIR "${vtkWebPython_BUILD_DEST}/autobahn")

  build_python_package("AutobahnPython" ${AutobahnPython_SOURCE_DIR}
    ${AutobahnPython_BINARY_DIR})

  install(DIRECTORY ${AutobahnPython_BINARY_DIR}
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
    COMPONENT Runtime)
endif()
