IF (VTK_DATA_ROOT)
  ADD_TEST(PolyDataPieces-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPolyDataPieces.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestPolyDataPieces.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(UnstructuredPieces-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestUnstructuredPieces.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestUnstructuredPieces.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ImageStreamer-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestImageStreamer.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestImageStreamer.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(PImageWriter-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPImageWriter.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestPImageWriter.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(CutMaterial-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestCutMaterial.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestCutMaterial.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ELSE (VTK_DATA_ROOT)
  ADD_TEST(PolyDataPieces ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPolyDataPieces.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(CutMaterial-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestCutMaterial.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ENDIF (VTK_DATA_ROOT)

