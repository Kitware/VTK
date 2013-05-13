# Tests with data
if(VTK_DATA_ROOT)
  vtk_add_test_cxx(IO
    TestProStarReader.cxx
    TestTecplotReader.cxx
    TestAMRReadWrite.cxx,NO_VALID
    )
endif()

vtk_add_test_cxx(NO_VALID
  TestSimplePointsReaderWriter.cxx
  )

set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()
if(VTK_LARGE_DATA_ROOT AND _known_little_endian)
  vtk_add_test_cxx(Parallel LARGE_DATA
    TestWindBladeReader.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)
