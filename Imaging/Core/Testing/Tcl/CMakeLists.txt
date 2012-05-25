# Tests with test images in Baseline/Imaging
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    ContinuousClose2D
    IdealHighPass
    LaplacianEdgeEnhance
    MultipleUpdateExtents
    reconstructSurface
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
    TestAccumulate
    TestAllBlends
    TestAllBlendsFloat
    TestAllFlips
    TestAllLogic
    TestAllMaskBits
    TestAllMathematics
    TestAllShrinks
    TestAnisotropicDiffusion2D
    TestAnisotropicDiffusion3D
    TestBlendStencil
    TestButterworthHighPass
    TestButterworthLowPass
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
    TestImageThresholdConnectivity
    TestImageWeightedSum
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
    TestThreshold
    TestVariance3D
    TestWipe
    TestWrapPad
    voxelModel
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Imaging/${tfile}.png
   )
endforeach()

# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    imageMCAll
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Graphics/${tfile}.png
   )
endforeach()

# Tests with test images in Baseline/Parallel
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    TestExtractVOI
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/${tfile}.png
   )
endforeach()
