SOURCE_FILES( Imaging_SRCS
vtkBooleanTexture.cxx
vtkExtractVOI.cxx
vtkGaussianSplatter.cxx
vtkImageAccumulate.cxx
vtkImageAnisotropicDiffusion2D.cxx
vtkImageAnisotropicDiffusion3D.cxx
vtkImageAppend.cxx
vtkImageAppendComponents.cxx
vtkImageBlend.cxx
vtkImageButterworthHighPass.cxx
vtkImageButterworthLowPass.cxx
vtkImageCacheFilter.cxx
vtkImageCanvasSource2D.cxx
vtkImageCast.cxx
vtkImageCheckerboard.cxx
vtkImageChangeInformation.cxx
vtkImageCityBlockDistance.cxx
vtkImageClip.cxx
vtkImageConnector.cxx
vtkImageConstantPad.cxx
vtkImageContinuousDilate3D.cxx
vtkImageContinuousErode3D.cxx
vtkImageConvolve.cxx
vtkImageCorrelation.cxx
vtkImageCursor3D.cxx
vtkImageDataStreamer.cxx
vtkImageDecomposeFilter.cxx
vtkImageDifference.cxx
vtkImageDilateErode3D.cxx
vtkImageDivergence.cxx
vtkImageDotProduct.cxx
vtkImageEllipsoidSource.cxx
vtkImageEuclideanDistance.cxx
vtkImageEuclideanToPolar.cxx
vtkImageExport.cxx
vtkImageExtractComponents.cxx
vtkImageFFT.cxx
vtkImageFlip.cxx
vtkImageFourierCenter.cxx
vtkImageFourierFilter.cxx
vtkImageGaussianSmooth.cxx
vtkImageGaussianSource.cxx
vtkImageGradient.cxx
vtkImageGradientMagnitude.cxx
vtkImageGridSource.cxx
vtkImageHSVToRGB.cxx
vtkImageHybridMedian2D.cxx
vtkImageIdealHighPass.cxx
vtkImageIdealLowPass.cxx
vtkImageToImageStencil.cxx
vtkImageImport.cxx
vtkImageIslandRemoval2D.cxx
vtkImageIterateFilter.cxx
vtkImageLaplacian.cxx
vtkImageLogarithmicScale.cxx
vtkImageLogic.cxx
vtkImageLuminance.cxx
vtkImageMagnify.cxx
vtkImageMagnitude.cxx
vtkImageMandelbrotSource.cxx
vtkImageMapToColors.cxx
vtkImageMapToRGBA.cxx
vtkImageMapToWindowLevelColors.cxx
vtkImageMask.cxx
vtkImageMaskBits.cxx
vtkImageMathematics.cxx
vtkImageMedian3D.cxx
vtkImageMirrorPad.cxx
vtkImageNoiseSource.cxx
vtkImageNonMaximumSuppression.cxx
vtkImageNormalize.cxx
vtkImageOpenClose3D.cxx
vtkImagePadFilter.cxx
vtkImagePermute.cxx
vtkImageQuantizeRGBToIndex.cxx
vtkImageReslice.cxx
vtkImageRFFT.cxx
vtkImageRGBToHSV.cxx
vtkImageRange3D.cxx
vtkImageResample.cxx
vtkImageSeedConnectivity.cxx
vtkImageSeparableConvolution.cxx
vtkImageShiftScale.cxx
vtkImageShrink3D.cxx
vtkImageSinusoidSource.cxx
vtkImageSkeleton2D.cxx
vtkImageSobel2D.cxx
vtkImageSobel3D.cxx
vtkImageSpatialFilter.cxx
vtkImageStencil.cxx
vtkImageStencilData.cxx
vtkImageStencilSource.cxx
vtkImageThreshold.cxx
vtkImageTranslateExtent.cxx
vtkImageVariance3D.cxx
vtkImageWrapPad.cxx
vtkImplicitFunctionToImageStencil.cxx
vtkPointLoad.cxx
vtkSampleFunction.cxx
vtkShepardMethod.cxx
vtkSimpleImageFilterExample.cxx
vtkSurfaceReconstructionFilter.cxx
vtkTriangularTexture.cxx
vtkVoxelModeller.cxx
)

ABSTRACT_FILES(
vtkImageDecomposeFilter
vtkImageIterateFilter
vtkImagePadFilter
vtkImageSpatialFilter
vtkImageFourierFilter
)

IF(VTK_MAKE_INSTANTIATOR)
  # Setup vtkInstantiator registration for this library's classes.
  VTK_MAKE_INSTANTIATOR(vtkImagingInstantiator ImagingInstantiator_SRCS
                        Imaging_SRCS
                        EXPORT_MACRO VTK_IMAGING_EXPORT
                        HEADER_LOCATION ${VTK_BINARY_DIR})
  ADD_LIBRARY(vtkImaging Imaging_SRCS ImagingInstantiator_SRCS)
  INSTALL_FILES(/include/vtk .h ${VTK_BINARY_DIR}/vtkImagingInstantiator)
ELSE(VTK_MAKE_INSTANTIATOR)
  ADD_LIBRARY(vtkImaging Imaging_SRCS)
ENDIF(VTK_MAKE_INSTANTIATOR)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Imaging/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Imaging/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkImagingTCL ImagingTCL_SRCS Imaging_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkImagingTCL SHARED ImagingTCL_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkImagingTCL ImagingTCL_SRCS)
  ENDIF (APPLE)
  TARGET_LINK_LIBRARIES (vtkImagingTCL
                         vtkFilteringTCL
                         vtkCommonTCL
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkImagingTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkImagingPython ImagingPython_SRCS Imaging_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkImagingPython MODULE ImagingPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkImagingPython SHARED ImagingPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkImagingPython
                           vtkFilteringPython
                           vtkCommonPython)
  ENDIF (APPLE)

  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkImagingPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkImagingPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)

  INSTALL_TARGETS(/lib/vtk vtkImagingPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkImagingJava ImagingJava_SRCS Imaging_SRCS)
  ADD_LIBRARY(vtkImagingJava SHARED ImagingJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkImagingJava
                         vtkCommonJava
                         vtkFilteringJava)
  INSTALL_TARGETS(/lib/vtk vtkImagingJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon
vtkFiltering
vtkImaging
)

INSTALL_TARGETS(/lib/vtk vtkImaging)
INSTALL_FILES(/include/vtk .h Imaging_SRCS)
