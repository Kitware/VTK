IF (VTK_USE_RENDERING)
  #
  # Add all the executables that require rendering
  #
  ADD_EXECUTABLE(ImportExport ImportExport.cxx)

  TARGET_LINK_LIBRARIES(ImportExport vtkRendering vtkIO)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(ImportExport-image ${CXX_TEST_PATH}/ImportExport
                -D ${VTK_DATA_ROOT}
                -V Baseline/Imaging/ImportExport.png)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)


