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
      3DMorph
      blankGrid
      Canny
      capCow
      capSphere
      cellCenters
      CellDataToPointData
      clipArt
      clipHex
      clipPyramid
      clipTet
      clipVolume
      clipWedge
      combColorIso
      combIso
      coneResolution
      contourCells
      contour2DAll
      contour3DAll
      createBFont
      cylMap
      dataSetSurfaceFilter
      deciFranFace
      Delaunay2D
      Delaunay2DAlpha
      Delaunay2DTransform
      dicer
      EnSight6Elements
      EnSightGoldElements
      extractPolyData
      extractUGrid
      ExtractTensors
      extractVectors
      geomFilter
      glyph2D
      Hyper
      KlineBottle
      motor
      MatrixToTransform
      MultidimensionalSolution
      nacaBinary
      OBBTree
      OBBCylinder
      PolyDataMapperAllPolygons
      PolyDataMapperAllWireframe
      PolyDataNormals
      probe
      probeComb
      progGlyphs
      progGlyphsBySource
      reconstructSurface
      quadricCut
      scalarConn
      schwarz
      shepards
      skinOrder
      smoothCyl
      smoothCyl2
      socbal
      spatialRepAll
      spToPd
      subdividePointData
      subDivideTetra
      teapotHulls
      TenEllip
      testDataSetTriangleFilter
      TestGraphLayoutFilter
      testQuadricDecimation
      TransformConcatenation
      TransformPolyData
      triangularTCoords
      ugFacet
      volTestClipRegions
      warpComb
      warplens
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

