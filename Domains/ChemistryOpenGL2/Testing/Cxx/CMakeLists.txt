vtk_add_test_cxx(vtkDomainsChemistryOpenGL2CxxTests tests
  TestPDBBallAndStickShadows.cxx
  TestPDBBallAndStickShadowsDOFSSAA.cxx
  TestPDBBallAndStickTranslucent.cxx
  )
vtk_test_cxx_executable(vtkDomainsChemistryOpenGL2CxxTests tests
  RENDERING_FACTORY
  )

# On some machines this test takes 260ish seconds to complete, this allows for
# some extra time
if(NOT VTK_TEST_TIMEOUT_TestPDBBallAndStickShadows)
  set(VTK_TEST_TIMEOUT_TestPDBBallAndStickShadows 360)
endif()
set_tests_properties(VTK::DomainsChemistryOpenGL2Cxx-TestPDBBallAndStickShadows
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPDBBallAndStickShadows}
               RUN_SERIAL ON)
