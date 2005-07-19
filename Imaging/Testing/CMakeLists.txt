SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Imaging ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Imaging"
    VTK_IMAGING_EXPORT
    vtkImageAppendComponents.h
    vtkImageCityBlockDistance.h
    vtkImageDivergence.h
    vtkImageDotProduct.h
    vtkImageFFT.h
    vtkImageFourierCenter.h
    vtkImageFourierFilter.h
    vtkImageHybridMedian2D.h
    vtkImageLuminance.h
    vtkImageMagnitude.h
    vtkImageMapToRGBA.h
    vtkImageMirrorPad.h
    vtkImageNormalize.h
    vtkImageRFFT.h
    vtkImageWrapPad.h
    vtkSimpleImageFilterExample.h
    )
ENDIF(PYTHON_EXECUTABLE)
