add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/disk_out_ref.ex2)

  add_subdirectory(Python)
endif ()
