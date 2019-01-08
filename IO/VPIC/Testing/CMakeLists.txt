if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/VPIC/global.vpc
    Data/VPIC/fields/T.0/,REGEX:.*
    Data/VPIC/fields/T.100/,REGEX:.*
    Data/VPIC/hydro/T.0/,REGEX:.*
    Data/VPIC/hydro/T.100/,REGEX:.*)

  add_subdirectory(Cxx)
endif ()
