IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      # Add test converted from Tcl
      SET (CONVERTED_TESTS)
      SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Wrapping/Python/vtkTclToPyConvertor.py")
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
        ADD_CUSTOM_COMMAND(
          OUTPUT "${output}"
          COMMAND ${PYTHON_EXECUTABLE}
          ARGS ${CONVERTER_SCRIPT}
            "-i" "${input}" "-o" "${output}" 
            "-A" "${VTK_BINARY_DIR}/Wrapping/Python" "-A" "${LIBRARY_OUTPUT_PATH}"
          DEPENDS "${input}" "${CONVERTER_SCRIPT}"
          COMMENT "Converting Tcl test"
          )

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Wrapping/Python/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Hybrid/${tfile}.png
          -A "${VTK_BINARY_DIR}/Wrapping/Python" 
          -A "${LIBRARY_OUTPUT_PATH}"
          )
      ENDFOREACH( tfile ) 
      ADD_CUSTOM_TARGET(HybridPythonTests ALL DEPENDS ${CONVERTED_TESTS}) 
      ADD_DEPENDENCIES(HybridPythonTests vtkpython_pyc)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
