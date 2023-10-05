 vtk_module_test_data(
   Data/FDSExample/,REGEX:.*
   Data/FDSExample2/,REGEX:.*
   Data/FDS/1000meca/,REGEX:.*
   )

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
