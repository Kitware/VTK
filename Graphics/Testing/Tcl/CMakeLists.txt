IF (HAVE_PNG)
 IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(nacaBinary-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/nacaBinary.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/nacaBinary.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(EnSight6Elements-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/EnSight6Elements.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/EnSight6Elements.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(EnSightGoldElements-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/EnSightGoldElements.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/EnSightGoldElements.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(clipArt-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/clipArt.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/clipArt.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(schwarz-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/schwarz.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/schwarz.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(Canny-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/Canny.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/Canny.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(volTestClipRegions-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/volTestClipRegions.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/volTestClipRegions.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(PolyDataMapperAllPolygons-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/PolyDataMapperAllPolygons.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/PolyDataMapperAllPolygons.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(PolyDataMapperAllWireframe-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/PolyDataMapperAllWireframe.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/PolyDataMapperAllWireframe.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(socbal-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/socbal.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/socbal.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(spToPd-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/spToPd.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/spToPd.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(createBFont-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/createBFont.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/createBFont.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(Delaunay2D-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/Delaunay2D.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/Delaunay2D.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(ugFacet-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/ugFacet.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/ugFacet.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(cylMap-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/cylMap.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/cylMap.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(dicer-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/dicer.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/dicer.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(OBBTree-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/OBBTree.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/OBBTree.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(probeComb-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/probeComb.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/probeComb.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(teapotHulls-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/teapotHulls.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/teapotHulls.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(extractUGrid-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/extractUGrid.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/extractUGrid.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(schwarz ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/schwarz.tcl
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ADD_TEST(Delaunay2D ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/Delaunay2D.tcl
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ENDIF (VTK_DATA_ROOT)

 ENDIF (VTK_USE_RENDERING)
ENDIF (HAVE_PNG)

