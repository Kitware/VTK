vtk_module_test_data(
  Data/Disc_BiQuadraticQuads_0_0.vtu
  Data/Disc_QuadraticQuads_0_0.vtu
  Data/blowGeom.vtk
  Data/cell_850113.vtu
  Data/cell_12851_26.vtu
  Data/concavePolyhedron.vtu
  Data/cone.vtp
  Data/headsq/,REGEX:.*
  Data/nonWatertightPolyhedron.vtu
  Data/onePolyhedron.vtu
  Data/poly_lines.vtu
  Data/polyhedron_mesh.vtu
  Data/quadraticTetra01.vtu
  Data/sliceOfPolyhedron.vtu
  Data/test_surface.vtp
  )

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/dgHexahedra.dg
    Data/disk_out_ref.ex2
  )
  add_subdirectory(Python)
endif ()
