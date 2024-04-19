option(VTK_BUILD_BODR_DATA_GENERATOR
  "If enabled, the test used to generate the vtkBlueObeliskDataInternal.h header will be built."
  OFF
)
mark_as_advanced(VTK_BUILD_BODR_DATA_GENERATOR)

if(VTK_BUILD_BODR_DATA_GENERATOR)
  add_executable(GenerateBlueObeliskHeader
    GenerateBlueObeliskHeader.cxx)
  target_link_libraries(GenerateBlueObeliskHeader
    PRIVATE
      VTK::DomainsChemistry)
  vtk_module_autolink(
    TARGETS GenerateBlueObeliskHeader
    MODULES VTK::DomainsChemistry)
  add_test(
    NAME    VTK::DomainsChemistry-GenerateBlueObeliskHeader
    COMMAND GenerateBlueObeliskHeader
            "${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif()

vtk_add_test_cxx(vtkDomainsChemistryCxxTests tests
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
  TestMoleculeMapperColors.cxx
  TestMoleculeMapperPropertyUpdate.cxx
  TestMoleculeToBondStickFilter.cxx,NO_VALID
  TestMoleculeToLines.cxx,NO_VALID
  TestMultiCylinderOn.cxx
  TestMultiCylinderOff.cxx
  TestPeriodicTable.cxx,NO_VALID
  TestPointSetToMoleculeFilter.cxx,NO_VALID
  TestProgrammableElectronicData.cxx,NO_VALID
  TestProteinRibbon.cxx
  TestSimpleBondPerceiver.cxx,NO_VALID
  TestVDWSpheres.cxx
  )
vtk_test_cxx_executable(vtkDomainsChemistryCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )

# On some machines this test takes 260ish seconds to complete, this allows for
# some extra time
if(NOT VTK_TEST_TIMEOUT_TestPDBBallAndStick)
  set(VTK_TEST_TIMEOUT_TestPDBBallAndStick 360)
endif()
set_tests_properties(VTK::DomainsChemistryCxx-TestPDBBallAndStick
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPDBBallAndStick}
               RUN_SERIAL ON)
