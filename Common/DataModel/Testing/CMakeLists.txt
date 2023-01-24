vtk_module_test_data(
  Data/Disc_BiQuadraticQuads_0_0.vtu
  Data/Disc_QuadraticQuads_0_0.vtu
  Data/blowGeom.vtk
  Data/headsq/,REGEX:.*
  Data/nonWatertightPolyhedron.vtu
  Data/poly_lines.vtu
  Data/quadraticTetra01.vtu)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/dgHexahedra.dg
  )
  add_subdirectory(Python)
endif ()
