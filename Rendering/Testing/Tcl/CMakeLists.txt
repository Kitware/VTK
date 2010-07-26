IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    SET( tests
      assembly
      CamBlur
      cells 
      ImageActor
      ImageActorStressed
      labeledContours
      labeledMesh
      multiLineText
      pickCells
      PickerWithLocator
      propAssembly
      rendererSource
      rotations
      ScalarBar
      SurfacePickerWithTexture
      TestFlyTo
      TestLeaderActor2D
      TestOnePoint
      TestOpacity2
      TestParallelCoordinates
      TestQuadricLODActor
      TestStyleTerrain
      TestTriangleStripCellColor
      TestWindowToImageTransparency
      TexturedSphere
    )
    IF(VTK_USE_TK)
      SET(tests ${tests}
        TestStyleJoystickActor
        TestStyleJoystickCamera
        TestStyleRubberBandZoom
        TestStyleTrackballActor
        TestStyleTrackballCamera
        )
    ENDIF(VTK_USE_TK)

    IF (VTK_USE_CG_SHADERS)
      # Tests testing Cg Shaders.
      SET(tests ${tests}
        TestCgShader
        TestGenericVertexAttributesCg
        )
    ENDIF (VTK_USE_CG_SHADERS)

    IF (VTK_USE_GLSL_SHADERS)
      # Tests testing GLSL Shaders.
      SET(tests ${tests}
        TestGLSLShader
        TestGenericVertexAttributesGLSL
        )
    ENDIF (VTK_USE_GLSL_SHADERS)

    FOREACH ( tfile ${tests} )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Rendering/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Rendering/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )

    FOREACH ( tfile
      TestFreetypeTextMapper
      TestFreetypeTextMapperBigger
      TestFreetypeTextOverlay
    )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Rendering/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Rendering/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )

    IF (VTK_USE_MANGLED_MESA)
      ADD_TEST(MesaText-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Rendering/Testing/Tcl/MesaText.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Rendering/MesaText.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF (VTK_USE_MANGLED_MESA)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-Rendering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Rendering)

ADD_TEST(TestSetObjectMacro-Rendering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Rendering/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
