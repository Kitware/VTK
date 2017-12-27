vtk_add_test_cxx(vtkParallelCoreCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestFieldDataSerialization.cxx
  )
vtk_test_cxx_executable(vtkParallelCoreCxxTests tests)

if (PYTHON_EXECUTABLE)
  vtk_module_test_executable(
    vtkParallelCore-TestSocketCommunicator TestSocketCommunicator.cxx)

  add_test(NAME vtkParallelCore-TestSocketCommunicator
           COMMAND ${PYTHON_EXECUTABLE}
                   ${VTK_CMAKE_DIR}/vtkTestDriver.py
                   # indicate that the processes are not run sequentially,
                   --process
                     $<TARGET_FILE:vtkParallelCore-TestSocketCommunicator>
                     -T ${VTK_TEST_OUTPUT_DIR}
                     --server
                   --process
                     $<TARGET_FILE:vtkParallelCore-TestSocketCommunicator>
                     -T ${VTK_TEST_OUTPUT_DIR}
          )
endif()
