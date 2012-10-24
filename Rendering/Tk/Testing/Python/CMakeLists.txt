if(VTK_PYTHON_EXE)
  foreach(tfile
    TestTkRenderWidget
    TestTkRenderWindowInteractor
    )
    if(VTK_DATA_ROOT)
      add_test(NAME ${vtk-module}Python-${tfile}
        COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -B ${VTK_DATA_ROOT}/Baseline/Rendering/
        )
    endif()
  endforeach()
endif()
