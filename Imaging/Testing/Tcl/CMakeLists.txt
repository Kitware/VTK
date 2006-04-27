IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

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
        TestAllMathematics
        TestAllShrinks
        TestBlendStencil
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
        TestImageWeightedSum
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
        TestThreshold
        TestVariance3D
        TestWipe
        TestWrapPad
        voxelModel
        )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Imaging/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
        )
    ENDFOREACH ( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-Imaging ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Imaging)

ADD_TEST(TestSetObjectMacro-Imaging ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Imaging/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
