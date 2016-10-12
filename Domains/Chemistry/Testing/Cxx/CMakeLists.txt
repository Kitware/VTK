option(VTK_BUILD_BODR_DATA_GENERATOR
  "If enabled, the test used to generate the vtkBlueObeliskDataInternal.h header will be built."
  OFF
)
mark_as_advanced(VTK_BUILD_BODR_DATA_GENERATOR)

set(generator_test)
if(VTK_BUILD_BODR_DATA_GENERATOR)
  set(generator_test
    "GenerateBlueObeliskHeader.cxx,NO_VALID,NO_DATA,NO_OUTPUT"
  )
endif()

set(TestVASPAnimationReader_ARGS DATA{../Data/VASP/NPT_Z_ANIMATE.out})
set(TestVASPTessellationReader_ARGS DATA{../Data/VASP/NPT_Z_TESSELLATE.out})

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${generator_test}
  TestBallAndStick.cxx
  TestPDBBallAndStick.cxx
  TestBondColorModeDiscreteByAtom.cxx
  TestBondColorModeSingleColor.cxx
  TestCompositeRender.cxx
  TestCustomArrayRadius.cxx
  TestFastRender.cxx
  TestLiquoriceSticks.cxx
  TestMolecule.cxx,NO_VALID
  TestMoleculeIOLegacy.cxx
  TestMoleculeSelection.cxx,NO_VALID
  TestMoleculeMapperPropertyUpdate.cxx
  TestMultiCylinderOn.cxx
  TestMultiCylinderOff.cxx
  TestPeriodicTable.cxx,NO_VALID
  TestProgrammableElectronicData.cxx,NO_VALID
  TestProteinRibbon.cxx
  TestSimpleBondPerceiver.cxx,NO_VALID
  TestVASPAnimationReader.cxx
  TestVASPTessellationReader.cxx
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
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPDBBallAndStick}
               RUN_SERIAL ON)
