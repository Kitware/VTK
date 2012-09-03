if(VTK_PYTHON_EXE)
  foreach(tfile
      TestAMRResampleFilter
      TestAMRExtractLevel
   )
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Python-${tfile}
      COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py -D ${VTK_DATA_ROOT})
  endif()
  endforeach()
endif()
