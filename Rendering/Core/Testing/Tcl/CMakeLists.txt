vtk_add_test_tcl(
  ImageActor.tcl
  PickerWithLocator.tcl
  PolyDataMapperAllPolygons.tcl
  PolyDataMapperAllWireframe.tcl
  SurfacePickerWithTexture.tcl
  TestCameraInterpolator.tcl
  TestOpacity2.tcl
  TexturedSphere.tcl
  cowHair2.tcl
  cowHair.tcl
  rendererSource.tcl
  rotations.tcl

  assembly.tcl
  CamBlur.tcl
  ImageActorStressed.tcl
  ScalarBar.tcl
  TestLeaderActor2D.tcl
  TestOnePoint.tcl
  TestWindowToImageTransparency.tcl
  pickCells.tcl
  propAssembly.tcl

  NoLightGeneration.tcl
  TestRotate.tcl
  TestTransformCoordinateSystems.tcl
  TestTransformInterpolator.tcl
  TransformConcatenation.tcl
  TransformCoverage.tcl
  TransformImageData.tcl
  TransformPolyData.tcl
  stereoDresdenMace.tcl
  )

if(NOT VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
vtk_add_test_tcl(
  # pass triangle cell scalars into a strip as field data
  # have the renderer render the triangle strip using the
  # original triangle cell data (now stored in field data)
  # for the opengl2 backend we are not supporting this
  # as strips are not recommended and this is an odd case
  # of trying to get higher performance from strips but yet
  # still trying to make them render as individual triangles
  # for opengl2 just skip the stripper and the result will
  # be what you want
  TestTriangleStripCellColor.tcl
)
endif()
