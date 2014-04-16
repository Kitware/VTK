vtk_add_test_python(
  TestBunykRayCastFunction.py
  TestLODProp3D.py
  TestPTZSweep.py
  gaussian.py
  volRCClipPlanes.py
  volRCCropRegions.py
  volRCRotateClip.py
  volTM2DCropRegions.py
  volTM2DRotateClip.py
  VolumeOutlineSource.py
  VolumeOutlineSourceClipped.py
  VolumePickerCrop.py
  cursor3D.py
  volTM3DCompressedCropRegions.py
  volTM3DCropRegions.py
  volTM3DRotateClip.py
  TestFixedPointRayCasterLinear.py,NO_RT
  TestFixedPointRayCasterLinearCropped.py,NO_RT
  TestFixedPointRayCasterNearest.py,NO_RT
  TestFixedPointRayCasterNearestCropped.py,NO_RT
  VolumePicker.py,NO_RT
  )

set_tests_properties(vtkRenderingVolumePython-volTM3DCompressedCropRegions
  PROPERTIES TIMEOUT 250)
set_tests_properties(vtkRenderingVolumePython-volTM3DCropRegions
  PROPERTIES TIMEOUT 250)
set_tests_properties(vtkRenderingVolumePython-volTM3DRotateClip
  PROPERTIES TIMEOUT 80)
