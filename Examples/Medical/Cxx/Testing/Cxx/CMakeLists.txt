######## Regression Testing ########
set(TestMedical1_ARGS ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}/Data/headsq/quarter)
set(TestMedical2_ARGS ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}/Data/headsq/quarter)
set(TestMedical3_ARGS ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}/Data/headsq/quarter)
set(TestMedical4_ARGS ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}/Data/headsq/quarter)
vtk_add_test_cxx(MedicalCxxTests tests
  TestMedical1.cxx
  TestMedical2.cxx
  TestMedical3.cxx
  TestMedical4.cxx
  )
vtk_test_cxx_executable(MedicalCxxTests tests
  RENDERING_FACTORY
  )
target_include_directories(MedicalCxxTests
  PRIVATE
    "${CMAKE_SOURCE_DIR}")
