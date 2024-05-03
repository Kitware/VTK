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
  TestHyperTreeGrid2DInterfaceShift.cxx
  TestHyperTreeGrid3DIntercepts.cxx
  TestHyperTreeGrid3DInterface.cxx
  TestHyperTreeGrid3DSimpleInterface.cxx
  TestHyperTreeGridBinary1DFeatureEdges.cxx
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
  TestHyperTreeGridBinary2DFeatureEdges.cxx
  TestHyperTreeGridBinary2DIJK.cxx
  TestHyperTreeGridBinary2DInterfaceMaterial.cxx
  TestHyperTreeGridBinary2DMaterial.cxx
  TestHyperTreeGridBinary2DMaterialIJK.cxx
  TestHyperTreeGridBinary2DThreshold.cxx
  TestHyperTreeGridBinary2DThresholdImplicit.cxx
  TestHyperTreeGridBinary2DThresholdDeep.cxx
  TestHyperTreeGridBinary2DThresholdMaterial.cxx
  TestHyperTreeGridBinary2DThresholdMaterialDeep.cxx
  TestHyperTreeGridBinary2DThresholdMaterialImplicit.cxx
  TestHyperTreeGridBinary2DVector.cxx
  TestHyperTreeGridBinary2DVectorAxisReflectionXCenter.cxx
  TestHyperTreeGridBinary2DVectorAxisReflectionYCenter.cxx
  TestHyperTreeGridBinary3DContour.cxx
  TestHyperTreeGridBinary3DContourDecomposePolyhedra.cxx
  TestHyperTreeGridBinary3DContourImplicit.cxx
  TestHyperTreeGridBinary3DContourImplicitInteger.cxx,NO_VALID,NO_OUTPUT
  TestHyperTreeGridBinary3DFeatureEdges.cxx, LOOSE_VALID
  TestHyperTreeGridBinary3DGeometry.cxx
  TestHyperTreeGridBinaryClipPlanes.cxx
  TestHyperTreeGridBinaryEllipseMaterial.cxx
  TestHyperTreeGridBinaryHyperbolicParaboloidMaterial.cxx
  TestHyperTreeGridExtractGhostCells.cxx,NO_VALID,NO_OUTPUT
  TestHyperTreeGridGeometryPassCellIds.cxx
  TestHyperTreeGridRemoveGhostCells.cxx,NO_VALID,NO_OUTPUT
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
# TestHyperTreeGridTernary3DGeometryLargeMaterialBits.cxx #19221
  TestHyperTreeGridTernary3DGeometryMaterial.cxx
  TestHyperTreeGridTernary3DGeometryMaterialBits.cxx
  TestHyperTreeGridTernary3DGradient.cxx
  TestHyperTreeGridTernary3DPlaneCutter.cxx
  TestHyperTreeGridTernary3DPlaneCutterDual.cxx
  TestHyperTreeGridTernary3DPlaneCutterDualMaterial.cxx
  TestHyperTreeGridTernary3DPlaneCutterMaterial.cxx
  TestHyperTreeGridTernary3DThreshold.cxx
  TestHyperTreeGridTernary3DThresholdDeep.cxx
  TestHyperTreeGridTernary3DThresholdImplicit.cxx
  TestHyperTreeGridTernary3DThresholdMaterial.cxx
  TestHyperTreeGridTernary3DThresholdMaterialDeep.cxx
  TestHyperTreeGridTernary3DThresholdMaterialImplicit.cxx
  TestHyperTreeGridTernary3DUnstructured.cxx
  TestHyperTreeGridTernary3DUnstructuredMaterial.cxx
  TestHyperTreeGridTernaryHyperbola.cxx
  TestHyperTreeGridTernarySphereMaterial.cxx
  TestHyperTreeGridTernarySphereMaterialReflections.cxx
  TestHyperTreeGridToDualGrid.cxx
  TestHyperTreeGridVisibleLeavesSize.cxx,NO_VALID,NO_OUPUT
  )

vtk_add_test_cxx(vtkFiltersHyperTreeCxxTests tests ${test_sources})
vtk_test_cxx_executable(vtkFiltersHyperTreeCxxTests tests)
