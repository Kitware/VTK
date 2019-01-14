vtk_module_test_data(
  Data/AMR/HierarchicalBoxDataset.v1.0.vthb
  Data/AMR/HierarchicalBoxDataset.v1.0/,REGEX:.*
  Data/AMR/HierarchicalBoxDataset.v1.1.vthb
  Data/AMR/HierarchicalBoxDataset.v1.1/,REGEX:.*
  Data/HyperTreeGrid.1.htg
  Data/ghost_cells.vtu
  Data/many_time_steps.vtu
  Data/polyhedron2pieces.vtu)

add_subdirectory(Cxx)

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
