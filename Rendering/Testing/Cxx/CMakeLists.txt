SET(KIT Rendering)

SET(RenderingTests
  otherCoordinate.cxx
  TestPriorityStreaming.cxx
  )

SET(RenderingTestsWithArguments)

IF(VTK_USE_DISPLAY)
  # For tests that actually use a vtkRenderWindow
  SET(RenderingTestsWithArguments
    ${RenderingTestsWithArguments}
    LoadOpenGLExtension.cxx
    TestOrderedTriangulator.cxx
    )

  # Do not run the timers test on X windows builds
  # until somebody can figure out how to make it quit
  # when it's done. Calling ExitCallback does not
  # exit from this unless a user is hovering over
  # the machine and wiggling the mouse over the
  # window... Arg.
  #
  IF(NOT VTK_USE_X)
    SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      TestInteractorTimers.cxx
      )
  ENDIF(NOT VTK_USE_X)

  IF(VTK_DATA_ROOT)
    SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      TestAreaSelections.cxx
      )
  ENDIF(VTK_DATA_ROOT)

ENDIF(VTK_USE_DISPLAY)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  ${RenderingTests};${RenderingTestsWithArguments}
  EXTRA_INCLUDE vtkTestDriver.h
  )

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)


SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables 
FOREACH (test ${RenderingTests})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
    -T ${VTK_BINARY_DIR}/Testing/Temporary
    ) 
ENDFOREACH (test) 

FOREACH (test ${RenderingTestsWithArguments})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  IF (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      )
  ENDIF (VTK_DATA_ROOT)
ENDFOREACH(test)

#
# Add other odd tests or executables
#
FOREACH (exe
    TimeRenderer
    TimeRenderer2
    VTKBenchMark
    )
  ADD_EXECUTABLE(${exe} ${exe}.cxx)
  TARGET_LINK_LIBRARIES(${exe} vtkRendering vtkIO)
  IF (APPLE)
    TARGET_LINK_LIBRARIES(${exe} "-framework GLUT")
  ENDIF (APPLE)
ENDFOREACH (exe)
