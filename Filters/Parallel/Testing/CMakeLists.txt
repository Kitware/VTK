vtk_module_test_data(
  Data/RectGrid2.vtk
  Data/SampleStructGrid.vtk
  Data/ironProt.vtk
  Data/periodicPiece.vtu
  Data/tetraMesh.vtk
  Data/two_vtk_logos_stacked.png)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtr
    Data/cth.vtr
    Data/elements.vtu
    Data/headsq/,REGEX:.*
    Data/,REGEX:simpleamrgrid\\.e-s[0-9]+)

  add_subdirectory(Python)
endif ()
