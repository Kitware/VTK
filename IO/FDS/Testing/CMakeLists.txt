 vtk_module_test_data(
   Data/FDS/exemple_kitware/,REGEX:.*
   Data/FDS/visibility_adjustment/,REGEX:.*
   Data/FDS/1000meca/,REGEX:.*
   )

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
