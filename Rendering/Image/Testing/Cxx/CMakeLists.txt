vtk_add_test_cxx(vtkRenderingImageCxxTests tests
  TestCellPickerImage.cxx
  TestImageSliceMapperAlpha.cxx
  TestImageResliceMapperAlpha.cxx
  TestImageSliceMapperBackground.cxx
  TestImageResliceMapperBackground.cxx
  TestImageSliceMapperBorder.cxx
  TestImageResliceMapperBorder.cxx
  TestImageSliceMapperInterpolation.cxx
  TestImageResliceMapperInterpolation.cxx
  TestImageSliceMapperOrient2D.cxx
  TestImageResliceMapperOffAxis.cxx
  TestImageSliceMapperOrient3D.cxx
  TestImageSliceMapperOriented3D.cxx
  TestImageResliceMapperOrient3D.cxx
  TestImageStack.cxx
  TestImageResliceMapperSlab.cxx
  )

vtk_test_cxx_executable(vtkRenderingImageCxxTests tests)
