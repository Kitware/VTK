set(Module_SRCS
  vtkImageStencilIterator.cxx
  vtkExtractVOI.cxx
  vtkImageAppendComponents.cxx
  vtkImageBlend.cxx
  vtkImageCacheFilter.cxx
  vtkImageCast.cxx
  vtkImageChangeInformation.cxx
  vtkImageClip.cxx
  vtkImageConstantPad.cxx
  vtkImageDataStreamer.cxx
  vtkImageDecomposeFilter.cxx
  vtkImageDifference.cxx
  vtkImageExtractComponents.cxx
  vtkImageFlip.cxx
  vtkImageIterateFilter.cxx
  vtkImageMagnify.cxx
  vtkImageMapToColors.cxx
  vtkImageMask.cxx
  vtkImageMirrorPad.cxx
  vtkImagePadFilter.cxx
  vtkImagePermute.cxx
  vtkImageResample.cxx
  vtkImageReslice.cxx
  vtkImageResliceToColors.cxx
  vtkImageShiftScale.cxx
  vtkImageShrink3D.cxx
  vtkImageThreshold.cxx
  vtkImageTranslateExtent.cxx
  vtkImageWrapPad.cxx
  vtkRTAnalyticSource.cxx
  vtkImageResize.cxx
  vtkImageBSplineCoefficients.cxx

  vtkImageStencilData.cxx
  vtkImageStencilAlgorithm.cxx
  vtkAbstractImageInterpolator.cxx
  vtkImageBSplineInternals.cxx
  vtkImageBSplineInterpolator.cxx
  vtkImageSincInterpolator.cxx
  vtkImageInterpolator.cxx

  vtkImageStencilSource.cxx # Needed by vtkImageStencilData
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAbstractImageInterpolator
  vtkImageBSplineInternals
  vtkImageDecomposeFilter
  vtkImageFourierFilter
  vtkImageIterateFilter
  vtkImageSpatialAlgorithm
  ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkImageStencilIterator
  vtkImageBSplineInternals
  WRAP_EXCLUDE
)

vtk_module_library(vtkImagingCore ${Module_SRCS})
