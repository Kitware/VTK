set(TEST_SRC)

if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  list(APPEND TEST_SRC TestAVIWriter.cxx)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_tolower)
if(vtkIOMovie_vtkoggtheora AND NOT (WIN32 AND cmake_build_type_tolower STREQUAL "debug"))
  list(APPEND TEST_SRC TestOggTheoraWriter.cxx)
endif()

vtk_add_test_cxx(vtkIOMovieCxxTests tests
  NO_VALID
  ${TEST_SRC}
  )

vtk_test_cxx_executable(vtkIOMovieCxxTests tests)
