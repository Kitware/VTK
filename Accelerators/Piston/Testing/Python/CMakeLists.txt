if (VTK_PYTHON_EXE)
  #
  # Test base functionality.
  #
  string (REPLACE "vtk" "" _baselinedname ${vtk-module})
  foreach ( tfile
      TestConversion
      TestContour
      TestRendering
      TestThreshold
      TestSlice
      TestSort
      )
    add_test(NAME ${vtk-module}Python-${tfile}
      COMMAND ${VTK_PYTHON_EXE}
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -B ${VTK_DATA_ROOT}/Baseline/${_baselinedname}
      -D ${VTK_DATA_ROOT}/Data
      --normalize)
  endforeach ( )

  #
  # Exercise origin and spacing aspect of vtkImageData
  #
  foreach ( tfile
      Contour
      Threshold
      Slice
      Sort
      )
    add_test(NAME ${vtk-module}Python-TestPlaced${tfile}
      COMMAND ${VTK_PYTHON_EXE}
      ${CMAKE_CURRENT_SOURCE_DIR}/Test${tfile}.py
      -B ${VTK_DATA_ROOT}/Baseline/${_baselinedname}
      -D ${VTK_DATA_ROOT}/Data)
  endforeach ( )

  #
  # Exercise thrust to gl direct on GPU rendering.
  #
  add_test(NAME ${vtk-module}Python-TestDirectRendering
    COMMAND ${VTK_PYTHON_EXE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestRendering.py
    -B ${VTK_DATA_ROOT}/Baseline/${_baselinedname}
    -D ${VTK_DATA_ROOT}/Data
    --gpu_render)

endif ()
