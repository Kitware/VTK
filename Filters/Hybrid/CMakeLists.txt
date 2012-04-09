set(Module_SRCS
  vtkBSplineTransform.cxx
  vtkDepthSortPolyData.cxx
  vtkDSPFilterDefinition.cxx
  vtkDSPFilterGroup.cxx
  vtkEarthSource.cxx
  vtkFacetReader.cxx
  vtkGreedyTerrainDecimation.cxx
  vtkGridTransform.cxx
  vtkImageDataLIC2D.cxx
  vtkImageDataLIC2DExtentTranslator.cxx
  vtkImageToPolyDataFilter.cxx
  vtkImplicitModeller.cxx
  vtkPCAAnalysisFilter.cxx
  vtkPolyDataSilhouette.cxx
  #vtkPolyDataToImageStencil.cxx
  vtkProcrustesAlignmentFilter.cxx
  vtkProjectedTerrainPath.cxx
  vtkRenderLargeImage.cxx
  vtkTemporalDataSetCache.cxx
  vtkTemporalInterpolator.cxx
  vtkTemporalShiftScale.cxx
  vtkTemporalSnapToTimeStep.cxx
  vtkTransformToGrid.cxx
  vtkWeightedTransformFilter.cxx
  # FIXME: These should not be in filters.
  vtkStructuredGridLIC2D.cxx
  vtkStructuredGridLIC2D_fs.cxx 
  )

set(shader_files
  vtkStructuredGridLIC2D_fs.glsl
  )

# FIXME: This should be in a rendering module (an OpenGL rendering module)
# for vtkStructuredGridLIC2D
SET( shader_h_files )
FOREACH( file ${shader_files} )
  #message("@@@@ file=${file}")
  GET_FILENAME_COMPONENT(file_we ${file} NAME_WE)
  SET( src ${CMAKE_CURRENT_SOURCE_DIR}/${file} )
  SET( res ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx )
  SET( resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h )
  SET( shader_h_files ${shader_h_files} "${resh}" )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file_we} --build-header VTK_HYBRID_EXPORT vtkSystemIncludes.h
    )
  SET( Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${res} )
  set_source_files_properties(${res} WRAP_EXCLUDE)
ENDFOREACH( file )

vtk_module_library(vtkFiltersHybrid ${Module_SRCS})
