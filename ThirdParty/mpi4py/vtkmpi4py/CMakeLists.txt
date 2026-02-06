project(mpi4py)

add_subdirectory(src)

vtk_module_add_module(VTK::mpi4py
  HEADER_ONLY)
vtk_module_include(VTK::mpi4py
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/mpi4py/include>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkmpi4py>")
