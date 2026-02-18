set(classes
  vtkAbstractImageInterpolator
  vtkExtractVOI
  vtkGenericImageInterpolator
  vtkImageAppendComponents
  vtkImageBinaryThreshold
  vtkImageBlend
  vtkImageBSplineCoefficients
  vtkImageBSplineInternals
  vtkImageBSplineInterpolator
  vtkImageCacheFilter
  vtkImageCast
  vtkImageChangeInformation
  vtkImageClip
  vtkImageConstantPad
  vtkImageDataStreamer
  vtkImageDecomposeFilter
  vtkImageDifference
  vtkImageExtractComponents
  vtkImageFlip
  vtkImageInterpolator
  vtkImageIterateFilter
  vtkImageMagnify
  vtkImageMapToColors
  vtkImageMask
  vtkImageMirrorPad
  vtkImagePadFilter
  vtkImagePermute
  vtkImagePointDataIterator
  vtkImagePointIterator
  vtkImageProbeFilter
  vtkImageResample
  vtkImageResize
  vtkImageReslice
  vtkImageResliceToColors
  vtkImageShiftScale
  vtkImageShrink3D
  vtkImageSincInterpolator
  vtkImageStencilAlgorithm
  vtkImageStencilData
  vtkImageStencilIterator
  vtkImageStencilSource # Needed by vtkImageStencilData
  vtkImageSSIM
  vtkImageThreshold
  vtkImageTranslateExtent
  vtkImageWrapPad
  vtkRTAnalyticSource)

set(nowrap_headers
  vtkInterpolationMath.h)

vtk_module_add_module(VTK::ImagingCore
  CLASSES        ${classes}
  NOWRAP_HEADERS ${nowrap_headers})
vtk_add_test_mangling(VTK::ImagingCore)
