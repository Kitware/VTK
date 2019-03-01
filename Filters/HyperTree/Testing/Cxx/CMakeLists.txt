# ---------------------------------------------
# outline of dualfilter is wrong sometime
# ---------------------------------------------
#   TestHyperTreeGridTernary3DDualContour.cxx
#   TestHyperTreeGridTernary3DDualContourMaterial.cxx
#   TestHyperTreeGridTernary3DAxisCut.cxx
#   TestHyperTreeGridTernary3DAxisCutMaterial.cxx
#   TestHyperTreeGridTernary3DPlaneCutterMaterial.cxx
#   TestHyperTreeGridTernary3DPlaneCutterDualMaterial.cxx
#   TestHyperTreeGridToDualGrid.cxx

set(test_sources
  TestHyperTreeGridBinary2D.cxx
  TestHyperTreeGridBinary2DAxisClipBox.cxx
  TestHyperTreeGridBinary2DAxisClipEllipse.cxx
  TestHyperTreeGridBinary2DAxisClipPlanes.cxx
  TestHyperTreeGridBinary2DAxisReflectionXCenter.cxx
  TestHyperTreeGridBinary2DAxisReflectionXCenterMaterial.cxx
  TestHyperTreeGridBinary2DAxisReflectionYCenter.cxx
  TestHyperTreeGridBinary2DAxisReflectionYCenterMaterial.cxx
  TestHyperTreeGridBinary2DCellCenters.cxx
  TestHyperTreeGridBinary2DCellCentersMaterial.cxx
  TestHyperTreeGridBinary2DContour.cxx
  TestHyperTreeGridBinary2DContourMaterial.cxx
  TestHyperTreeGridBinary2DDepthLimiter.cxx
  TestHyperTreeGridBinary2DDepthLimiterMaterial.cxx
  TestHyperTreeGridBinary2DIJK.cxx
  TestHyperTreeGridBinary2DInterfaceMaterial.cxx
  TestHyperTreeGridBinary2DMaterial.cxx
  TestHyperTreeGridBinary2DMaterialIJK.cxx
  TestHyperTreeGridBinary2DThreshold.cxx
  TestHyperTreeGridBinary2DThresholdMaterial.cxx
  TestHyperTreeGridBinary2DVector.cxx
  TestHyperTreeGridBinary2DVectorAxisReflectionXCenter.cxx
  TestHyperTreeGridBinary2DVectorAxisReflectionYCenter.cxx
  TestHyperTreeGridBinary3DContour.cxx
  TestHyperTreeGridBinary3DGeometry.cxx
  TestHyperTreeGridBinaryClipPlanes.cxx
  TestHyperTreeGridBinaryEllipseMaterial.cxx
  TestHyperTreeGridBinaryHyperbolicParaboloidMaterial.cxx
  TestHyperTreeGridTernary2D.cxx
  TestHyperTreeGridTernary2DBiMaterial.cxx
  TestHyperTreeGridTernary2DFullMaterialBits.cxx
  TestHyperTreeGridTernary2DMaterial.cxx
  TestHyperTreeGridTernary2DMaterialBits.cxx
  TestHyperTreeGridTernary3DAxisClipBox.cxx
  TestHyperTreeGridTernary3DAxisClipCylinder.cxx
  TestHyperTreeGridTernary3DAxisClipPlanes.cxx
  TestHyperTreeGridTernary3DAxisCut.cxx
  TestHyperTreeGridTernary3DAxisCutMaterial.cxx
  TestHyperTreeGridTernary3DAxisReflectionXCenter.cxx
  TestHyperTreeGridTernary3DAxisReflectionXCenterMaterial.cxx
  TestHyperTreeGridTernary3DAxisReflectionYZCenter.cxx
  TestHyperTreeGridTernary3DAxisReflectionYZCenterMaterial.cxx
  TestHyperTreeGridTernary3DCellCenters.cxx
  TestHyperTreeGridTernary3DCellCentersMaterial.cxx
  TestHyperTreeGridTernary3DClip.cxx
  TestHyperTreeGridTernary3DContour.cxx
  TestHyperTreeGridTernary3DContourMaterial.cxx
  TestHyperTreeGridTernary3DDualContour.cxx
  TestHyperTreeGridTernary3DDualContourMaterial.cxx
  TestHyperTreeGridTernary3DGeometry.cxx
  TestHyperTreeGridTernary3DGeometryLargeMaterialBits.cxx
  TestHyperTreeGridTernary3DGeometryMaterial.cxx
  TestHyperTreeGridTernary3DGeometryMaterialBits.cxx
  TestHyperTreeGridTernary3DPlaneCutter.cxx
  TestHyperTreeGridTernary3DPlaneCutterDual.cxx
  TestHyperTreeGridTernary3DPlaneCutterDualMaterial.cxx
  TestHyperTreeGridTernary3DPlaneCutterMaterial.cxx
  TestHyperTreeGridTernary3DThreshold.cxx
  TestHyperTreeGridTernary3DThresholdMaterial.cxx
  TestHyperTreeGridTernary3DUnstructured.cxx
  TestHyperTreeGridTernary3DUnstructuredMaterial.cxx
  TestHyperTreeGridTernaryHyperbola.cxx
  TestHyperTreeGridTernarySphereMaterial.cxx
  TestHyperTreeGridTernarySphereMaterialReflections.cxx
  TestHyperTreeGridToDualGrid.cxx
  )

vtk_add_test_cxx(vtkFiltersHyperTreeCxxTests tests ${test_sources})
vtk_test_cxx_executable(vtkFiltersHyperTreeCxxTests tests)
