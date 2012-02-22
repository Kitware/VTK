add_subdirectory(Cxx)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-AMR ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/AMR"
    VTK_AMR_EXPORT
    )
ENDIF(PYTHON_EXECUTABLE)