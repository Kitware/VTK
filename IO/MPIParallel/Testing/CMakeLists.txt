set(enable_large_data_tests FALSE)
if (VTK_USE_LARGE_DATA AND NOT WIN32)
  if (DEFINED VTK_WORDS_BIGENDIAN AND NOT VTK_WORDS_BIGENDIAN)
    vtk_module_test_data(
      Data/WindBladeReader/,REGEX:.*
      Data/WindBladeReader/field/,REGEX:.*
      Data/WindBladeReader/turbine/,REGEX:.*)
    set(enable_large_data_tests TRUE)
  endif ()
endif ()

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/multi-bin.xyz
    Data/multi-bin-oflow.q
    Data/multi-bin.f)

  add_subdirectory(Python)
endif ()
