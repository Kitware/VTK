IF (VTK_USE_DISPLAY)
IF (VTK_DATA_ROOT)
  ADD_TEST(RectOutline-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/RectOutline.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/RectOutline.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ExtractVOI-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestExtractVOI.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestExtractVOI.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(TestExtractCTHPart ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestExtractCTHPart.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestExtractCTHPart.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ExtrudePiece-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestExtrudePiece.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestExtrudePiece.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(PolyDataPieces-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPolyDataPieces.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestPolyDataPieces.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(UnstructuredPieces-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestUnstructuredPieces.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestUnstructuredPieces.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ImageStreamer-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestImageStreamer.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestImageStreamer.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(PImageWriter-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPImageWriter.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestPImageWriter.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(CutMaterial-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestCutMaterial.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestCutMaterial.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(BranchExtentTranslator-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestBranchExtentTranslator.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestBranchExtentTranslator.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(PDataSetReaderGrid-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPDataSetReaderGrid.tcl
           -D ${VTK_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V Baseline/Parallel/TestPDataSetReaderGrid.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  IF(PVTK_EXECUTABLE AND VTK_MPIRUN_EXE)
    ADD_TEST(TestPProbe-image 
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
             ${CXX_TEST_PATH}${CXX_TEST_CONFIG}/pvtk${CMAKE_EXECUTABLE_SUFFIX}
             ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl
             ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPProbe.tcl
             -D ${VTK_DATA_ROOT}
             -T ${VTK_BINARY_DIR}/Testing/Temporary
             -V Baseline/Parallel/TestPProbe.png
             -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
             ${VTK_MPI_POSTFLAGS})
  ENDIF(PVTK_EXECUTABLE AND VTK_MPIRUN_EXE)
ELSE (VTK_DATA_ROOT)
  # All rtImageTest tests require VTKData to pass...
  # Do not add any rtImageTest based tests in this block.
ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-Parallel ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Parallel)

ADD_TEST(TestSetObjectMacro-Parallel ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Parallel/vtk\\\\*.h"
         "vtkSetObjectMacro")
