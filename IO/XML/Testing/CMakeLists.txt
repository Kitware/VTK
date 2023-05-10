vtk_module_test_data(
  Data/AMR/HierarchicalBoxDataset.v1.0.vthb
  Data/AMR/HierarchicalBoxDataset.v1.0/,REGEX:.*
  Data/AMR/HierarchicalBoxDataset.v1.1.vthb
  Data/AMR/HierarchicalBoxDataset.v1.1/,REGEX:.*
  Data/AMR/HyperTreeGridWithInterface.htg
  Data/mg_diff/mg_diff_0000.vtm
  Data/mg_diff/mg_diff_0000/,REGEX:.*
  Data/mg_diff/mg_diff_0062.vtm
  Data/mg_diff/mg_diff_0062/,REGEX:.*
  Data/HyperTreeGrid.1.htg
  Data/ghost_cells.vtu
  Data/many_time_steps.vtu
  Data/polyhedron2pieces.vtu
  Data/mb-of-mps.vtm
  Data/mixed-mb.vtm
  Data/mb-of-mps/,REGEX:.*
  Data/xml-without-idtype-tag.vtu
  Data/multicomb_0.vts
  Data/time_series.vtu)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtk
    Data/blow.vtk
    Data/combq.bin
    Data/combxyz.bin
    Data/cube-with-time.vtu
    Data/fran_cut.vtk
    Data/headsq/,REGEX:.*
    Data/vehicle_data.csv)

  add_subdirectory(Python)
endif ()
