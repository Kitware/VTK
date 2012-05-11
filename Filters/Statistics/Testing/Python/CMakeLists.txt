if(VTK_PYTHON_EXE)
  foreach(tfile
   kMeansDistanceCalculator
   )
    add_test(NAME ${vtk-module}Python-${tfile}
      COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py)
  endforeach()
endif()
