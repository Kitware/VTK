set(test_exclusions)

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "ubsan")
  list(APPEND test_exclusions
    # Unknown failure; needs investigation. No output, no memcheck errors
    # either.
    "VTK::FiltersPointsCxx-TestPoissonDiskSampler")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  list(APPEND test_exclusions
    # GPURayCast doesn't work with the CI's VNC setup.
    "TestGPURayCast"

    # New baseline?
    "^VTK::RenderingMatplotlibCxx-TestScalarBarCombinatorics$"

    # Numerical problems?
    "^VTK::FiltersOpenTURNSCxx-TestOTKernelSmoothing$"

    # These tests all seem to have some problem with the rendering order of
    # some components of the scenes that are being tested. Needs investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098
    "^VTK::CommonDataModelPython-TestHyperTreeGrid3DMandel$"
    "^VTK::FiltersCorePython-contourCells$"
    "^VTK::FiltersCorePython-contourQuadraticCells$"
    "^VTK::FiltersCorePython-TestPolyDataPlaneClipper2$"
    "^VTK::FiltersFlowPathsCxx-TestBSPTree$"
    "^VTK::FiltersGeneralCxx-TestDateToNumeric$"
    "^VTK::FiltersGeneralCxx-TestDensifyPolyData$"
    "^VTK::FiltersGeneralCxx-TestYoungsMaterialInterface$"
    "^VTK::FiltersGeneralPython-clipQuadraticCells$"
    "^VTK::FiltersGeneralPython-edgePoints$"
    "^VTK::FiltersGeneralPython-TestCellDerivs$"
    "^VTK::FiltersGeneralPython-TestDiscreteFlyingEdgesClipper2D$"
    "^VTK::FiltersGeneralPython-TestFEDiscreteClipper2D$"
    "^VTK::FiltersGeneralPython-TestSampleImplicitFunctionFilter$"
    "^VTK::FiltersGeometryCxx-TestExplicitStructuredGridSurfaceFilter$"
    "^VTK::FiltersGeometryCxx-TestLinearToQuadraticCellsFilter$"
    "^VTK::FiltersGeometryPython-LagrangeGeometricOperations$"
    "^VTK::FiltersHybridPython-depthSort$"
    "^VTK::FiltersHybridPython-imageToPolyData$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridBinaryEllipseMaterial$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridBinaryHyperbolicParaboloidMaterial$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DAxisClipBox$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DDualContour$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernary3DPlaneCutterDual$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernaryHyperbola$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernarySphereMaterial$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridTernarySphereMaterialReflections$"
    "^VTK::FiltersHyperTreeCxx-TestHyperTreeGridToDualGrid$"
    "^VTK::FiltersModelingCxx-TestQuadRotationalExtrusionMultiBlock$"
    "^VTK::FiltersModelingPython-TestCookieCutter$"
    "^VTK::FiltersModelingPython-TestCookieCutter3$"
    "^VTK::FiltersModelingPython-TestImprintFilter2$"
    "^VTK::FiltersModelingPython-TestImprintFilter3$"
    "^VTK::FiltersParallelDIY2Cxx-MPI-TestRedistributeDataSetFilterOnIoss$"
    "^VTK::FiltersPointsPython-TestConnectedPointsFilter$"
    "^VTK::FiltersPointsPython-TestFitImplicitFunction$"
    "^VTK::FiltersPointsPython-TestHierarchicalBinningFilter$"
    "^VTK::FiltersPointsPython-TestPCANormalEstimation$"
    "^VTK::FiltersPointsPython-TestPCANormalEstimation2$"
    "^VTK::FiltersPointsPython-TestRadiusOutlierRemoval$"
    "^VTK::FiltersPointsPython-TestSignedDistanceFilter$"
    "^VTK::FiltersPointsPython-TestVoxelGridFilter$"
    "^VTK::FiltersSourcesPython-TestStaticCellLocatorLineIntersection$"
    "^VTK::FiltersTexturePython-textureThreshold$"
    "^VTK::GeovisGDALCxx-TestRasterReprojectionFilter$"
    "^VTK::ImagingCorePython-Spectrum$"
    "^VTK::ImagingCorePython-TestMapToWindowLevelColors2$"
    "^VTK::InteractionWidgetsCxx-TestDijkstraImageGeodesicPath$"
    "^VTK::InteractionWidgetsCxx-TestPickingManagerWidgets$"
    "^VTK::InteractionWidgetsCxx-TestSeedWidget2$"
    "^VTK::InteractionWidgetsPython-TestPointCloudWidget$"
    "^VTK::InteractionWidgetsPython-TestPointCloudWidget2$"
    "^VTK::InteractionWidgetsPython-TestTensorWidget$"
    "^VTK::InteractionWidgetsPython-TestTensorWidget2$"
    "^VTK::IOGeometryPython-ParticleReader$"
    "^VTK::IOImageCxx-TestCompressedTIFFReader$"
    "^VTK::IOImageCxx-TestDICOMImageReader$"
    "^VTK::IOImageCxx-TestDICOMImageReaderFileCollection$"
    "^VTK::IOImageCxx-TestTIFFReaderMulti$"
    "^VTK::IOImportCxx-OBJImport-MixedOrder1$"
    "^VTK::IOImportCxx-OBJImport-MTLwithoutTextureFile$"
    "^VTK::IOIossCxx-MPI-TestIossExodusParitionedFiles$"
    "^VTK::IOIossCxx-MPI-TestIossExodusRestarts$"
    "^VTK::IOLASCxx-TestLASReader_test_1$"
    "^VTK::IOLASCxx-TestLASReader_test_2$"
    "^VTK::IOPDALCxx-TestPDALReader_test_1$"
    "^VTK::IOPDALCxx-TestPDALReader_test_2$"
    "^VTK::RenderingAnnotationCxx-TestCornerAnnotation$"
    "^VTK::RenderingCoreCxx-TestEdgeFlags$"
    "^VTK::RenderingCoreCxx-TestTextureRGBA$"
    "^VTK::RenderingCoreCxx-TestTextureRGBADepthPeeling$"
    "^VTK::RenderingCorePython-PickerWithLocator$"
    "^VTK::RenderingExternalCxx-TestGLUTRenderWindow$"
    "^VTK::RenderingFreeTypeCxx-TestFontDPIScaling$"
    "^VTK::RenderingFreeTypeCxx-TestFreeTypeTextMapper$"
    "^VTK::RenderingFreeTypeCxx-TestFreeTypeTextMapperWithColumns$"
    "^VTK::RenderingFreeTypeCxx-TestMathTextFreeTypeTextRenderer$"
    "^VTK::RenderingImagePython-TestDepthImageToPointCloud$"
    "^VTK::RenderingOpenGL2Cxx-TestCameraShiftScale$"
    "^VTK::RenderingOpenGL2Cxx-TestCoincident$"
    "^VTK::RenderingOpenGL2Cxx-TestCompositePolyDataMapper2CameraShiftScale$"
    "^VTK::RenderingOpenGL2Cxx-TestCompositePolyDataMapper2CellScalars$"
    "^VTK::RenderingOpenGL2Python-TestTopologyResolution$"
    "^VTK::RenderingVolumeCxx-TestRemoveVolumeNonCurrentContext$"
    "^VTKExample-Medical/Cxx$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # Image size mismatches
    "^VTK::ChartsCoreCxx-TestMultipleScalarsToColors$"
    "^VTK::FiltersCorePython-TestOrientedFlyingEdgesPlaneCutter2$"
    "^VTK::RenderingOpenGL2Cxx-TestToneMappingPass$"

    # Something is wrong. #18144
    "^VTK::FiltersCorePython-QuadricDecimation2$"

    # PATH manipulations needed
    "^VTKExample-ImageProcessing/Cxx$"
    "^VTKExample-IO/Cxx$"
    "^VTKExample-Medical/Cxx$"
    "^VTKExample-Modelling/Cxx$"
    "^VTKExample-Modules/UsingVTK$"
    "^VTKExample-Modules/Wrapping$"

    # Blank test image
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWindowWithDisabledInteractor$"

    # Image corruption
    "^VTK::RenderingCorePython-TestWindowToImageTransparency$"
    "^VTK::RenderingCorePython-rendererSource$"
    "^VTK::RenderingImagePython-TestDepthImageToPointCloud$"
    "^VTK::RenderingImagePython-TestDepthImageToPointCloud-TwoInputs$"
    "^VTK::RenderingOpenGL2Cxx-TestWindowBlits$"

    # Timeouts; need investigation.
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetPicking$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetQWidgetWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetWithDisabledInteractor$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetWithMSAA$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetPicking$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetQWidgetWidget$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithDisabledInteractor$"
    "^VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithMSAA$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
  list(APPEND test_exclusions
    # Seems to always fail.
    "^VTK::InteractionWidgetsPython-TestInteractorEventRecorder$"

    # This is a flaky test. It sometimes passes.
    "^VTK::RenderingOpenGL2Cxx-TestGlyph3DMapperPickability$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "stdthread") # They failed also on OpenMP build (non tested)
  list(APPEND test_exclusions
    # Theses tests fail for stdthread + openmp builds they may be link to a bad use of ThreadLocal
    # Need investigations https://gitlab.kitware.com/vtk/vtk/-/issues/18222
    "^VTK::FiltersCoreCxx-TestProbeFilterImageInput$"
    "^VTK::FiltersCoreCxx-TestResampleWithDataSet$"
    "^VTK::FiltersCorePython-TestProbeFilterImageInput$"
    "^VTK::FiltersExtractionCxx-TestExtractSelectionUsingDataAssembly$"
    "^VTK::FiltersExtractionPython-ExtractTensors$"
    "^VTK::FiltersGeneralCxx-TestAnimateModes$"
    "^VTK::FiltersParallelCxx-MPI-ParallelResampling$"
    "^VTK::FiltersParallelDIY2Cxx-TestRedistributeDataSetFilterOnIoss$"
    "^VTK::FiltersPointsPython-TestPointSmoothingFilter$"
    "^VTK::IOIossCxx-TestIossExodus$"
    "^VTK::IOIossCxx-TestIossSuperelements$"
    "^VTK::ImagingHybridPython-TestCheckerboardSplatter$"
    "^VTK::InteractionWidgetsCxx-TestSplineWidget$"
    "^VTK::ParallelMPICxx-MPI-TestPProbe$")
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
    "^VTK::RenderingAnnotationCxx-TestCubeAxesWithYLines$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  # Screenshot issue for test comparison with background buffer
  list(APPEND test_exclusions
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget$"
    "^VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow$")
endif ()

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()
