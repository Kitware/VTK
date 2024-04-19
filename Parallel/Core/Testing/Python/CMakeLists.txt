vtk_add_test_python(
  TestPolyDataPieces.py
  TestUnstructuredPieces.py
  )

# In VTK 5, TestSockets.py used to be run as follows on UNIX,
# it required SocketServer.cxx and SocketClient.cxx to be
# compiled as executables.
#
#   ADD_TEST(Sockets-image ${PYTHON_EXECUTABLE}
#      ${VTK_SOURCE_DIR}/Parallel/Testing/Cxx/TestSockets.py
#      ${EXECUTABLE_OUTPUT_PATH}/SocketServer
#      ${EXECUTABLE_OUTPUT_PATH}/SocketClient
#      ${VTK_DATA_ROOT}
#      Baseline/Parallel/SocketClient.cxx.png
#      ${VTK_BINARY_DIR}/Testing/Temporary)
