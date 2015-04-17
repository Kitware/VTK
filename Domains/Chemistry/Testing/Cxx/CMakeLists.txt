vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestBallAndStick.cxx
  TestPDBBallAndStick.cxx
  TestBondColorModeDiscreteByAtom.cxx
  TestBondColorModeSingleColor.cxx
  TestCompositeRender.cxx
  TestFastRender.cxx
  TestLiquoriceSticks.cxx
  TestMolecule.cxx,NO_VALID
  TestMoleculeSelection.cxx,NO_VALID
  TestMoleculeMapperPropertyUpdate.cxx
  TestMultiCylinderOn.cxx
  TestMultiCylinderOff.cxx
  TestPeriodicTable.cxx,NO_VALID
  TestProgrammableElectronicData.cxx,NO_VALID
  TestProteinRibbon.cxx
  TestSimpleBondPerceiver.cxx,NO_VALID
  TestVDWSpheres.cxx
  TestCMLMoleculeReader.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )

# On some machines this test takes 260ish seconds to complete, this allows for
# some extra time
if(NOT VTK_TEST_TIMEOUT_TestPDBBallAndStick)
  set(VTK_TEST_TIMEOUT_TestPDBBallAndStick 360)
endif()
set_tests_properties(${vtk-module}Cxx-TestPDBBallAndStick
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPDBBallAndStick})
