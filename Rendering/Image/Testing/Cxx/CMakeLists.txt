vtk_add_test_cxx(vtkRenderingImageCxxTests tests
  TestCellPickerImage.cxx
  TestImageResliceMapperAlpha.cxx
  TestImageResliceMapperBackground.cxx
  TestImageResliceMapperBorder.cxx
  TestImageResliceMapperInterpolation.cxx
  TestImageResliceMapperOffAxis.cxx
  TestImageResliceMapperOrient3D.cxx
  TestImageResliceMapperOriented3D.cxx
  TestImageResliceMapperOrientedFlip.cxx
  TestImageResliceMapperSlab.cxx
  TestImageSliceMapperAlpha.cxx
  TestImageSliceMapperBackground.cxx
  TestImageSliceMapperBorder.cxx
  TestImageSliceMapperInterpolation.cxx
  TestImageSliceMapperLargeImage.cxx
  TestImageSliceMapperOrient2D.cxx
  TestImageSliceMapperOrient3D.cxx
  TestImageSliceMapperOriented3D.cxx
  TestImageSliceMapperSelection.cxx
  TestImageStack.cxx
  )

vtk_test_cxx_executable(vtkRenderingImageCxxTests tests)
