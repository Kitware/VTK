vtk_module_test_data(
  Data/dgEdges.dg
  Data/dgGhostHexahedra.dg
  Data/dgHexahedra.dg
  Data/dgMixed.dg
  Data/dgPyramid19.dg
  Data/dgPyramids.dg
  Data/dgQuadraticQuadrilaterals.dg
  Data/dgQuadrilateral.dg
  Data/dgTetrahedra.dg
  Data/dgTriangle.dg
  Data/dgWedges.dg
  Data/fandisk.dg
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
