vtk_add_test_cxx(
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
  )

vtk_add_test_cxx(TestCMLMoleculeReader.cxx)

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
