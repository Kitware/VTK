set(MyTests
  TestBallAndStick.cxx
  TestBondColorModeDiscreteByAtom.cxx
  TestBondColorModeSingleColor.cxx
  TestCompositeRender.cxx
  TestFastRender.cxx
  TestLiquoriceSticks.cxx
  TestMolecule.cxx
  TestMoleculeSelection.cxx
  TestMultiCylinderOn.cxx
  TestMultiCylinderOff.cxx
  TestPeriodicTable.cxx
  TestProgrammableElectronicData.cxx
  TestSimpleBondPerceiver.cxx
  TestVDWSpheres.cxx
  )

# Tests with data
if(VTK_DATA_ROOT)
  set(MyTests
    ${MyTests}
    TestCMLMoleculeReader.cxx
    )
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include("${VTK_SOURCE_DIR}/Testing/Rendering/vtkTestingObjectFactory.cmake")
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
remove(TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 10)
    endif()
    add_test(NAME Chemistry-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Chemistry/${TName}.png
        -E ${_error_threshold})
  else()
    add_test(NAME Chemistry-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach ()
