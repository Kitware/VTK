IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
        ContinuousClose2D
        IdealHighPass
        LaplacianEdgeEnhance
	reconstructSurface
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
        TestAllBlends
        TestAllBlendsFloat
        TestAllFlips
        TestAllLogic
        TestAllMaskBits
        TestAllShrinks
        TestButterworthLowPass
        TestButterworthHighPass
        TestCache
        TestChangeInformation
	TestCheckerboard
        TestCityBlockDistance
        TestConvolve
        TestCorrelation
        TestDilateErode3D
        TestDivergence
        TestDotProduct
        TestEuclideanDistance
        TestEuclideanToPolar
        TestGradient
        TestGradientMagnitude
        TestGradientMagnitude2
        TestGridSource
        TestHSVToRGB
        TestHybridMedian2D
        TestIdealLowPass
        TestInPlaceFilter
        TestIslandRemoval2D
        TestMapToRGBABlockStreaming
        TestMapToWindowLevelColors
        TestMask2
        TestMedian3D
        TestNormalize
        TestOpenClose3D
        TestPermute
        TestQuantizeTo16Colors 
        TestRange3D
        TestResample
        TestSimpleImageExample
        TestSkeleton2D
        TestSobel2D
        TestSobel3D
        TestStencilWithFunction
        TestStencilWithImage
        TestVariance3D
        TestWrapPad
	voxelModel
        )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Imaging/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
        )
    ENDFOREACH ( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

