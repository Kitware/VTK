 vtk_module_test_data(
   Data/FDSExample/,REGEX:.*
   Data/FDSExample2/,REGEX:.*
   )

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
