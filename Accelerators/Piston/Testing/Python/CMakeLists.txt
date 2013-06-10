if (VTK_PYTHON_EXE)
  #
  # Test base functionality.
  #
  set(TestConversion_OPTS NO_VALID)
  foreach ( tfile
      TestConversion
      TestContour
      TestRendering
      TestThreshold
      TestSlice
      TestSort
      )
    set(${tfile}_ARGS --normalize)
    vtk_add_test_python(${tfile}.py NO_RT ${${tfile}_OPTS})
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
    ExternalData_add_test(VTKData
      NAME ${vtk-module}Python-TestPlaced${tfile}
      COMMAND ${VTK_PYTHON_EXE}
      ${CMAKE_CURRENT_SOURCE_DIR}/Test${tfile}.py
      -B "DATA{../Data/Baseline/,REGEX:Test${tfile}(_[0-9]+)?.png}"
      -D ${VTK_TEST_DATA_DIR}/Data)
  endforeach ( )

  #
  # Exercise thrust to gl direct on GPU rendering.
  #
  ExternalData_add_test(VTKData
    NAME ${vtk-module}Python-TestDirectRendering
    COMMAND ${VTK_PYTHON_EXE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestRendering.py
    -B "DATA{../Data/Baseline/,REGEX:TestRendering(_[0-9]+)?.png}"
    -D ${VTK_TEST_DATA_DIR}/Data
    --gpu_render)

endif ()
