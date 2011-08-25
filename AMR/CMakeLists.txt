## Set the Kit name
SET(KIT AMR)
SET(UKIT AMR)
SET(KIT_TCL_LIBS vtkCommonTCL vtkFilteringTCL vtkParallelTCL vtkGenericFilteringTCL )
SET(KIT_PYTHON_LIBS vtkCommonPythonD vtkFilteringPythonD vtkParallelPythonD vtkGenericFilteringPythonD )
SET(KIT_JAVA_LIBS vtkCommonJava vtkFilteringJava vtkParallelJava vtkGenericFilteringJava )
SET(KIT_INTERFACE_LIBRARIES vtkCommon vtkFiltering vtkParallel vtkGenericFiltering )

## Get the kit sources
SET(Kit_SRCS
    vtkAMRBaseParticlesReader.cxx
    vtkAMRBaseReader.cxx
    vtkAMRConnectivityFilter.cxx
    vtkAMRContourFilter.cxx
    vtkAMRCutPlane.cxx
    vtkAMRDataSetCache.cxx
    vtkAMRDualMeshExtractor.cxx
    vtkAMREnzoParticlesReader.cxx
    vtkAMREnzoReader.cxx
    vtkAMREnzoReaderInternal.cxx
    vtkAMRFlashParticlesReader.cxx
    vtkAMRFlashReader.cxx
    vtkAMRFlashReaderInternal.cxx
    vtkAMRGhostCellExtruder.cxx
    vtkAMRGhostExchange.cxx
    vtkAMRGridIndexEncoder.cxx
    vtkAMRHomogenizationFilter.cxx
    vtkAMRInterBlockConnectivity.cxx
    vtkAMRLink.cxx
    vtkAMRProbeFilter.cxx
    vtkAMRSliceFilter.cxx
    vtkAMRToGrid.cxx
    )

## Set the libraries required by this kit
SET(KIT_LIBS vtkParallel vtkCommon vtkFiltering vtkGenericFiltering ${VTK_HDF5_LIBRARIES})

SET_SOURCE_FILES_PROPERTIES(
    vtkAMRBaseReader
    vtkAMRBaseParticlesReader
    ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkAMRGridIndexEncoder
  vtkAMRLink
  vtkAMREnzoReaderInternal
  vtkAMRFlashReaderInternal
  WRAP_EXCLUDE
)
## Include CMAKE code common to all kits
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake )
