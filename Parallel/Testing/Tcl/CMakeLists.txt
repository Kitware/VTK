IF (VTK_DATA_ROOT)
  ADD_TEST(RectOutline-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/RectOutline.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/RectOutline.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ExtractVOI-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestExtractVOI.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestExtractVOI.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ExtrudePiece-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestExtrudePiece.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestExtrudePiece.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
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
  ADD_TEST(BranchExtentTranslator-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestBranchExtentTranslator.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestBranchExtentTranslator.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(PDataSetReaderGrid-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPDataSetReaderGrid.tcl
           -D ${VTK_DATA_ROOT}
           -V Baseline/Parallel/TestPDataSetReaderGrid.tcl.png
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  IF(PVTK_EXECUTABLE AND UNIX)
    ADD_TEST(TestPProbe-image ${VTK_MPIRUN_EXE} ${VTK_MPI_PREFLAGS} ${PVTK_EXECUTABLE}
             ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl
             ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPProbe.tcl
             -D ${VTK_DATA_ROOT}
             -V Baseline/Parallel/TestPProbe.png
             -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
             ${VTK_MPI_POSTFLAGS})
  ELSE(PVTK_EXECUTABLE AND UNIX)
    ADD_TEST(TestPProbe-image ${VTK_EXECUTABLE}
             ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl
             ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPProbe.tcl
             -D ${VTK_DATA_ROOT}
             -V Baseline/Parallel/TestPProbe.png
             -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ENDIF(PVTK_EXECUTABLE AND UNIX)
ELSE (VTK_DATA_ROOT)
  ADD_TEST(PolyDataPieces ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPolyDataPieces.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(CutMaterial-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestCutMaterial.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(BranchExtentTranslator-image ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestBranchExtentTranslator.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ENDIF (VTK_DATA_ROOT)

ADD_TEST(PrintSelf-Parallel ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Parallel)

ADD_TEST(TestSetObjectMacro-Parallel ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
	 "${VTK_SOURCE_DIR}/Parallel/vtk\\\\*.h"
	 "vtkSetObjectMacro")
