vtk_module_test_data(
  Data/DataObjects/hyper_tree_grid_template.htg
  Data/DataObjects/image_data_template.vti
  Data/DataObjects/poly_data_template.vtp
  Data/DataObjects/rectilinear_grid_template.vtr
  Data/DataObjects/structured_grid_template.vts
  Data/DataObjects/unstructured_grid_wavelet_duplicate.vtu
  Data/DataObjects/partitioned_dataset_collection_template.vtpc
  Data/DataObjects/multiblock_dataset_template.vtm)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif()
