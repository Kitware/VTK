IF (HAVE_PNG)
 IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
        ContinuousClose2D
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
        TestCityBlockDistance
        TestCorrelation
        TestDilateErode3D
        TestDivergence
        TestDotProduct
        TestInPlaceFilter
        TestFFT
        TestGradient
        TestGradientMagnitude
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
ENDIF (HAVE_PNG)

