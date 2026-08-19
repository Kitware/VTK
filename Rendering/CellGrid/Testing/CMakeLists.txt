vtk_module_test_data(
  Data/dgEdge-GBasis.dg
  Data/dgHexahedra.dg
  Data/dgQuad-GBasis.dg
  Data/dgQuadraticQuadrilaterals.dg
  Data/dgQuadrilateral.dg
  Data/dgTetrahedra.dg
  Data/dgTriangle-GBasis.dg
  Data/dgTriangle.dg
)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
