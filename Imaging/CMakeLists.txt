SET( Imaging_SRCS
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
vtkImageChangeInformation.cxx
vtkImageCheckerboard.cxx
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
vtkImageRFFT.cxx
vtkImageRGBToHSV.cxx
vtkImageRange3D.cxx
vtkImageRectilinearWipe.cxx
vtkImageResample.cxx
vtkImageReslice.cxx
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
vtkImageToImageStencil.cxx
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

SET_SOURCE_FILES_PROPERTIES(
vtkImageDecomposeFilter
vtkImageFourierFilter
vtkImageIterateFilter
vtkImagePadFilter
vtkImageSpatialFilter
ABSTRACT
)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkImagingInstantiator ImagingInstantiator_SRCS
                       ${Imaging_SRCS}
                       EXPORT_MACRO VTK_IMAGING_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkImaging ${Imaging_SRCS} ${ImagingInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkImaging PROPERTIES LINK_FLAGS "-init _oft_initIma")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Imaging/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Imaging/LocalUserOptions.cmake OPTIONAL)

TARGET_LINK_LIBRARIES (vtkImaging vtkFiltering)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkImagingTCL ImagingTCL_SRCS ${Imaging_SRCS})
  ADD_LIBRARY(vtkImagingTCL ${ImagingTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkImagingTCL
                         vtkFilteringTCL
                         vtkImaging
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkImagingTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkImagingPython ImagingPython_SRCS ${Imaging_SRCS})
  ADD_LIBRARY(vtkImagingPython MODULE ${ImagingPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkImagingPython
                           vtkFilteringPython
                           vtkCommonPython)
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkImagingPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkImagingPython vtkImaging)

  INSTALL_TARGETS(/lib/vtk vtkImagingPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkImagingJava ImagingJava_SRCS ${Imaging_SRCS})
  ADD_LIBRARY(vtkImagingJava SHARED ${ImagingJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkImagingJava
                         vtkCommonJava
                         vtkFilteringJava
			 vtkImaging)
  INSTALL_TARGETS(/lib/vtk vtkImagingJava)
ENDIF (VTK_WRAP_JAVA)

INSTALL_TARGETS(/lib/vtk vtkImaging)
INSTALL_FILES(/include/vtk .h ${Imaging_SRCS})
