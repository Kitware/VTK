IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      IF(NOT VTK_USE_COCOA)
        FOREACH ( tfile
                  TestTkRenderWidget
                  TestTkRenderWindowInteractor
                  )
          ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
            ${VTK_SOURCE_DIR}/Rendering/Testing/Python/${tfile}.py
            -B ${VTK_DATA_ROOT}/Baseline/Rendering
            -D ${VTK_DATA_ROOT}/Data)
        ENDFOREACH( tfile )
      ENDIF(NOT VTK_USE_COCOA)

      # Add test converted from Tcl
      SET (CONVERTED_TESTS)
      SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkTclToPyConvertor.py")
      SET (TESTS_TO_CONVERT)
      SET (CONVERSIONLIST)
      IF(WIN32)
        # VS-6.0 does not use current environment PATH when running custom commands,
        # hence it might fail to locate the python dlls if we run vtkpython.
        # Hence we use python executable. Python executable cannot be used to import
        # vtk modules on Sun/SGI hence, for non-windows machines we use vtkpython.
        SET(CMD ${PYTHON_EXECUTABLE})
      ELSE(WIN32)
        SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
        IF(MAKE_SYSTEM OR UNIX)
          SET(CFG_INIT "")
        ENDIF(MAKE_SYSTEM OR UNIX)
        SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkpython${EXE_EXT})
      ENDIF(WIN32)
      FOREACH( tfile
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
          #TestStyleJoystickActor --file source
          #TestStyleJoystickCamera
          TestStyleRubberBandZoom
          #TestStyleTerrain  -- file source
          #TestStyleTrackballActor -- file source
          #TestStyleTrackballCamera -- file source
          TestTiling
          TestTriangleStripCellColor
          )
        
        SET (input "${VTK_SOURCE_DIR}/Rendering/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/Rendering/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        SET (CONVERSIONLIST ${CONVERSIONLIST} "${input};${output}")
        SET (TESTS_TO_CONVERT ${TESTS_TO_CONVERT} "${input}")

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Rendering/${tfile}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDFOREACH( tfile ) 
      WRITE_FILE(${VTK_BINARY_DIR}/Rendering/Testing/Python/conversionlist
        "${CONVERSIONLIST}")

      ADD_CUSTOM_COMMAND(
        OUTPUT "${VTK_BINARY_DIR}/Rendering/Testing/Python/conversion_complete"
        COMMAND ${CMD}
        ARGS ${CONVERTER_SCRIPT}
          -l "${VTK_BINARY_DIR}/Rendering/Testing/Python/conversionlist"
          -t "${VTK_BINARY_DIR}/Rendering/Testing/Python/conversion_complete"
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_BINARY_DIR}/Wrapping/Python"
          -A "${VTK_LIBRARY_DIR}"
        DEPENDS ${TESTS_TO_CONVERT} 
            ${VTK_BINARY_DIR}/Rendering/Testing/Python/conversionlist 
            ${CONVERTER_SCRIPT}
        COMMENT "Converting Tcl test"
        )
      ADD_CUSTOM_TARGET(RenderingPythonTests ALL 
        DEPENDS "${VTK_BINARY_DIR}/Rendering/Testing/Python/conversion_complete")

      ADD_DEPENDENCIES(RenderingPythonTests vtktcltest2py_pyc)
    
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

