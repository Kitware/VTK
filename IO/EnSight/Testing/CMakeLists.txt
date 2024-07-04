vtk_module_test_data(
  Data/EnSight/,REGEX:.*
  Data/EnSight/UndefAndPartial/,REGEX:.*
  Data/EnSight/UndefAndPartialAscii/,REGEX:.*
  Data/EnSight/particles/,REGEX:.*
  Data/EnSight/.EnsightData_temporalCache/,REGEX:.*
  )

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
