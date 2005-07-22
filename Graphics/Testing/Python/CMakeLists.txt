IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_PYTHON_EXE)
    SET (CONVERTED_TESTS)
    SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkTclToPyConvertor.py")
    IF (VTK_DATA_ROOT)
      # Older tests
      FOREACH ( tfile
                cameraWarpedCone
                glyphComb
                streamComb
                tubeComb
                streamSurface
                streamSurface2
                )
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_SOURCE_DIR}/Graphics/Testing/Python/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -V Baseline/Graphics/${tfile}.png
          -A ${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py
          -A ${LIBRARY_OUTPUT_PATH})
      ENDFOREACH( tfile )

      # Add test converted from Tcl
      FOREACH( tfile
          AutomaticPlaneGeneration
          Canny
          CellDataToPointData
          Delaunay2D
          Delaunay2DAlpha
          Delaunay2DTransform
          Delaunay3D
          Disk
          EnSight6Elements
          EnSight6OfficeBin
          EnSightBlow1ASCII
          EnSightBlow1Bin
          EnSightBlow2ASCII
          EnSightBlow2Bin
          EnSightBlow3Bin
          EnSightBlow4Bin
          EnSightBlow5ASCII
          EnSightCompound
          EnSightGoldElements
          EnSightIronProtASCII
          EnSightIronProtBin
          EnSightMandelbrot
          EnSightOfficeASCII
          EnSightOfficeBin
          EnSightRectGridASCII
          EnSightRectGridBin
          EnSightSelectArrays
          ExtractEdgesQuadraticCells
          ExtractTensors
          Hyper
          HyperScalarBar
          KlineBottle
          LineIntersectQuadraticCells 
          MatrixToTransform
          MultidimensionalSolution
          NoLightGeneration
          OBBCylinder
          OSCone
          PolyDataMapperAllPolygons 
          PolyDataMapperAllWireframe
          QuadricDecimation
          StreamPolyData
          TenEllip
          TestBandedContourFilter
          TestBoxFunction
          TestButtonSource
          TestCameraInterpolator
          TestCellDerivs
          TestCurvatures
          TestDiscreteMarchingCubes
          TestGraphLayoutFilter
          TestGridSynchronizedTemplates3D
          TestImageMarchingCubes
          TestMarchingSquares
          TestMultiBlockStreamer
          TestParametricFunctions
          #TestPlatonicSolids -- uses eval to expand list
          TestQuadricClustering
          TestRectilinearGridToTetrahedra
          TestRectilinearSynchronizedTemplates
          TestRegularPolygonSource
          TestRibbonAndTube
          TestRotate
          TestRuledSurface
          TestRuledSurface2
          TestSpherePuzzle
          TestSpherePuzzleArrows
          TestSplineFilter
          TestStructuredGrid
          TestSynchronizedTemplates2D
          TestSynchronizedTemplates3D
          TestTransformInterpolator
          TestTriangleFilter
          TransformConcatenation
          TransformCoverage
          TransformPolyData
          blankGrid
          capCow
          capSphere
          clipArt 
          clipComb 
          clipImage
          clipQuadraticCells 
          clipVolume
          clipVolume2
          clipVolume3
          combStreamers
          combStreamers2
          coneResolution
          constrainedDelaunay
          #contour2DAll -- complex use of string eval.
          #contour3DAll -- complex use of string eval.
          contourCells
          contourQuadraticCells
          contoursToSurface
          cowHair
          cowHair2
          createBFont
          cursor3D
          cutLoop
          cylMap
          cylindrical
          dataSetSurfaceFilter
          #deciPlane  -- complex use of string eval
          dicer
          dispPlot
          edgePoints
          eleState
          extractPolyData
          extractRectGrid
          extractUGrid
          extractVectors
          extrudeCopyCD
          fieldToPolyData
          fieldToRGrid
          fieldToSGrid
          fieldToUGrid
          financialField
          genHead
          geomFilter
          glyph2D
          headBone
          hull
          iceCream
          #imageMCAll --  sunstitution in method call.
          mcubes
          mergeFilter
          motor
          multipleComponentContour
          multipleIso
          nacaBinary
          officeStreamPoints
          polyConn
          probeComb
          progGlyphs
          progGlyphsBySource
          #quadricCut -- switch case
          rectGrid
          recursiveDC
          reverseNormals
          scalarColors
          scalarConn
          schwarz
          shepards
          #skinOrder -- sources a script.
          smoothCyl
          smoothCyl2
          smoothMeshOnMesh
          socbal
          #spatialRepAll -- complex string substitutions
          spherical
          splitVectors
          stereoDresdenMace
          streamTracer
          stripF
          subDivideTetra
          subPixelPositionMin
          subdividePointData
          #sync3dAll -- substitution in method call.
          teapotHulls
          testDataSetTriangleFilter
          testDataSetTriangleFilter2
          testHexaPenta
          testReflect
          textureThreshold
          triangularTCoords
          triangularTexture
          warplens
          )
        #Convert the tcl file.
        SET (input "${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/Graphics/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        ADD_CUSTOM_COMMAND(
          OUTPUT "${output}"
          COMMAND ${PYTHON_EXECUTABLE}
          ARGS ${CONVERTER_SCRIPT}
            -i "${input}" 
            -o "${output}" 
            -A "${VTK_BINARY_DIR}/Wrapping/Python"
            -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
            -A "${VTK_LIBRARY_DIR}"
          DEPENDS "${input}" "${CONVERTER_SCRIPT}"
          COMMENT "Converting Tcl test"
          )

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Graphics/${tfile}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDFOREACH( tfile ) 

      # Newer tests using vtk.test.Testing.
      FOREACH ( tfile
                TestTensorGlyph
                TestTextureGlyph
                )
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_SOURCE_DIR}/Graphics/Testing/Python/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -B ${VTK_DATA_ROOT}/Baseline/Graphics)
      ENDFOREACH( tfile )
    ELSE (VTK_DATA_ROOT)
       # Add test converted from Tcl
      FOREACH(tfile
          capSphere
          CellDataToPointData
          clipVolume
          clipVolume2
          clipVolume3
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
          TestBoxFunction
          TestGraphLayoutFilter
          TestQuadricDecimation
          triangularTCoords
          triangularTexture
          warplens
          TestGraphLayoutFilter
          TransformConcatenation
          TransformCoverage
          TransformPolyData
          )
        #Convert the tcl file.
        SET (input "${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/Graphics/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        ADD_CUSTOM_COMMAND(
          OUTPUT "${output}"
          COMMAND ${PYTHON_EXECUTABLE}
          ARGS ${CONVERTER_SCRIPT}
            -i "${input}" 
            -o "${output}" 
            -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
            -A "${VTK_BINARY_DIR}/Wrapping/Python"
            -A "${VTK_LIBRARY_DIR}"
          DEPENDS "${input}" "${CONVERTER_SCRIPT}"
          COMMENT "Converting Tcl test"
          )

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Graphics/${tfile}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDFOREACH( tfile )      
    ENDIF (VTK_DATA_ROOT)
    ADD_CUSTOM_TARGET(GraphicsPythonTests ALL DEPENDS ${CONVERTED_TESTS}) 
    ADD_DEPENDENCIES(GraphicsPythonTests vtktcltest2py_pyc)

  ENDIF (VTK_PYTHON_EXE)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

