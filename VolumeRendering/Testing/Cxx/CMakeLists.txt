# if we are using the display add the tests
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT VolumeRendering)
  # add tests that do not require data
  SET(MyTests)
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests
      HomogeneousRayIntegration.cxx
      LinearRayIntegration.cxx
      PartialPreIntegration.cxx
      PreIntegrationIncremental.cxx
      PreIntegrationNonIncremental.cxx
      ProjectedTetrahedraZoomIn.cxx
      TestFinalColorWindowLevel.cxx
      TestGPURayCastCompositeMaskBlend.cxx
      TestGPURayCastCompositeMask.cxx
      TestGPURayCastCompositeShadeMask.cxx
      TestGPURayCastCompositeToMIP.cxx
      TestGPURayCastCropping.cxx
      TestGPURayCastDataTypesMinIP.cxx
      TestGPURayCastDataTypesMIP.cxx
      TestGPURayCastFourComponentsComposite.cxx
      TestGPURayCastFourComponentsCompositeStreaming.cxx
      TestGPURayCastFourComponentsMinIP.cxx
      TestGPURayCastFourComponentsMIP.cxx
      TestGPURayCastMIPToComposite.cxx
      TestGPURayCastNearestDataTypesMIP.cxx
      TestHAVSVolumeMapper.cxx
      TestMinIntensityRendering.cxx
      TestProjectedTetrahedra.cxx
      volProt.cxx
      ZsweepConcavities.cxx
      )
    SET(MyTestSupport
      ExerciseUnstructuredGridRayCastMapper.cxx
      )
  ENDIF (VTK_DATA_ROOT)

# Force to not enable floating point exception as we hit some exception
# inside the OpenGL driver at vtkgl::LinkProgram() (nVidia 185.18.36).
set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN)
set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "${CMAKE_TESTDRIVER_BEFORE_TESTMAIN}
    try {")

  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests} ${MyTestSupport} )
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkVolumeRendering)
  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)

  #
  # Add all the executables 
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
