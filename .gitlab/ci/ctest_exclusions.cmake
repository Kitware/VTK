set(test_exclusions
  # Flaky failures https://gitlab.kitware.com/vtk/vtk/-/issues/19896
  "^VTK::RenderingOpenGL2Cxx-TestFluidMapper(SerDes)?$"
  # https://gitlab.kitware.com/vtk/vtk/-/issues/19427
  "^VTK::RenderingOpenGL2Cxx-TestGlyph3DMapperPickability(SerDes)?$")

if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # Flaky; timesout sometimes on macOS and Linux
    "^VTK::RenderingVolumeOpenGL2Cxx-TestGPURayCastDepthPeelingBoxWidget(SerDes)?$"
  )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora[0-9]*_x86_64" OR
    "$ENV{CMAKE_CONFIGURATION}" MATCHES "el8")
  list(APPEND test_exclusions
    # See this issue to track the status of these tests.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098

    # Line rendering differences
    "^VTK::FiltersCorePython-contourCells$"
    "^VTK::FiltersCorePython-contourQuadraticCells$"
    "^VTK::FiltersFlowPathsCxx-TestBSPTree(SerDes)?$"
    "^VTK::FiltersGeneralCxx-TestDensifyPolyData(SerDes)?$" # valid image looks weird too
    "^VTK::FiltersGeneralPython-clipQuadraticCells$"
    "^VTK::FiltersGeneralPython-edgePoints$"
    "^VTK::FiltersGeneralPython-TestFEDiscreteClipper2D$"
    "^VTK::FiltersGeometryCxx-TestLinearToQuadraticCellsFilter(SerDes)?$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DDualContourMaterial(SerDes)?$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DGeometryLargeMaterialBits(SerDes)?$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DPlaneCutterDual(SerDes)?$"
    "^VTK::FiltersModelingPython-TestCookieCutter3$"
    "^VTK::FiltersModelingPython-TestImprintFilter3$"
    "^VTK::FiltersModelingPython-TestImprintFilter6$"
    "^VTK::FiltersSourcesPython-TestStaticCellLocatorLineIntersection$"
    "^VTK::InteractionWidgetsCxx-TestPickingManagerWidgets(SerDes)?$"
    "^VTK::InteractionWidgetsPython-TestTensorWidget2$"
    "^VTK::RenderingOpenGL2Cxx-TestCoincident(SerDes)?$"
    "^VTK::RenderingOpenGL2Python-TestTopologyResolution$"
    "^VTK::RenderingVolumeCxx-TestGPURayCastMapperRectilinearGrid(SerDes)?$"

    # Timeout; needs investigation
    "^VTK::RenderingOpenGL2Cxx-TestFloor(SerDes)?$"

    # Point rendering differences
    "^VTK::FiltersPointsPython-TestConnectedPointsFilter$" # other differences too
    "^VTK::FiltersPointsPython-TestPCANormalEstimation$"
    "^VTK::FiltersPointsPython-TestPCANormalEstimation2$"
    "^VTK::FiltersPointsPython-TestRadiusOutlierRemoval$"
    "^VTK::FiltersPointsPython-TestVoxelGridFilter$"
    "^VTK::IOGeometryPython-ParticleReader$"
    "^VTK::InteractionWidgetsPython-TestPointCloudWidget$"
    "^VTK::InteractionWidgetsPython-TestPointCloudWidget2$"

    # Floating point imprecision?
    "^VTK::FiltersGeneralPython-TestSampleImplicitFunctionFilter$"

    # Test image looks "dim"; image rendering seems to be common
    # (some also have vertical line rendering differences)
    "^VTK::FiltersModelingPython-TestCookieCutter$"
    "^VTK::RenderingCoreCxx-OpenGL-TestTextureRGBADepthPeeling(SerDes)?$" # seems to just not work here

    # Flaky timeouts https://gitlab.kitware.com/vtk/vtk/-/issues/18861
    "^VTK::InteractionWidgetsCxx-TestPickingManagerSeedWidget(SerDes)?$"

    # Flaky failures https://gitlab.kitware.com/vtk/vtk/-/issues/19040
    "^VTK::ViewsInfovisCxx-TestGraphLayoutView(SerDes)?$"
    "^VTK::ViewsInfovisCxx-TestRenderView(SerDes)?$"

    # ANARI volume rendering issue: https://gitlab.kitware.com/vtk/vtk/-/work_items/20024
    "^VTK::RenderingAnariCxx-TestAnariCameraInside"
    "^VTK::RenderingAnariCxx-TestAnariCameraInsideSmallSpacing"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "el8")
  list(APPEND test_exclusions
    # Matplotlib related issues. See #19302.
    # Matplotlib fails to render anything.
    "^VTK::RenderingMatplotlibCxx-TestContextMathTextImage$"
    "^VTK::RenderingMatplotlibCxx-TestIndexedLookupScalarBar$"
    "^VTK::RenderingMatplotlibCxx-TestMathTextActor$"
    "^VTK::RenderingMatplotlibCxx-TestMathTextActor3D$"
    "^VTK::RenderingMatplotlibCxx-TestRenderString$"
    "^VTK::RenderingMatplotlibCxx-TestScalarBarCombinatorics$"
    "^VTK::RenderingMatplotlibCxx-TestStringToPath$"
    "^VTK::RenderingMatplotlibPython-TestMathTextActor$"
    "^VTK::RenderingMatplotlibPython-TestMathTextActor3D$"
    "^VTK::RenderingMatplotlibPython-TestRenderString$"
    "^VTK::RenderingMatplotlibPython-TestStringToPath$"
    # Freetype, which depends on Matplotlib, fails to render anything
    "^VTK::RenderingFreeTypeCxx-TestFontDPIScaling$"
    "^VTK::RenderingFreeTypeCxx-TestFreeTypeTextMapper$"
    "^VTK::RenderingFreeTypeCxx-TestFreeTypeTextMapperWithColumns$"
    "^VTK::RenderingFreeTypeCxx-TestMathTextFonts$"
    "^VTK::RenderingFreeTypeCxx-TestMathTextFreeTypeTextRenderer$"

    # Consistent timeout. Needs investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19303
    "^VTK::RenderingOpenGL2Cxx-TestFloor$"

    # Intermittent flakiness; may be related to CI runner OpenGL config.
    # Appears as a colormap or color-range failure:
    "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora[0-9]*_x86_64")
  list(APPEND test_exclusions
    # See this issue to track the status of these tests.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098

    # Point rendering differences
    "^VTK::IOLASCxx-TestLASReader_test_1(SerDes)?$"
    "^VTK::IOLASCxx-TestLASReader_test_2(SerDes)?$"
    "^VTK::IOPDALCxx-TestPDALReader_test_1(SerDes)?$"
    "^VTK::IOPDALCxx-TestPDALReader_test_2(SerDes)?$"

    # Syntax error in generated shader program.
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow(SerDes)?$"

    # Flaky timeouts
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18984
    "^VTK::ViewsInfovisCxx-TestGraphLayoutView(SerDes)?$"

    # Rendering in the wrong order.
    "^VTK::InteractionWidgetsCxx-TestResliceCursorWidget2(SerDes)?$"
    "^VTK::InteractionWidgetsCxx-TestResliceCursorWidget3(SerDes)?$"

    # MPI detects bad memory handling
    "^VTK::IOPIOPython-MPI-TestPIOReader$"

    # Widget with strange artifacts in resulting image
    # https://gitlab.kitware.com/vtk/vtk/-/work_items/20113
    "^VTK::InteractionWidgetsCxx-TestFinitePlaneWidget$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora44_x86_64" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "mpi")
  list(APPEND test_exclusions
    # Failures that appear related to OpenGL driver.
    #    MESA: error: ZINK: vkCreateInstance failed (VK_ERROR_INCOMPATIBLE_DRIVER)
    #    glx: failed to create drisw screen
    #    failed to load driver: zink
    "^VTK::RenderingCellGridPython-TestCellGridRendering$"
    )
endif ()


if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora[0-9]*_x86_64" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "cuda")
  list(APPEND test_exclusions
    # Failure since viskores: https://gitlab.kitware.com/vtk/vtk/-/issues/19739
    "^VTK::AcceleratorsVTKmFiltersCxx-TestVTKMAbort$"
    "^VTK::AcceleratorsVTKmFiltersCxx-TestVTKMGradient$"
    "^VTK::AcceleratorsVTKmFiltersCxx-TestVTKMGradientAndVorticity$"
    "^VTK::AcceleratorsVTKmFiltersCxx-TestVTKMProbe$"
    "^VTK::AcceleratorsVTKmFiltersCxx-TestVTKMSlice$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  list(APPEND test_exclusions
    # Failed to open the display.
    # After (https://gitlab.kitware.com/vtk/vtk/-/issues/19453) is resolved,
    # these tests could be smarter by skipping when there is no display.
    "^VTK::FiltersSourcesPython-squadViewer$"
    "^VTK::GUISupportQtCxx"
    "^VTK::GUISupportQtQuickCxx"
    "^VTK::GUISupportQtSQLCxx-TestQtSQLDatabase$"
    "^VTK::RenderingCoreCxx-OpenGL-TestInteractorTimers$"
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow$"
    "^VTK::RenderingLICOpenGL2Cxx-TestSurfaceLICTranslucencyWithoutVectors$"
    "^VTK::RenderingQtCxx-TestQtInitialization$"
    "^VTK::RenderingTkPython"
    "^VTK::ViewsQtCxx-TestVtkQtTableView$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # Image size mismatches
    "^VTK::ChartsCoreCxx-TestMultipleScalarsToColors(SerDes)?$"
    "^VTK::FiltersCorePython-TestOrientedFlyingEdgesPlaneCutter2$"
    "^VTK::RenderingOpenGL2Cxx-TestToneMappingPass(SerDes)?$"

    # PATH manipulations needed
    "^VTKExample-ImageProcessing/Cxx$"
    "^VTKExample-IO/Cxx$"
    "^VTKExample-Medical/Cxx$"
    "^VTKExample-Modelling/Cxx$"
    "^VTKExample-Modules/UsingVTK$"
    "^VTKExample-Modules/Wrapping$"

    # Blank test image
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWindowWithDisabledInteractor$"

    # Timeouts; need investigation.
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetPicking$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetQWidgetWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetWithChartHistogram2D$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetWithDisabledInteractor$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetWithMSAA$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$"

    # Flaky on windows for some reasons:
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18640
    "^VTK::FiltersStatisticsCxx-TestMultiCorrelativeStatistics(SerDes)?$"

    # Fail to present D3D resources (see #18657)
    "^VTK::RenderingOpenGL2Cxx-TestWin32OpenGLDXRenderWindow(SerDes)?$"

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19183
    "^VTK::RenderingCellGridPython-TestCellGridRendering$"
    "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19400
    "^VTK::RenderingCoreCxx-OpenGL-TestResizingWindowToImageFilter(SerDes)?$"
  )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
  list(APPEND test_exclusions
    # Flaky tests. They sometimes pass.
    "^VTK::InteractionWidgetsPython-TestInteractorEventRecorder$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora[0-9]*_aarch64")
  list(APPEND test_exclusions
    # floating point precision issues (fma optimizations change results)
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19418
    "^VTK::CommonDataModelCxx-TestHyperTreeGridGeometricLocator$"
    "^VTK::ChartsCoreCxx-TestLinePlot3D$"
    "^VTK::FiltersCoreCxx-TestImplicitPolyDataDistanceCube$"
    "^VTK::FiltersCorePython-TestSphereTreeFilter$"
    "^VTK::FiltersFlowPathsCxx-TestEvenlySpacedStreamlines2D$"
    "^VTK::FiltersFlowPathsCxx-TestParticleTracers$"
    "^VTK::RenderingCorePython-pickImageData$"

    # MPI detects bad memory handling
    "^VTK::IOPIOPython-MPI-TestPIOReader$"

    # Numerical issue?
    "^VTK::ImagingColorCxx-TestRGBToLAB$"
    # "incoherent result" from HTG line intersection. numerical?
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestProbeLineFilter$"
    "^VTK::IOHDFCxx-TestHDFWriter$"

    # Baseline failures.
    "^VTK::InteractionWidgetsPython-TestTensorWidget2$"
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow$" # also leaks

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19578
    "^VTK::FiltersGeneralCxx-TestContourTriangulatorHoles$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  list(APPEND test_exclusions
    # floating point precision issues (fma optimizations change results)
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19418
    "^VTK::CommonDataModelCxx-TestHyperTreeGridGeometricLocator(SerDes)?$"
    "^VTK::ChartsCoreCxx-TestLinePlot3D(SerDes)?$"
    "^VTK::FiltersCoreCxx-TestImplicitPolyDataDistanceCube(SerDes)?$"
    "^VTK::FiltersCorePython-TestSphereTreeFilter$"
    "^VTK::FiltersFlowPathsCxx-TestEvenlySpacedStreamlines2D(SerDes)?$"
    "^VTK::FiltersFlowPathsCxx-TestParticleTracers(SerDes)?$"
    "^VTK::FiltersModelingPython-Hyper$"
    "^VTK::RenderingAnnotationPython-xyPlot$"
    "^VTK::RenderingAnnotationPython-xyPlot2$"
    "^VTK::RenderingAnnotationPython-xyPlot4$"
    "^VTK::RenderingCorePython-pickImageData$"

    # Crowded geometry?
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18230
    "^VTK::ViewsInfovisCxx-TestTreeMapView(SerDes)?$"

    # Line rendering differences.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18229
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridBinaryClipPlanes(SerDes)?$"
    "^VTK::RenderingAnnotationCxx-TestCubeAxes3(SerDes)?$"
    "^VTK::RenderingAnnotationCxx-TestCubeAxesWithYLines(SerDes)?$"

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19578
    "^VTK::FiltersGeneralCxx-TestContourTriangulatorHoles(SerDes)?$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  list(APPEND test_exclusions
    # line differences https://gitlab.kitware.com/vtk/vtk/-/issues/18229
    "^VTK::FiltersSourcesCxx-TestHyperTreeGridPreConfiguredSource(SerDes)?$"
    "^VTK::FiltersSourcesCxx-TestRandomHyperTreeGridSource(SerDes)?$"

    # geometry shader issues (observed on M4 hardware)
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19555
    "^VTK::IOIOSSCxx-TestIOSSApplyDisplacementsCGNS(SerDes)?$"
    "^VTK::IOADIOS2Cxx-TestADIOS2BPReaderSingleTimeStep(SerDes)?$"
    "^VTK::CommonDataModelPython-TestClipPolyhedra$"
    "^VTK::ImagingCoreCxx-TestStencilWithPolyDataContour(SerDes)?$"

    # edge rendering issues (OpenGL support abandoned on macos)
    "^VTK::FiltersMeshingPython-TestVoronoi3D-Lissajous$"
    "^VTK::FiltersMeshingPython-TestVoronoi3D2$"
    "^VTK::FiltersMeshingPython-TestVoronoi2D3$"
    "^VTK::FiltersMeshingPython-TestVoronoi2D$"
    "^VTK::FiltersMeshingCxx-TestVoronoiHull2(SerDes)?$"
    "^VTK::FiltersMeshingCxx-TestVoronoi2DSurfaceNets(SerDes)?$")

  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "arm64")
    # Unknown NSInternalInconsistencyException when using macos arm64
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19916
    list(APPEND test_exclusions
      "^vtkJavaTests-Regression$")
  endif()
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel_macos" AND
    "$ENV{CMAKE_CONFIGURATION}" MATCHES "arm64")
  list(APPEND test_exclusions
    # floating point precision issues (fma optimizations change results)
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19418
    "^VTK::ChartsCoreCxx-TestLinePlot3D$"
    "^VTK::FiltersCorePython-TestSphereTreeFilter$"
    "^VTK::FiltersModelingPython-Hyper$"
    "^VTK::RenderingAnnotationPython-xyPlot$"
    "^VTK::RenderingAnnotationPython-xyPlot2$"
    "^VTK::RenderingAnnotationPython-xyPlot4$"
    "^VTK::RenderingCorePython-pickImageData$"

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19578
    "^VTK::FiltersGeneralCxx-TestContourTriangulatorHoles$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  list(APPEND test_exclusions
    # Screenshot issue for test comparison with background buffer (intermittent)
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$"
    # MacOS OpenGL issue (intermittent)
    "^VTK::RenderingCellGridPython-TestCellGridRendering$"
    "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19372
    "^VTK::IOIOSSPython-TestIOSSCellGridReader$"
    "^VTK::FiltersCellGridPython-TestCellGridToUnstructuredGrid$"
    "^VTK::FiltersCellGridPython-TestCellGridTransform$"
    "^VTK::FiltersCellGridPython-TestCellGridCellCenters$"
    "^VTK::RenderingOpenGL2Python-TestArrayRenderer$"
  )
endif ()

if (("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "ext_vtk") OR
    ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa"))
  # These tests fail when using an external VTK because the condition
  # VTK_USE_X, vtk_can_do_onscreen AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN should be false
  # in Interaction/Style/Testing/Python/CMakeLists.txt
  list(APPEND test_exclusions
    "^VTK::InteractionStylePython-TestStyleJoystickActor$"
    "^VTK::InteractionStylePython-TestStyleJoystickCamera$"
    "^VTK::InteractionStylePython-TestStyleRubberBandZoomPerspective$"
    "^VTK::InteractionStylePython-TestStyleTrackballCamera$"
    "^VTK::RenderingCoreCxx-OpenGL-TestInteractorTimers$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel" AND "$ENV{CMAKE_CONFIGURATION}" MATCHES "egl")
  # These tests fail when using an external VTK because the condition
  # VTK_USE_X, vtk_can_do_onscreen AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN should be false
  # in Interaction/Style/Testing/Python/CMakeLists.txt
  list(APPEND test_exclusions
    "^VTK::InteractionStylePython-TestStyleJoystickActor$"
    "^VTK::InteractionStylePython-TestStyleJoystickCamera$"
    "^VTK::InteractionStylePython-TestStyleRubberBandZoomPerspective$"
    "^VTK::InteractionStylePython-TestStyleTrackballCamera$"
    "^VTK::InteractionWidgetsPython-TestInteractorEventRecorder$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "stdthread")
  list(APPEND test_exclusions
    # Test is flaky with STDThread
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19741
    "^VTK::FiltersVerdictCxx-TestCellQuality$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel")
  list(APPEND test_exclusions
    # The wheels have a broken `proj.db`.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18750
    "^VTK::GeovisCorePython-TestGeoProjection$")

  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux" OR
      "$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
    list(APPEND test_exclusions
      # Line rendering differences.
      # https://gitlab.kitware.com/vtk/vtk/-/issues/18098
      "^VTK::FiltersCorePython-contourCells$"
      "^VTK::FiltersGeneralPython-TestFEDiscreteClipper2D$"
      "^VTK::FiltersModelingPython-TestCookieCutter$"
      "^VTK::FiltersModelingPython-TestCookieCutter3$"
      "^VTK::FiltersModelingPython-TestImprintFilter3$"
      "^VTK::FiltersModelingPython-TestImprintFilter6$"
      "^VTK::FiltersSourcesPython-TestStaticCellLocatorLineIntersection$"
      "^VTK::InteractionWidgetsPython-TestTensorWidget2$"
      # Intermittent flakiness; may be related to CI runner OpenGL config.
      # Appears as a colormap or color-range failure:
      "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"
    )
  endif ()
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux.*_aarch64")
    list(APPEND test_exclusions
      # floating point precision issues (fma optimizations change results)
      # https://gitlab.kitware.com/vtk/vtk/-/issues/19418
      "^VTK::FiltersCorePython-TestSphereTreeFilter$"
      "^VTK::RenderingAnnotationPython-xyPlot$"
      "^VTK::RenderingAnnotationPython-xyPlot2$"
      "^VTK::RenderingAnnotationPython-xyPlot4$"
      "^VTK::RenderingCorePython-pickImageData$")
  endif ()
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos.*_x86_64")
    list(APPEND test_exclusions
      # MacOS OpenGL issue (intermittent). See #19372.
      "^VTK::FiltersCellGridPython-TestCellGridCellCenters$"
      "^VTK::FiltersCellGridPython-TestCellGridToUnstructuredGrid$"
      "^VTK::FiltersCellGridPython-TestCellGridTransform$"
      "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"
      "^VTK::IOIOSSPython-TestIOSSCellGridReader$"
      "^VTK::RenderingCellGridPython-TestCellGridRendering$"
      "^VTK::RenderingOpenGL2Python-TestArrayRenderer$"
    )
  endif()
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
    # Windows wheels are intermittently flaky and not reproducible.
    list(APPEND test_exclusions
      "^VTK::RenderingCellGridPython-TestCellGridRendering$"
    )
  endif()
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "qt" AND
    NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "qt5")
  list(APPEND test_exclusions
    # Qt6 test failures that need investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18946
    "^VTK::GUISupportQtCxx-TestQtDebugLeaksView$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "ospray")
  list(APPEND test_exclusions
    # Fails with:
    #     ZINK: vkCreateInstance failed (VK_ERROR_INCOMPATIBLE_DRIVER)
    #     glx: failed to create drisw screen
    #     failed to load driver: zink
    "^VTK::RenderingCellGridPython-TestCellGridRendering$"
    # Cache segfaults on docker
    "^VTK::RenderingRayTracingCxx-TestOSPRayCache$"
    # https://github.com/ospray/ospray/issues/571
    "^VTK::RenderingRayTracingCxx-TestPathTracerMaterials$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "^wasm(32|64)")
  list(APPEND test_exclusions
    # OpenGL tests that fail on WebGL2, tracked in
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19343
    #
    # The comment above each entry records what was actually observed when the
    # test was run against chrome + ANGLE/SwiftShader on a Linux host.
    #
    # The float RGBA readback disagrees with the unsigned char readback of the
    # same buffer, which passes a few lines earlier:
    #   TestReadPixels.cxx:72 ERR| Unsatisfied pixel value condition at line 72:
    #   static_cast<int>(f32RGBA->GetTuple(0)[0] * 255) == 100
    "^VTK::RenderingCoreCxx-OpenGL-TestReadPixels$"
    # Lines drawn coincident with the surface are almost entirely hidden - only
    # the point primitives survive - so the polygon-offset based coincident
    # topology resolution does not take effect. Largest error of the group by a
    # wide margin.
    "^VTK::RenderingCoreCxx-OpenGL-TestCoincident$"
    # Hardware selection reads back the wrong ids for the composite case:
    #   TestHardwareSelector.cxx:280 Expected selected Ids = 1160 1163 1169 ...
    "^VTK::RenderingCoreCxx-OpenGL-TestHardwareSelector$"
    # RenderingCoreCxx tests that fail with WebGPU.
    # see https://gitlab.kitware.com/vtk/vtk/-/issues/19921
    "^VTK::RenderingCoreCxx-WebGPU-TestAreaSelections$"
    "^VTK::RenderingCoreCxx-WebGPU-TestBackfaceTexture$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperBlockOpacities$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperBlockTextures$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperCameraShiftScale$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperCustomShader$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperMixedGeometryCellScalars$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperMixedGeometryEdges$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperPickability$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperPicking$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperSpheres$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperToggleScalarVisibilities$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperVertices$"
    "^VTK::RenderingCoreCxx-WebGPU-TestEdgeFlags$"
    "^VTK::RenderingCoreCxx-WebGPU-TestEdgeOpacity$"
    "^VTK::RenderingCoreCxx-WebGPU-TestEdgeThickness$"
    "^VTK::RenderingCoreCxx-WebGPU-TestFollowerPicking$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperBackfaceColor$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperPointSize$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperTreeIndexing$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGradientBackground$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGradientBackgroundWithTiledViewport$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGradientBackgroundWithTiledViewports$"
    "^VTK::RenderingCoreCxx-WebGPU-TestImageAndAnnotations$"
    "^VTK::RenderingCoreCxx-WebGPU-TestInteractorStyleImageProperty$"
    "^VTK::RenderingCoreCxx-WebGPU-TestLabeledContourMapper$"
    "^VTK::RenderingCoreCxx-WebGPU-TestLabeledContourMapperNoLabels$"
    "^VTK::RenderingCoreCxx-WebGPU-TestLabeledContourMapperWithActorMatrix$"
    "^VTK::RenderingCoreCxx-WebGPU-TestMixedGeometryCellScalars$"
    "^VTK::RenderingCoreCxx-WebGPU-TestOffAxisStereo$"
    "^VTK::RenderingCoreCxx-WebGPU-TestOpacity$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPickTextActor$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPointSelection$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPointSelectionWithCellData$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPolyDataMapperNormals$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderPointsAsSpheres$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderPointsAsSpheresOrthoCamera$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderLinesAsTubes$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderLinesAsTubesOrthoCamera$"
    "^VTK::RenderingCoreCxx-WebGPU-TestResizingWindowToImageFilter$"
    "^VTK::RenderingCoreCxx-WebGPU-TestSplitViewportStereoHorizontal$"
    "^VTK::RenderingCoreCxx-WebGPU-TestStereoBackgroundLeft$"
    "^VTK::RenderingCoreCxx-WebGPU-TestStereoBackgroundRight$"
    "^VTK::RenderingCoreCxx-WebGPU-TestStereoEyeSeparation$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTexturedBackground$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTexturedCylinder$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTextureSize$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTextureWrap$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTransformCoordinateUseDouble$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentImageActorAlphaBlending$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentImageActorDepthPeeling$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTAlphaBlending$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTDepthPeeling$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTTextureAlphaBlending$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTTextureDepthPeeling$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsColorsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsNormalsColorsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsNormalsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestWindowToImageFilter$"
    "^VTK::RenderingOpenGL2Cxx-TestCoincident$"
    "^VTK::RenderingOpenGL2Cxx-TestCompositeDataOverlappingCells$"
    "^VTK::RenderingOpenGL2Cxx-TestCompositeDataPointGaussian$"
    "^VTK::RenderingOpenGL2Cxx-TestCompositeDataPointGaussianSelection$"
    "^VTK::RenderingOpenGL2Cxx-TestFlipRenderFramebuffer$"
    "^VTK::RenderingOpenGL2Cxx-TestFluidMapper$"
    "^VTK::RenderingOpenGL2Cxx-TestFramebufferHDR$" # flaky
    "^VTK::RenderingOpenGL2Cxx-TestGaussianBlurPass$"
    "^VTK::RenderingOpenGL2Cxx-TestGlyph3DMapperEdges$"
    "^VTK::RenderingOpenGL2Cxx-TestMultiTexturing$"
    "^VTK::RenderingOpenGL2Cxx-TestMultiTexturingInterpolateScalars$"
    "^VTK::RenderingOpenGL2Cxx-TestPBRClearCoat$"
    "^VTK::RenderingOpenGL2Cxx-TestPBREdgeTint$"
    "^VTK::RenderingOpenGL2Cxx-TestPBRHdrEnvironment$"
    "^VTK::RenderingOpenGL2Cxx-TestPBRIrradianceHDR$"
    "^VTK::RenderingOpenGL2Cxx-TestPBRMultipleScatteringIBL$"
    "^VTK::RenderingOpenGL2Cxx-TestPointFillPass$"
    "^VTK::RenderingOpenGL2Cxx-TestPointGaussianMapper$"
    "^VTK::RenderingOpenGL2Cxx-TestPointGaussianMapperAnisotropic$"
    "^VTK::RenderingOpenGL2Cxx-TestPointGaussianMapperOpacity$"
    "^VTK::RenderingOpenGL2Cxx-TestPointGaussianSelection$"
    "^VTK::RenderingOpenGL2Cxx-TestProgramPointSize$"
    "^VTK::RenderingOpenGL2Cxx-TestRemoveActorNonCurrentContext$"
    "^VTK::RenderingOpenGL2Cxx-TestDirectSelectionRendering$"
    "^VTK::RenderingOpenGL2Cxx-TestSkyboxRotation$"
    "^VTK::RenderingOpenGL2Cxx-TestSkyboxRotationVectors$"
    "^VTK::RenderingOpenGL2Cxx-TestSimpleMotionBlur$" # flaky
    "^VTK::RenderingOpenGL2Cxx-TestSpherePoints$"
    "^VTK::RenderingOpenGL2Cxx-TestSphereVertex$"
    "^VTK::RenderingOpenGL2Cxx-TestSurfaceInterpolationSwitch$"
    "^VTK::RenderingOpenGL2Cxx-TestTexture16Bits$"
    "^VTK::RenderingOpenGL2Cxx-TestValuePassFloatingPoint$"
    "^VTK::RenderingOpenGL2Cxx-TestValuePassFloatingPoint2$"
    "^VTK::RenderingOpenGL2Cxx-TestVBOPLYMapper$"
    "^VTK::RenderingOpenGL2Cxx-TestWindowBlits$"
    # WebGPU failures observed on Linux hosts, tracked with the other wasm
    # WebGPU RenderingCore failures in
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19921
    # TestCoincident hangs (times out); TestGlyph3DMapperPicking is an image
    # mismatch; TestHardwareSelector returns mismatched selected ids.
    "^VTK::RenderingCoreCxx-WebGPU-TestCoincident$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperPicking$"
    "^VTK::RenderingCoreCxx-WebGPU-TestHardwareSelector$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora44_x86_64_webgpu")
  list(APPEND test_exclusions
    # RenderingCoreCxx tests that fail with WebGPU.
    # see https://gitlab.kitware.com/vtk/vtk/-/issues/19921
    "^VTK::RenderingCoreCxx-WebGPU-TestAreaSelections$"
    "^VTK::RenderingCoreCxx-WebGPU-TestBackfaceTexture$"
    "^VTK::RenderingCoreCxx-WebGPU-TestBlockOpacity$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperBlockOpacities$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperBlockTextures$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperCameraShiftScale$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperCustomShader$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperMixedGeometryCellScalars$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperMixedGeometryEdges$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperPickability$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperPicking$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperSpheres$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperToggleScalarVisibilities$"
    "^VTK::RenderingCoreCxx-WebGPU-TestCompositePolyDataMapperVertices$"
    "^VTK::RenderingCoreCxx-WebGPU-TestEdgeFlags$"
    "^VTK::RenderingCoreCxx-WebGPU-TestEdgeOpacity$"
    "^VTK::RenderingCoreCxx-WebGPU-TestEdgeThickness$"
    "^VTK::RenderingCoreCxx-WebGPU-TestFollowerPicking$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperBackfaceColor$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperPointSize$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGlyph3DMapperTreeIndexing$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGradientBackground$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGradientBackgroundWithTiledViewport$"
    "^VTK::RenderingCoreCxx-WebGPU-TestGradientBackgroundWithTiledViewports$"
    "^VTK::RenderingCoreCxx-WebGPU-TestImageAndAnnotations$"
    "^VTK::RenderingCoreCxx-WebGPU-TestInteractorStyleImageProperty$"
    "^VTK::RenderingCoreCxx-WebGPU-TestLabeledContourMapper$"
    "^VTK::RenderingCoreCxx-WebGPU-TestLabeledContourMapperNoLabels$"
    "^VTK::RenderingCoreCxx-WebGPU-TestLabeledContourMapperWithActorMatrix$"
    "^VTK::RenderingCoreCxx-WebGPU-TestMixedGeometryCellScalars$"
    "^VTK::RenderingCoreCxx-WebGPU-TestOffAxisStereo$"
    "^VTK::RenderingCoreCxx-WebGPU-TestOpacity$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPickTextActor$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPointSelection$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPointSelectionWithCellData$"
    "^VTK::RenderingCoreCxx-WebGPU-TestPolyDataMapperNormals$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderPointsAsSpheres$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderPointsAsSpheresOrthoCamera$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderLinesAsTubes$"
    "^VTK::RenderingCoreCxx-WebGPU-TestRenderLinesAsTubesOrthoCamera$"
    "^VTK::RenderingCoreCxx-WebGPU-TestResizingWindowToImageFilter$"
    "^VTK::RenderingCoreCxx-WebGPU-TestSplitViewportStereoHorizontal$"
    "^VTK::RenderingCoreCxx-WebGPU-TestStereoBackgroundLeft$"
    "^VTK::RenderingCoreCxx-WebGPU-TestStereoBackgroundRight$"
    "^VTK::RenderingCoreCxx-WebGPU-TestStereoEyeSeparation$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTexturedBackground$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTexturedCylinder$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTextureSize$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTextureWrap$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTransformCoordinateUseDouble$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentImageActorAlphaBlending$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentImageActorDepthPeeling$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTAlphaBlending$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTDepthPeeling$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTTextureAlphaBlending$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTranslucentLUTTextureDepthPeeling$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsColorsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsNormalsColorsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsNormalsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestTStripsTCoords$"
    "^VTK::RenderingCoreCxx-WebGPU-TestVertexVisibility$"
    "^VTK::RenderingCoreCxx-WebGPU-TestWindowToImageFilter$"
    # RenderingLabelCxx tests that fail with WebGPU.
    # see https://gitlab.kitware.com/vtk/vtk/-/issues/19921
    "^VTK::RenderingLabelCxx-WebGPU-TestLabelPlacementMapper2D$"
    "^VTK::RenderingLabelCxx-WebGPU-TestLabelPlacementMapperCoincidentPoints$"
    "^VTK::RenderingLabelCxx-WebGPU-TestLabelPlacer$"
    "^VTK::RenderingLabelCxx-WebGPU-TestLabelPlacementMapper$"
    "^VTK::RenderingLabelCxx-WebGPU-TestClipLabels$"
    # Crashes randomly with mesa-vulkan-drivers
    "^VTK::RenderingWebGPUCxx-TestComputeFrustumCulling$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows" AND
    "$ENV{CMAKE_CONFIGURATION}" MATCHES "debug")
  # Timeouts from debug builds (even with 5 minute limits). See #19212
  list(APPEND test_exclusions
    "^TestLoggerDisableSignalHandler$"
    "^VTK::FiltersCoreCxx-TestFeatureEdges$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestGhostCellsGenerator$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleWithDataSet$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPUniformGridGhostDataGenerator$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPUnstructuredGridGhostCellsGenerator$"
    "^VTK::FiltersParallelFlowPathsCxx-MPI-TestPParticleTracers$"
    "^VTK::FiltersParallelGeometryCxx-MPI-ParallelConnectivity4$"
    "^VTK::GUISupportQtCxx-TestQVTKTableModelAdapter$"
    "^VTK::IOCGNSReaderCxx-TestCGNSUnsteadyTemporalSolution$"
    "^VTK::ParallelCoreCxx-TestThreadedCallbackQueue$"
    "^VTK::RenderingVolumeCxx-TestGPURayCastLabelMapValidity$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "helide")
  list(APPEND test_exclusions
    # ANARI test requires the latest changes from anari-sdk PR
    # (https://github.com/KhronosGroup/ANARI-SDK/pull/335)
    "^VTK::RenderingAnariCxx-TestAnariPolyDataTexture$"
    "^VTK::RenderingAnariCxx-TestAnariRenderMesh$")
endif ()

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()
