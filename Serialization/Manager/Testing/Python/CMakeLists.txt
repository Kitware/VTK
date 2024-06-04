vtk_add_test_python(
  TestBlobs.py,NO_VALID
  TestCompositeDataSets.py,NO_VALID
  TestCompositePolyDataMapper.py,NO_VALID
  TestChartsScatter.py,NO_VALID
  TestDynamic.py,NO_VALID
  TestGlyph3DMapper.py,NO_VALID
  TestInitialize.py,NO_VALID
  TestLabelMapper.py,NO_VALID
  # TestScalarBarWidget.py,NO_VALID needs more work, turn off interactor
  # TestSilhouette.py,NO_VALID vtkPolyDataSilhouette.cxx needs to mark cells as modified.
  TestSingleUpdate.py,NO_VALID
  TestTexture.py,NO_VALID
  TestSkybox.py,NO_VALID,
)

# vtkFixedPointVolumeRayCastMapper needs inspection before using either of VTK_MARSHAL(AUTO|MANUAL).
# vtk_add_test_python(
#   TestFixedPointVolumeMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID FixedPoint)

# vtkOpenGLGPUVolumeRayCastMapper needs inspection before using either of VTK_MARSHAL(AUTO|MANUAL).
# vtk_add_test_python(
#   TestGPUVolumeMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID GPU)

# vtkGPUVolumeRayCastMapper needs inspection before using either of VTK_MARSHAL(AUTO|MANUAL).
# vtk_add_test_python(
#   TestRayCastVolumeMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID RayCast)

vtk_add_test_python(
  TestSmartVolumeMapper,TestVolume.py,NO_DATA,NO_OUTPUT,NO_VALID Smart)
