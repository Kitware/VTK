IF (VTK_USE_RENDERING)
  #
  # Add executable tests with the data
  #
  FOREACH(test CellLocator
               PointLocator
               Mace
               expCos
               volProt
               RGrid
               FrustumClip)
    ADD_EXECUTABLE(${test} ${test}.cxx)
    TARGET_LINK_LIBRARIES(${test} vtkRendering vtkIO)

    IF (VTK_DATA_ROOT)
      ADD_TEST(${test}-image ${CXX_TEST_PATH}/${test}
                             -D ${VTK_DATA_ROOT}
                             -V Baseline/Graphics/${test}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(${test} ${CXX_TEST_PATH}/${test})
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH(test)
ENDIF (VTK_USE_RENDERING)
