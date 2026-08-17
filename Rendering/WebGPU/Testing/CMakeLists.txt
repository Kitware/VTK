add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)

  vtk_module_test_data(
    Data/wgsl/TestMultiply.wgsl
  )

  add_subdirectory(Python)
endif ()
