# Tests with test images in Baseline/VolumeRendering
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    gaussian
    TestBunykRayCastFunction
    TestFixedPointRayCasterLinearCropped
    TestFixedPointRayCasterLinear
    TestFixedPointRayCasterNearestCropped
    TestFixedPointRayCasterNearest
    TestLODProp3D
    TestPTZSweep
    volRCClipPlanes
    volRCCropRegions
    volRCRotateClip
    volTM2DCropRegions
    volTM2DRotateClip
    volTM3DCompressedCropRegions
    volTM3DCropRegions
    volTM3DRotateClip
    VolumeOutlineSourceClipped
    VolumeOutlineSource
    VolumePickerCrop
    VolumePicker
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/VolumeRendering/${tfile}.png
   )
endforeach()
