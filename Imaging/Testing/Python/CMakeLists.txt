INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      SET(tests
        ContinuousClose2D
        IdealHighPass
        LaplacianEdgeEnhance
        MultipleUpdateExtents
        #reconstructSurface -- scanf
        resampledTexture
        ResliceBSpline
        ResliceColorWrap
        ResliceInformationInput
        ResliceInterpolationModes
        ResliceInterpolationOblique
        ResliceLanczosKaiser
        ResliceMirrorOblique
        ResliceMirrorPad
        ResliceOptimizationOff
        ReslicePermutations
        ReslicePermuteResample
        ReslicePermuteSlab
        ResliceSlabModes
        ResliceToColors
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
        TestImageProjection
        #TestImageThresholdConnectivity -- uses lists in lists etc...
        TestInPlaceFilter
        TestIslandRemoval2D
        TestLassoStencil
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
        TestROIStencil
        TestSeparableFilter
        TestShiftScale
        TestShiftScale2
        TestSimpleImageExample
        TestSkeleton2D
        TestSobel2D
        TestSobel3D
        TestStencilToImage
        TestStencilWithFunction
        TestStencilWithImage
        #TestThreshold -- uses lists in lists etc...
        TestVariance3D
        #TestWipe  --  method call has string substitution
        TestWrapPad
        voxelModel
        )

      CONVERT_TCL_TEST_TO_PY (
         tests
         Imaging)

      ENDIF (VTK_PYTHON_EXE)
    ENDIF (VTK_DATA_ROOT)
  ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
