# Tests with data
if(VTK_DATA_ROOT)
  # VS6 builds do not handle out-of-range double assignment to float
  # properly. Do not run TestMultiBlockExodusWrite on VS6 builds.
  #
  if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
    vtk_add_test_cxx(
      TestMultiBlockExodusWrite.cxx
      )
  endif()
  vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
endif()
