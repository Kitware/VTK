vtk_add_test_python(
  NO_RT
  TestBiDirectionalSerialization.py,NO_VALID
  TestBlobs.py,NO_VALID
  TestChartsScatter.py
  TestCompositeDataSets.py,NO_VALID
  TestCompositePolyDataMapper.py
  TestDataSetImportExport.py,NO_VALID
  TestDynamic.py,NO_VALID
  TestGlyph3DMapper.py
  TestInitialize.py,NO_VALID
  TestLabelMapper.py
  TestPolyDataMapperMTime.py,NO_VALID
  TestRendererActorCollection.py,NO_VALID
  TestScalarBarWidget.py
  # TestSilhouette.py,NO_VALID vtkPolyDataSilhouette.cxx needs to mark cells as modified.
  TestSingleUpdate.py
  TestSkybox.py
  TestTexture.py,NO_DATA,NO_VALID
  TestUpdateObjectFromState.py,NO_DATA,NO_VALID
  TestUpdateStateFromObject.py,NO_DATA,NO_VALID
)

# vtkFixedPointVolumeRayCastMapper needs inspection before using either of VTK_MARSHAL(AUTO|MANUAL).
# vtk_add_test_python(
#   TestVolumeFixedPointMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID FixedPoint)

# vtkOpenGLGPUVolumeRayCastMapper needs inspection before using either of VTK_MARSHAL(AUTO|MANUAL).
# vtk_add_test_python(
#   TestVolumeGPUMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID GPU)

# vtkGPUVolumeRayCastMapper needs inspection before using either of VTK_MARSHAL(AUTO|MANUAL).
# vtk_add_test_python(
#   TestVolumeRayCastMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID RayCast)

vtk_add_test_python(
  TestVolumeSmartMapper,TestVolume.py,NO_DATA,NO_RT Smart)
