find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkCollectGraph.cxx
  vtkCollectPolyData.cxx
  vtkCollectTable.cxx
  vtkCutMaterial.cxx
  vtkDistributedDataFilter.cxx
  vtkDuplicatePolyData.cxx
  vtkExtractCTHPart.cxx
  vtkExtractPolyDataPiece.cxx
  vtkExtractUnstructuredGridPiece.cxx
  vtkExtractUserDefinedPiece.cxx
  vtkPassThroughFilter.cxx
  vtkPCellDataToPointData.cxx
  vtkPCosmoHaloFinder.cxx
  vtkPExtractArraysOverTime.cxx
  vtkPieceRequestFilter.cxx
  vtkPieceScalars.cxx
  vtkPipelineSize.cxx
  vtkPKdTree.cxx
  vtkPLinearExtrusionFilter.cxx
  vtkPOutlineCornerFilter.cxx
  vtkPOutlineFilter.cxx
  vtkPPolyDataNormals.cxx
  vtkPProbeFilter.cxx
  vtkPReflectionFilter.cxx
  vtkProcessIdScalars.cxx
  vtkPSphereSource.cxx
  vtkPTableToStructuredGrid.cxx
  vtkRectilinearGridOutlineFilter.cxx
  vtkTransmitPolyDataPiece.cxx
  vtkTransmitRectilinearGridPiece.cxx
  vtkTransmitStructuredGridPiece.cxx
  vtkTransmitUnstructuredGridPiece.cxx
  vtkPYoungsMaterialInterface.cxx
  vtkPMaskPoints.cxx
  )
set_source_files_properties(
  vtkBlockDistribution.h
  HEADER_FILE_ONLY
  )

# Overrides for object factory.
set(parallel_overrides
  PolyDataNormals
  SphereSource
  LinearExtrusionFilter
  OutlineCornerFilter
  OutlineFilter
  ProbeFilter
  )

foreach(_override ${parallel_overrides})
  vtk_add_override(vtk${_override} vtkP${_override})
endforeach()

foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersParallel ${Module_SRCS})
