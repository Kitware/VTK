if (NOT vtk_testing_cxx_disabled)
vtk_module_test_data(
  Data/amr_contour_expected.vtpd
  Data/amr_contour_expected/,REGEX:.*
  Data/amr_contour_options_expected.vtpd
  Data/amr_contour_options_expected/,REGEX:.*
  Data/amr_contour_multigrid_expected.vtpd
  Data/amr_contour_multigrid_expected/,REGEX:.*
  Data/amr_interface_expected.vtpd
  Data/amr_interface_expected/,REGEX:.*
  Data/amr_interface_multigrid_expected.vtpd
  Data/amr_interface_multigrid_expected/,REGEX:.*
  )

  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/AMR/Enzo/DD0010/,REGEX:.*)

  add_subdirectory(Python)
endif ()
