INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      IF(VTK_USE_TK)
        FOREACH ( tfile
                  TestTkRenderWidget
                  TestTkRenderWindowInteractor
                  )
          ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
            ${VTK_SOURCE_DIR}/Rendering/Testing/Python/${tfile}.py
            -B ${VTK_DATA_ROOT}/Baseline/Rendering
            -D ${VTK_DATA_ROOT}/Data)
        ENDFOREACH( tfile )
      ENDIF(VTK_USE_TK)
      
      #############################
      # Add GL2PS specific tests.
      IF(VTK_USE_GL2PS)
        FOREACH ( tfile
                  TestGL2PSExporter
                  )
          ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
            ${VTK_SOURCE_DIR}/Rendering/Testing/Python/${tfile}.py
            -B ${VTK_DATA_ROOT}/Baseline/Rendering
            -D ${VTK_DATA_ROOT}/Data)
        ENDFOREACH( tfile )
      ENDIF(VTK_USE_GL2PS)
      #############################

      # Add test converted from Tcl
      SET(tests
          assembly
          CamBlur
          #cells -- too complex file IO
          ImageActor
          ImageActorStressed
          labeledContours
          labeledMesh
          multiLineText
          pickCells
          propAssembly
          rendererSource
          rotations
          ScalarBar
          TestFlyTo
          TestOnePoint
          TestParallelCoordinates
          #TestStyleTerrain  -- file source
          TestTiling
          TestTriangleStripCellColor
          )
        SET(tests ${tests}
          #TestStyleJoystickActor --file source
          #TestStyleJoystickCamera
          TestStyleRubberBandZoom
          #TestStyleTrackballActor -- file source
          #TestStyleTrackballCamera -- file source
          )
        CONVERT_TCL_TEST_TO_PY(
          tests
          Rendering)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

