IF (HAVE_PNG)
 IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(TestFFT-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestFFT.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestFFT.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAccumulate-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAccumulate.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAccumulate.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAllBlends-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAllBlends.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAllBlends.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAllBlendsFloat-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAllBlendsFloat.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAllBlendsFloat.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAllFlips-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAllFlips.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAllFlips.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAllLogic-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAllLogic.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAllLogic.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAllMaskBits-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAllMaskBits.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAllMaskBits.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestAllShrinks-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestAllShrinks.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestAllShrinks.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestGradient-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestGradient.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestGradient.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(TestGradientMagnitude-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/TestGradientMagnitude.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Imaging/TestGradientMagnitude.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
#    ADD_TEST(Delaunay2D ${VTK_EXECUTABLE}
#      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
#      ${VTK_SOURCE_DIR}/Imaging/Testing/Tcl/Delaunay2D.tcl
#      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ENDIF (VTK_DATA_ROOT)

 ENDIF (VTK_USE_RENDERING)
ENDIF (HAVE_PNG)

