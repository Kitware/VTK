SET(KIT Imaging)
SET(UKIT IMAGING)
SET(KIT_TCL_LIBS vtkFilteringTCL)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkFilteringPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkFilteringJava)
SET(KIT_LIBS vtkFiltering)

SET( Kit_SRCS
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
vtkImageSpatialFilter
  ABSTRACT
)

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

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
                       "${Kit_SRCS}"
                       VTK_${UKIT}_EXPORT
                       ${VTK_BINARY_DIR} "")

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL3(vtk${KIT}TCL KitTCL_SRCS 
                "${Kit_SRCS}"
                "${Kit_TCL_EXTRA_CMDS}")
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}TCL)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_TCL_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}TCL ${KIT_TCL_DEPS})
  ENDIF(KIT_TCL_DEPS)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON3(vtk${KIT}Python KitPython_SRCS "${Kit_SRCS}")
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS}
                          ${VTK_PYTHON_LIBRARIES})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    IF(NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}PythonD)
    ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32)
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Python)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_PYTHON_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}Python ${KIT_PYTHON_DEPS})
  ENDIF(KIT_PYTHON_DEPS)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA3(vtk${KIT}Java KitJava_SRCS "${Kit_SRCS}")
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Java)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  IF(KIT_JAVA_DEPS)
    ADD_DEPENDENCIES(vtk${KIT}Java ${KIT_JAVA_DEPS})
  ENDIF(KIT_JAVA_DEPS)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT})
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h ${Kit_SRCS})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)

# End of common section
# -----------------------------------------------------------------
