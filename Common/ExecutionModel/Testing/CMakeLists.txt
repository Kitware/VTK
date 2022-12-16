vtk_module_test_data(
  Data/AMR/HierarchicalBoxDataset.v1.1.vthb
  Data/AMR/HierarchicalBoxDataset.v1.1/,REGEX:.*
  Data/many_blocks/many_blocks.vtm
  Data/many_blocks/many_blocks/,REGEX:.*)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
