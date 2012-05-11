if (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  if(VTK_DATA_ROOT)
    foreach( tfile
      squadViewer
      )
      add_test(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
        ${vtkFiltersSources_SOURCE_DIR}/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Filtering/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    endforeach()
    if(VTK_USE_TK)
      add_test(squadViewer-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl
        ${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/squadViewer.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Filtering/squadViewer.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    endif()
  #
  # If we do not have the data, still run the tests that we can
  #
  else()
  endif()

endif()
