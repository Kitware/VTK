vtk_module_test_data(
  Data/dgHexahedra.dg
  Data/dgPyramids.dg
  Data/dgQuadrilateral.dg
  Data/dgTetrahedra.dg
  Data/dgTriangle.dg
  Data/dgWedges.dg
)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/can.ex2
  )
  add_subdirectory(Python)
endif ()
