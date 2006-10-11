SET(KIT Hybrid)
SET(UKIT HYBRID)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO)
IF(VTK_HAS_EXODUS)
  SET(KIT_LIBS ${KIT_LIBS} vtkexoIIc)
ENDIF(VTK_HAS_EXODUS)

IF (WIN32)
  IF (VTK_VFW_SUPPORTS_CAPTURE)
    IF(BORLAND)
      INCLUDE ("${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake")
      CHECK_LIBRARY_EXISTS("vfw32" "printf" "" HAVE_VFW32)
      IF(NOT HAVE_VFW32)
        MESSAGE(SEND_ERROR "Bad installation of Borland compiler. PSDK is not in ilink32.cfg and in bcc32.cfg.")
      ENDIF(NOT HAVE_VFW32)
    ENDIF(BORLAND)
    SET(KIT_LIBS ${KIT_LIBS} vfw32)
  ENDIF (VTK_VFW_SUPPORTS_CAPTURE)
  IF (VTK_USE_MATROX_IMAGING)
    SET(KIT_LIBS ${KIT_LIBS} ${MIL_LIBRARY})
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET ( Kit_SRCS
vtk3DSImporter.cxx
vtkArcPlotter.cxx
vtkAnnotatedCubeActor.cxx
vtkAxesActor.cxx
vtkCaptionActor2D.cxx
vtkCornerAnnotation.cxx
vtkCubeAxesActor2D.cxx
vtkDepthSortPolyData.cxx
vtkEarthSource.cxx
vtkFacetReader.cxx
vtkGreedyTerrainDecimation.cxx
vtkGridTransform.cxx
vtkImageToPolyDataFilter.cxx
vtkImplicitModeller.cxx
vtkIterativeClosestPointTransform.cxx
vtkLandmarkTransform.cxx
vtkLegendBoxActor.cxx
vtkLSDynaReader.cxx
vtkPCAAnalysisFilter.cxx
vtkPolyDataToImageStencil.cxx
vtkProcrustesAlignmentFilter.cxx
vtkProjectedTerrainPath.cxx
vtkRIBExporter.cxx
vtkRIBLight.cxx
vtkRIBProperty.cxx
vtkRenderLargeImage.cxx
vtkTemporalDataSetCache.cxx
vtkTemporalInterpolator.cxx
vtkTemporalShiftScale.cxx
vtkThinPlateSplineTransform.cxx
vtkTransformToGrid.cxx
vtkVRMLImporter.cxx
vtkVectorText.cxx
vtkVideoSource.cxx
vtkWeightedTransformFilter.cxx
vtkXYPlotActor.cxx
)
IF(VTK_HAS_EXODUS)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkPExodusReader.cxx
    vtkExodusReader.cxx
    vtkDSPFilterDefinition.cxx
    vtkExodusModel.cxx
    vtkDSPFilterGroup.cxx
    )
ENDIF(VTK_HAS_EXODUS)

IF (WIN32)
  IF (VTK_VFW_SUPPORTS_CAPTURE)
    SET (Kit_SRCS ${Kit_SRCS} vtkWin32VideoSource.cxx)
  ENDIF (VTK_VFW_SUPPORTS_CAPTURE)

  IF (VTK_USE_MATROX_IMAGING)
    SET (Kit_SRCS ${Kit_SRCS} vtkMILVideoSource.cxx)
    # Include directory needed for vtkMILVideoSource.cxx.
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(
vtkVRML
WRAP_EXCLUDE)

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

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vtk3DS.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
