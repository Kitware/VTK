vtk_add_test_python(
  TestEllipsoidalGaussianKernel.py
  TestPointInterpolator.py
  TestPointInterpolator2.py
  TestPointInterpolator2D.py
  TestPointInterpolator2D2.py
  TestSPHInterpolator.py
  TestSPHInterpolator2D.py
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_python(
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
    TestRadiusOutlierRemoval.py
    TestSignedDistanceFilter.py
    TestStatisticalOutlierRemoval.py
    TestVoxelGridFilter.py
    )
endif ()
