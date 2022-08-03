set(test_exclusions
  # Flaky; timesout sometimes on macOS and Linux
  "^VTK::RenderingVolumeOpenGL2Cxx-TestGPURayCastDepthPeelingBoxWidget$"

  # This test just seems to be incorrect.
  "^VTK::FiltersSelectionCxx-TestLinearSelector3D$")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "el8")
  list(APPEND test_exclusions
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18603
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestPResampleToImageCompositeDataSet$"
    )
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
    "^VTK::FiltersModelingPython-TestImprintFilter2$"
    "^VTK::FiltersModelingPython-TestImprintFilter3$"
    "^VTK::FiltersModelingPython-TestImprintFilter6$"
    "^VTK::FiltersSourcesPython-TestStaticCellLocatorLineIntersection$"
    "^VTK::InteractionWidgetsCxx-TestPickingManagerWidgets$"
    "^VTK::InteractionWidgetsPython-TestTensorWidget2$"
    "^VTK::RenderingOpenGL2Cxx-TestCameraShiftScale$"
    "^VTK::RenderingOpenGL2Cxx-TestCoincident$"
    "^VTK::RenderingOpenGL2Cxx-TestCompositePolyDataMapper2CameraShiftScale$"
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

    # Geometry looks "off"
    "^VTK::IOImportCxx-OBJImport-MixedOrder1$"
    "^VTK::IOImportCxx-OBJImport-MTLwithoutTextureFile$"

    # Font rendering differences (new baseline?)
    "^VTK::RenderingFreeTypeCxx-TestFreeTypeTextMapperWithColumns$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  list(APPEND test_exclusions
    # See this issue to track the status of these tests.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098

    # Line rendering differences
    "^VTK::FiltersGeneralCxx-TestYoungsMaterialInterface$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridBinaryEllipseMaterial$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DAxisClipBox$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DDualContour$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridToDualGrid$"
    "^VTK::InteractionWidgetsPython-TestTensorWidget$"
    "^VTK::RenderingCoreCxx-TestEdgeFlags$"
    "^VTK::RenderingImagePython-TestDepthImageToPointCloud$"

    # Point rendering differences
    "^VTK::FiltersGeneralPython-TestCellDerivs$"
    "^VTK::FiltersPointsPython-TestFitImplicitFunction$"
    "^VTK::FiltersPointsPython-TestHierarchicalBinningFilter$"
    "^VTK::FiltersPointsPython-TestSignedDistanceFilter$"
    "^VTK::IOLASCxx-TestLASReader_test_1$"
    "^VTK::IOLASCxx-TestLASReader_test_2$"
    "^VTK::IOPDALCxx-TestPDALReader_test_1$"
    "^VTK::IOPDALCxx-TestPDALReader_test_2$"

    # Test image looks "dim"; image rendering seems to be common
    # (some also have vertical line rendering differences)
    "^VTK::FiltersGeometryCxx-TestExplicitStructuredGridSurfaceFilter$"
    "^VTK::FiltersHybridPython-depthSort$"
    "^VTK::FiltersTexturePython-textureThreshold$"
    "^VTK::GeovisGDALCxx-TestRasterReprojectionFilter$"
    "^VTK::ImagingCorePython-Spectrum$"
    "^VTK::ImagingCorePython-TestMapToWindowLevelColors2$"
    "^VTK::InteractionWidgetsCxx-TestSeedWidget2$"
    "^VTK::IOImageCxx-TestCompressedTIFFReader$"
    "^VTK::IOImageCxx-TestDICOMImageReader$"
    "^VTK::IOImageCxx-TestDICOMImageReaderFileCollection$" # also warns about file I/O
    "^VTK::IOImageCxx-TestTIFFReaderMulti$"
    "^VTK::RenderingAnnotationCxx-TestCornerAnnotation$"
    "^VTK::RenderingCoreCxx-TestTextureRGBA$"
    "^VTK::RenderingCorePython-PickerWithLocator$"
    "^VTK::RenderingVolumeCxx-TestGPURayCastRenderDepthToImage$"
    "^VTK::RenderingVolumeCxx-TestGPURayCastRenderDepthToImage2$"

    # Test image has a different background.
    "^VTK::InteractionWidgetsCxx-TestDijkstraImageGeodesicPath$"

    # Test image looks "better", but seems to have holes
    "^VTK::FiltersModelingCxx-TestQuadRotationalExtrusionMultiBlock$"

    # Numerical problems?
    "^VTK::FiltersOpenTURNSCxx-TestOTKernelSmoothing$"

    # Syntax error in generated shader program.
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow$"

    # Needs investigation
    "^VTKExample-Medical/Cxx")

  if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
    list(APPEND test_exclusions
      # Passes on `offscreen`, fails elsewhere with MPI errors.
      "^VTK::FiltersParallelDIY2Cxx-MPI-TestRedistributeDataSetFilterOnIOSS$"
      "^VTK::IOIOSSCxx-MPI-TestIOSSExodusParitionedFiles$"
      "^VTK::IOIOSSCxx-MPI-TestIOSSExodusRestarts$")
  endif ()
endif ()

# We build CUDA without Qt, the following tests insist in using Qt
# binaries/libs which in turns fails for obscure reasons.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "cuda")
  list(APPEND test_exclusions
    "^VTKExample-GUI/Qt"
    "^VTKExample-Infovis/Cxx"
    "^VTK::IOExportCxx.*PNG$"
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
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetPicking$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetQWidgetWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithChartHistogram2D$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithDisabledInteractor$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithMSAA$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$"

    # Timeout; needs investigated
    "^VTK::FiltersPointsPython-TestPointSmoothingFilter$"
  )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
  list(APPEND test_exclusions
    # Seems to always fail.
    "^VTK::InteractionWidgetsPython-TestInteractorEventRecorder$"

    # This is a flaky test. It sometimes passes.
    "^VTK::RenderingOpenGL2Cxx-TestGlyph3DMapperPickability$")
endif ()


if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  list(APPEND test_exclusions
    # Crowded geometry?
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18230
    "^VTK::ViewsInfovisCxx-TestTreeMapView$"

    # Line rendering differences.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18229
    "^VTK::FiltersGeneralPython-TestCellDerivs$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridBinaryClipPlanes$"
    "^VTK::RenderingAnnotationCxx-TestCubeAxes3$"
    "^VTK::RenderingAnnotationCxx-TestCubeAxesWithYLines$"

    # Random Memory Leak #18599
    "^VTK::FiltersCorePython-probe$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  # Screenshot issue for test comparison with background buffer (intermittent)
  list(APPEND test_exclusions
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$"

    # Random Memory Leak #18599
    "^VTK::FiltersCorePython-probe$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "ext_vtk")
    # Theses tests fail when using an external VTK because the condition
    # VTK_CAN_DO_ONSCREEN AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN should be false
    # in Interaction/Style/Testing/Python/CMakeLists.txt
    list(APPEND test_exclusions
      "^VTK::InteractionStylePython-TestStyleJoystickActor$"
      "^VTK::InteractionStylePython-TestStyleJoystickCamera$"
      "^VTK::InteractionStylePython-TestStyleRubberBandZoomPerspective$"
      "^VTK::InteractionStylePython-TestStyleTrackballActor$"
      "^VTK::InteractionStylePython-TestStyleTrackballCamera$")
  endif ()
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "stdthread")
  list(APPEND test_exclusions
    # Timeout; needs investigated
    # See #18477
    "^VTK::FiltersModelingPython-TestCookieCutter4$"
    # See #18623
    "^VTK::CommonDataModelCxx-TestPolyhedronCombinatorialContouring$"

    # Test fails sometimes with STDThread
    # See #18555
    "^VTK::FiltersFlowPathsCxx-TestEvenlySpacedStreamlines2D$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "vtkmoverride")
  list(APPEND test_exclusions
    # vtkmContour behaves differently than vtkContourFilter for these tests.
    # Further investigation is needed to determine how to best handle these cases.
    "^VTK::FiltersModelingPython-TestBoxFunction$"
    "^VTK::FiltersCorePython-TestContourCases$")
endif ()

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()
