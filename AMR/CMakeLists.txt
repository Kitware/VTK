## Set the Kit name
SET(KIT AMR)
SET(UKIT AMR)
SET(KIT_TCL_LIBS vtkCommonTCL vtkFilteringTCL vtkParallelTCL)
SET(KIT_PYTHON_LIBS vtkCommonPythonD vtkFilteringPythonD vtkParallelPythonD)
SET(KIT_JAVA_LIBS vtkCommonJava vtkFilteringJava vtkParallelJava)
SET(KIT_INTERFACE_LIBRARIES vtkCommon vtkFiltering vtkParallel)

## Get the kit sources
FILE( GLOB Kit_SRCS *.cxx )

## Set the libraries required by this kit
SET(KIT_LIBS vtkParallel vtkCommon vtkFiltering ${VTK_HDF5_LIBRARIES})

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
