IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)

    IF (VTK_MANGLE_MESA)
      ADD_TEST(MesaCone-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/MesaCone.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Graphics/MesaCone.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF (VTK_MANGLE_MESA)

    FOREACH ( tfile
      Canny
      clipArt
      createBFont
      cylMap
      dataSetSurfaceFilter
      Delaunay2D
      dicer
      EnSight6Elements
      EnSightGoldElements
      extractUGrid
      extractVectors
      glyph2D
      KlineBottle
      nacaBinary
      OBBTree
      PolyDataMapperAllPolygons
      PolyDataMapperAllWireframe
      probeComb
      reconstructSurface
      quadricCut
      scalarConn
      schwarz
      shepards
      socbal
      spToPd
      subDivideTetra
      teapotHulls
      TenEllip
      testDataSetTriangleFilter
      testQuadricDecimation
      TransformConcatenation
      TransformPolyData
      triangularTCoords
      ugFacet
      volTestClipRegions
      warpComb
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Graphics/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )

  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    IF (VTK_MANGLE_MESA)
      ADD_TEST(MesaCone ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/MesaCone.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF (VTK_MANGLE_MESA)

    FOREACH ( tfile
      Delaunay2D
      schwarz
      )
      ADD_TEST(${tfile} ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

