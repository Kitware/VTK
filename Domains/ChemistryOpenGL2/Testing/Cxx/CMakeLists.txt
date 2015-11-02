vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestPDBBallAndStickShadows.cxx
  TestPDBBallAndStickShadowsDOFSSAA.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )

# On some machines this test takes 260ish seconds to complete, this allows for
# some extra time
if(NOT VTK_TEST_TIMEOUT_TestPDBBallAndStickShadows)
  set(VTK_TEST_TIMEOUT_TestPDBBallAndStickShadows 360)
endif()
set_tests_properties(${vtk-module}Cxx-TestPDBBallAndStickShadows
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPDBBallAndStickShadows}
               RUN_SERIAL ON)
