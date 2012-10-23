if(VTK_PYTHON_EXE)
  foreach(tfile
      TestAMRResampleFilter
      TestAMRExtractLevel
      TestAMRSliceFilter
   )
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Python-${tfile}
      COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py -D ${VTK_DATA_ROOT})
    set_tests_properties(${vtk-module}Python-${tfile} PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  endif()
  endforeach()
endif()