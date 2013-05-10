vtk_add_test_cxx(Chemistry
  TestBallAndStick.cxx
  TestPDBBallAndStick.cxx
  TestBondColorModeDiscreteByAtom.cxx
  TestBondColorModeSingleColor.cxx
  TestCompositeRender.cxx
  TestFastRender.cxx
  TestLiquoriceSticks.cxx
  TestMolecule.cxx
  TestMoleculeSelection.cxx
  TestMoleculeMapperPropertyUpdate.cxx
  TestMultiCylinderOn.cxx
  TestMultiCylinderOff.cxx
  TestPeriodicTable.cxx
  TestProgrammableElectronicData.cxx
  TestProteinRibbon.cxx
  TestSimpleBondPerceiver.cxx
  TestVDWSpheres.cxx
  )

# Tests with data
if(VTK_DATA_ROOT)
  vtk_add_test_cxx(TestCMLMoleculeReader.cxx Chemistry)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
