SET(KIT Imaging)
SET(UKIT IMAGING)
SET(KIT_TCL_LIBS vtkFilteringTCL)
SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
SET(KIT_JAVA_LIBS vtkFilteringJava)
SET(KIT_LIBS vtkFiltering)

SET( Kit_SRCS
vtkBooleanTexture.cxx
vtkExtractVOI.cxx
vtkFastSplatter.cxx
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
vtkImageHSIToRGB.cxx
vtkImageHSVToRGB.cxx
vtkImageHybridMedian2D.cxx
vtkImageIdealHighPass.cxx
vtkImageIdealLowPass.cxx
vtkImageImport.cxx
vtkImageImportExecutive.cxx
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
vtkImageRGBToHSI.cxx
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
vtkImageSpatialAlgorithm.cxx
vtkImageStencil.cxx
vtkImageStencilData.cxx
vtkImageStencilSource.cxx
vtkImageThreshold.cxx
vtkImageToImageStencil.cxx
vtkImageTranslateExtent.cxx
vtkImageVariance3D.cxx
vtkImageWrapPad.cxx
vtkImageWeightedSum.cxx
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
vtkImageSpatialAlgorithm
  ABSTRACT
)

IF(CMAKE_COMPILER_IS_GNUCXX AND VTK_GNUCXX_VERSION)
  STRING(REGEX MATCH "3\\.2\\.[0-9]" is_gcc32 ${VTK_GNUCXX_VERSION})
  IF(is_gcc32)
    SET_SOURCE_FILES_PROPERTIES(
     vtkImageEuclideanDistance.cxx
     vtkImageLogic.cxx
     PROPERTIES
     COMPILE_FLAGS -O0
     )
  ENDIF(is_gcc32)
ENDIF(CMAKE_COMPILER_IS_GNUCXX AND VTK_GNUCXX_VERSION)

# conditionally compile legacy classes
IF (NOT VTK_LEGACY_REMOVE)
  SET (Kit_SRCS ${Kit_SRCS} 
    vtkImageSpatialFilter.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(
    vtkImageSpatialFilter
    ABSTRACT
    )
ENDIF (NOT VTK_LEGACY_REMOVE)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
