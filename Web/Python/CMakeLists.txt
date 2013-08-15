vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp)

include(vtkPythonPackages)

if(NOT vtkWebPython_BUILD_DEST)
  set(vtkWebPython_BUILD_DEST
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../Wrapping/Python")
endif()

set(vtkWebPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(vtkWebPython_BINARY_DIR "${vtkWebPython_BUILD_DEST}/vtk/web")

build_python_package("vtkWebPython" ${vtkWebPython_SOURCE_DIR}
  ${vtkWebPython_BINARY_DIR})

install(DIRECTORY ${vtkWebPython_BINARY_DIR}
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
  COMPONENT Runtime)
