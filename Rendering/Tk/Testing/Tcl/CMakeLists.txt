# Tests that use the vtkTkRenderWidget
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    # require Rendering/Volume
    cursor3D
    # require Rendering/FreeType
    # TestTextActor3D # test has been broken since before VTK 5.10
  )
endif()

foreach(tfile ${tests})
  if("${tfile}" STREQUAL "cursor3D")
    set(baseline_kit VolumeRendering)
  else()
    set(baseline_kit Rendering)
  endif()

  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/${baseline_kit}/${tfile}.png
   )
endforeach()
