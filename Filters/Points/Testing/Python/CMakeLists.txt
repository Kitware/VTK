vtk_add_test_python(
  TestDensifyPointCloudFilter2.py
  TestEllipsoidalGaussianKernel.py
  TestPointInterpolator.py
  TestPointInterpolator2.py
  TestPointInterpolator2D.py
  TestPointInterpolator2D2.py
  TestPointOccupancyFilter.py
  TestSPHInterpolator.py
  TestSPHInterpolator2D.py
  TestSPHInterpolatorWithCutoff.py
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_python(
    TestDensifyPointCloudFilter.py
    TestEuclideanClusterExtraction.py
    TestEuclideanClusterExtraction2.py
    TestExtractPoints.py
    TestFitImplicitFunction.py
    TestHierarchicalBinningFilter.py
    TestPCACurvatureEstimation.py
    TestPCACurvatureEstimation2.py
    TestPCANormalEstimation.py
    TestPCANormalEstimation2.py
    TestPointDensityFilter.py
    TestPointDensityGradient.py
    TestRadiusOutlierRemoval.py
    TestSignedDistanceFilter.py
    TestStatisticalOutlierRemoval.py
    TestUnsignedDistanceFilter.py
    TestVoxelGridFilter.py
    )
endif ()
