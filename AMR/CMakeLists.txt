## Set the Kit name
SET(KIT AMR)
SET(UKIT AMR)
SET(KIT_TCL_LIBS vtkCommonTCL)
SET(KIT_PYTHON_LIBS vtkCommonPythonD)
SET(KIT_JAVA_LIBS vtkCommonJava)
SET(KIT_INTERFACE_LIBRARIES vtkCommon)

## Get the kit sources
FILE( GLOB Kit_SRCS *.cxx )

## Set the libraries required by this kit
SET(KIT_LIBS vtkParallel vtkCommon vtkFiltering ${VTK_HDF5_LIBRARIES})

SET_SOURCE_FILES_PROPERTIES(
    vtkAMRBaseReader
    ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
  vtkAMRGridIndexEncoder
  vtkAMRLink
  WRAP_EXCLUDE
)
## Include CMAKE code common to all kits
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake )