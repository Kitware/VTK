IF (EXECUTABLE_OUTPUT_PATH)
  SET (PARALLEL_EXEC_PATH ${EXECUTABLE_OUTPUT_PATH})
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (PARALLEL_EXEC_PATH ${VTK_BINARY_DIR}/Parallel/Testing/Cxx/${CMAKE_CFG_INTDIR})
ENDIF (EXECUTABLE_OUTPUT_PATH)

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
ELSE (VTK_DATA_ROOT)
  ADD_TEST(PolyDataPieces ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestPolyDataPieces.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(UnstructuredPieces ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestUnstructuredPieces.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ADD_TEST(ImageStreamer ${VTK_EXECUTABLE}
           ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
           ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestImageStreamer.tcl
           -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ENDIF (VTK_DATA_ROOT)

#IF (VTK_DATA_ROOT)
#  ADD_TEST(Sockets-image ${TCLSHCOMMAND}
#    ${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/TestSockets.tcl 
#    ${PARALLEL_EXEC_PATH}/SocketServer
#    ${PARALLEL_EXEC_PATH}/SocketClient
#    ${VTK_DATA_ROOT}
#    Baseline/Parallel/SocketClient.png)
#ENDIF (VTK_DATA_ROOT)

