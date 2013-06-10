# Tests with data
vtk_add_test_cxx(
  TestProStarReader.cxx
  TestTecplotReader.cxx
  TestAMRReadWrite.cxx,NO_VALID
  )

vtk_add_test_cxx(NO_VALID
  TestSimplePointsReaderWriter.cxx
  )

set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()
if(VTK_USE_LARGE_DATA AND _known_little_endian)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/field/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/turbine/,REGEX:.*}"
    )

  vtk_add_test_cxx(
    TestWindBladeReader.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)
