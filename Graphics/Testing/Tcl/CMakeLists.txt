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
      blankGrid
      Canny
      capCow
      capSphere
      cellCenters
      CellDataToPointData
      clipArt
      clipVolume
      combColorIso
      combIso
      combStreamers
      combStreamers2
      coneResolution
      constrainedDelaunay
      contourCells
      contoursToSurface
      contour2DAll
      contour3DAll
      cowHair
      createBFont
      cursor3D
      cutLoop
      cylMap
      dataSetSurfaceFilter
      deciFranFace
      Delaunay2D
      Delaunay2DAlpha
      Delaunay2DTransform
      dicer
      Disk
      edgePoints
      eleState
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
      officeStreamPoints
      polyConn
      PolyDataMapperAllPolygons
      PolyDataMapperAllWireframe
      PolyDataNormals
      probe
      probeComb
      progGlyphs
      progGlyphsBySource
      reconstructSurface
      quadricCut
      rectGrid
      recursiveDC
      scalarConn
      schwarz
      shepards
      skinOrder
      smoothCyl
      smoothCyl2
      socbal
      spatialRepAll
      spToPd
      stripF
      subdividePointData
      subDivideTetra
      teapotHulls
      TenEllip
      TestCellDerivs
      testDataSetTriangleFilter
      testDataSetTriangleFilter2
      TestGraphLayoutFilter
      testQuadricDecimation
      TestQuadricClustering
      TestStructuredGrid
      TransformConcatenation
      TransformCoverage
      TransformPolyData
      triangularTCoords
      triangularTexture
      ugFacet
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
      capSphere
      cellCenters
      CellDataToPointData
      clipVolume
      coneResolution
      constrainedDelaunay
      contourCells
      contoursToSurface
      cutLoop
      Delaunay2D
      Delaunay2DAlpha
      Delaunay2DTransform
      Disk
      extractPolyData
      glyph2D
      Hyper
      KlineBottle
      MatrixToTransform
      OBBCylinder
      progGlyphs
      progGlyphsBySource
      quadricCut
      scalarConn
      schwarz
      shepards
      smoothCyl
      smoothCyl2
      subdividePointData
      subDivideTetra
      TestGraphLayoutFilter
      testQuadricDecimation
      triangularTCoords
      triangularTexture
      warplens
      TestGraphLayoutFilter
      TransformConcatenation
      TransformCoverage
      TransformPolyData
      )
      ADD_TEST(${tfile} ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

