set(test_exclusions
  # Random Memory Leak #18599
  "^VTK::FiltersCorePython-probe$"

  # This test just seems to be incorrect.
  "^VTK::FiltersSelectionCxx-TestLinearSelector3D$")

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
  # Screenshot issue for test comparison with background buffer (intermittent)
  list(APPEND test_exclusions
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$")
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
