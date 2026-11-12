vtk_module_test_data(
  Data/AMR/htg3d.htg
  Data/Ce_Co_Fe_Gd_small_10.1038_s43246-022-00259-x.vti
  Data/RectGrid2.vtk
  Data/SampleStructGrid.vtk
  Data/ironProt.vtk
  Data/periodicPiece.vtu
  Data/tetraMesh.vtk
  Data/two_vtk_logos_stacked.png)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtr
    Data/cth.vtr
    Data/elements.vtu
    Data/headsq/,REGEX:.*
    Data/,REGEX:simpleamrgrid\\.e-s[0-9]+)

  add_subdirectory(Python)
endif ()
