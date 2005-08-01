IF (VTK_USE_RENDERING)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      # Add test converted from Tcl
      SET (CONVERTED_TESTS)
      SET (TESTS_TO_CONVERT)
      SET (CONVERSIONLIST)
      SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkTclToPyConvertor.py")
      SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
      IF(MAKE_SYSTEM OR UNIX)
        SET(CFG_INIT "")
      ENDIF(MAKE_SYSTEM OR UNIX)
      SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkpython${EXE_EXT})
      FOREACH( tfile
          3DMorph
          appendImplicitModel
          appendImplicitModelNoBounds
          banana
          bore
          camera
          clipHex
          clipPyramid
          clipTet
          clipWedge
          cubeAxes
          depthSort
          earth
          imageToPolyData
          #financialField2 -- arrays
          #financialField3 -- arrays
          largeImageOffset
          largeImageParallel
          #MassProperties -- complicated global/local mess up 
          OverlayTextOnImage
          TestCaptionActor
          TestFacetReader
          TestGreedyTerrainDecimation
          TestGridWarp3D
          TestGridWarpCubic
          TestGridWarpLinear
          TestGridWarpNearest
          #TestICPTransform  --complex arrays
          TestImageStencilWithPolydata
          TestPCA
          TestProcrustes
          TestProjectedTerrainPath
          TestThinPlateWarp
          TestThinPlateWarp3D
          text
          VRMLImporter
          WarpPolyData
          xyPlot
          xyPlot2
          xyPlot3
          xyPlot4
          )
        
        SET (input "${VTK_SOURCE_DIR}/Hybrid/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/Hybrid/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        SET (CONVERSIONLIST ${CONVERSIONLIST} "${input};${output}")
        SET (TESTS_TO_CONVERT ${TESTS_TO_CONVERT} "${input}")

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Hybrid/${tfile}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDFOREACH( tfile ) 
      
      WRITE_FILE(${VTK_BINARY_DIR}/Hybrid/Testing/Python/conversionlist
        "${CONVERSIONLIST}")

      ADD_CUSTOM_COMMAND(
        OUTPUT "${VTK_BINARY_DIR}/Hybrid/Testing/Python/conversion_complete"
        COMMAND ${CMD}
        ARGS ${CONVERTER_SCRIPT}
          -l "${VTK_BINARY_DIR}/Hybrid/Testing/Python/conversionlist"
          -t "${VTK_BINARY_DIR}/Hybrid/Testing/Python/conversion_complete"
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
        DEPENDS ${TESTS_TO_CONVERT} 
            ${VTK_BINARY_DIR}/Hybrid/Testing/Python/conversionlist 
            ${CONVERTER_SCRIPT}
        COMMENT "Converting Tcl test"
        )
      ADD_CUSTOM_TARGET(HybridPythonTests ALL 
        DEPENDS "${VTK_BINARY_DIR}/Hybrid/Testing/Python/conversion_complete") 
      ADD_DEPENDENCIES(HybridPythonTests vtktcltest2py_pyc)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING)
