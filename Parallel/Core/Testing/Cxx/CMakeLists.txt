vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestFieldDataSerialization.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)

if (PYTHON_EXECUTABLE)
  vtk_module_test_executable(
    ${vtk-module}-TestSocketCommunicator TestSocketCommunicator.cxx)

  add_test(NAME ${vtk-module}-TestSocketCommunicator
           COMMAND ${PYTHON_EXECUTABLE}
                   ${VTK_CMAKE_DIR}/vtkTestDriver.py
                   # indicate that the processes are not run sequentially,
                   --process
                     $<TARGET_FILE:${vtk-module}-TestSocketCommunicator>
                     -T ${VTK_TEST_OUTPUT_DIR}
                     --server
                   --process
                     $<TARGET_FILE:${vtk-module}-TestSocketCommunicator>
                     -T ${VTK_TEST_OUTPUT_DIR}
          )
endif()
