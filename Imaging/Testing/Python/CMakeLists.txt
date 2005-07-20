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
          ContinuousClose2D
          IdealHighPass
          LaplacianEdgeEnhance
          #reconstructSurface -- scanf
          resampledTexture
          ResliceColorWrap
          ResliceInterpolationModes
          ResliceInterpolationOblique
          ResliceMirrorOblique
          ResliceMirrorPad
          ResliceOptimizationOff
          ReslicePermutations
          ReslicePermuteResample
          ResliceWrapOblique
          ResliceWrapPad
          Spectrum
          TestAnisotropicDiffusion2D
          TestAnisotropicDiffusion3D
          TestAccumulate
          #TestAllBlends -- complicated string substitutions
          #TestAllBlendsFloat -- complicated string substitutions
          TestAllFlips
          #TestAllLogic --  method call has string substitution
          #TestAllMaskBits --  method call has string substitution
          #TestAllMathematics --  method call has string substitution
          #TestAllShrinks  --  method call has string substitution
          TestButterworthLowPass
          TestButterworthHighPass
          TestCache
          TestChangeInformation
          TestCheckerboard
          TestCityBlockDistance
          TestConvolve
          TestCorrelation
          TestDivergence
          TestDotProduct
          TestEuclideanDistance
          TestEuclideanDistanceCached
          TestEuclideanToPolar
          TestFFTCorrelation
          TestGradientMagnitude
          TestGradientMagnitude2
          TestHSIToRGB
          TestHSVToRGB
          TestHybridMedian2D
          TestIdealLowPass
          TestImageCanvas
          TestInPlaceFilter
          TestIslandRemoval2D
          TestMapToRGBABlockStreaming
          TestMapToWindowLevelColors
          TestMapToWindowLevelColors2
          TestMask2
          TestMedian3D
          TestNormalize
          TestOpenClose3D
          TestPermute
          TestQuantizeTo16Colors 
          TestRange3D
          TestResample
          TestSeparableFilter
          TestShiftScale
          TestShiftScale2
          TestSimpleImageExample
          TestSkeleton2D
          TestSobel2D
          TestSobel3D
          TestStencilWithFunction
          TestStencilWithImage
          #TestThreshold -- uses lists in lists etc...
          TestVariance3D
          #TestWipe  --  method call has string substitution
          TestWrapPad
          voxelModel
          )
        
        SET (input "${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/Imaging/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        ADD_CUSTOM_COMMAND(
          OUTPUT "${output}"
          COMMAND ${PYTHON_EXECUTABLE}
          ARGS ${CONVERTER_SCRIPT}
            "-i" "${input}" "-o" "${output}" 
            "-A" "${VTK_BINARY_DIR}/Wrapping/Python" "-A" "${LIBRARY_OUTPUT_PATH}"
            -A "${LIBRARY_OUTPUT_PATH}/Debug"
            -A "${LIBRARY_OUTPUT_PATH}/Release"
          DEPENDS "${input}" "${CONVERTER_SCRIPT}"
          COMMENT "Converting Tcl test"
          )

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Wrapping/Python/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Imaging/${tfile}.png
          -A "${VTK_BINARY_DIR}/Wrapping/Python" 
          -A "${LIBRARY_OUTPUT_PATH}"
          )
      ENDFOREACH( tfile ) 
      ADD_CUSTOM_TARGET(ImagingPythonTests ALL DEPENDS ${CONVERTED_TESTS}) 
      ADD_DEPENDENCIES(ImagingPythonTests vtkpython_pyc)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
