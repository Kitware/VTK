SOURCE_FILES( Imaging_SRCS
vtkBooleanTexture
vtkExtractVOI
vtkGaussianSplatter
vtkImageAccumulate
vtkImageAnisotropicDiffusion2D
vtkImageAnisotropicDiffusion3D
vtkImageAppend
vtkImageAppendComponents
vtkImageBlend
vtkImageButterworthHighPass
vtkImageButterworthLowPass
vtkImageCacheFilter
vtkImageCanvasSource2D
vtkImageCast
vtkImageCheckerboard
vtkImageChangeInformation
vtkImageCityBlockDistance
vtkImageClip
vtkImageConnector
vtkImageConstantPad
vtkImageContinuousDilate3D
vtkImageContinuousErode3D
vtkImageConvolve
vtkImageCorrelation
vtkImageCursor3D
vtkImageDataStreamer
vtkImageDecomposeFilter
vtkImageDifference
vtkImageDilateErode3D
vtkImageDivergence
vtkImageDotProduct
vtkImageEllipsoidSource
vtkImageEuclideanDistance
vtkImageEuclideanToPolar
vtkImageExport
vtkImageExtractComponents
vtkImageFFT
vtkImageFlip
vtkImageFourierCenter
vtkImageFourierFilter
vtkImageGaussianSmooth
vtkImageGaussianSource
vtkImageGradient
vtkImageGradientMagnitude
vtkImageGridSource
vtkImageHSVToRGB
vtkImageHybridMedian2D
vtkImageIdealHighPass
vtkImageIdealLowPass
vtkImageToImageStencil
vtkImageImport
vtkImageIslandRemoval2D
vtkImageIterateFilter
vtkImageLaplacian
vtkImageLogarithmicScale
vtkImageLogic
vtkImageLuminance
vtkImageMagnify
vtkImageMagnitude
vtkImageMandelbrotSource
vtkImageMapToColors
vtkImageMapToRGBA
vtkImageMapToWindowLevelColors
vtkImageMask
vtkImageMaskBits
vtkImageMathematics
vtkImageMedian3D
vtkImageMirrorPad
vtkImageNoiseSource
vtkImageNonMaximumSuppression
vtkImageNormalize
vtkImageOpenClose3D
vtkImagePadFilter
vtkImagePermute
vtkImageQuantizeRGBToIndex
vtkImageReslice
vtkImageRFFT
vtkImageRGBToHSV
vtkImageRange3D
vtkImageResample
vtkImageSeedConnectivity
vtkImageShiftScale
vtkImageShrink3D
vtkImageSinusoidSource
vtkImageSkeleton2D
vtkImageSobel2D
vtkImageSobel3D
vtkImageSpatialFilter
vtkImageStencil
vtkImageStencilData
vtkImageStencilSource 
vtkImageThreshold
vtkImageTranslateExtent
vtkImageVariance3D
vtkImageWrapPad
vtkImplicitFunctionToImageStencil
vtkPointLoad
vtkSampleFunction
vtkShepardMethod
vtkSimpleImageFilterExample
vtkSurfaceReconstructionFilter 
vtkTriangularTexture
vtkVoxelModeller
vtkWindowToImageFilter 
)

ABSTRACT_FILES(
vtkImageDecomposeFilter 
vtkImageInPlaceFilter 
vtkImageIterateFilter 
vtkImageMultipleInputFilter 
vtkImageMultipleInputOutputFilter 
vtkImagePadFilter 
vtkImageSpatialFilter 
vtkImageToImageFilter 
vtkImageTwoInputFilter 
vtkImageFourierFilter 
)

ADD_LIBRARY(vtkImaging Imaging_SRCS)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Imaging/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Imaging/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkImagingTCL ImagingTCL_SRCS Imaging_SRCS)
  ADD_LIBRARY(vtkImagingTCL ImagingTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL 
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkImagingTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkImagingPython ImagingPython_SRCS Imaging_SRCS)
  ADD_LIBRARY(vtkImagingPython SHARED ImagingPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkImagingPython 
                         vtkFilteringPython 
                         vtkCommonPython )
  TARGET_LINK_LIBRARIES (vtkImagingPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkImagingPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkImagingJava ImagingJava_SRCS Imaging_SRCS)
  ADD_LIBRARY(vtkImagingJava ImagingJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkImagingJava 
                         vtkCommonJava 
                         vtkFilteringJava)
  INSTALL_TARGETS(/lib vtkImagingJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkImaging 
)

INSTALL_TARGETS(/lib vtkImaging)
INSTALL_FILES(/include/vtk .h Imaging_SRCS)
