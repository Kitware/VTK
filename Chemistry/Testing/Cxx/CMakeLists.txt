IF (VTK_USE_RENDERING AND VTK_USE_CHEMISTRY)

  SET(KIT Chemistry)

  SET(MyTests
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
  IF(VTK_DATA_ROOT)
    SET(MyTests
      ${MyTests}
      TestCMLMoleculeReader.cxx
      )
    IF(VTK_USE_OPENQUBE)
      INCLUDE(${QT_USE_FILE})
      # Add Eigen to the include path for the OpenQube headers
      INCLUDE_DIRECTORIES(${EIGEN2_INCLUDE_DIR})
      SET(MyTests
        ${MyTests}
        TestOpenQubeElectronicData.cxx
        TestOpenQubeMOPACDensity.cxx
        TestOpenQubeMOPACOrbital.cxx
      )
    ENDIF(VTK_USE_OPENQUBE)
  ENDIF(VTK_DATA_ROOT)

  # Use the testing object factory, to reduce boilerplate code in tests.
  INCLUDE("${VTK_SOURCE_DIR}/Rendering/vtkTestingObjectFactory.cmake")
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests
                        vtkChemistry
                        vtkRendering
                        vtkVolumeRendering)

  SET (TestsToRun ${Tests})
  LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

  # Add all the executables
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      IF(${${TName}Error})
        SET(_error_threshold ${${TName}Error})
      ELSE()
        SET(_error_threshold 10)
      ENDIF()
      ADD_TEST(Chemistry-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png
        -E ${_error_threshold})
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(Chemistry-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test)

ENDIF (VTK_USE_RENDERING AND VTK_USE_CHEMISTRY)
