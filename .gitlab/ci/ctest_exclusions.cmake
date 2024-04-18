set(test_exclusions
  # Random Memory Leak #18599
  "^VTK::FiltersCorePython-probe$")

if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # Flaky; timesout sometimes on macOS and Linux
    "^VTK::RenderingVolumeOpenGL2Cxx-TestGPURayCastDepthPeelingBoxWidget$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora" OR
    "$ENV{CMAKE_CONFIGURATION}" MATCHES "el8")
  list(APPEND test_exclusions
    # See this issue to track the status of these tests.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098

    # Line rendering differences
    "^VTK::FiltersCorePython-contourCells$"
    "^VTK::FiltersCorePython-contourQuadraticCells$"
    "^VTK::FiltersFlowPathsCxx-TestBSPTree$"
    "^VTK::FiltersGeneralCxx-TestDensifyPolyData$" # valid image looks weird too
    "^VTK::FiltersGeneralPython-clipQuadraticCells$"
    "^VTK::FiltersGeneralPython-edgePoints$"
    "^VTK::FiltersGeneralPython-TestFEDiscreteClipper2D$"
    "^VTK::FiltersGeometryCxx-TestLinearToQuadraticCellsFilter$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DDualContourMaterial$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DGeometryLargeMaterialBits$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DPlaneCutterDual$"
    "^VTK::FiltersModelingPython-TestCookieCutter3$"
    "^VTK::FiltersModelingPython-TestImprintFilter3$"
    "^VTK::FiltersModelingPython-TestImprintFilter6$"
    "^VTK::FiltersSourcesPython-TestStaticCellLocatorLineIntersection$"
    "^VTK::InteractionWidgetsCxx-TestPickingManagerWidgets$"
    "^VTK::InteractionWidgetsPython-TestTensorWidget2$"
    "^VTK::RenderingOpenGL2Cxx-TestCoincident$"
    "^VTK::RenderingOpenGL2Python-TestTopologyResolution$"
    "^VTK::RenderingVolumeCxx-TestGPURayCastMapperRectilinearGrid$"

    # Timeout; needs investigation
    "^VTK::RenderingOpenGL2Cxx-TestFloor$"

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

    # Gets the wrong selection (sometimes).
    "^VTK::RenderingOpenGL2Cxx-TestGlyph3DMapperPickability$"

    # Test image looks "dim"; image rendering seems to be common
    # (some also have vertical line rendering differences)
    "^VTK::FiltersModelingPython-TestCookieCutter$"
    "^VTK::RenderingCoreCxx-TestTextureRGBADepthPeeling$" # seems to just not work here

    # Font rendering differences (new baseline?)
    "^VTK::RenderingFreeTypeCxx-TestFreeTypeTextMapperWithColumns$"

    # Flaky timeouts https://gitlab.kitware.com/vtk/vtk/-/issues/18861
    "^VTK::InteractionWidgetsCxx-TestPickingManagerSeedWidget$"

    # Flaky failures https://gitlab.kitware.com/vtk/vtk/-/issues/19040
    "^VTK::ViewsInfovisCxx-TestGraphLayoutView$"
    "^VTK::ViewsInfovisCxx-TestRenderView$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "el8")
  list(APPEND test_exclusions
    # Matplotlib fails to render anything. See #19302.
    "^VTK::RenderingMatplotlibCxx-TestRenderString$"
    "^VTK::RenderingMatplotlibCxx-TestScalarBarCombinatorics$"
    "^VTK::RenderingMatplotlibCxx-TestStringToPath$"
    "^VTK::RenderingMatplotlibPython-TestMathTextActor$"
    "^VTK::RenderingMatplotlibPython-TestMathTextActor3D$"
    "^VTK::RenderingMatplotlibPython-TestRenderString$"
    "^VTK::RenderingMatplotlibPython-TestStringToPath$"

    # Consistent timeout. Needs investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19303
    "^VTK::RenderingOpenGL2Cxx-TestFloor$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  list(APPEND test_exclusions
    # See this issue to track the status of these tests.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098

    # Point rendering differences
    "^VTK::IOLASCxx-TestLASReader_test_1$"
    "^VTK::IOLASCxx-TestLASReader_test_2$"
    "^VTK::IOPDALCxx-TestPDALReader_test_1$"
    "^VTK::IOPDALCxx-TestPDALReader_test_2$"

    # Numerical problems?
    "^VTK::FiltersOpenTURNSCxx-TestOTKernelSmoothing$"

    # Syntax error in generated shader program.
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow$"

    # Flaky timeouts
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18984
    "^VTK::ViewsInfovisCxx-TestGraphLayoutView$"

    # Rendering in the wrong order.
    "^VTK::InteractionWidgetsCxx-TestResliceCursorWidget2$"
    "^VTK::InteractionWidgetsCxx-TestResliceCursorWidget3$"

    # MPI detects bad memory handling
    "^VTK::IOPIOPython-MPI-TestPIOReader$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" STREQUAL "fedora39_mpi_ospray_python_qt_tbb")
  list(APPEND test_exclusions
    # MPI initialization failures from inside of IOSS. Needs investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19314
    "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"
    "^VTK::FiltersCoreCxx-TestAppendSelection$"
    "^VTK::FiltersCoreCxx-TestFeatureEdges$"
    "^VTK::FiltersCorePython-TestCompositeDataSetPlaneCutter$"
    "^VTK::FiltersExtractionCxx-TestExpandMarkedElements$"
    "^VTK::FiltersExtractionCxx-TestExtractionExpression$"
    "^VTK::FiltersExtractionCxx-TestExtractSelectionUsingDataAssembly$"
    "^VTK::FiltersGeneralCxx-TestAnimateModes$"
    "^VTK::FiltersGeneralCxx-TestGradientAndVorticity$"
    "^VTK::FiltersHybridCxx-TestTemporalCacheUndefinedTimeStep$"
    "^VTK::FiltersHybridCxx-TestTemporalInterpolator$"
    "^VTK::FiltersHybridCxx-TestTemporalInterpolatorFactorMode$"
    "^VTK::FiltersParallelCxx-MPI-AggregateDataSet$"
    "^VTK::FiltersParallelCxx-MPI-ParallelResampling$"
    "^VTK::FiltersParallelCxx-MPI-PTextureMapToSphere$"
    "^VTK::FiltersParallelCxx-MPI-TestGenerateProcessIds$"
    "^VTK::FiltersParallelCxx-MPI-TestHyperTreeGridGhostCellsGenerator$"
    "^VTK::FiltersParallelCxx-MPI-TestPartitionBalancer$"
    "^VTK::FiltersParallelCxx-MPI-TransmitImageData$"
    "^VTK::FiltersParallelCxx-MPI-TransmitImageDataRenderPass$"
    "^VTK::FiltersParallelCxx-MPI-TransmitRectilinearGrid$"
    "^VTK::FiltersParallelCxx-MPI-TransmitStructuredGrid$"
    "^VTK::FiltersParallelCxx-MPI-UnitTestPMaskPoints$"
    "^VTK::FiltersParallelCxx-TestPOutlineFilter$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-DIYAggregateDataSet$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestAdaptiveResampleToImage$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestDIYGenerateCuts$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestGenerateGlobalIds$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestOverlappingCellsDetector$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleHyperTreeGridWithDataSet$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleToImage$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleToImageCompositeDataSet$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleWithDataSet$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleWithDataSet2$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestProbeLineFilter$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPUnstructuredGridGhostCellsGenerator$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestRedistributeDataSetFilter$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestRedistributeDataSetFilterImplicitArray$"
    "^VTK::FiltersParallelDIY2Cxx-TestAdaptiveResampleToImage$"
    "^VTK::FiltersParallelDIY2Cxx-TestExtractSubsetWithSeed$"
    "^VTK::FiltersParallelDIY2Cxx-TestGenerateGlobalIds$"
    "^VTK::FiltersParallelDIY2Cxx-TestGenerateGlobalIdsSphere$"
    "^VTK::FiltersParallelDIY2Cxx-TestOverlappingCellsDetector$"
    "^VTK::FiltersParallelDIY2Cxx-TestRedistributeDataSetFilter$"
    "^VTK::FiltersParallelDIY2Cxx-TestRedistributeDataSetFilterOnIoss$"
    "^VTK::FiltersParallelDIY2Cxx-TestRedistributeDataSetFilterWithPolyData$"
    "^VTK::FiltersParallelDIY2Cxx-TestStitchImageDataWithGhosts$"
    "^VTK::FiltersParallelDIY2Cxx-TestUniformGridGhostDataGenerator$"
    "^VTK::FiltersParallelFlowPathsCxx-MPI-TestPLagrangianParticleTracker$"
    "^VTK::FiltersParallelFlowPathsCxx-MPI-TestPParticleTracers$"
    "^VTK::FiltersParallelFlowPathsCxx-MPI-TestPStreamAMR$"
    "^VTK::FiltersParallelFlowPathsCxx-MPI-TestPStreamGeometry$"
    "^VTK::FiltersParallelGeometryCxx-MPI-ParallelConnectivity1$"
    "^VTK::FiltersParallelGeometryCxx-MPI-ParallelConnectivity4$"
    "^VTK::FiltersParallelGeometryCxx-MPI-TestPolyhedralMeshDistributedDataFilter$"
    "^VTK::FiltersParallelGeometryCxx-MPI-TestPStructuredGridConnectivity$"
    "^VTK::FiltersParallelMPICxx-MPI-TestDistributedPointCloudFilter1$"
    "^VTK::FiltersParallelMPICxx-MPI-TestDistributedPointCloudFilter5$"
    "^VTK::FiltersParallelStatisticsCxx-MPI-TestPCorrelativeStatistics$"
    "^VTK::FiltersParallelVerdictCxx-MPI-PCellSizeFilter$"
    "^VTK::FiltersSourcesCxx-MPI-TestRandomHyperTreeGridSourceMPI3$"
    "^VTK::FiltersSourcesCxx-MPI-TestSpatioTemporalHarmonicsSourceDistributed$"
    "^VTK::IOADIOS2Cxx-MPI-TestADIOS2BPReaderMPIMultiTimeSteps2D$"
    "^VTK::IOADIOS2Cxx-TestIOADIOS2VTX_VTU1DRendering$"
    "^VTK::IOADIOS2Cxx-TestIOADIOS2VTX_VTU2DRendering$"
    "^VTK::IOIOSSCxx-MPI-TestIOSSExodusParallelWriter$"
    "^VTK::IOIossCxx-MPI-TestIossExodusParitionedFiles$"
    "^VTK::IOIossCxx-MPI-TestIossExodusRestarts$"
    "^VTK::IOIossCxx-TestIossExodusRestarts$"
    "^VTK::IOIossCxx-TestIossNoElementBlocks$"
    "^VTK::IOIOSSCxx-TestIOSSReadAllFilesToDetermineStructure$"
    "^VTK::IOIossCxx-TestIossTri6$"
    "^VTK::IOIossCxx-TestIossUnsupported$"
    "^VTK::IOIOSSCxx-TestIOSSWedge21$"
    "^VTK::ParallelDIYCxx-MPI-TestDIYDataExchanger$"
    "^VTK::ParallelDIYCxx-MPI-TestDIYUtilities$"
    "^VTK::ParallelMPICxx-MPI-TestPProbe$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  list(APPEND test_exclusions
    # Failed to open the display
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # Image size mismatches
    "^VTK::ChartsCoreCxx-TestMultipleScalarsToColors$"
    "^VTK::FiltersCorePython-TestOrientedFlyingEdgesPlaneCutter2$"
    "^VTK::RenderingOpenGL2Cxx-TestToneMappingPass$"

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
    "^VTK::FiltersStatisticsCxx-TestMultiCorrelativeStatistics$"

    # Fail to present D3D resources (see #18657)
    "^VTK::RenderingOpenGL2Cxx-TestWin32OpenGLDXRenderWindow$"

    # https://gitlab.kitware.com/vtk/vtk/-/issues/19183 
    "^VTK::RenderingCellGridPython-TestCellGridRendering$"
  )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
  list(APPEND test_exclusions
    # Flaky tests. They sometimes pass.
    "^VTK::InteractionWidgetsPython-TestInteractorEventRecorder$"
    "^VTK::RenderingOpenGL2Cxx-TestGlyph3DMapperPickability$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  list(APPEND test_exclusions
    # Crowded geometry?
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18230
    "^VTK::ViewsInfovisCxx-TestTreeMapView$"

    # Line rendering differences.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18229
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridBinaryClipPlanes$"
    "^VTK::RenderingAnnotationCxx-TestCubeAxes3$"
    "^VTK::RenderingAnnotationCxx-TestCubeAxesWithYLines$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  list(APPEND test_exclusions
    # Screenshot issue for test comparison with background buffer (intermittent)
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$"
    # MacOS OpenGL issue (intermittent)
    "^VTK::RenderingCellGridPython-TestCellGridRendering$"
    "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"
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
    "^VTK::RenderingCoreCxx-TestInteractorTimers$")
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
    # See #18555
    "^VTK::FiltersFlowPathsCxx-TestEvenlySpacedStreamlines2D$"
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
      "^VTK::InteractionWidgetsPython-TestTensorWidget2$")
  endif ()
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos.*_x86_64")
    list(APPEND test_exclusions
      # MacOS OpenGL issue (intermittent)
      "^VTK::RenderingCellGridPython-TestCellGridRendering$"
      "^VTK::FiltersCellGridPython-TestUnstructuredGridToCellGrid$"
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
    # Cache segfaults on docker
    "^VTK::RenderingRayTracingCxx-TestOSPRayCache$"
    # https://github.com/ospray/ospray/issues/571
    "^VTK::RenderingRayTracingCxx-TestPathTracerMaterials$")
endif ()

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()
