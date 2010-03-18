SET(KIT Hybrid)
SET(UKIT HYBRID)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_INTERFACE_LIBRARIES vtkRendering vtkIO)
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

# for vtkImageDataLIC2D and vtkStructuredGridLIC2D
SET( KIT_LIBS ${KIT_LIBS} vtkftgl )

SET ( Kit_SRCS
vtk3DSImporter.cxx
vtkArcPlotter.cxx
vtkAnnotatedCubeActor.cxx
vtkAxisActor.cxx
vtkAxesActor.cxx
vtkBarChartActor.cxx
vtkCaptionActor2D.cxx
vtkCornerAnnotation.cxx
vtkCubeAxesActor.cxx
vtkCubeAxesActor2D.cxx
vtkDepthSortPolyData.cxx
vtkEarthSource.cxx
vtkFacetReader.cxx
vtkGreedyTerrainDecimation.cxx
vtkGridTransform.cxx
vtkImageDataLIC2D.cxx 
vtkImageDataLIC2DExtentTranslator.cxx
vtkImageToPolyDataFilter.cxx
vtkImplicitModeller.cxx
vtkIterativeClosestPointTransform.cxx
vtkLandmarkTransform.cxx
vtkLegendBoxActor.cxx
vtkLegendScaleActor.cxx
vtkLSDynaReader.cxx
vtkPCAAnalysisFilter.cxx
vtkPieChartActor.cxx
vtkPolyDataSilhouette.cxx
vtkPolyDataToImageStencil.cxx
vtkProcrustesAlignmentFilter.cxx
vtkProjectedTerrainPath.cxx
vtkRIBExporter.cxx
vtkRIBLight.cxx
vtkRIBProperty.cxx
vtkRenderLargeImage.cxx
vtkSpiderPlotActor.cxx
vtkStructuredExtent.cxx
vtkStructuredGridLIC2D.cxx
vtkTemporalDataSetCache.cxx
vtkTemporalInterpolator.cxx
vtkTemporalShiftScale.cxx
vtkTemporalSnapToTimeStep.cxx
vtkThinPlateSplineTransform.cxx
vtkTransformToGrid.cxx
vtkVRMLImporter.cxx
vtkVectorText.cxx
vtkVideoSource.cxx
vtkWeightedTransformFilter.cxx
vtkXYPlotActor.cxx
vtkX3D.cxx
vtkX3DExporter.cxx
vtkX3DExporterWriter.cxx
vtkX3DExporterXMLWriter.cxx
vtkX3DExporterFIWriter.cxx
)

IF(VTK_HAS_EXODUS)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkExodusIICache.cxx
    vtkExodusIIReader.cxx
    vtkExodusIIReaderParser.cxx
    vtkExodusIIReaderVariableCheck.cxx
    vtkExodusReader.cxx
    vtkDSPFilterDefinition.cxx
    vtkExodusModel.cxx
    vtkDSPFilterGroup.cxx
    )
  IF(VTK_USE_PARALLEL)
    SET(Kit_SRCS ${Kit_SRCS}
      vtkPExodusIIReader.cxx
      vtkPExodusReader.cxx
      )
    SET(KIT_INTERFACE_LIBRARIES ${KIT_INTERFACE_LIBRARIES} vtkParallel)
  ENDIF(VTK_USE_PARALLEL)
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
vtkStructuredGridLIC2D
vtkVRML
vtkX3D
vtkX3DExporterWriter
vtkX3DExporterFIWriter
vtkX3DExporterXMLWriter
vtkExodusIIReaderParser
vtkExodusIIReaderVariableCheck
WRAP_EXCLUDE)

# for vtkStructuredGridLIC2D
SET( glsl_files
     vtkStructuredGridLIC2D_fs
   )

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

# for vtkStructuredGridLIC2D
SET( glsl_h_files )
FOREACH( file ${glsl_files} )
  SET( src ${VTK_SOURCE_DIR}/Hybrid/${file}.glsl )
  SET( res ${VTK_BINARY_DIR}/Hybrid/${file}.cxx )
  SET( resh ${VTK_BINARY_DIR}/Hybrid/${file}.h )
  SET( glsl_h_files ${glsl_h_files} "${resh}" )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file} --build-header VTK_HYBRID_EXPORT vtkSystemIncludes.h
    )
  SET( Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${res} )
ENDFOREACH( file )

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
